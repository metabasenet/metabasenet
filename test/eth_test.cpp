// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <sodium.h>

#include "crypto.h"
#include "destination.h"
#include "devcommon/util.h"
#include "evmexec.h"
#include "libdevcore/RLP.h"
#include "libdevcore/SHA3.h"
#include "libdevcrypto/Common.h"
#include "libethcore/TransactionBase.h"
#include "libevm/ExtMemVM.h"
#include "libevm/ExtVMFace.h"
#include "libevm/PithyEvmc.h"
#include "libevm/VMFactory.h"
#include "memvmhost.h"
#include "test_big.h"
#include "transaction.h"
#include "type.h"
#include "util.h"

using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace mtbase;
using namespace metabasenet;
using namespace metabasenet::crypto;
using namespace metabasenet::mvm;

//./build-release/test/test_big --log_level=all --run_test=eth_tests/rlptest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/signtest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/txtest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/bytestest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/addresstest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/txsigntest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/evmtest
//./build-release/test/test_big --log_level=all --run_test=eth_tests/evm_memevm_create_test
//./build-release/test/test_big --log_level=all --run_test=eth_tests/evm_settest_create_test

BOOST_FIXTURE_TEST_SUITE(eth_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(rlptest)
{
    RLPStream ss;
    ss.append(10);
    u256 n(30);
    ss << n;
    bytes btData = {
        1,
        2,
        3,
    };
    ss << btData;
    printf("ss size: %ld, hex: %s\n", ss.out().size(), ToHexString(ss.out()).c_str());

    auto h = sha3(ss.out());
    printf("hash size: %d, hash hex: %s\n", h.size, h.hex().c_str());
}

BOOST_AUTO_TEST_CASE(signtest)
{
    KeyPair kp = KeyPair::create();
    const Secret& privkey = kp.secret();
    const Public& pub = kp.pub();
    const Address& address = kp.address();
    h256 hash(1);

    printf("secret: [%d] %s\n", privkey.size, ToHexString(privkey.data(), privkey.size).c_str());
    printf("pub: [%d] %s\n", pub.size, pub.hex().c_str());
    printf("address: [%d] %s\n", address.size, address.hex().c_str());

    Signature signdata = sign(privkey, hash);
    printf("sign size: %d, sign hex: %s\n", signdata.size, signdata.hex().c_str());

    SignatureStruct ss(signdata);
    if (ss.isValid())
    {
        printf("r: %s\n", ss.r.hex().c_str());
        printf("s: %s\n", ss.s.hex().c_str());
        printf("v: %d\n", ss.v);
    }
    else
    {
        printf("SignatureStruct is invalid\n");
    }

    Public pubkey = recover(signdata, hash);
    printf("pubkey: [%d] %s\n", pubkey.size, pubkey.hex().c_str());

    bool resultVerify = verify(pubkey, signdata, hash);
    printf("verify result: %s\n", (resultVerify ? "true" : "false"));

    Public pubkey2 = toPublic(privkey);
    printf("privkey to pubkey: [%d] %s\n", pubkey2.size, pubkey2.hex().c_str());

    Address addr1 = toAddress(privkey);
    printf("address: %s\n", addr1.hex().c_str());

    Address addr2 = toAddress(pubkey);
    printf("address: %s\n", addr2.hex().c_str());

    //-----------------------------
    CCryptoKey key;
    uint512 ret = CryptoMakeNewKey(key);

    printf("ret: %s\n", ret.GetHex().c_str());
    printf("secret: %s\n", key.secret.GetHex().c_str());
    printf("pubkey: %s\n", key.pubkey.GetHex().c_str());

    bytes t;
    t.assign(key.secret.begin(), key.secret.end());
    Secret s(t);
    KeyPair keyPair(s);

    bytesConstRef r = keyPair.secret().ref();
    bytes btSecret;
    btSecret.assign(r.data(), r.data() + r.size());
    printf("m_secret: %s\n", ToHexString(btSecret).c_str());
    printf("m_public: 0x%s\n", keyPair.pub().hex().c_str());
    printf("m_address: 0x%s\n", keyPair.address().hex().c_str());
}

BOOST_AUTO_TEST_CASE(txtest)
{
    try
    {
        //KeyPair kpForm = KeyPair::create();
        KeyPair kpTo = KeyPair::create();
        //const Secret& privkey = kpForm.secret();

        uint256 mpk("0x6f19d5b077190361103c10293c262c373cb49e8594cc4421e22d8026560e4461");
        mpk.reverse();
        std::cout << "mpk: " << mpk.GetHex() << std::endl;
        Secret privkey(mpk.GetHex());

        // CCryptoKey key;
        // CryptoMakeNewKey(key);
        // std::cout << "secret: " << key.secret.GetHex() << ", pubkey: " << key.pubkey.GetHex() << std::endl;

        // uint256 h2 = key.secret;
        // h2.reverse();
        // std::cout << "h2: " << h2.GetHex() << std::endl;
        // Secret privkey(h2.GetHex());

        Address destFrom = toAddress(privkey);
        Address destTo = toAddress(kpTo.secret());

        printf("src from: 0x%s\n", destFrom.hex().c_str());
        printf("to: 0x%s\n", destTo.hex().c_str());

        TransactionSkeleton ts;

        //bool creation = false;
        ts.chainId = 201;
        ts.from = destFrom;
        ts.to = destTo;
        ts.value = u256(200000);
        //ts.data = bytes(kpTo.pub().data(), kpTo.pub().data() + kpTo.pub().size);
        ts.nonce = u256(12345678);
        ts.gas = u256(990000);
        ts.gasPrice = u256(6000);

        TransactionBase tx(ts, privkey);

        bytes btTxRlp = tx.rlp(WithSignature);
        printf("tx rlp: [%lu] %s\n", btTxRlp.size(), ToHexString(btTxRlp).c_str());

        h256 hashTx = tx.sha3();
        printf("tx hash: [%u] 0x%s\n", hashTx.size, hashTx.hex().c_str());

        h256 hashTxNoSign = tx.sha3(WithoutSignature);
        printf("no sign tx hash: [%u] 0x%s\n", hashTxNoSign.size, hashTxNoSign.hex().c_str());

        SignatureStruct const& sgs = tx.signature();
        printf("sign struct, r: 0x%s, s: 0x%s, v: %d\n", sgs.r.hex().c_str(), sgs.s.hex().c_str(), sgs.v);

        //----------------------------
        TransactionBase tx2(btTxRlp, CheckTransaction::Everything);

        printf("-------------------------\n");
        printf("tx from: 0x%s\n", tx2.from().hex().c_str());
        printf("to: 0x%s\n", tx2.to().hex().c_str());
        printf("value: %s\n", tx2.value().str().c_str());
        printf("gasPrice: %s\n", tx2.gasPrice().str().c_str());
        printf("gas: %s\n", tx2.gas().str(0, std::ios::hex | std::ios::showbase).c_str()); // | std::ios::uppercase
        printf("nonce: 0x%s\n", tx2.nonce().str(0, _Ios_Fmtflags::_S_hex).c_str());
        printf("data: [%lu] %s\n", tx2.data().size(), ToHexString(tx2.data()).c_str());

        string strGas = tx2.gas().str(0, std::ios::hex | std::ios::showbase);
        std::transform(strGas.begin(), strGas.end(), strGas.begin(), ::tolower);
        printf("gas lower: %s\n", strGas.c_str());
        printf("gas lower: %s : %ld\n", u256_to_hex(tx2.gas()).c_str(), (uint64_t)tx2.gas());

        SignatureStruct const& sgs2 = tx2.signature();
        printf("sign struct, r: 0x%s, s: 0x%s, v: %d\n", sgs2.r.hex().c_str(), sgs2.s.hex().c_str(), sgs2.v);

        if (tx2.from() != destFrom)
        {
            printf("verify from error, ori from: %s\n", destFrom.hex().c_str());
        }
        else
        {
            printf("verify from ok!\n");
        }

        printf("-------------------------\n");
        u256 h(9900000000000000L);

        string strBalance = formatBalance(h);
        printf("strBalance: %s\n", strBalance.c_str());

        Address addr = toAddress("0x2f21e7a8db5124e9d458a80122b44d55d430d46d");
        printf("addr: 0x%s\n", addr.hex().c_str());
    }
    catch (exception& e)
    {
        printf("error: %s\n", e.what());
    }
}

BOOST_AUTO_TEST_CASE(bytestest)
{
    u256 a = u256("0x11223344");
    printf("a: %s\n", a.str(0, std::ios::hex | std::ios::showbase).c_str());

    //h256 h("0x0000000000000000000000000000000000000000000000000000112233445566");
    h256 h;
    uint64 da = 0x11223344;
    memcpy(h.data(), &da, 8);
    printf("h: %s\n", h.hex().c_str());
    uint8* p = h.data();
    for (size_t i = 0; i < 32; i++)
    {
        printf("%2.2x ", *p++);
    }
    printf("\n");

    //uint256 hu("0x0000000000000000000000000000000000000000000000000000112233445566");
    uint256 hu; // = 0x11223344;
    memcpy(hu.begin(), &da, 8);
    printf("hu: %s\n", hu.GetHex().c_str());
    p = hu.begin();
    for (size_t i = 0; i < 32; i++)
    {
        printf("%2.2x ", *p++);
    }
    printf("\n");

    memcpy(h.data(), hu.begin(), 32);
    printf("h: %s\n", h.hex().c_str());
}

BOOST_AUTO_TEST_CASE(addresstest)
{
    const CDestination dest("0x371e47741b63562bdb7598e10f0c5724b6bf7ad0");
    printf("dest: %s\n", dest.ToString().c_str());
    const CDestination to = CDestination("0x371e47741b63562bdb7598e10f0c5724b6bf7ad0");
    printf("to: %s\n", to.ToString().c_str());
}

BOOST_AUTO_TEST_CASE(txsigntest)
{
    CCryptoKey key;
    CryptoMakeNewKey(key);

    CEthTxSkeleton ets;
    bytes btEthTxData;
    uint256 hashTx;

    const CDestination from("0x371e47741b63562bdb7598e10f0c5724b6bf7ad0");
    const CDestination to("0x371e47741b63562bdb7598e10f0c5724b6bf7ad0");

    ets.creation = false;
    ets.from = static_cast<uint160>(from);
    ets.to = static_cast<uint160>(to);
    ets.value = uint256(0x12345678);
    ets.data = ParseHexString("0x371e47741b63562bdb7598e10f0c5724b6bf7ad0");
    ets.nonce = 9;
    ets.gas = uint256(0x182B8);
    ets.gasPrice = uint256(0xE8D4A51000);

    BOOST_CHECK(GetEthTxData(key.secret, ets, hashTx, btEthTxData));

    printf("tx data: %s\n", ToHexString(btEthTxData).c_str());
    printf("tx hash: %s\n", hashTx.GetHex().c_str());

    CTransaction tx;
    BOOST_CHECK(tx.SetEthTx(btEthTxData, false));

    printf("txid: %s\n", tx.GetHash().GetHex().c_str());
    printf("from: %s\n", tx.GetFromAddress().ToString().c_str());
}

class CLastBHF : public LastBlockHashesFace
{
public:
    ~CLastBHF() {}

    /// Get hashes of 256 consecutive blocks preceding and including @a _mostRecentHash
    /// Hashes are returned in the order of descending height,
    /// i.e. result[0] is @a _mostRecentHash, result[1] is its parent, result[2] is grandparent etc.
    h256s precedingHashes(h256 const& _mostRecentHash) const override
    {
        return h256s();
    }

    /// Clear any cached result
    void clear() override
    {
    }
};

bool evmexec(const bytes& btCode, const bytes& btData, const bool fCreate)
{
    try
    {
        int64_t block_number = 10;
        u256 block_gasLimit = 20000000;
        int64_t block_timestamp = 1675740807;
        Address block_author("0xe7b8f76934716d00813eae13a5cfeff5d2fe46b5");
        u256 block_prev_randao = 0xFF;

        BlockHeader _current(block_number, block_gasLimit, block_timestamp, block_author, block_prev_randao);
        CLastBHF _lh;
        u256 _gasUsed = 0;
        u256 _chainID = 1;
        EnvInfo _envInfo(_current, _lh, _gasUsed, _chainID);
        Address _myAddress("0x8fa079a96ce08f6e8a53c1c00566c434b248bfc4");
        Address _caller(69);
        Address _origin(69);
        u256 _value = 0;
        u256 _gasPrice = 0;
        bytesConstRef _data(btData.data(), btData.size());
        bytesConstRef _code(btCode.data(), btCode.size());
        h256 _codeHash = sha3(btCode);
        u256 _version = 0;
        unsigned _depth = 0;
        bool _isCreate = fCreate;
        bool _staticCall = false;

        printf("_codeHash: 0x%s\n", _codeHash.hex().c_str());

        ExtMemVM m_ext(_envInfo, _myAddress,
                       _caller, _origin, _value, _gasPrice, _data,
                       _code, _codeHash, _version, _depth,
                       _isCreate, _staticCall);

        u256 m_gas = 990000;
        auto _onOp = [](uint64_t steps, uint64_t PC, Instruction instr,
                        bigint newMemSize, bigint gasCost, bigint gas, VMFace const*, ExtVMFace const*) {
            printf("_onOp==================================\n");
        };

        auto vm = VMFactory::create();
        auto out = vm->exec(m_gas, m_ext, _onOp);
        bytes btRet = out.toBytes();
        printf("exec success, size: %lu : %lu\n", out.size(), btRet.size());
        printf("ret: [%lu] %s\n", btRet.size(), ToHexString(btRet).c_str());
    }
    catch (RevertInstruction& _e)
    {
        printf("RevertInstruction: %s\n", _e.what());
        return false;
    }
    catch (VMException const& _e)
    {
        printf("VMException: %s\n", _e.what());
        return false;
    }
    catch (InternalVMError const& _e)
    {
        printf("InternalVMError: %s\n", _e.what());
        return false;
    }
    catch (Exception const& _e)
    {
        printf("Exception: %s\n", _e.what());
        return false;
    }
    catch (std::exception const& _e)
    {
        printf("std::exception: %s\n", _e.what());
        return false;
    }
    return true;
}

BOOST_AUTO_TEST_CASE(evmtest)
{
    auto funcReadFileData = [](const std::string& strFileName, bytes& btOut) -> bool {
        printf("Read file name: %s\n", strFileName.c_str());
        FILE* f = fopen(strFileName.c_str(), "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            auto filesize = ftell(f);
            fseek(f, 0, SEEK_SET);

            printf("filesize: %lu\n", filesize);

            uint8_t* readBuf = new uint8_t[filesize];
            if (readBuf == nullptr)
            {
                printf("new fail\n");
                return false;
            }

            auto nLen = fread(readBuf, 1, filesize, f);
            if (nLen != filesize)
            {
                delete[] readBuf;
                fclose(f);
                printf("fread size error, filesize: %lu readlen: %lu\n", filesize, nLen);
                return false;
            }
            btOut = ParseHexString(std::string(readBuf, readBuf + filesize));

            delete[] readBuf;
            fclose(f);
            return true;
        }
        return false;
    };

    bytes btCode;
    bytes btData;

    //char const* pFileName = "erc20/erc20.bin";
    //char const* pFileName = "test/test.bin";
    //char const* pFileName = "add/add.bin";
    //char const* pFileName = "block/block.bin";
    char const* pFileName = "block/blockpush1.bin";
    std::string fullpath = boost::filesystem::initial_path<boost::filesystem::path>().string() + "/test/solctest/";
    if (!funcReadFileData(fullpath + pFileName, btCode))
    {
        printf("Read file fail\n");
        return;
    }
    printf("code: [%lu] %s\n", btCode.size(), ToHexString(btCode).c_str());

    evmexec(btCode, btData, true);
}

BOOST_AUTO_TEST_CASE(evmcreatetest)
{
    bytes btCode = ParseHexString("608060405234801561001057600080fd5b5060d28061001f6000396000f3fe608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029");
    bytes btData;

    evmexec(btCode, btData, true);
}

BOOST_AUTO_TEST_CASE(evmcalltest)
{
    bytes btCode = ParseHexString("608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029");
    bytes btData = ParseHexString("8a39ec3a00000000000000000000000000000000000000000000000000000000000000190000000000000000000000000000000000000000000000000000000000000003");

    evmexec(btCode, btData, false);
}

bool MemEvmExec(const bytes& btCode, const bytes& btData, const bool fCreate)
{
    if (!InitLog("/home/cch/.metabasenet/testnet/logs", true, false, 1024 * 1024 * 200, 1024 * 1024 * 2000))
    {
        printf("Init log fail\n");
    }

    CMemVmHost memhost;
    CEvmExec vm(memhost, uint256(5566), 10, 123);

    CDestination from("0x8fa079a96ce08f6e8a53c1c00566c434b248bfa4");
    CDestination to;
    CDestination destContract;
    CDestination destCodeOwner;
    uint64 nTxGasLimit = 990000;
    uint256 nGasPrice = 1000;
    uint256 nTxAmount = 0;
    CDestination destBlockMint;
    uint64 nBlockTimestamp = 0;
    int nBlockHeight = 0;
    uint64 nBlockGasLimit = 20000000;
    bytes btContractCode = btCode;
    bytes btRunParam = btData;
    CTxContractData txcd;

    if (!fCreate)
    {
        to = CDestination("0xe7bF45E20df9F7d85433ddFf8a376a2427122362");
        destContract = to;
    }

    if (!vm.evmExec(from, to, destContract, destCodeOwner, nTxGasLimit,
                    nGasPrice, nTxAmount, destBlockMint, nBlockTimestamp,
                    nBlockHeight, nBlockGasLimit, btContractCode, btRunParam, txcd))
    {
        printf("exec fail\n");
        return false;
    }
    printf("exec success, gas used: %lu, ret: [%lu] %s\n",
           nTxGasLimit - vm.nGasLeft, vm.vResult.size(), ToHexString(vm.vResult).c_str());

    return true;
}

BOOST_AUTO_TEST_CASE(evm_memevm_create_test)
{
    bytes btCode = ParseHexString("608060405234801561001057600080fd5b5060d28061001f6000396000f3fe608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029");
    bytes btData;

    MemEvmExec(btCode, btData, true);
}

BOOST_AUTO_TEST_CASE(evm_memevm_call_test)
{
    bytes btCode = ParseHexString("608060405260043610603f576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680638a39ec3a146044575b600080fd5b348015604f57600080fd5b50608360048036036040811015606457600080fd5b8101908080359060200190929190803590602001909291905050506099565b6040518082815260200191505060405180910390f35b600081830190509291505056fea165627a7a72305820a28c5a1aa380b1998c2e97560956d46e687dc70624e969e3e13f027c223d07310029");
    bytes btData = ParseHexString("8a39ec3a00000000000000000000000000000000000000000000000000000000000000190000000000000000000000000000000000000000000000000000000000000003");

    MemEvmExec(btCode, btData, false);
}

BOOST_AUTO_TEST_CASE(evm_settest_create_test)
{
    bytes btCode = ParseHexString("608060405234801561001057600080fd5b506175306000803373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055506101ec806100656000396000f3fe60806040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b4cef28d14610051578063f38255f9146100b6575b600080fd5b34801561005d57600080fd5b506100a06004803603602081101561007457600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610129565b6040518082815260200191505060405180910390f35b3480156100c257600080fd5b5061010f600480360360408110156100d957600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610171565b604051808215151515815260200191505060405180910390f35b60008060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020549050919050565b6000816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550600190509291505056fea165627a7a72305820cd3f834c0cad45a0e0cbe343010ab43df5f2ec56fd297f515ba55c3cc6057a520029");
    bytes btData;

    MemEvmExec(btCode, btData, true);
}

BOOST_AUTO_TEST_CASE(evm_settest_call_set_test)
{
    bytes btCode = ParseHexString("0x60806040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b4cef28d14610051578063f38255f9146100b6575b600080fd5b34801561005d57600080fd5b506100a06004803603602081101561007457600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610129565b6040518082815260200191505060405180910390f35b3480156100c257600080fd5b5061010f600480360360408110156100d957600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610171565b604051808215151515815260200191505060405180910390f35b60008060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020549050919050565b6000816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550600190509291505056fea165627a7a72305820cd3f834c0cad45a0e0cbe343010ab43df5f2ec56fd297f515ba55c3cc6057a520029");
    bytes btData = ParseHexString("0xf38255f90000000000000000000000008fa079a96ce08f6e8a53c1c00566c434b248bfa40000000000000000000000000000000000000000000000000000000000001388");

    MemEvmExec(btCode, btData, false);
}

BOOST_AUTO_TEST_CASE(evm_settest_call_get_test)
{
    bytes btCode = ParseHexString("0x60806040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063b4cef28d14610051578063f38255f9146100b6575b600080fd5b34801561005d57600080fd5b506100a06004803603602081101561007457600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610129565b6040518082815260200191505060405180910390f35b3480156100c257600080fd5b5061010f600480360360408110156100d957600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610171565b604051808215151515815260200191505060405180910390f35b60008060008373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020549050919050565b6000816000808573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002081905550600190509291505056fea165627a7a72305820cd3f834c0cad45a0e0cbe343010ab43df5f2ec56fd297f515ba55c3cc6057a520029");
    bytes btData = ParseHexString("0xb4cef28d0000000000000000000000008fa079a96ce08f6e8a53c1c00566c434b248bfa4");

    MemEvmExec(btCode, btData, false);
}

BOOST_AUTO_TEST_CASE(u256test)
{
    u256 _n(MIN_GAS_PRICE.GetValueHex());
    printf("MIN_GAS_PRICE: %s\n", u256_to_hex(_n).c_str());

    bytes b = toBigEndian(_n);
    printf("b: %s\n", ToHexString(b).c_str());
}

BOOST_AUTO_TEST_CASE(txsizetest)
{
    {
        CTransaction tx;

        CDestination from("0x5962974eeb0b17b43edabfc9b747839317aa852f"), to("0x9835c12d95f059eb4eaecb4b00f2ea2c99b2a0d4");
        uint256 nAmount = 1;
        uint256 nGasLimit = TX_BASE_GAS;
        uint256 nGasPrice = MIN_GAS_PRICE;
        bytes btSignData;
        btSignData.resize(65, 1);

        tx.SetTxType(CTransaction::TX_TOKEN);
        tx.SetChainId(201);
        tx.SetNonce(123);
        tx.SetFromAddress(from);
        tx.SetToAddress(to);
        tx.SetAmount(nAmount);
        tx.SetGasPrice(nGasLimit);
        tx.SetGasLimit(nGasPrice);
        tx.SetSignData(btSignData);

        mtbase::CBufStream ss;
        ss << tx;

        printf("token tx size: %lu\n", ss.GetSize());
    }

    {
        CTransaction tx;

        tx.SetTxType(CTransaction::TX_VOTE_REWARD);
        tx.SetChainId(0xFFFFFFFF);
        tx.SetNonce(~((uint64)0));
        tx.SetToAddress(CDestination(~uint160()));
        tx.SetAmount((~uint256()) >> 160);

        mtbase::CBufStream ss;
        ss << tx;

        printf("reward tx size: %lu\n", ss.GetSize());
    }

    {
        try
        {
            KeyPair kpTo = KeyPair::create();

            uint256 mpk("0x6f19d5b077190361103c10293c262c373cb49e8594cc4421e22d8026560e4461");
            mpk.reverse();
            std::cout << "mpk: " << mpk.GetHex() << std::endl;
            Secret privkey(mpk.GetHex());

            Address destFrom = toAddress(privkey);
            Address destTo = toAddress(kpTo.secret());

            printf("src from: 0x%s\n", destFrom.hex().c_str());
            printf("to: 0x%s\n", destTo.hex().c_str());

            TransactionSkeleton ts;

            //bool creation = false;
            ts.chainId = 201;
            ts.from = destFrom;
            ts.to = destTo;
            ts.value = u256(1);
            ts.nonce = u256(123);
            ts.gas = u256(TX_BASE_GAS.Get64());
            ts.gasPrice = u256(MIN_GAS_PRICE.Get64());

            TransactionBase tx(ts, privkey);

            bytes btTxRlp = tx.rlp(WithSignature);
            printf("eth tx rlp: [%lu] %s\n", btTxRlp.size(), ToHexString(btTxRlp).c_str());

            h256 hashTx = tx.sha3();
            printf("eth tx hash: [%u] 0x%s\n", hashTx.size, hashTx.hex().c_str());

            CTransaction htx;

            htx.SetEthTx(btTxRlp, true);

            mtbase::CBufStream ss;
            ss << htx;

            printf("eth tx size: %lu\n", ss.GetSize());
        }
        catch (exception& e)
        {
            printf("error: %s\n", e.what());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
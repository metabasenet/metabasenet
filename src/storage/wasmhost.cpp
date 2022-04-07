// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmhost.h"

#include <evmc/loader.h>

#include "block.h"
#include "util.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////////
// CWasmHost

CWasmHost::CWasmHost(const evmc_tx_context& _tx_context, CHostBaseDB& dbHostIn)
  : tx_context(_tx_context), dbHost(dbHostIn)
{
}

bool CWasmHost::account_exists(const evmc::address& addr) const noexcept
{
    uint256 hashAddr(bytes(&(addr.bytes[0]), &(addr.bytes[0]) + sizeof(addr.bytes)));
    uint256 hashBalance;
    if (!dbHost.GetBalance(hashAddr, hashBalance))
    {
        StdLog("CWasmHost", "account_exists: Find fail, addr: 0x%s, hashAddr: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str());
        return false;
    }
    return true;
}

evmc::bytes32 CWasmHost::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept
{
    CBufStream ss;
    ss.Write((char*)&(addr.bytes[0]), sizeof(addr.bytes));
    ss.Write((char*)&(key.bytes[0]), sizeof(key.bytes));
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    auto it = cacheKv.find(hash);
    if (it == cacheKv.end())
    {
        bytes vValue;
        if (!dbHost.Get(hash, vValue))
        {
            StdLog("CWasmHost", "get_storage: Get fail, addr: 0x%s, key: 0x%s, hash: 0x%s",
                   ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(),
                   ToHexString(&(key.bytes[0]), sizeof(key.bytes)).c_str(),
                   hash.GetHex().c_str());
            return {};
        }
        evmc::bytes32 value;
        if (vValue.size() == sizeof(value.bytes))
        {
            memcpy(value.bytes, vValue.data(), sizeof(value.bytes));
            StdLog("CWasmHost", "get_storage: Get success, addr: 0x%s, key: 0x%s, hash: %s, value: 0x%s",
                   ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
                   ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
                   hash.GetHex().c_str(),
                   ToHexString(value.bytes, sizeof(value.bytes)).c_str());
            return value;
        }
        else
        {
            StdLog("CWasmHost", "get_storage: vValue size error, size: %ld, addr: 0x%s",
                   vValue.size(), ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str());
            return {};
        }
    }
    else
    {
        StdLog("CWasmHost", "get_storage: Get cache success, addr: 0x%s, key: 0x%s, hash: %s, value: 0x%s",
               ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
               ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
               hash.GetHex().c_str(),
               ToHexString(it->second.bytes, sizeof(it->second.bytes)).c_str());
        return it->second;
    }
    return {};
}

evmc_storage_status CWasmHost::set_storage(const evmc::address& addr,
                                           const evmc::bytes32& key,
                                           const evmc::bytes32& value) noexcept
{
    CBufStream ss;
    ss.Write((char*)&(addr.bytes[0]), sizeof(addr.bytes));
    ss.Write((char*)&(key.bytes[0]), sizeof(key.bytes));
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    cacheKv[hash] = value;
    StdLog("CWasmHost", "set_storage: addr: 0x%s, key: 0x%s, hash: %s, value: 0x%s",
           ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
           ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
           hash.GetHex().c_str(),
           ToHexString(value.bytes, sizeof(value.bytes)).c_str());
    return EVMC_STORAGE_UNCHANGED;
}

evmc::uint256be CWasmHost::get_balance(const evmc::address& addr) const noexcept
{
    uint256 hashAddr(bytes(&(addr.bytes[0]), &(addr.bytes[0]) + sizeof(addr.bytes)));
    uint256 hashBalance;
    if (!dbHost.GetBalance(hashAddr, hashBalance))
    {
        StdLog("CWasmHost", "get_balance: Get balance fail, addr: 0x%s, hashAddr: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str());
        return {};
    }

    evmc::uint256be ret;
    memcpy(ret.bytes, hashBalance.begin(), hashBalance.size());
    std::reverse(std::begin(ret.bytes), std::end(ret.bytes));

    StdLog("CWasmHost", "get_balance: Get balance success, addr: 0x%s, hashAddr: %s, ret: %s",
           ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str(),
           ToHexString(&(ret.bytes[0]), sizeof(ret.bytes)).c_str());
    return ret;
}

size_t CWasmHost::get_code_size(const evmc::address& addr) const noexcept
{
    uint256 hashAddr(bytes(&(addr.bytes[0]), &(addr.bytes[0]) + sizeof(addr.bytes)));
    CTxContractData txcd;
    if (!dbHost.GetWasmCreateCode(hashAddr, txcd))
    {
        StdLog("CWasmHost", "get_code_size: Get wasm src code fail, addr: 0x%s, hashAddr: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str());
        return 0;
    }
    return txcd.GetCode().size();
}

evmc::bytes32 CWasmHost::get_code_hash(const evmc::address& addr) const noexcept
{
    uint256 hashAddr(bytes(&(addr.bytes[0]), &(addr.bytes[0]) + sizeof(addr.bytes)));
    CTxContractData txcd;
    if (!dbHost.GetWasmCreateCode(hashAddr, txcd))
    {
        StdLog("CWasmHost", "get_code_hash: Get wasm src code fail, addr: 0x%s, hashAddr: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str());
        return {};
    }
    uint256 hashWasmCreateCode = txcd.GetWasmCreateCodeHash();
    evmc::bytes32 byOut = {};
    memcpy(byOut.bytes, hashWasmCreateCode.begin(), min(sizeof(byOut.bytes), (size_t)(hashWasmCreateCode.size())));
    return byOut;
}

size_t CWasmHost::copy_code(const evmc::address& addr,
                            size_t code_offset,
                            uint8_t* buffer_data,
                            size_t buffer_size) const noexcept
{
    if (buffer_data == nullptr || buffer_size == 0)
    {
        return 0;
    }

    uint256 hashAddr(bytes(&(addr.bytes[0]), &(addr.bytes[0]) + sizeof(addr.bytes)));
    CTxContractData txcd;
    if (!dbHost.GetWasmCreateCode(hashAddr, txcd))
    {
        StdLog("CWasmHost", "copy_code: Get wasm src code fail, addr: 0x%s, hashAddr: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), hashAddr.GetHex().c_str());
        return {};
    }
    bytes btWasmCreateCode = txcd.GetCode();

    if (code_offset >= btWasmCreateCode.size())
    {
        return 0;
    }
    const auto n = min(buffer_size, btWasmCreateCode.size() - code_offset);
    if (n > 0)
    {
        std::copy_n(&btWasmCreateCode[code_offset], n, buffer_data);
    }
    return n;
}

void CWasmHost::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept
{
}

evmc::result CWasmHost::call(const evmc_message& msg) noexcept
{
    if (msg.input_size == 0)
    {
        uint256 from(msg.sender.bytes, sizeof(msg.sender.bytes));
        uint256 to(msg.destination.bytes, sizeof(msg.destination.bytes));
        uint256 amount(msg.value.bytes, sizeof(msg.value.bytes));
        amount.reverse();

        if (!dbHost.Transfer(from, to, amount))
        {
            return { EVMC_REVERT, msg.gas, nullptr, 0 };
        }
        return { EVMC_SUCCESS, msg.gas, nullptr, 0 };
    }

    if (msg.kind == evmc_call_kind::EVMC_CREATE3)
    {
        return CreateContract3(msg);
    }

    return CallContract(msg);
}

evmc_tx_context CWasmHost::get_tx_context() const noexcept
{
    return tx_context;
}

evmc::bytes32 CWasmHost::get_block_hash(int64_t number) const noexcept
{
    return tx_context.block_difficulty;
}

void CWasmHost::emit_log(const evmc::address& addr,
                         const uint8_t* data,
                         size_t data_size,
                         const evmc::bytes32 topics[],
                         size_t topics_count) noexcept
{
    logAddr = addr;
    StdLog("CWasmHost", "emit_log: addr: 0x%s", ToHexString(&(addr.bytes[0]), 32).c_str());
    for (size_t i = 0; i < topics_count; i++)
    {
        StdLog("CWasmHost", "emit_log: topics: 0x%s", ToHexString(&(topics[i].bytes[0]), 32).c_str());
        vLogTopics.push_back(topics[i]);
    }
    if (data && data_size > 0)
    {
        StdLog("CWasmHost", "emit_log: data: 0x%s", ToHexString(data, data_size).c_str());
        vLogData.assign(data, data + data_size);
    }
    else
    {
        StdLog("CWasmHost", "emit_log: data: null");
    }
}

/////////////////////////////////////////
evmc::result CWasmHost::CreateContract3(const evmc_message& msg)
{
    uint256 from(msg.sender.bytes, sizeof(msg.sender.bytes));
    uint256 hashSrcContract(msg.destination.bytes, sizeof(msg.destination.bytes));
    uint256 salt(msg.create2_salt.bytes, sizeof(msg.create2_salt.bytes));

    hashSrcContract.reverse();

    CBufStream ss;
    ss << from << salt;
    uint256 hashNewContract = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    CTxContractData txcd;
    if (!dbHost.GetWasmCreateCode(hashSrcContract, txcd))
    {
        StdLog("CWasmHost", "Create contract3: Get wasm src code fail, hashSrcContract: %s", hashSrcContract.GetHex().c_str());
        return { EVMC_INTERNAL_ERROR, msg.gas, nullptr, 0 };
    }
    uint256 hashWasmCreateCode = txcd.GetWasmCreateCodeHash();
    bytes btWasmCreateCode = txcd.GetCode();
    CDestination destCodeOwner = txcd.GetCodeOwner();

    CHostBaseDBPtr ptrNewHostDB = dbHost.CloneHostDB(hashNewContract);

    shared_ptr<CWasmHost> host(new CWasmHost(tx_context, *ptrNewHostDB));

    evmc::address destination = {};
    memcpy(destination.bytes, hashNewContract.begin(), min(sizeof(destination.bytes), (size_t)(hashNewContract.size())));

    evmc_message new_msg{ EVMC_CALL,
                          0,         // flags;
                          msg.depth, // The call depth;
                          msg.gas,   // nTxGasLimit,
                          destination,
                          msg.sender, // sender,
                          msg.input_data,
                          msg.input_size,
                          {}, // amount,
                          {} };

    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();
    struct evmc_vm* vm = evmc_load_and_create();

    StdLog("CWasmHost", "Create contract3: execute prev: gas: %ld", new_msg.gas);
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &new_msg, btWasmCreateCode.data(), btWasmCreateCode.size());
    StdLog("CWasmHost", "Create contract3: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
           new_msg.gas, result.gas_left, new_msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    ptrNewHostDB->SaveGasUsed(destCodeOwner, new_msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        if (result.output_data && result.output_size > 32)
        {
            bytes btWasmRunCode;
            std::map<uint256, bytes> mapCacheKv;
            CTransactionLogs logs;

            btWasmRunCode.assign(result.output_data, result.output_data + result.output_size);
            for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
            {
                mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
            }
            for (size_t i = 0; i < sizeof(host->logAddr.bytes); i++)
            {
                if (host->logAddr.bytes[i] != 0)
                {
                    logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
                    break;
                }
            }
            logs.data = host->vLogData;
            for (auto& vd : host->vLogTopics)
            {
                bytes btTopics;
                btTopics.assign(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes));
                logs.topics.push_back(btTopics);
            }

            if (!ptrNewHostDB->SaveWasmRunCode(hashNewContract, btWasmRunCode, txcd))
            {
                StdLog("CWasmHost", "Create contract3: Save run code fail, hashNewContract: %s", hashNewContract.GetHex().c_str());
                evmc::result ret(EVMC_INTERNAL_ERROR, result.gas_left, nullptr, 0);
                if (result.release)
                {
                    result.release(&result);
                }
                return ret;
            }
            ptrNewHostDB->SaveRunResult(hashNewContract, logs, mapCacheKv);
        }
        else
        {
            StdLog("CWasmHost", "Create contract3: output data error, output_size: %ld, hashNewContract: %s", result.output_size, hashNewContract.GetHex().c_str());
            evmc::result ret(EVMC_INTERNAL_ERROR, result.gas_left, nullptr, 0);
            if (result.release)
            {
                result.release(&result);
            }
            return ret;
        }
    }
    else
    {
        StdLog("CWasmHost", "Create contract3: Execute fail, status_code: %ld, hashNewContract: %s", result.status_code, hashNewContract.GetHex().c_str());
        return evmc::result(result);
    }

    CDestination destNewContract;
    destNewContract.SetContractId(hashNewContract);
    StdLog("CWasmHost", "Create contract3: Create success, destNewContract: %s, hashWasmCreateCode: %s",
           destNewContract.ToString().c_str(), hashWasmCreateCode.GetHex().c_str());

    evmc::result ret(result);
    memcpy(ret.create_address.bytes, hashNewContract.begin(), min(sizeof(ret.create_address.bytes), (size_t)(hashNewContract.size())));
    return ret;
}

evmc::result CWasmHost::CallContract(const evmc_message& msg)
{
    uint256 hashWasm(msg.destination.bytes, sizeof(msg.destination.bytes));

    uint256 hashWasmCreateCode;
    CDestination destCodeOwner;
    bytes btWasmRunCode;
    if (!dbHost.GetWasmRunCode(hashWasm, hashWasmCreateCode, destCodeOwner, btWasmRunCode))
    {
        StdLog("CWasmHost", "Call wasm: Get wasm run code fail, hashWasm: %s", hashWasm.GetHex().c_str());
        return { EVMC_INTERNAL_ERROR, msg.gas, nullptr, 0 };
    }

    CHostBaseDBPtr ptrNewHostDB = dbHost.CloneHostDB(hashWasm);

    shared_ptr<CWasmHost> host(new CWasmHost(tx_context, *ptrNewHostDB));

    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();
    struct evmc_vm* vm = evmc_load_and_create();

    StdLog("CWasmHost", "Call wasm: execute prev: gas: %ld", msg.gas);
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg, btWasmRunCode.data(), btWasmRunCode.size());
    StdLog("CWasmHost", "Call wasm: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
           msg.gas, result.gas_left, msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    ptrNewHostDB->SaveGasUsed(destCodeOwner, msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        StdLog("CWasmHost", "Call wasm: execute success, sender: 0x%s, destination: 0x%s, value: 0x%s, output_size: %ld, output_data: %s",
               hnbase::ToHexString(msg.sender.bytes, sizeof(msg.sender.bytes)).c_str(),
               hnbase::ToHexString(msg.destination.bytes, sizeof(msg.destination.bytes)).c_str(),
               hnbase::ToHexString(msg.value.bytes, sizeof(msg.value.bytes)).c_str(),
               result.output_size, ToHexString(result.output_data, result.output_size).c_str());

        CTransactionLogs logs;
        std::map<uint256, bytes> mapCacheKv;

        for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
        {
            mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
        }
        for (size_t i = 0; i < sizeof(host->logAddr.bytes); i++)
        {
            if (host->logAddr.bytes[i] != 0)
            {
                logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
                break;
            }
        }
        logs.data = host->vLogData;
        for (auto& vd : host->vLogTopics)
        {
            logs.topics.push_back(bytes(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes)));
        }

        ptrNewHostDB->SaveRunResult(hashWasm, logs, mapCacheKv);
    }
    else
    {
        StdLog("CWasmHost", "Call wasm: execute fail, err: %ld", result.status_code);
    }
    return evmc::result(result);
}

} // namespace storage
} // namespace metabasenet

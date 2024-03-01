// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "evmhost.h"

#include "../meth/evmc/include/evmc/evmc.h"
#include "../meth/libaleth-interpreter/interpreter.h"
#include "../meth/libevm/PithyEvmc.h"
#include "block.h"
#include "mutil.h"
#include "precompiled.h"
#include "util.h"

using namespace std;
using namespace mtbase;
using namespace dev;
using namespace dev::eth;

namespace metabasenet
{
namespace mvm
{

//////////////////////////////////
// CEvmHost

CEvmHost::CEvmHost(const evmc_tx_context& _tx_context, CVmHostFaceDB& dbHostIn, SHP_HOST_CACHE_KV pCacheKvIn)
  : tx_context(_tx_context), dbHost(dbHostIn)
{
    if (pCacheKvIn)
    {
        pCacheKv = pCacheKvIn;
    }
    else
    {
        pCacheKv = SHP_HOST_CACHE_KV(new CHostCacheKv());
    }
}

CDestination CEvmHost::AddressToDestination(const evmc::address& addr)
{
    return CDestination(&(addr.bytes[0]), sizeof(addr.bytes));
}

evmc::address CEvmHost::DestinationToAddress(const CDestination& dest)
{
    evmc::address addr;
    bytes d = dest.GetBytes();
    memcpy(&(addr.bytes[0]), d.data(), std::min(d.size(), sizeof(addr.bytes)));
    return addr;
}

bool CEvmHost::account_exists(const evmc::address& addr) const noexcept
{
    CDestination dest = AddressToDestination(addr);
    uint256 hashBalance;
    if (!dbHost.GetBalance(dest, hashBalance))
    {
        StdLog("CEvmHost", "account_exists: Find fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
        return false;
    }
    return true;
}

evmc::bytes32 CEvmHost::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept
{
    CBufStream ss;
    ss.Write((char*)&(addr.bytes[0]), sizeof(addr.bytes));
    ss.Write((char*)&(key.bytes[0]), sizeof(key.bytes));
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    CDestination dest = AddressToDestination(addr);
    bytes cacheValue;
    if (pCacheKv->GetValue(dest, hash, cacheValue))
    {
        StdDebug("CEvmHost", "get_storage: Get cache success, addr: %s, key: %s, hash: %s, value: %s",
                 ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
                 ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
                 hash.GetHex().c_str(),
                 ToHexString(cacheValue).c_str());
        evmc::bytes32 value;
        memcpy(value.bytes, cacheValue.data(), min(cacheValue.size(), sizeof(value.bytes)));
        return value;
    }

    bytes vValue;
    if (!dbHost.Get(hash, vValue))
    {
        StdLog("CEvmHost", "get_storage: Get fail, addr: %s, key: %s, hash: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(),
               ToHexString(&(key.bytes[0]), sizeof(key.bytes)).c_str(),
               hash.GetHex().c_str());
        return {};
    }
    evmc::bytes32 value;
    if (vValue.size() == sizeof(value.bytes))
    {
        memcpy(value.bytes, vValue.data(), sizeof(value.bytes));
        StdDebug("CEvmHost", "get_storage: Get success, addr: %s, key: %s, hash: %s, value: %s",
                 ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
                 ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
                 hash.GetHex().c_str(),
                 ToHexString(value.bytes, sizeof(value.bytes)).c_str());
        return value;
    }
    StdLog("CEvmHost", "get_storage: vValue size error, size: %ld, addr: %s",
           vValue.size(), ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str());
    return {};
}

evmc_storage_status CEvmHost::set_storage(const evmc::address& addr,
                                          const evmc::bytes32& key,
                                          const evmc::bytes32& value) noexcept
{
    CBufStream ss;
    ss.Write((char*)&(addr.bytes[0]), sizeof(addr.bytes));
    ss.Write((char*)&(key.bytes[0]), sizeof(key.bytes));
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    evmc_storage_status status = EVMC_STORAGE_UNCHANGED;
    string strModifyName;

    CDestination dest = AddressToDestination(addr);
    bytes cacheValue;
    if (!pCacheKv->GetValue(dest, hash, cacheValue))
    {
        bytes originalValue;
        if (!dbHost.Get(hash, originalValue))
        {
            status = EVMC_STORAGE_ADDED;
            strModifyName = "ADDED";
        }
        else
        {
            evmc::bytes32 zeroValue = {};
            if (originalValue.size() == sizeof(zeroValue.bytes) && memcmp(originalValue.data(), &(zeroValue.bytes[0]), sizeof(zeroValue.bytes)) == 0)
            {
                status = EVMC_STORAGE_ADDED;
                strModifyName = "ADDED";
            }
            else if (memcmp(&(value.bytes[0]), &(zeroValue.bytes[0]), sizeof(zeroValue.bytes)) == 0)
            {
                status = EVMC_STORAGE_DELETED;
                strModifyName = "MODIFIED_DELETED";
            }
            else
            {
                status = EVMC_STORAGE_MODIFIED;
                strModifyName = "MODIFIED";
            }
            // if (originalValue.size() == sizeof(value.bytes) && memcmp(originalValue.data(), &(value.bytes[0]), sizeof(value.bytes)) == 0)
            // {
            //     status = EVMC_STORAGE_UNCHANGED;
            //     strModifyName = "UNCHANGED-DBSAME";
            // }
            // else
            // {
            //     status = EVMC_STORAGE_MODIFIED;
            //     strModifyName = "MODIFIED";
            // }
        }
    }
    else
    {
        status = EVMC_STORAGE_MODIFIED;
        strModifyName = "MODIFIED";
        // if (cacheValue.size() == sizeof(value.bytes)
        //     && memcmp(cacheValue.data(), &(value.bytes[0]), sizeof(value.bytes)) == 0)
        // {
        //     status = EVMC_STORAGE_UNCHANGED;
        //     strModifyName = "UNCHANGED-CACHESAME";
        // }
        // else
        // {
        //     evmc::bytes32 tempValue = {};
        //     if (cacheValue.size() == sizeof(tempValue.bytes)
        //         && memcmp(cacheValue.data(), &(tempValue.bytes[0]), sizeof(tempValue.bytes)) == 0)
        //     {
        //         status = EVMC_STORAGE_DELETED;
        //         strModifyName = "MODIFIED_DELETED";
        //     }
        //     else
        //     {
        //         status = EVMC_STORAGE_MODIFIED_AGAIN;
        //         strModifyName = "MODIFIED_AGAIN";
        //     }
        // }
    }
    if (status != EVMC_STORAGE_UNCHANGED)
    {
        pCacheKv->SetValue(dest, hash, bytes(&(value.bytes[0]), &(value.bytes[0]) + sizeof(value.bytes)));
    }

    StdDebug("CEvmHost", "set_storage: addr: %s, key: %s, hash: %s, value: %s, modify: %s",
             ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
             ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
             hash.GetHex().c_str(),
             ToHexString(value.bytes, sizeof(value.bytes)).c_str(), strModifyName.c_str());

    return status;
}

evmc::uint256be CEvmHost::get_balance(const evmc::address& addr) const noexcept
{
    CDestination dest = AddressToDestination(addr);
    uint256 hashBalance;
    if (!dbHost.GetBalance(dest, hashBalance))
    {
        StdLog("CEvmHost", "get_balance: Get balance fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
        return {};
    }

    evmc::uint256be ret;
    memcpy(ret.bytes, hashBalance.begin(), hashBalance.size());
    std::reverse(std::begin(ret.bytes), std::end(ret.bytes));

    StdDebug("CEvmHost", "get_balance: Get balance success, addr: %s, dest: %s, ret: %s",
             ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str(),
             ToHexString(&(ret.bytes[0]), sizeof(ret.bytes)).c_str());
    return ret;
}

size_t CEvmHost::get_code_size(const evmc::address& addr) const noexcept
{
    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CEvmHost", "get_code_size: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return 0;
    }
    if (fDestroy)
    {
        StdLog("CEvmHost", "get_code_size: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return 0;
    }
    return btContractRunCode.size();
}

evmc::bytes32 CEvmHost::get_code_hash(const evmc::address& addr) const noexcept
{
    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CEvmHost", "get_code_hash: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return {};
    }
    if (fDestroy)
    {
        StdLog("CEvmHost", "get_code_hash: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return {};
    }
    evmc::bytes32 byOut = {};
    memcpy(byOut.bytes, hashContractRunCode.begin(), min(sizeof(byOut.bytes), (size_t)(hashContractRunCode.size())));
    return byOut;
}

size_t CEvmHost::copy_code(const evmc::address& addr,
                           size_t code_offset,
                           uint8_t* buffer_data,
                           size_t buffer_size) const noexcept
{
    if (buffer_data == nullptr || buffer_size == 0)
    {
        return 0;
    }

    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CEvmHost", "copy_code: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return 0;
    }
    if (fDestroy)
    {
        StdLog("CEvmHost", "copy_code: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return 0;
    }

    if (code_offset >= btContractRunCode.size())
    {
        return 0;
    }
    const auto n = min(buffer_size, btContractRunCode.size() - code_offset);
    if (n > 0)
    {
        std::copy_n(&btContractRunCode[code_offset], n, buffer_data);
    }
    return n;
}

void CEvmHost::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept
{
    //(void)addr;
    //assert(fromEvmC(_addr) == m_extVM.myAddress);
    //m_extVM.selfdestruct(fromEvmC(_beneficiary));
    StdDebug("CEvmHost", "selfdestruct: addr: %s, beneficiary: %s", ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), ToHexString(&(beneficiary.bytes[0]), sizeof(beneficiary.bytes)).c_str());
    assert(AddressToDestination(addr) == dbHost.GetContractAddress());
    dbHost.Selfdestruct(AddressToDestination(beneficiary));
}

evmc::result CEvmHost::call(const evmc_message& msg) noexcept
{
    StdDebug("CEvmHost", "call: sender: %s - %s", ToHexString(&(msg.sender.bytes[0]), sizeof(msg.sender.bytes)).c_str(), AddressToDestination(msg.sender).ToString().c_str());
    StdDebug("CEvmHost", "call: destination: %s", ToHexString(&(msg.destination.bytes[0]), sizeof(msg.destination.bytes)).c_str());
    StdDebug("CEvmHost", "call: input_data: [%lu] %s", msg.input_size, ToHexString(msg.input_data, msg.input_size).c_str());
    StdDebug("CEvmHost", "call: depth: %u", msg.depth);
    StdDebug("CEvmHost", "call: gas: %lu", msg.gas);

    CDestination to = AddressToDestination(msg.destination);
    if (isFunctionContractAddress(to))
    {
        uint64 nGasLeft = msg.gas;
        bytes btResult;
        bool fRet = false;

        if (msg.input_data && msg.input_size > 0)
        {
            CDestination from = AddressToDestination(msg.sender);
            bytes btData(msg.input_data, msg.input_data + msg.input_size);
            fRet = dbHost.ExecFunctionContract(from, to, btData, msg.gas, nGasLeft, btResult);
        }
        StdDebug("CEvmHost", "call: function contract exec, ret: %s, result: %s", (fRet ? "true" : "false"), ToHexString(btResult).c_str());

        uint8_t* p = nullptr;
        if (fRet && btResult.size() > 0)
        {
            p = new uint8_t[btResult.size()];
            memcpy(p, btResult.data(), btResult.size());
        }

        evmc_result evmcResult = {};
        evmcResult.status_code = (fRet ? EVMC_SUCCESS : EVMC_FAILURE);
        evmcResult.gas_left = nGasLeft;
        evmcResult.output_data = p;
        evmcResult.output_size = (fRet ? btResult.size() : 0);
        evmcResult.release = [](evmc_result const* _result) {
            if (_result->output_data)
            {
                delete[] _result->output_data;
            }
        };
        return evmc::result(evmcResult);
    }

    if (msg.input_size == 0)
    {
        CDestination from = AddressToDestination(msg.sender);
        uint256 amount(msg.value.bytes, sizeof(msg.value.bytes));
        amount.reverse();

        uint64 nGasLeft = msg.gas;
        if (!dbHost.ContractTransfer(from, to, amount, msg.gas, nGasLeft))
        {
            StdLog("CEvmHost", "call: Contract transfer fail, depth: %u, gas: %lu", msg.depth, msg.gas);
            return { EVMC_REVERT, nGasLeft, nullptr, 0 };
        }
        if (!dbHost.IsContractAddress(to))
        {
            return { EVMC_SUCCESS, nGasLeft, nullptr, 0 };
        }
    }

    // if (msg.depth >= 1024)
    // {
    //     StdLog("CEvmHost", "call: depth is out of range, depth: %u", msg.depth);
    //     return { EVMC_STACK_OVERFLOW, msg.gas, nullptr, 0 };
    // }

    if (msg.kind == evmc_call_kind::EVMC_CREATE || msg.kind == evmc_call_kind::EVMC_CREATE2)
    {
        return Create(msg);
    }

    return Call(msg);
}

evmc_tx_context CEvmHost::get_tx_context() const noexcept
{
    return tx_context;
}

evmc::bytes32 CEvmHost::get_block_hash(int64_t number) const noexcept
{
    uint256 hash = dbHost.GetBlockHash(number);
    if (hash == 0)
    {
        return {};
    }
    evmc::bytes32 out;
    memcpy(&(out.bytes[0]), hash.begin(), min(sizeof(out.bytes), (size_t)hash.size()));
    return out;
}

void CEvmHost::emit_log(const evmc::address& addr,
                        const uint8_t* data,
                        size_t data_size,
                        const evmc::bytes32 topics[],
                        size_t topics_count) noexcept
{
    CTransactionLogs logs;

    logs.address.SetBytes(&(addr.bytes[0]), sizeof(addr.bytes));
    //StdLog("CEvmHost", "emit_log: addr: %s", logs.address.ToString().c_str());
    if (data && data_size > 0)
    {
        logs.data.assign(data, data + data_size);
        //logs.data.SetBytes(data, data_size);
        //StdLog("CEvmHost", "emit_log: data: %s", logs.data.ToString().c_str());
    }
    for (size_t i = 0; i < topics_count; i++)
    {
        uint256 t;
        t.SetBytes(&(topics[i].bytes[0]), sizeof(topics[i].bytes));
        logs.topics.push_back(t);
        //StdLog("CEvmHost", "emit_log: topics: %s", t.ToString().c_str());
    }

    vLogs.push_back(logs);

    // logAddr = addr;
    // StdLog("CEvmHost", "emit_log: addr: %s", ToHexString(&(addr.bytes[0]), 32).c_str());
    // for (size_t i = 0; i < topics_count; i++)
    // {
    //     StdLog("CEvmHost", "emit_log: topics: %s", ToHexString(&(topics[i].bytes[0]), 32).c_str());
    //     vLogTopics.push_back(topics[i]);
    // }
    // if (data && data_size > 0)
    // {
    //     StdLog("CEvmHost", "emit_log: data: %s", ToHexString(data, data_size).c_str());
    //     vLogData.assign(data, data + data_size);
    // }
    // else
    // {
    //     StdLog("CEvmHost", "emit_log: data: null");
    // }
}

/////////////////////////////////////////
evmc::result CEvmHost::Create(const evmc_message& msg)
{
    StdDebug("CEvmHost", "Create: sender: %s - %s", ToHexString(&(msg.sender.bytes[0]), sizeof(msg.sender.bytes)).c_str(), AddressToDestination(msg.sender).ToString().c_str());

    uint256 hashContractCreateCode = crypto::CryptoHash(msg.input_data, msg.input_size);
    bytes btContractCreateCode(msg.input_data, msg.input_data + msg.input_size);
    CDestination destCodeOwner = dbHost.GetCodeOwnerAddress();
    CTxContractData txcd(btContractCreateCode, false);
    txcd.destCodeOwner = dbHost.GetCodeOwnerAddress();

    CDestination destNewContract;
    if (msg.kind == EVMC_CREATE)
    {
        StdDebug("CEvmHost", "Create EVMC_CREATE");
        Address _sender = EthFromEvmC(msg.sender);
        u256 _nonce = dbHost.GetTxNonce().Get64(); //m_s.getNonce(_sender);
        Address newAddress = createContractAddress(_sender, _nonce);
        destNewContract = destFromAddress(newAddress);
    }
    else
    {
        StdDebug("CEvmHost", "Create EVMC_CREATE2");
        Address _sender = EthFromEvmC(msg.sender);
        u256 salt = u256FromEvmC(msg.create2_salt);
        Address newAddress = createContractAddress(_sender, btContractCreateCode, salt);
        destNewContract = destFromAddress(newAddress);
    }
    StdDebug("CEvmHost", "destNewContract: %s", destNewContract.ToString().c_str());

    uint256 nNewBalance;
    if (dbHost.GetBalance(destNewContract, nNewBalance))
    {
        StdLog("CEvmHost", "Contract address exist, destNewContract: %s", destNewContract.ToString().c_str());
        return { EVMC_FAILURE, msg.gas, nullptr, 0 };
    }

    CVmHostFaceDBPtr ptrNewHostDB = dbHost.CloneHostDB(destNewContract);
    shared_ptr<CEvmHost> host(new CEvmHost(tx_context, *ptrNewHostDB, pCacheKv));
    evmc::address destination = DestinationToAddress(destNewContract);

    evmc_message new_msg{
        msg.kind,       // kind,
        0,              // flags,
        msg.depth,      // depth,
        msg.gas,        // gas,
        destination,    // destination,
        msg.sender,     // sender,
        msg.input_data, // input_data,
        msg.input_size, // input_size,
        msg.value,      // value,
        {}              // create2_salt
    };

    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();
    struct evmc_vm* vm = evmc_create_aleth_interpreter();

    StdDebug("CEvmHost", "Create contract: execute prev: gas: %ld", new_msg.gas);
    evmc::result result = evmc::result{ vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &new_msg, btContractCreateCode.data(), btContractCreateCode.size()) };
    StdDebug("CEvmHost", "Create contract: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
             new_msg.gas, result.gas_left, new_msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    ptrNewHostDB->SaveGasUsed(destCodeOwner, new_msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        if (result.output_data && result.output_size >= 32)
        {
            bytes btContractRunCode;
            std::map<uint256, bytes> mapCacheKv;
            //CTransactionLogs logs;

            if (result.output_data && result.output_size > 0)
            {
                btContractRunCode.assign(result.output_data, result.output_data + result.output_size);
            }
            host->GetCacheKv(mapCacheKv);
            // if (addressFromEvmC(host->logAddr) != ZeroAddress)
            // {
            //     logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
            // }
            // logs.data = host->vLogData;
            // for (auto& vd : host->vLogTopics)
            // {
            //     bytes btTopics;
            //     btTopics.assign(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes));
            //     logs.topics.push_back(btTopics);
            // }

            bytes btSaveCreateCode;
            if (FetchContractCreateCode(btContractCreateCode, btContractRunCode, btSaveCreateCode))
            {
                StdDebug("CEvmHost", "Create contract: save create code: %s", ToHexString(btSaveCreateCode).c_str());
                txcd.btCode = btSaveCreateCode;
            }
            else
            {
                StdLog("CEvmHost", "Create contract: fetch create code fail, run code size: %lu", btContractRunCode.size());
            }
            if (btContractRunCode.empty() || !ptrNewHostDB->SaveContractRunCode(destNewContract, btContractRunCode, txcd))
            {
                StdLog("CEvmHost", "Create contract: Save run code fail, destNewContract: %s", destNewContract.ToString().c_str());
                return { EVMC_FAILURE, result.gas_left, nullptr, 0 };
            }
            for (auto& kv : mapCacheKv)
            {
                StdDebug("CEvmHost", "Create contract: key: %s, value: %s",
                         kv.first.ToString().c_str(), ToHexString(kv.second).c_str());
            }
            ptrNewHostDB->SaveRunResult(destNewContract, host->vLogs, mapCacheKv);
            host->vLogs.clear();
        }
        else
        {
            StdLog("CEvmHost", "Create contract: output data error, output_size: %ld, destNewContract: %s", result.output_size, destNewContract.ToString().c_str());
            return { EVMC_INTERNAL_ERROR, result.gas_left, nullptr, 0 };
        }
    }
    else
    {
        StdLog("CEvmHost", "Create contract: Execute fail, status: [%ld] %s, destNewContract: %s", result.status_code, GetStatusInfo(result.status_code).c_str(), destNewContract.ToString().c_str());
        return result;
    }

    StdDebug("CEvmHost", "Create contract: Create success, destNewContract: %s, hashContractCreateCode: %s",
             destNewContract.ToString().c_str(), hashContractCreateCode.GetHex().c_str());

    result.create_address = DestinationToAddress(destNewContract);
    return result;
}

evmc::result CEvmHost::Call(const evmc_message& msg)
{
    StdDebug("CEvmHost", "Call: sender: %s - %s", ToHexString(&(msg.sender.bytes[0]), sizeof(msg.sender.bytes)).c_str(), AddressToDestination(msg.sender).ToString().c_str());
    StdDebug("CEvmHost", "Call: destination: %s - %s", ToHexString(&(msg.destination.bytes[0]), sizeof(msg.destination.bytes)).c_str(), AddressToDestination(msg.destination).ToString().c_str());
    StdDebug("CEvmHost", "Call: GetContractAddress: %s", dbHost.GetContractAddress().ToString().c_str());

    Address _addr = addressFromEvmC(msg.destination);
    bytes _out;
    uint64_t _usedgas = 0;
    bool _result = false;
    if (CPrecompliled::execute(_addr, bytesConstRef(msg.input_data, msg.input_size), _out, _usedgas, _result))
    {
        if (!_result)
        {
            StdLog("CEvmHost", "Call contract: Precompliled execute result is false, destination: %s", destFromAddress(_addr).ToString().c_str());
        }
        else
        {
            if (_out.size() == 0)
            {
                StdLog("CEvmHost", "Call contract: Precompliled execute out size is 0, destination: %s", destFromAddress(_addr).ToString().c_str());
            }
        }
        uint8_t* p = nullptr;
        if (msg.gas < _usedgas)
        {
            _result = false;
            _usedgas = msg.gas;
        }
        if (_result && _out.size() > 0)
        {
            p = new uint8_t[_out.size()];
            memcpy(p, _out.data(), _out.size());
        }
        else
        {
            _result = false;
        }

        evmc_result evmcResult = {};
        evmcResult.status_code = (_result ? EVMC_SUCCESS : EVMC_PRECOMPILE_FAILURE);
        evmcResult.gas_left = msg.gas - _usedgas;
        evmcResult.output_data = p;
        evmcResult.output_size = _out.size();
        evmcResult.release = [](evmc_result const* _result) {
            if (_result->output_data)
            {
                delete[] _result->output_data;
            }
        };
        return evmc::result(evmcResult);
    }

    CDestination destCodeContract = AddressToDestination(msg.destination);
    CDestination destContract = destCodeContract;
    evmc::address destination = msg.destination;

    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destCodeContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CEvmHost", "Call contract: Get contract run code fail, destCodeContract: %s", destCodeContract.ToString().c_str());
        return { EVMC_INTERNAL_ERROR, msg.gas, nullptr, 0 };
    }
    if (fDestroy)
    {
        StdLog("CEvmHost", "Call contract: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return { EVMC_REJECTED, msg.gas, nullptr, 0 };
    }

    CVmHostFaceDBPtr ptrNewHostDB;
    CVmHostFaceDB* pHostDB = nullptr;

    shared_ptr<CEvmHost> hostNew;
    CEvmHost* host = nullptr;

    if (msg.kind == EVMC_DELEGATECALL)
    {
        pHostDB = static_cast<CVmHostFaceDB*>(&dbHost);
        host = this;
        destContract = dbHost.GetContractAddress();
        destination = DestinationToAddress(destContract);
    }
    else
    {
        ptrNewHostDB = dbHost.CloneHostDB(destContract);
        hostNew = shared_ptr<CEvmHost>(new CEvmHost(tx_context, *ptrNewHostDB, pCacheKv));
        pHostDB = ptrNewHostDB.get();
        host = hostNew.get();
    }

    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();
    struct evmc_vm* vm = evmc_create_aleth_interpreter();

    evmc_message new_msg{
        msg.kind,       // kind,
        msg.flags,      // flags,
        msg.depth,      // depth,
        msg.gas,        // gas,
        destination,    // destination,
        msg.sender,     // sender,
        msg.input_data, // input_data,
        msg.input_size, // input_size,
        {},             // value,
        {}              // create2_salt
    };

    StdDebug("CEvmHost", "Call contract: execute depth: %d, prev: gas: %ld", msg.depth, msg.gas);
    evmc::result result = evmc::result{ vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &new_msg, btContractRunCode.data(), btContractRunCode.size()) };
    StdDebug("CEvmHost", "Call contract: execute depth: %d, last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
             msg.depth, msg.gas, result.gas_left, msg.gas - result.gas_left, destCodeOwner.ToString().c_str());

    if (msg.gas >= result.gas_left)
    {
        pHostDB->SaveGasUsed(destCodeOwner, msg.gas - result.gas_left);
    }

    if (result.status_code == EVMC_SUCCESS)
    {
        StdDebug("CEvmHost", "Call contract: execute success, sender: %s, destination: %s, value: %s, output_size: %ld, output_data: %s",
                 mtbase::ToHexString(msg.sender.bytes, sizeof(msg.sender.bytes)).c_str(),
                 mtbase::ToHexString(msg.destination.bytes, sizeof(msg.destination.bytes)).c_str(),
                 mtbase::ToHexString(msg.value.bytes, sizeof(msg.value.bytes)).c_str(),
                 result.output_size, ToHexString(result.output_data, result.output_size).c_str());

        //CTransactionLogs logs;
        std::map<uint256, bytes> mapCacheKv;

        host->GetCacheKv(mapCacheKv);
        // if (addressFromEvmC(host->logAddr) != ZeroAddress)
        // {
        //     logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
        // }
        // logs.data = host->vLogData;
        // for (auto& vd : host->vLogTopics)
        // {
        //     logs.topics.push_back(bytes(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes)));
        // }

        pHostDB->SaveRunResult(destContract, host->vLogs, mapCacheKv);
        host->vLogs.clear();
    }
    else
    {
        StdLog("CEvmHost", "Call contract: execute fail, err: [%ld] %s", result.status_code, GetStatusInfo(result.status_code).c_str());
    }
    return result;
}

} // namespace mvm
} // namespace metabasenet

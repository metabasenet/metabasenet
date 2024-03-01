// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmhost.h"

#include "block.h"
#include "evmc/loader.h"
#include "util.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace mvm
{

//////////////////////////////////
// CWasmHost

CWasmHost::CWasmHost(const evmc_tx_context& _tx_context, CVmHostFaceDB& dbHostIn)
  : tx_context(_tx_context), dbHost(dbHostIn)
{
}

CDestination CWasmHost::AddressToDestination(const evmc::address& addr)
{
    return CDestination(uint256(&(addr.bytes[0]), sizeof(addr.bytes)));
}

evmc::address CWasmHost::DestinationToAddress(const CDestination& dest)
{
    evmc::address addr;
    memcpy(&(addr.bytes[0]), dest.ToHash().begin(), sizeof(addr.bytes));
    return addr;
}

bool CWasmHost::account_exists(const evmc::address& addr) const noexcept
{
    CDestination dest = AddressToDestination(addr);
    uint256 hashBalance;
    if (!dbHost.GetBalance(dest, hashBalance))
    {
        StdLog("CWasmHost", "account_exists: Find fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
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
            StdLog("CWasmHost", "get_storage: Get fail, addr: %s, key: %s, hash: %s",
                   ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(),
                   ToHexString(&(key.bytes[0]), sizeof(key.bytes)).c_str(),
                   hash.GetHex().c_str());
            return {};
        }
        evmc::bytes32 value;
        if (vValue.size() == sizeof(value.bytes))
        {
            memcpy(value.bytes, vValue.data(), sizeof(value.bytes));
            StdLog("CWasmHost", "get_storage: Get success, addr: %s, key: %s, hash: %s, value: %s",
                   ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
                   ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
                   hash.GetHex().c_str(),
                   ToHexString(value.bytes, sizeof(value.bytes)).c_str());
            return value;
        }
        else
        {
            StdLog("CWasmHost", "get_storage: vValue size error, size: %ld, addr: %s",
                   vValue.size(), ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str());
            return {};
        }
    }
    else
    {
        StdLog("CWasmHost", "get_storage: Get cache success, addr: %s, key: %s, hash: %s, value: %s",
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
    StdLog("CWasmHost", "set_storage: addr: %s, key: %s, hash: %s, value: %s",
           ToHexString(addr.bytes, sizeof(addr.bytes)).c_str(),
           ToHexString(key.bytes, sizeof(key.bytes)).c_str(),
           hash.GetHex().c_str(),
           ToHexString(value.bytes, sizeof(value.bytes)).c_str());
    return EVMC_STORAGE_UNCHANGED;
}

evmc::uint256be CWasmHost::get_balance(const evmc::address& addr) const noexcept
{
    CDestination dest = AddressToDestination(addr);
    uint256 hashBalance;
    if (!dbHost.GetBalance(dest, hashBalance))
    {
        StdLog("CWasmHost", "get_balance: Get balance fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
        return {};
    }

    evmc::uint256be ret;
    memcpy(ret.bytes, hashBalance.begin(), hashBalance.size());
    std::reverse(std::begin(ret.bytes), std::end(ret.bytes));

    StdLog("CWasmHost", "get_balance: Get balance success, addr: %s, dest: %s, ret: %s",
           ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str(),
           ToHexString(&(ret.bytes[0]), sizeof(ret.bytes)).c_str());
    return ret;
}

size_t CWasmHost::get_code_size(const evmc::address& addr) const noexcept
{
    // CDestination dest = AddressToDestination(addr);
    // CTxContractData txcd;
    // if (!dbHost.GetContractCreateCode(dest, txcd))
    // {
    //     StdLog("CWasmHost", "get_code_size: Get contract create code fail, addr: %s, dest: %s",
    //            ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
    //     return 0;
    // }
    // return txcd.GetCode().size();
    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CWasmHost", "get_code_size: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return 0;
    }
    if (fDestroy)
    {
        StdLog("CWasmHost", "get_code_size: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return 0;
    }
    return btContractRunCode.size();
}

evmc::bytes32 CWasmHost::get_code_hash(const evmc::address& addr) const noexcept
{
    // CDestination dest = AddressToDestination(addr);
    // CTxContractData txcd;
    // if (!dbHost.GetContractCreateCode(dest, txcd))
    // {
    //     StdLog("CWasmHost", "get_code_hash: Get contract create code fail, addr: %s, dest: %s",
    //            ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
    //     return {};
    // }
    // uint256 hashContractCreateCode = txcd.GetContractCreateCodeHash();
    // evmc::bytes32 byOut = {};
    // memcpy(byOut.bytes, hashContractCreateCode.begin(), min(sizeof(byOut.bytes), (size_t)(hashContractCreateCode.size())));
    // return byOut;
    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CWasmHost", "get_code_hash: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return {};
    }
    if (fDestroy)
    {
        StdLog("CWasmHost", "get_code_hash: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return {};
    }
    evmc::bytes32 byOut = {};
    memcpy(byOut.bytes, hashContractRunCode.begin(), min(sizeof(byOut.bytes), (size_t)(hashContractRunCode.size())));
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

    // CDestination dest = AddressToDestination(addr);
    // CTxContractData txcd;
    // if (!dbHost.GetContractCreateCode(dest, txcd))
    // {
    //     StdLog("CWasmHost", "copy_code: Get contract src code fail, addr: %s, dest: %s",
    //            ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), dest.ToString().c_str());
    //     return {};
    // }
    // bytes btWasmCreateCode = txcd.GetCode();

    CDestination destContract = AddressToDestination(addr);
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CWasmHost", "copy_code: Get contract run code fail, addr: %s, dest: %s",
               ToHexString(&(addr.bytes[0]), sizeof(addr.bytes)).c_str(), destContract.ToString().c_str());
        return 0;
    }
    if (fDestroy)
    {
        StdLog("CWasmHost", "copy_code: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
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

void CWasmHost::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept
{
}

evmc::result CWasmHost::call(const evmc_message& msg) noexcept
{
    if (msg.input_size == 0)
    {
        CDestination from = AddressToDestination(msg.sender);
        CDestination to = AddressToDestination(msg.destination);
        uint256 amount(msg.value.bytes, sizeof(msg.value.bytes));
        amount.reverse();

        uint64 nGasLeft = msg.gas;
        if (!dbHost.ContractTransfer(from, to, amount, msg.gas, nGasLeft))
        {
            return { EVMC_REVERT, nGasLeft, nullptr, 0 };
        }
        return { EVMC_SUCCESS, nGasLeft, nullptr, 0 };
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
    uint256 hash = dbHost.GetBlockHash(number);
    if (hash == 0)
    {
        return {};
    }
    evmc::bytes32 out;
    memcpy(&(out.bytes[0]), hash.begin(), min(sizeof(out.bytes), (size_t)hash.size()));
    return out;
}

void CWasmHost::emit_log(const evmc::address& addr,
                         const uint8_t* data,
                         size_t data_size,
                         const evmc::bytes32 topics[],
                         size_t topics_count) noexcept
{
    CTransactionLogs logs;

    logs.address.SetBytes(&(addr.bytes[0]), sizeof(addr.bytes));
    StdLog("CWasmHost", "emit_log: addr: %s", logs.address.ToString().c_str());
    if (data && data_size > 0)
    {
        logs.data.assign(data, data + data_size);
        //logs.data.SetBytes(data, data_size);
        //StdLog("CWasmHost", "emit_log: data: %s", logs.data.ToString().c_str());
    }
    for (size_t i = 0; i < topics_count; i++)
    {
        uint256 t;
        t.SetBytes(&(topics[i].bytes[0]), sizeof(topics[i].bytes));
        logs.topics.push_back(t);
        StdLog("CWasmHost", "emit_log: topics: %s", t.ToString().c_str());
    }

    vLogs.push_back(logs);

    // logAddr = addr;
    // StdLog("CWasmHost", "emit_log: addr: %s", ToHexString(&(addr.bytes[0]), 32).c_str());
    // for (size_t i = 0; i < topics_count; i++)
    // {
    //     StdLog("CWasmHost", "emit_log: topics: %s", ToHexString(&(topics[i].bytes[0]), 32).c_str());
    //     vLogTopics.push_back(topics[i]);
    // }
    // if (data && data_size > 0)
    // {
    //     StdLog("CWasmHost", "emit_log: data: %s", ToHexString(data, data_size).c_str());
    //     vLogData.assign(data, data + data_size);
    // }
    // else
    // {
    //     StdLog("CWasmHost", "emit_log: data: null");
    // }
}

/////////////////////////////////////////
evmc::result CWasmHost::CreateContract3(const evmc_message& msg)
{
    CDestination from = AddressToDestination(msg.sender);
    uint256 hashSrcContract(msg.destination.bytes, sizeof(msg.destination.bytes));
    uint256 salt(msg.create2_salt.bytes, sizeof(msg.create2_salt.bytes));

    hashSrcContract.reverse();
    CDestination destSrcContract(hashSrcContract);

    CBufStream ss;
    ss << from << salt;
    uint256 hashNew = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    CDestination destNewWasm;
    destNewWasm.SetContractId(hashNew);

    CTxContractData txcd;
    if (!dbHost.GetContractCreateCode(destSrcContract, txcd))
    {
        StdLog("CWasmHost", "Create contract3: Get contract src code fail, destSrcContract: %s", destSrcContract.ToString().c_str());
        return { EVMC_INTERNAL_ERROR, msg.gas, nullptr, 0 };
    }
    uint256 hashContractCreateCode = txcd.GetContractCreateCodeHash();
    bytes btWasmCreateCode = txcd.GetCode();
    CDestination destCodeOwner = txcd.GetCodeOwner();

    CVmHostFaceDBPtr ptrNewHostDB = dbHost.CloneHostDB(destNewWasm);

    shared_ptr<CWasmHost> host(new CWasmHost(tx_context, *ptrNewHostDB));

    evmc::address destination = DestinationToAddress(destNewWasm);

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
            bytes btContractRunCode;
            std::map<uint256, bytes> mapCacheKv;
            //CTransactionLogs logs;

            btContractRunCode.assign(result.output_data, result.output_data + result.output_size);
            for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
            {
                mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
            }
            // for (size_t i = 0; i < sizeof(host->logAddr.bytes); i++)
            // {
            //     if (host->logAddr.bytes[i] != 0)
            //     {
            //         logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
            //         break;
            //     }
            // }
            // logs.data = host->vLogData;
            // for (auto& vd : host->vLogTopics)
            // {
            //     bytes btTopics;
            //     btTopics.assign(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes));
            //     logs.topics.push_back(btTopics);
            // }

            if (btContractRunCode.empty() || !ptrNewHostDB->SaveContractRunCode(destNewWasm, btContractRunCode, txcd))
            {
                StdLog("CWasmHost", "Create contract3: Save run code fail, destNewWasm: %s", destNewWasm.ToString().c_str());
                evmc::result ret(EVMC_INTERNAL_ERROR, result.gas_left, nullptr, 0);
                if (result.release)
                {
                    result.release(&result);
                }
                return ret;
            }
            ptrNewHostDB->SaveRunResult(destNewWasm, host->vLogs, mapCacheKv);
            host->vLogs.clear();
        }
        else
        {
            StdLog("CWasmHost", "Create contract3: output data error, output_size: %ld, destNewWasm: %s", result.output_size, destNewWasm.ToString().c_str());
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
        StdLog("CWasmHost", "Create contract3: Execute fail, status_code: %ld, destNewWasm: %s", result.status_code, destNewWasm.ToString().c_str());
        return evmc::result(result);
    }

    StdLog("CWasmHost", "Create contract3: Create success, destNewWasm: %s, hashContractCreateCode: %s",
           destNewWasm.ToString().c_str(), hashContractCreateCode.GetHex().c_str());

    evmc::result ret(result);
    ret.create_address = DestinationToAddress(destNewWasm);
    return ret;
}

evmc::result CWasmHost::CallContract(const evmc_message& msg)
{
    CDestination destContract = AddressToDestination(msg.destination);

    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!dbHost.GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("CWasmHost", "Call contract: Get contract run code fail, destContract: %s", destContract.ToString().c_str());
        return { EVMC_INTERNAL_ERROR, msg.gas, nullptr, 0 };
    }
    if (fDestroy)
    {
        StdLog("CWasmHost", "Call contract: Contract has been destroyed, destContract: %s", destContract.ToString().c_str());
        return { EVMC_REJECTED, msg.gas, nullptr, 0 };
    }

    CVmHostFaceDBPtr ptrNewHostDB = dbHost.CloneHostDB(destContract);

    shared_ptr<CWasmHost> host(new CWasmHost(tx_context, *ptrNewHostDB));

    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();
    struct evmc_vm* vm = evmc_load_and_create();

    StdLog("CWasmHost", "Call contract: execute prev: gas: %ld", msg.gas);
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg, btContractRunCode.data(), btContractRunCode.size());
    StdLog("CWasmHost", "Call contract: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
           msg.gas, result.gas_left, msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    ptrNewHostDB->SaveGasUsed(destCodeOwner, msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        StdLog("CWasmHost", "Call contract: execute success, sender: %s, destination: %s, value: %s, output_size: %ld, output_data: %s",
               mtbase::ToHexString(msg.sender.bytes, sizeof(msg.sender.bytes)).c_str(),
               mtbase::ToHexString(msg.destination.bytes, sizeof(msg.destination.bytes)).c_str(),
               mtbase::ToHexString(msg.value.bytes, sizeof(msg.value.bytes)).c_str(),
               result.output_size, ToHexString(result.output_data, result.output_size).c_str());

        //CTransactionLogs logs;
        std::map<uint256, bytes> mapCacheKv;

        for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
        {
            mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
        }
        // for (size_t i = 0; i < sizeof(host->logAddr.bytes); i++)
        // {
        //     if (host->logAddr.bytes[i] != 0)
        //     {
        //         logs.address.assign(&(host->logAddr.bytes[0]), &(host->logAddr.bytes[0]) + sizeof(host->logAddr.bytes));
        //         break;
        //     }
        // }
        // logs.data = host->vLogData;
        // for (auto& vd : host->vLogTopics)
        // {
        //     logs.topics.push_back(bytes(&(vd.bytes[0]), &(vd.bytes[0]) + sizeof(vd.bytes)));
        // }

        ptrNewHostDB->SaveRunResult(destContract, host->vLogs, mapCacheKv);
        host->vLogs.clear();
    }
    else
    {
        StdLog("CWasmHost", "Call contract: execute fail, err: %ld", result.status_code);
    }
    return evmc::result(result);
}

} // namespace mvm
} // namespace metabasenet

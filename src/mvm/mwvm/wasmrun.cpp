// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmrun.h"

#include "block.h"
#include "evmc/evmc.hpp"
#include "evmc/loader.h"
#include "wasmhost.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace mvm
{

//////////////////////////////
// CContractRun

bool CContractRun::RunContract(const CDestination& from, const CDestination& to, const CDestination& destContractIn, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                               const uint256& nGasPrice, const uint256& nTxAmount, const CDestination& destBlockMint, const uint64 nBlockTimestamp,
                               const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btContractCode, const bytes& btRunParam, const CTxContractData& txcd)
{
    evmc::address sender = CWasmHost::DestinationToAddress(from);

    evmc_tx_context c = {};
    memcpy(c.tx_gas_price.bytes, nGasPrice.begin(), min(sizeof(c.tx_gas_price.bytes), (size_t)(nGasPrice.size())));
    c.tx_origin = sender;
    c.block_coinbase = CWasmHost::DestinationToAddress(destBlockMint);
    c.block_timestamp = nBlockTimestamp;
    c.block_number = nBlockHeight;
    c.block_gas_limit = nBlockGasLimit;
    memcpy(c.block_difficulty.bytes, hashFork.begin(), min(sizeof(c.block_difficulty.bytes), (size_t)(hashFork.size())));
    memcpy(c.chain_id.bytes, hashFork.begin(), min(sizeof(c.chain_id.bytes), (size_t)(hashFork.size())));

    shared_ptr<CWasmHost> host(new CWasmHost(c, dbHost));
    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();

    evmc::address destination = CWasmHost::DestinationToAddress(destContractIn);

    evmc::uint256be amount;
    memcpy(amount.bytes, nTxAmount.begin(), min(sizeof(amount.bytes), (size_t)(nTxAmount.size())));

    StdLog("CContractRun", "Run contract: from: %s, to: %s, nTxAmount: %s, nTxGasLimit: %lu, btRunParam: %s",
           from.ToString().c_str(), to.ToString().c_str(), nTxAmount.GetHex().c_str(), nTxGasLimit, ToHexString(btRunParam).c_str());

    evmc_message msg{ EVMC_CALL,
                      0, // flags;
                      0, // The call depth;
                      nTxGasLimit,
                      destination,
                      sender,
                      btRunParam.data(),
                      btRunParam.size(),
                      amount,
                      {} };

    struct evmc_vm* vm = evmc_load_and_create();

    StdLog("CContractRun", "Run contract: execute prev: gas: %ld", msg.gas);
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg, btContractCode.data(), btContractCode.size());
    StdLog("CContractRun", "Run contract: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
           msg.gas, result.gas_left, msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    dbHost.SaveGasUsed(destCodeOwner, msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        StdLog("CContractRun", "Run contract: Run contract success, gas left: %ld, gas used: %ld, to: %s",
               result.gas_left, nTxGasLimit - result.gas_left, to.ToString().c_str());

        nStatusCode = result.status_code;
        vResult.assign(result.output_data, result.output_data + result.output_size);
        for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
        {
            mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
        }
        nGasLeft = result.gas_left;

        // CTransactionLogs logs;
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
        if (result.release)
        {
            result.release(&result);
        }

        if (to == 0)
        {
            if (vResult.empty() || !dbHost.SaveContractRunCode(destContractIn, vResult, txcd))
            {
                StdLog("CContractRun", "Run contract: Save contract run code fail, destContractIn: %s, hashContractCreateCode: %s, to: %s",
                       destContractIn.ToString().c_str(), txcd.GetContractCreateCodeHash().ToString().c_str(), to.ToString().c_str());

                nStatusCode = EVMC_INTERNAL_ERROR;
                vResult.clear();
                return false;
            }
        }
        dbHost.SaveRunResult(destContractIn, host->vLogs, mapCacheKv);
        host->vLogs.clear();
        return true;
    }

    StdLog("CContractRun", "Run contract: Run contract fail, status code: %ld, gas left: %ld, gas used: %ld, to: %s",
           result.status_code, result.gas_left, nTxGasLimit - result.gas_left, to.ToString().c_str());

    nStatusCode = result.status_code;
    nGasLeft = result.gas_left;
    if (result.release)
    {
        result.release(&result);
    }

    return false;
}

} // namespace mvm
} // namespace metabasenet

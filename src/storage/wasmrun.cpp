// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmrun.h"

#include <evmc/evmc.hpp>
#include <evmc/loader.h>

#include "block.h"
#include "wasmhost.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CWasmRun

bool CWasmRun::RunWasm(const uint256& from, const uint256& to, const uint256& hashWasmDest, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                       const uint256& nGasPrice, const uint256& nTxAmount, const uint256& hashBlockMintDest, const uint32 nBlockTimestamp,
                       const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btWasmCode, const bytes& btRunParam, const CTxContractData& txcd)
{
    evmc::address sender;
    memcpy(sender.bytes, from.begin(), min(sizeof(sender.bytes), (size_t)(from.size())));

    evmc_tx_context c;
    memcpy(c.tx_gas_price.bytes, nGasPrice.begin(), min(sizeof(c.tx_gas_price.bytes), (size_t)(nGasPrice.size())));
    c.tx_origin = sender;
    memcpy(c.block_coinbase.bytes, hashBlockMintDest.begin(), min(sizeof(c.block_coinbase.bytes), (size_t)(hashBlockMintDest.size())));
    c.block_timestamp = nBlockTimestamp;
    c.block_number = nBlockHeight;
    c.block_gas_limit = nBlockGasLimit;
    memcpy(c.block_difficulty.bytes, hashFork.begin(), min(sizeof(c.block_difficulty.bytes), (size_t)(hashFork.size())));
    memcpy(c.chain_id.bytes, hashFork.begin(), min(sizeof(c.chain_id.bytes), (size_t)(hashFork.size())));

    shared_ptr<CWasmHost> host(new CWasmHost(c, dbHost));
    evmc_host_context* context = host->to_context();
    const evmc_host_interface* host_interface = &evmc::Host::get_interface();

    evmc::address destination;
    memcpy(destination.bytes, hashWasmDest.begin(), min(sizeof(destination.bytes), (size_t)(hashWasmDest.size())));

    evmc::uint256be amount;
    memcpy(amount.bytes, nTxAmount.begin(), min(sizeof(amount.bytes), (size_t)(nTxAmount.size())));

    //StdLog("CWasmRun", "Run Wasm: nTxAmount: %s", nTxAmount.GetHex().c_str());

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

    StdLog("CWasmRun", "Run Wasm: execute prev: gas: %ld", msg.gas);
    evmc_result result = vm->execute(vm, host_interface, context, EVMC_MAX_REVISION, &msg, btWasmCode.data(), btWasmCode.size());
    StdLog("CWasmRun", "Run Wasm: execute last: gas: %ld, gas_left: %ld, gas used: %ld, owner: %s",
           msg.gas, result.gas_left, msg.gas - result.gas_left, destCodeOwner.ToString().c_str());
    dbHost.SaveGasUsed(destCodeOwner, msg.gas - result.gas_left);

    if (result.status_code == EVMC_SUCCESS)
    {
        StdLog("CWasmRun", "Run Wasm: Run wasm success, gas left: %ld, gas used: %ld, to: %s",
               result.gas_left, nTxGasLimit - result.gas_left, to.ToString().c_str());

        nStatusCode = result.status_code;
        vResult.assign(result.output_data, result.output_data + result.output_size);
        for (auto it = host->cacheKv.begin(); it != host->cacheKv.end(); ++it)
        {
            mapCacheKv.insert(make_pair(it->first, bytes(&(it->second.bytes[0]), &(it->second.bytes[0]) + sizeof(it->second.bytes))));
        }
        nGasLeft = result.gas_left;
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
        if (result.release)
        {
            result.release(&result);
        }

        if (to == 0)
        {
            if (!dbHost.SaveWasmRunCode(hashWasmDest, vResult, txcd))
            {
                StdLog("CWasmRun", "Run Wasm: Save wasm run code fail, hashWasmDest: %s, hashWasmCreateCode: %s, to: %s",
                       hashWasmDest.ToString().c_str(), txcd.GetWasmCreateCodeHash().ToString().c_str(), to.ToString().c_str());

                nStatusCode = EVMC_INTERNAL_ERROR;
                vResult.clear();
                return false;
            }
        }
        dbHost.SaveRunResult(hashWasmDest, logs, mapCacheKv);
        return true;
    }

    StdLog("CWasmRun", "Run Wasm: Run wasm fail, status code: %ld, gas left: %ld, gas used: %ld, to: %s",
           result.status_code, result.gas_left, nTxGasLimit - result.gas_left, to.ToString().c_str());

    nStatusCode = result.status_code;
    nGasLeft = result.gas_left;
    if (result.release)
    {
        result.release(&result);
    }

    return false;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "evmexec.h"

#include "../meth/libaleth-interpreter/interpreter.h"
#include "../meth/libevm/PithyEvmc.h"
#include "block.h"
#include "evmc/evmc.h"
#include "evmhost.h"
#include "mutil.h"

using namespace std;
using namespace mtbase;
using namespace dev::eth;
using namespace dev;

namespace metabasenet
{
namespace mvm
{

//////////////////////////////
CDestination CreateContractAddressByNonce(const CDestination& _sender, const uint256& _nonce)
{
    Address _out = createContractAddress(addressFromDest(_sender), u256(_nonce.Get64()));
    return destFromAddress(_out);
}

//////////////////////////////
// CEvmExec

bool CEvmExec::evmExec(const CDestination& from, const CDestination& to, const CDestination& destContractIn, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                       const uint256& nGasPrice, const uint256& nTxAmount, const CDestination& destBlockMint, const uint64 nBlockTimestamp,
                       const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btContractCode, const bytes& btRunParam, const CTxContractData& txcd)
{
    evmc::address sender = CEvmHost::DestinationToAddress(from);

    evmc_tx_context c = {};
    c.tx_gas_price = uint256ToEvmC(nGasPrice);
    c.tx_origin = sender;
    c.block_coinbase = CEvmHost::DestinationToAddress(destBlockMint);
    c.block_timestamp = nBlockTimestamp;
    c.block_number = nBlockHeight;
    c.block_gas_limit = nBlockGasLimit;
    memcpy(c.block_prev_randao.bytes, nAgreement.begin(), min(sizeof(c.block_prev_randao.bytes), (size_t)(nAgreement.size())));
    c.chain_id = uint256ToEvmC(uint256(static_cast<uint32>(chainId)));
    c.block_base_fee = uint256ToEvmC(MIN_GAS_PRICE);

    evmc::address destination = CEvmHost::DestinationToAddress(destContractIn);
    evmc::uint256be amount = uint256ToEvmC(nTxAmount);

    // StdDebug("CEvmExec", "Exec: from: %s, to: %s, nTxAmount: %s, nTxGasLimit: %lu, btRunParam: %s, btContractCode: %s",
    //          from.ToString().c_str(), to.ToString().c_str(), CoinToTokenBigFloat(nTxAmount).c_str(), nTxGasLimit,
    //          ToHexString(btRunParam).c_str(), ToHexString(btContractCode).c_str());

    // StdLog("CEvmExec", "evmc_tx_context: tx_gas_price: %s", ToHexString(&(c.tx_gas_price.bytes[0]), sizeof(c.tx_gas_price.bytes)).c_str());
    // StdLog("CEvmExec", "evmc_tx_context: tx_origin: %s", ToHexString(&(c.tx_origin.bytes[0]), sizeof(c.tx_origin.bytes)).c_str());
    // StdLog("CEvmExec", "evmc_tx_context: block_coinbase: %s", ToHexString(&(c.block_coinbase.bytes[0]), sizeof(c.block_coinbase.bytes)).c_str());
    // StdLog("CEvmExec", "evmc_tx_context: block_timestamp: %lu", c.block_timestamp);
    // StdLog("CEvmExec", "evmc_tx_context: block_number: %lu", c.block_number);
    // StdLog("CEvmExec", "evmc_tx_context: block_gas_limit: %lu", c.block_gas_limit);
    // StdLog("CEvmExec", "evmc_tx_context: block_prev_randao: %s", ToHexString(&(c.block_prev_randao.bytes[0]), sizeof(c.block_prev_randao.bytes)).c_str());
    // StdLog("CEvmExec", "evmc_tx_context: chain_id: %s", ToHexString(&(c.chain_id.bytes[0]), sizeof(c.chain_id.bytes)).c_str());
    // StdLog("CEvmExec", "evmc_tx_context: block_base_fee: %s", ToHexString(&(c.block_base_fee.bytes[0]), sizeof(c.block_base_fee.bytes)).c_str());

    // StdLog("CEvmExec", "destination: %s", ToHexString(&(destination.bytes[0]), sizeof(destination.bytes)).c_str());
    // StdLog("CEvmExec", "sender: %s", ToHexString(&(sender.bytes[0]), sizeof(sender.bytes)).c_str());
    // StdLog("CEvmExec", "amount: %s", ToHexString(&(amount.bytes[0]), sizeof(amount.bytes)).c_str());

    if (btContractCode.empty())
    {
        StdLog("CEvmExec", "Exec: Code is empty");
        nStatusCode = EVMC_FAILURE;
        nGasLeft = nTxGasLimit;
        return false;
    }

    CEvmHost host(c, dbHost);
    bool fCreate = to.IsNull();

    PVMPtr vm = PithyEVMC::createVm();
    if (vm == nullptr)
    {
        StdLog("CEvmExec", "Exec: Create vm fail");
        nStatusCode = EVMC_FAILURE;
        nGasLeft = nTxGasLimit;
        return false;
    }
    //evmc::result result = vm->exec(host, fCreate, nTxGasLimit, destination, sender, amount, btRunParam, btContractCode);

    evmc::result result = vm->exec(host, fCreate, nTxGasLimit * 2, destination, sender, amount, btRunParam, btContractCode);
    if (result.status_code != EVMC_SUCCESS)
    {
        if (result.gas_left < nTxGasLimit)
        {
            result.gas_left = 0;
        }
        else
        {
            result.gas_left -= nTxGasLimit;
        }
        uint64 nUsedGas = nTxGasLimit - result.gas_left;

        StdLog("CEvmExec", "Exec: from: %s, exec fail, status: [%ld] %s, tx amount: %s, call param: %s, gas limit: %ld, gas left: %ld, gas used: %ld, to: %s, out: %s",
               from.ToString().c_str(), result.status_code, GetStatusInfo(result.status_code).c_str(), CoinToTokenBigFloat(nTxAmount).c_str(),
               ToHexString(btRunParam).c_str(), nTxGasLimit, result.gas_left, nUsedGas, to.ToString().c_str(), ToHexString(result.output_data, result.output_size).c_str());

        dbHost.SaveGasUsed(destCodeOwner, nUsedGas);

        nStatusCode = result.status_code;
        if (result.output_data && result.output_size > 0)
        {
            vResult.assign(result.output_data, result.output_data + result.output_size);
        }
        else
        {
            vResult.clear();
        }
        nGasLeft = result.gas_left;
        return false;
    }
    if (result.gas_left < nTxGasLimit)
    {
        uint64 nNeedGas = nTxGasLimit * 2 - result.gas_left;
        StdLog("CEvmExec", "Exec: from: %s, execution has run out of gas, tx amount: %s, call param: %s, gas limit: %ld, need gas: %ld, to: %s",
               from.ToString().c_str(), CoinToTokenBigFloat(nTxAmount).c_str(), ToHexString(btRunParam).c_str(), nTxGasLimit, nNeedGas, to.ToString().c_str());

        dbHost.SaveGasUsed(destCodeOwner, nTxGasLimit);

        nStatusCode = EVMC_OUT_OF_GAS;
        nGasLeft = 0;
        return false;
    }
    result.gas_left -= nTxGasLimit;
    uint64 nUsedGas = nTxGasLimit - result.gas_left;

    StdDebug("CEvmExec", "Exec: from: %s, exec success, tx amount: %s, call param: %s, gas limit: %ld, gas left: %ld, gas used: %ld, to: %s, out: %s",
             from.ToString().c_str(), CoinToTokenBigFloat(nTxAmount).c_str(), ToHexString(btRunParam).c_str(), nTxGasLimit,
             result.gas_left, nUsedGas, to.ToString().c_str(), ToHexString(result.output_data, result.output_size).c_str());

    dbHost.SaveGasUsed(destCodeOwner, nUsedGas);
    nStatusCode = result.status_code;
    if (result.output_data && result.output_size > 0)
    {
        vResult.assign(result.output_data, result.output_data + result.output_size);
    }
    else
    {
        vResult.clear();
    }

    host.GetCacheKv(mapCacheKv);
    nGasLeft = result.gas_left;

    if (to == 0)
    {
        CTxContractData txcdNew = txcd;
        bytes btSaveCreateCode;
        if (FetchContractCreateCode(btContractCode, vResult, btSaveCreateCode))
        {
            StdDebug("CEvmExec", "Exec: save create code: %s", ToHexString(btSaveCreateCode).c_str());
            txcdNew.btCode = btSaveCreateCode;
        }
        else
        {
            StdLog("CEvmExec", "Exec: fetch create code fail, run code size: %lu", vResult.size());
        }
        if (vResult.empty() || !dbHost.SaveContractRunCode(destContractIn, vResult, txcdNew))
        {
            StdLog("CEvmExec", "Exec: Save contract run code fail, destContractIn: %s, hashContractCreateCode: %s, to: %s",
                   destContractIn.ToString().c_str(), txcd.GetContractCreateCodeHash().ToString().c_str(), to.ToString().c_str());

            nStatusCode = EVMC_INTERNAL_ERROR;
            vResult.clear();
            return false;
        }
    }
    dbHost.SaveRunResult(destContractIn, host.vLogs, mapCacheKv);
    host.vLogs.clear();
    return true;
}

} // namespace mvm
} // namespace metabasenet

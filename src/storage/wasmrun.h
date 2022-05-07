// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_WASMRUN_H
#define STORAGE_WASMRUN_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hcode.h"
#include "transaction.h"
#include "uint256.h"
#include "wasmbase.h"

namespace metabasenet
{
namespace storage
{

class CWasmRun
{
public:
    CWasmRun(CHostBaseDB& dbHostIn, const uint256& hashForkIn)
      : dbHost(dbHostIn), hashFork(hashForkIn) {}

    bool RunWasm(const uint256& from, const uint256& to, const uint256& hashWasmDest, const CDestination& destCodeOwner, const uint64 nTxGasLimit,
                 const uint256& nGasPrice, const uint256& nTxAmount, const uint256& hashBlockMintDest, const uint32 nBlockTimestamp,
                 const int nBlockHeight, const uint64 nBlockGasLimit, const bytes& btWasmCode, const bytes& btRunParam, const CTxContractData& txcd);

protected:
    CHostBaseDB& dbHost;
    uint256 hashFork;

public:
    uint64 nGasLeft;
    int nStatusCode;
    bytes vResult;
    CTransactionLogs logs;
    std::map<uint256, bytes> mapCacheKv;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_WASMRUN_H

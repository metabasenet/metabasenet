// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_WASMBASE_H
#define STORAGE_WASMBASE_H

#include "hcode.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CHostBaseDB;
typedef boost::shared_ptr<CHostBaseDB> CHostBaseDBPtr;

class CHostBaseDB
{
public:
    virtual ~CHostBaseDB(){};

    virtual bool Get(const uint256& key, bytes& value) const = 0;
    virtual bool GetBalance(const uint256& addr, uint256& balance) const = 0;
    virtual bool Transfer(const uint256& from, const uint256& to, const uint256& amount) = 0;

    virtual bool GetWasmRunCode(const uint256& hashWasm, uint256& hashWasmCreateCode, CDestination& destCodeOwner, bytes& btWasmRunCode) = 0;
    virtual bool GetWasmCreateCode(const uint256& hashWasm, CTxContractData& txcd) = 0;
    virtual CHostBaseDBPtr CloneHostDB(const uint256& hashWasm) = 0;
    virtual void SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed) = 0;
    virtual void SaveRunResult(const uint256& hashWasm, const CTransactionLogs& logs, const std::map<uint256, bytes>& mapCacheKv) = 0;
    virtual bool SaveWasmRunCode(const uint256& hashWasm, const bytes& btWasmRunCode, const CTxContractData& txcd) = 0;

protected:
    CHostBaseDB() = default;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_WASMBASE_H

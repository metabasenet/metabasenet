// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVM_VMHOSTFACE_H
#define MVM_VMHOSTFACE_H

#include "destination.h"
#include "mtbase.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{
namespace mvm
{

class CVmHostFaceDB;
typedef boost::shared_ptr<CVmHostFaceDB> CVmHostFaceDBPtr;

class CVmHostFaceDB
{
public:
    virtual ~CVmHostFaceDB(){};

    virtual bool Get(const uint256& key, bytes& value) const = 0;
    virtual uint256 GetTxid() const = 0;
    virtual uint256 GetTxNonce() const = 0;
    virtual CDestination GetContractAddress() const = 0;
    virtual CDestination GetCodeOwnerAddress() const = 0;
    virtual bool GetBalance(const CDestination& addr, uint256& balance) const = 0;
    virtual bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft) = 0;
    virtual uint256 GetBlockHash(const uint64 nBlockNumber) = 0;
    virtual bool IsContractAddress(const CDestination& addr) = 0;

    virtual bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy) = 0;
    virtual bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd) = 0;
    virtual CVmHostFaceDBPtr CloneHostDB(const CDestination& destContractIn) = 0;
    virtual void SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed) = 0;
    virtual void SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv) = 0;
    virtual bool SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd) = 0;
    virtual bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult) = 0;
    virtual bool Selfdestruct(const CDestination& destBeneficiaryIn) = 0;

protected:
    CVmHostFaceDB() = default;
};

} // namespace mvm
} // namespace metabasenet

#endif // MVM_VMHOSTFACE_H

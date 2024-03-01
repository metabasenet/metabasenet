// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVM_MEMVMHOST_H
#define MVM_MEMVMHOST_H

#include "vmhostface.h"

namespace metabasenet
{
namespace mvm
{

class CMemAddress
{
public:
    CMemAddress() {}

public:
    uint256 nBalance;
    std::map<uint256, bytes> mapKv;
};

class CMemVmHost : public CVmHostFaceDB
{
public:
    CMemVmHost(const CDestination& destContractIn = CDestination(), const CDestination& destCodeOwnerIn = CDestination())
      : destContract(destContractIn), destCodeOwner(destCodeOwnerIn) {}

    virtual bool Get(const uint256& key, bytes& value) const;
    virtual uint256 GetTxid() const;
    virtual uint256 GetTxNonce() const;
    virtual CDestination GetContractAddress() const;
    virtual CDestination GetCodeOwnerAddress() const;
    virtual bool GetBalance(const CDestination& addr, uint256& balance) const;
    virtual bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft);
    virtual uint256 GetBlockHash(const uint64 nBlockNumber);
    virtual bool IsContractAddress(const CDestination& addr);

    virtual bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy);
    virtual bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd);
    virtual CVmHostFaceDBPtr CloneHostDB(const CDestination& destContractIn);
    virtual void SaveGasUsed(const CDestination& destCodeOwnerIn, const uint64 nGasUsed);
    virtual void SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv);
    virtual bool SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd);
    virtual bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult);
    virtual bool Selfdestruct(const CDestination& destBeneficiaryIn);

protected:
    CDestination destContract;
    CDestination destCodeOwner;
    std::map<uint256, bytes> mapKv;
    std::map<CDestination, CMemAddress> mapState;
};

} // namespace mvm
} // namespace metabasenet

#endif // MVM_VMHOSEFACE_H

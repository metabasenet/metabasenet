// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "memvmhost.h"

namespace metabasenet
{
namespace mvm
{

bool CMemVmHost::Get(const uint256& key, bytes& value) const
{
    auto it = mapKv.find(key);
    if (it == mapKv.end())
    {
        return false;
    }
    value = it->second;
    return true;
}

uint256 CMemVmHost::GetTxid() const
{
    return uint256(1);
}

uint256 CMemVmHost::GetTxNonce() const
{
    return uint256(1);
}

CDestination CMemVmHost::GetContractAddress() const
{
    return destContract;
}

CDestination CMemVmHost::GetCodeOwnerAddress() const
{
    return destCodeOwner;
}

bool CMemVmHost::GetBalance(const CDestination& addr, uint256& balance) const
{
    auto it = mapState.find(addr);
    if (it != mapState.end())
    {
        balance = it->second.nBalance;
        return true;
    }
    return false;
}

bool CMemVmHost::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft)
{
    return false;
}

uint256 CMemVmHost::GetBlockHash(const uint64 nBlockNumber)
{
    return uint256();
}

bool CMemVmHost::IsContractAddress(const CDestination& addr)
{
    return false;
}

bool CMemVmHost::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    return false;
}

bool CMemVmHost::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    return false;
}

CVmHostFaceDBPtr CMemVmHost::CloneHostDB(const CDestination& destContractIn)
{
    return CVmHostFaceDBPtr(new CMemVmHost(destContractIn, destCodeOwner));
}

void CMemVmHost::SaveGasUsed(const CDestination& destCodeOwnerIn, const uint64 nGasUsed)
{
}

void CMemVmHost::SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv)
{
}

bool CMemVmHost::SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd)
{
    return true;
}

bool CMemVmHost::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult)
{
    return true;
}

bool CMemVmHost::Selfdestruct(const CDestination& destBeneficiaryIn)
{
    return true;
}

} // namespace mvm
} // namespace metabasenet

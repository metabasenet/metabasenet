// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CONSENSUS_H
#define METABASENET_CONSENSUS_H

#include "base.h"
#include "delegate.h"
#include "delegatevotesave.h"

namespace metabasenet
{

class CDelegateContext
{
public:
    CDelegateContext();
    CDelegateContext(const crypto::CKey& keyDelegateIn, const CDestination& destOwnerIn, const uint32 nRewardRatioIn);
    const CDestination GetDestination() const
    {
        return destDelegate;
    }
    bool BuildEnrollTx(CTransaction& tx, const int nBlockHeight, const int64 nTime, const uint256& hashFork, const CChainId nChainId, const bool fNeedSetBlspubkey, const std::vector<unsigned char>& vchData);

protected:
    CDestination destDelegate;
    crypto::CKey keyDelegate;
    uint32 nRewardRatio;
    CDestination destOwner;
    CTemplatePtr templDelegate;
};

class CConsensus : public IConsensus
{
public:
    CConsensus();
    ~CConsensus();
    void PrimaryUpdate(const CBlockChainUpdate& update, CDelegateRoutine& routine) override;
    bool AddNewDistribute(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchDistribute) override;
    bool AddNewPublish(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchPublish) override;
    void GetAgreement(int nTargetHeight, uint256& nAgreement, std::size_t& nWeight, std::vector<CDestination>& vBallot) override;
    void GetProof(int nTargetHeight, std::vector<unsigned char>& vchDelegateData) override;
    bool GetNextConsensus(CAgreementBlock& consParam) override;
    bool LoadConsensusData(int& nStartHeight, CDelegateRoutine& routine) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool LoadDelegateTx();
    bool GetInnerAgreement(int nTargetHeight, uint256& nAgreement, size_t& nWeight, vector<CDestination>& vBallot, bool& fCompleted);
    int64 GetAgreementWaitTime(int nTargetHeight);

protected:
    boost::mutex mutex;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    storage::CDelegateVoteSave datVoteSave;
    delegate::CDelegate delegate;
    std::map<CDestination, CDelegateContext> mapContext;
    CAgreementBlock cacheAgreementBlock;
};

} // namespace metabasenet

#endif //METABASENET_CONSENSUS_H

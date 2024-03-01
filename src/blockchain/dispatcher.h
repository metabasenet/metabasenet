// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_DISPATCHER_H
#define METABASENET_DISPATCHER_H

#include "base.h"
#include "peernet.h"

namespace metabasenet
{

class CDispatcher : public IDispatcher
{
public:
    CDispatcher();
    ~CDispatcher();
    Errno AddNewBlock(const CBlock& block, uint64 nNonce = 0) override;
    Errno AddNewTx(const uint256& hashFork, const CTransaction& tx, uint64 nNonce = 0) override;
    bool AddNewDistribute(const uint256& hashAnchor, const CDestination& dest,
                          const std::vector<unsigned char>& vchDistribute) override;
    bool AddNewPublish(const uint256& hashAnchor, const CDestination& dest,
                       const std::vector<unsigned char>& vchPublish) override;
    void SetConsensus(const CAgreementBlock& agreeBlock) override;
    void CheckAllSubForkLastBlock() override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    void UpdatePrimaryBlock(const CBlock& block, const CBlockChainUpdate& updateBlockChain, const uint64 nNonce);
    void ActivateFork(const uint256& hashFork, const uint64& nNonce);
    bool ProcessForkTx(const uint256& txid, const CTransaction& tx);
    void CheckSubForkLastBlock(const uint256& hashFork);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    IForkManager* pForkManager;
    IConsensus* pConsensus;
    IService* pService;
    IBlockMaker* pBlockMaker;
    network::INetChannel* pNetChannel;
    network::IBlockChannel* pBlockChannel;
    network::ICertTxChannel* pCertTxChannel;
    network::IUserTxChannel* pUserTxChannel;
    network::IDelegatedChannel* pDelegatedChannel;
    IDataStat* pDataStat;
};

} // namespace metabasenet

#endif //METABASENET_DISPATCHER_H

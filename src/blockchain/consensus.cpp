// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"

#include "block.h"
#include "template/delegate.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{

//////////////////////////////
// CDelegateContext

CDelegateContext::CDelegateContext()
{
}

CDelegateContext::CDelegateContext(const crypto::CKey& keyDelegateIn, const CDestination& destOwnerIn, const uint32 nRewardRatioIn)
  : keyDelegate(keyDelegateIn), destOwner(destOwnerIn), nRewardRatio(nRewardRatioIn)
{
    templDelegate = CTemplate::CreateTemplatePtr(new CTemplateDelegate(CDestination(keyDelegateIn.GetPubKey()), destOwner, nRewardRatioIn));
    destDelegate.SetTemplateId(templDelegate->GetTemplateId());
}

bool CDelegateContext::BuildEnrollTx(CTransaction& tx, const int nBlockHeight, const int64 nTime, const uint256& hashFork, const CChainId nChainId, const bool fNeedSetBlspubkey, const vector<unsigned char>& vchData)
{
    tx.SetNull();

    tx.SetTxType(CTransaction::TX_CERT);
    tx.SetChainId(nChainId);
    tx.SetNonce(nBlockHeight);
    tx.SetFromAddress(destDelegate);
    tx.SetToAddress(destDelegate);

    tx.AddTxData(CTransaction::DF_CERTTXDATA, vchData);

    if (fNeedSetBlspubkey)
    {
        CCryptoBlsKey blsKey;
        if (!keyDelegate.GetBlsKey(blsKey))
        {
            StdLog("CDelegateContext", "Build Enroll Tx: Get bls key fail");
            return false;
        }

        CBufStream ss;
        ss << blsKey.pubkey;
        bytes btKeyData;
        ss.GetData(btKeyData);

        tx.AddTxData(CTransaction::DF_BLSPUBKEY, btKeyData);
    }

    bytes btSigData;
    if (!keyDelegate.Sign(tx.GetSignatureHash(), btSigData))
    {
        StdLog("CDelegateContext", "Build Enroll Tx: Sign fail");
        return false;
    }
    tx.SetSignData(btSigData);
    return true;
}

//////////////////////////////
// CConsensus

CConsensus::CConsensus()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

CConsensus::~CConsensus()
{
}

bool CConsensus::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool");
        return false;
    }

    if (MintConfig()->nPeerType != NODE_TYPE_FORK)
    {
        for (auto& kv : MintConfig()->mapMint)
        {
            const uint256& keyMint = kv.first;
            const CDestination& destOwner = kv.second.first;
            const uint32 nRewardRatio = kv.second.second;

            crypto::CKey key;
            key.SetSecret(crypto::CCryptoKeyData(keyMint.begin(), keyMint.end()));

            CDelegateContext ctxt(key, destOwner, nRewardRatio);
            mapContext.insert(make_pair(ctxt.GetDestination(), ctxt));

            delegate.AddNewDelegate(ctxt.GetDestination());

            StdLog("CConsensus", "AddNew delegate : %s", ctxt.GetDestination().ToString().c_str());
        }
    }
    return true;
}

void CConsensus::HandleDeinitialize()
{
    mapContext.clear();

    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

bool CConsensus::HandleInvoke()
{
    boost::unique_lock<boost::mutex> lock(mutex);

    if (!delegate.Initialize())
    {
        Error("Failed to initialize delegate");
        return false;
    }

    if (!LoadDelegateTx())
    {
        Error("Failed to load delegate tx");
        return false;
    }

    if (!datVoteSave.Initialize(Config()->pathData))
    {
        Error("Failed to initialize vote data");
        return false;
    }

    if (!datVoteSave.Load(delegate))
    {
        Error("Failed to load vote data");
        return false;
    }

    return true;
}

void CConsensus::HandleHalt()
{
    boost::unique_lock<boost::mutex> lock(mutex);

    datVoteSave.Save(delegate);

    delegate.Deinitialize();
}

void CConsensus::PrimaryUpdate(const CBlockChainUpdate& update, CDelegateRoutine& routine)
{
    boost::unique_lock<boost::mutex> lock(mutex);

    //for (int i = update.vBlockAddNew.size() - 1; i >= 0; i--)
    if (!update.vBlockAddNew.empty())
    {
        uint256 hash = update.vBlockAddNew[update.vBlockAddNew.size() - 1].GetHash();
        int nBlockHeight = CBlock::GetBlockHeightByHash(hash);

        StdTrace("CConsensus", "Primary Update: Add new block, last height: %d, block: [%d] %s", update.nLastBlockHeight, nBlockHeight, hash.ToString().c_str());

        CDelegateEnrolled enrolled;
        if (!pBlockChain->GetBlockDelegateEnrolled(hash, enrolled))
        {
            StdError("CConsensus", "Primary Update: GetBlockDelegateEnrolled fail, hash: %s", hash.GetHex().c_str());
        }
        else
        {
            delegate::CDelegateEvolveResult result;
            delegate.Evolve(nBlockHeight, enrolled.mapWeight, enrolled.mapEnrollData, result, hash);

            std::map<CDestination, uint256> mapDelegateVote;
            if (!pBlockChain->GetBlockDelegateVote(hash, mapDelegateVote))
            {
                StdError("CConsensus", "Primary Update: Get block delegate vote fail, hash: %s", hash.GetHex().c_str());
            }
            else
            {
                map<pair<uint256, CDestination>, CDestination> mapSortVote;
                for (auto it = mapDelegateVote.begin(); it != mapDelegateVote.end(); ++it)
                {
                    if (it->second >= DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT)
                    {
                        mapSortVote.insert(make_pair(make_pair(it->second, it->first), it->first));
                    }
                }
                std::map<CDestination, uint256> mapValidDelegateVote;
                for (auto it = mapSortVote.rbegin(); it != mapSortVote.rend() && mapValidDelegateVote.size() < MAX_DELEGATE_THRESH; ++it)
                {
                    const CDestination& destDelegate = it->second;
                    if (enrolled.mapEnrollData.find(destDelegate) != enrolled.mapEnrollData.end() || mapContext.find(destDelegate) != mapContext.end())
                    {
                        mapValidDelegateVote.insert(make_pair(destDelegate, it->first.first));
                    }
                }

                for (auto it = result.mapEnrollData.begin(); it != result.mapEnrollData.end(); ++it)
                {
                    const CDestination& destDelegate = it->first;

                    auto mi = mapContext.find(destDelegate);
                    if (mi == mapContext.end())
                    {
                        StdTrace("CConsensus", "Primary Update: mapContext find fail, destDelegate: %s", destDelegate.ToString().c_str());
                        continue;
                    }
                    CDelegateContext& ctxDelegate = mi->second;

                    auto nt = mapValidDelegateVote.find(destDelegate);
                    if (nt == mapValidDelegateVote.end())
                    {
                        uint256 nVoteAmount;
                        auto ht = mapDelegateVote.find(destDelegate);
                        if (ht != mapDelegateVote.end())
                        {
                            nVoteAmount = ht->second;
                        }
                        StdTrace("CConsensus", "Primary Update: Not selected, Vote amount: %s, destDelegate: %s", CoinToTokenBigFloat(nVoteAmount).c_str(), destDelegate.ToString().c_str());
                        continue;
                    }
                    const uint256& nVoteAmount = nt->second;
                    StdTrace("CConsensus", "Primary Update: Vote amount: %s, destDelegate: %s", CoinToTokenBigFloat(nVoteAmount).c_str(), destDelegate.ToString().c_str());

                    bool fNeedSetBlspubkey = false;
                    uint384 blsPubkey;
                    if (!pBlockChain->RetrieveBlsPubkeyContext(pCoreProtocol->GetGenesisBlockHash(), hash, destDelegate, blsPubkey))
                    {
                        fNeedSetBlspubkey = true;
                    }

                    CTransaction tx;
                    if (ctxDelegate.BuildEnrollTx(tx, nBlockHeight, GetNetTime(), pCoreProtocol->GetGenesisBlockHash(), pCoreProtocol->GetGenesisChainId(), fNeedSetBlspubkey, it->second))
                    {
                        StdTrace("CConsensus", "Primary Update: Build enroll tx success, vote token: %s, destDelegate: %s, block: [%d] %s",
                                 CoinToTokenBigFloat(nVoteAmount).c_str(), (*it).first.ToString().c_str(),
                                 nBlockHeight, hash.GetHex().c_str());
                        routine.vEnrollTx.push_back(tx);
                    }
                    else
                    {
                        StdError("CConsensus", "Primary Update: Build enroll tx fail, vote token: %s, destDelegate: %s, block: [%d] %s",
                                 CoinToTokenBigFloat(nVoteAmount).c_str(), (*it).first.ToString().c_str(),
                                 nBlockHeight, hash.GetHex().c_str());
                    }
                }
            }

            int nDistributeTargetHeight = nBlockHeight + CONSENSUS_DISTRIBUTE_INTERVAL + 1;
            int nPublishTargetHeight = nBlockHeight + 1;

            StdTrace("CConsensus", "result.mapDistributeData size: %llu", result.mapDistributeData.size());
            for (map<CDestination, vector<unsigned char>>::iterator it = result.mapDistributeData.begin();
                 it != result.mapDistributeData.end(); ++it)
            {
                delegate.HandleDistribute(nDistributeTargetHeight, hash, (*it).first, (*it).second);
            }
            routine.vDistributeData.push_back(make_pair(hash, result.mapDistributeData));

            //if (i == 0 && result.mapPublishData.size() > 0)
            if (result.mapPublishData.size() > 0)
            {
                StdTrace("CConsensus", "result.mapPublishData size: %llu", result.mapPublishData.size());
                for (map<CDestination, vector<unsigned char>>::iterator it = result.mapPublishData.begin();
                     it != result.mapPublishData.end(); ++it)
                {
                    bool fCompleted = false;
                    delegate.HandlePublish(nPublishTargetHeight, result.hashDistributeOfPublish, (*it).first, (*it).second, fCompleted);
                    routine.fPublishCompleted = (routine.fPublishCompleted || fCompleted);
                }
                routine.mapPublishData = result.mapPublishData;
                routine.hashDistributeOfPublish = result.hashDistributeOfPublish;
            }

            routine.vEnrolledWeight.push_back(make_pair(hash, enrolled.mapWeight));
        }
    }
}

bool CConsensus::AddNewDistribute(const uint256& hashDistributeAnchor, const CDestination& destFrom, const vector<unsigned char>& vchDistribute)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    int nTargetHeight = CBlock::GetBlockHeightByHash(hashDistributeAnchor) + CONSENSUS_DISTRIBUTE_INTERVAL + 1;
    return delegate.HandleDistribute(nTargetHeight, hashDistributeAnchor, destFrom, vchDistribute);
}

bool CConsensus::AddNewPublish(const uint256& hashDistributeAnchor, const CDestination& destFrom, const vector<unsigned char>& vchPublish)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    int nTargetHeight = CBlock::GetBlockHeightByHash(hashDistributeAnchor) + CONSENSUS_DISTRIBUTE_INTERVAL + 1;
    bool fCompleted = false;
    return delegate.HandlePublish(nTargetHeight, hashDistributeAnchor, destFrom, vchPublish, fCompleted);
}

void CConsensus::GetAgreement(int nTargetHeight, uint256& nAgreement, size_t& nWeight, vector<CDestination>& vBallot)
{
    if (nTargetHeight >= CONSENSUS_INTERVAL)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        uint256 hashBlock;
        if (!pBlockChain->GetBlockHashByHeightSlot(pCoreProtocol->GetGenesisBlockHash(), nTargetHeight - CONSENSUS_DISTRIBUTE_INTERVAL - 1, 0, hashBlock))
        {
            Error("GetAgreement Get block hash by height slot error, distribution height: %d", nTargetHeight - CONSENSUS_DISTRIBUTE_INTERVAL - 1);
            return;
        }

        map<CDestination, size_t> mapBallot;
        delegate.GetAgreement(nTargetHeight, hashBlock, nAgreement, nWeight, mapBallot);

        if (nAgreement != 0 && mapBallot.size() > 0)
        {
            CDelegateEnrolled enrolled;
            if (!pBlockChain->GetBlockDelegateEnrolled(hashBlock, enrolled))
            {
                Error("GetAgreement CBlockChain::GetBlockDelegateEnrolled error, hash: %s", hashBlock.ToString().c_str());
                return;
            }
            uint256 nMoneySupply = pBlockChain->GetBlockMoneySupply(hashBlock);
            if (nMoneySupply == 0)
            {
                Error("GetAgreement GetBlockMoneySupply fail, hash: %s", hashBlock.ToString().c_str());
                return;
            }

            pCoreProtocol->GetDelegatedBallot(nTargetHeight, nAgreement, nWeight, mapBallot, enrolled.vecAmount, nMoneySupply, vBallot);
        }
    }
}

void CConsensus::GetProof(int nTargetHeight, vector<unsigned char>& vchDelegateData)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    delegate.GetProof(nTargetHeight, vchDelegateData);
}

bool CConsensus::GetNextConsensus(CAgreementBlock& consParam)
{
    boost::unique_lock<boost::mutex> lock(mutex);

    consParam.nWaitTime = 1;
    consParam.ret = false;

    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        Error("GetNextConsensus CBlockChain::GetLastBlock fail");
        return false;
    }
    consParam.hashPrev = status.hashBlock;
    consParam.nPrevTime = status.nBlockTime;
    consParam.nPrevHeight = status.nBlockHeight;
    consParam.nPrevNumber = status.nBlockNumber;
    consParam.nPrevMintType = status.nMintType;
    consParam.agreement.Clear();

    int64 nNextBlockTime = pCoreProtocol->GetNextBlockTimestamp(status.nBlockTime);
    consParam.nWaitTime = nNextBlockTime - GetNetTime();
    // if (consParam.nWaitTime >= -60)
    // {
    //     int64 nAgreementWaitTime = GetAgreementWaitTime(status.nBlockHeight + 1);
    //     if (nAgreementWaitTime > 0 && consParam.nWaitTime < nAgreementWaitTime)
    //     {
    //         consParam.nWaitTime = nAgreementWaitTime;
    //     }
    // }
    if (consParam.nWaitTime > 0)
    {
        return false;
    }

    if (status.hashBlock != cacheAgreementBlock.hashPrev)
    {
        if (!GetInnerAgreement(status.nBlockHeight + 1, consParam.agreement.nAgreement, consParam.agreement.nWeight,
                               consParam.agreement.vBallot, consParam.fCompleted))
        {
            Error("GetNextConsensus GetInnerAgreement fail");
            return false;
        }
        cacheAgreementBlock = consParam;
    }
    else
    {
        if (cacheAgreementBlock.agreement.IsProofOfWork() && !cacheAgreementBlock.fCompleted)
        {
            if (!GetInnerAgreement(status.nBlockHeight + 1, cacheAgreementBlock.agreement.nAgreement, cacheAgreementBlock.agreement.nWeight,
                                   cacheAgreementBlock.agreement.vBallot, cacheAgreementBlock.fCompleted))
            {
                Error("GetNextConsensus GetInnerAgreement fail");
                return false;
            }
            if (!cacheAgreementBlock.agreement.IsProofOfWork())
            {
                StdDebug("CConsensus", "GetNextConsensus: consensus change pos, target height: %d", cacheAgreementBlock.nPrevHeight + 1);
            }
        }
        consParam = cacheAgreementBlock;
        consParam.nWaitTime = nNextBlockTime - GetNetTime();
    }
    if (!cacheAgreementBlock.agreement.IsProofOfWork())
    {
        nNextBlockTime = pCoreProtocol->GetNextBlockTimestamp(status.nBlockTime);
        consParam.nWaitTime = nNextBlockTime - GetNetTime();
        if (consParam.nWaitTime > 0)
        {
            return false;
        }
    }
    consParam.ret = true;
    return true;
}

bool CConsensus::LoadConsensusData(int& nStartHeight, CDelegateRoutine& routine)
{
    int nLashBlockHeight = pBlockChain->GetBlockCount(pCoreProtocol->GetGenesisBlockHash()) - 1;
    nStartHeight = nLashBlockHeight - CONSENSUS_DISTRIBUTE_INTERVAL;
    if (nStartHeight < 1)
    {
        nStartHeight = 1;
    }

    for (int i = nStartHeight; i <= nLashBlockHeight; i++)
    {
        uint256 hashBlock;
        if (!pBlockChain->GetBlockHashByHeightSlot(pCoreProtocol->GetGenesisBlockHash(), i, 0, hashBlock))
        {
            StdError("CConsensus", "LoadConsensusData: Get block hash by height slot fail, height: %d", i);
            return false;
        }

        CDelegateEnrolled enrolled;
        if (!pBlockChain->GetBlockDelegateEnrolled(hashBlock, enrolled))
        {
            StdError("CConsensus", "LoadConsensusData: GetBlockDelegateEnrolled fail, height: %d, block: %s", i, hashBlock.GetHex().c_str());
            return false;
        }

        delegate::CDelegateEvolveResult result;
        delegate.GetEvolveData(i, result, hashBlock);

        routine.vDistributeData.push_back(make_pair(hashBlock, result.mapDistributeData));
        if (i == nLashBlockHeight && result.mapPublishData.size() > 0)
        {
            routine.mapPublishData = result.mapPublishData;
            routine.hashDistributeOfPublish = result.hashDistributeOfPublish;
        }

        routine.vEnrolledWeight.push_back(make_pair(hashBlock, enrolled.mapWeight));
    }

    return true;
}

bool CConsensus::LoadDelegateTx()
{
    return true;
}

bool CConsensus::GetInnerAgreement(int nTargetHeight, uint256& nAgreement, size_t& nWeight, vector<CDestination>& vBallot, bool& fCompleted)
{
    if (nTargetHeight >= CONSENSUS_INTERVAL)
    {
        uint256 hashBlock;
        if (!pBlockChain->GetBlockHashByHeightSlot(pCoreProtocol->GetGenesisBlockHash(), nTargetHeight - CONSENSUS_DISTRIBUTE_INTERVAL - 1, 0, hashBlock))
        {
            Error("GetAgreement: Get block hash by height slot error, distribution height: %d", nTargetHeight - CONSENSUS_DISTRIBUTE_INTERVAL - 1);
            return false;
        }

        fCompleted = delegate.IsCompleted(nTargetHeight);

        map<CDestination, size_t> mapBallot;
        delegate.GetAgreement(nTargetHeight, hashBlock, nAgreement, nWeight, mapBallot);

        if (nAgreement != 0 && mapBallot.size() > 0)
        {
            CDelegateEnrolled enrolled;
            if (!pBlockChain->GetBlockDelegateEnrolled(hashBlock, enrolled))
            {
                Error("GetAgreement CBlockChain::GetBlockDelegateEnrolled error, hash: %s", hashBlock.ToString().c_str());
                return false;
            }
            uint256 nMoneySupply = pBlockChain->GetBlockMoneySupply(hashBlock);
            if (nMoneySupply == 0)
            {
                Error("GetAgreement GetBlockMoneySupply fail, hash: %s", hashBlock.ToString().c_str());
                return false;
            }
            pCoreProtocol->GetDelegatedBallot(nTargetHeight, nAgreement, nWeight, mapBallot, enrolled.vecAmount, nMoneySupply, vBallot);
        }
    }
    else
    {
        fCompleted = true;
    }
    return true;
}

int64 CConsensus::GetAgreementWaitTime(int nTargetHeight)
{
    if (nTargetHeight >= CONSENSUS_INTERVAL)
    {
        int64 nPublishedTime = delegate.GetPublishedTime(nTargetHeight);
        if (nPublishedTime <= 0)
        {
            return -1;
        }
        return nPublishedTime + WAIT_AGREEMENT_PUBLISH_TIMEOUT - GetTime();
    }
    return 0;
}

} // namespace metabasenet

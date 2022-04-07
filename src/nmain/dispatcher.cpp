// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dispatcher.h"

#include <chrono>
#include <future>
#include <thread>

#include "event.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{

//////////////////////////////
// CDispatcher

CDispatcher::CDispatcher()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
    pConsensus = nullptr;
    pService = nullptr;
    pBlockMaker = nullptr;
    pNetChannel = nullptr;
    pDelegatedChannel = nullptr;
    pDataStat = nullptr;
}

CDispatcher::~CDispatcher()
{
}

bool CDispatcher::HandleInitialize()
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

    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }

    if (!GetObject("consensus", pConsensus))
    {
        Error("Failed to request consensus");
        return false;
    }

    if (!GetObject("service", pService))
    {
        Error("Failed to request service");
        return false;
    }

    if (!GetObject("blockmaker", pBlockMaker))
    {
        Error("Failed to request blockmaker");
        return false;
    }

    if (!GetObject("netchannel", pNetChannel))
    {
        Error("Failed to request netchannel");
        return false;
    }

    if (!GetObject("delegatedchannel", pDelegatedChannel))
    {
        Error("Failed to request delegatedchanne");
        return false;
    }

    if (!GetObject("datastat", pDataStat))
    {
        Error("Failed to request datastat");
        return false;
    }
    strCmd = dynamic_cast<const CBasicConfig*>(Config())->strBlocknotify;
    return true;
}

void CDispatcher::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
    pConsensus = nullptr;
    pService = nullptr;
    pBlockMaker = nullptr;
    pNetChannel = nullptr;
    pDelegatedChannel = nullptr;
    pDataStat = nullptr;
}

bool CDispatcher::HandleInvoke()
{
    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        Error("Failed to get last block");
        return false;
    }

    vector<uint256> vActive;
    if (!pForkManager->GetActiveFork(vActive))
    {
        Error("Failed to load for context");
        return false;
    }
    for (const uint256& hashFork : vActive)
    {
        ActivateFork(hashFork, 0);
    }

    CDelegateRoutine routine;
    int nStartHeight = 0;
    if (pConsensus->LoadConsensusData(nStartHeight, routine) && !routine.vEnrolledWeight.empty())
    {
        pDelegatedChannel->PrimaryUpdate(nStartHeight - 1,
                                         routine.vEnrolledWeight, routine.vDistributeData,
                                         routine.mapPublishData, routine.hashDistributeOfPublish);
    }
    return true;
}

void CDispatcher::HandleHalt()
{
}

Errno CDispatcher::AddNewBlock(const CBlock& block, uint64 nNonce)
{
    Errno err = OK;
    if (!pBlockChain->Exists(block.hashPrev))
    {
        StdError("Dispatcher", "Add New Block: prev block not exist, block: %s, prev: %s", block.GetHash().GetHex().c_str(), block.hashPrev.GetHex().c_str());
        return ERR_MISSING_PREV;
    }

    CBlockChainUpdate updateBlockChain;
    if (!block.IsOrigin())
    {
        err = pBlockChain->AddNewBlock(block, updateBlockChain);
        if (err == OK)
        {
            if (!nNonce)
            {
                pDataStat->AddBlockMakerStatData(updateBlockChain.hashFork, block.IsProofOfWork(), block.vtx.size());
            }
            else
            {
                pDataStat->AddP2pSynRecvStatData(updateBlockChain.hashFork, 1, block.vtx.size());
            }
        }
    }
    else
    {
        err = pBlockChain->AddNewOrigin(block, updateBlockChain);
    }

    if (err != OK || updateBlockChain.IsNull())
    {
        return err;
    }

    if (!pTxPool->SynchronizeBlockChain(updateBlockChain))
    {
        StdError("Dispatcher", "Add New Block: TxPool Synchronize block chain fail, block: %s", block.GetHash().GetHex().c_str());
        return ERR_SYS_DATABASE_ERROR;
    }

    if (!block.IsOrigin())
    {
        pNetChannel->BroadcastBlockInv(updateBlockChain.hashFork, block.GetHash());
        pDataStat->AddP2pSynSendStatData(updateBlockChain.hashFork, 1, block.vtx.size());
    }

    if (!block.IsVacant())
    {
        vector<uint256> vActive, vDeactive;
        pForkManager->ForkUpdate(updateBlockChain, vActive, vDeactive);

        for (const uint256& hashFork : vActive)
        {
            ActivateFork(hashFork, nNonce);
        }

        for (const uint256& hashFork : vDeactive)
        {
            pNetChannel->UnsubscribeFork(hashFork);
        }
    }

    if (block.IsPrimary())
    {
        UpdatePrimaryBlock(block, updateBlockChain, nNonce);
    }

    return OK;
}

Errno CDispatcher::AddNewTx(const CTransaction& tx, uint64 nNonce)
{
    Errno err = OK;
    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        StdError("Dispatcher", "AddNewTx: GetLastBlock fail, fork: %s", pCoreProtocol->GetGenesisBlockHash().GetHex().c_str());
        return ERR_NOT_FOUND;
    }

    err = pCoreProtocol->ValidateTransaction(tx, status.nBlockHeight);
    if (err != OK)
    {
        StdError("Dispatcher", "AddNewTx: ValidateTransaction fail, txid: %s", tx.GetHash().GetHex().c_str());
        return err;
    }

    err = pTxPool->Push(tx);
    if (err != OK)
    {
        StdError("Dispatcher", "AddNewTx: TxPool Push fail, txid: %s", tx.GetHash().GetHex().c_str());
        return err;
    }

    pDataStat->AddP2pSynTxSynStatData(tx.hashFork, !!nNonce);
    if (!nNonce)
    {
        pNetChannel->BroadcastTxInv(tx.hashFork);
    }
    return OK;
}

bool CDispatcher::AddNewDistribute(const uint256& hashAnchor, const CDestination& dest, const vector<unsigned char>& vchDistribute)
{
    return pConsensus->AddNewDistribute(hashAnchor, dest, vchDistribute);
}

bool CDispatcher::AddNewPublish(const uint256& hashAnchor, const CDestination& dest, const vector<unsigned char>& vchPublish)
{
    return pConsensus->AddNewPublish(hashAnchor, dest, vchPublish);
}

void CDispatcher::SetConsensus(const CAgreementBlock& agreeBlock)
{
    CConsensusParam consParam;
    consParam.hashPrev = agreeBlock.hashPrev;
    consParam.nPrevTime = agreeBlock.nPrevTime;
    consParam.nPrevHeight = agreeBlock.nPrevHeight;
    consParam.nPrevNumber = agreeBlock.nPrevNumber;
    consParam.nPrevMintType = agreeBlock.nPrevMintType;
    consParam.nWaitTime = agreeBlock.nWaitTime;
    consParam.fPow = agreeBlock.agreement.IsProofOfWork();
    consParam.ret = agreeBlock.ret;
    pNetChannel->SubmitCachePowBlock(consParam);
}

void CDispatcher::CheckAllSubForkLastBlock()
{
    map<uint256, CForkStatus> mapForkStatus;
    pBlockChain->GetForkStatus(mapForkStatus);
    for (map<uint256, CForkStatus>::iterator it = mapForkStatus.begin(); it != mapForkStatus.end(); ++it)
    {
        const uint256& hashFork = it->first;
        if (hashFork != pCoreProtocol->GetGenesisBlockHash() && pForkManager->IsAllowed(hashFork))
        {
            CheckSubForkLastBlock(hashFork);
        }
    }
}

////////////////////////////////
void CDispatcher::UpdatePrimaryBlock(const CBlock& block, const CBlockChainUpdate& updateBlockChain, const uint64& nNonce)
{
    if (!strCmd.empty())
    {
        std::string cmd = strCmd;
        std::string block_hash = " " + updateBlockChain.hashFork.GetHex();
        for (auto ite = updateBlockChain.vBlockAddNew.rbegin(); ite != updateBlockChain.vBlockAddNew.rend(); ++ite)
        {
            block_hash += " " + ite->GetHash().GetHex();
        }
        cmd += block_hash;
        static std::future<int> fut;
        fut = std::async(std::launch::async, [cmd]()
                         { return ::system(cmd.c_str()); });
    }

    CDelegateRoutine routineDelegate;
    pConsensus->PrimaryUpdate(updateBlockChain, routineDelegate);

    pDelegatedChannel->PrimaryUpdate(updateBlockChain.nLastBlockHeight - updateBlockChain.vBlockAddNew.size(),
                                     routineDelegate.vEnrolledWeight, routineDelegate.vDistributeData,
                                     routineDelegate.mapPublishData, routineDelegate.hashDistributeOfPublish);

    for (const CTransaction& tx : routineDelegate.vEnrollTx)
    {
        Errno err = AddNewTx(tx, nNonce);
        if (err == OK)
        {
            StdLog("Dispatcher", "Send DelegateTx success, txid: %s.",
                   tx.GetHash().GetHex().c_str());
        }
        else
        {
            StdLog("Dispatcher", "Send DelegateTx fail, err: [%d] %s, txid: %s.",
                   err, ErrorString(err), tx.GetHash().GetHex().c_str());
        }
    }

    CEventBlockMakerUpdate* pBlockMakerUpdate = new CEventBlockMakerUpdate(0);
    if (pBlockMakerUpdate != nullptr)
    {
        CProofOfSecretShare proof;
        proof.Load(block.vchProof);
        pBlockMakerUpdate->data.hashParent = updateBlockChain.hashParent;
        pBlockMakerUpdate->data.nOriginHeight = updateBlockChain.nOriginHeight;
        pBlockMakerUpdate->data.hashPrevBlock = updateBlockChain.hashPrevBlock;
        pBlockMakerUpdate->data.hashBlock = updateBlockChain.hashLastBlock;
        pBlockMakerUpdate->data.nBlockTime = updateBlockChain.nLastBlockTime;
        pBlockMakerUpdate->data.nBlockHeight = updateBlockChain.nLastBlockHeight;
        pBlockMakerUpdate->data.nBlockNumber = updateBlockChain.nLastBlockNumber;
        pBlockMakerUpdate->data.nAgreement = proof.nAgreement;
        pBlockMakerUpdate->data.nWeight = proof.nWeight;
        pBlockMakerUpdate->data.nMintType = block.txMint.nType;
        pBlockMaker->PostEvent(pBlockMakerUpdate);
    }
}

void CDispatcher::ActivateFork(const uint256& hashFork, const uint64& nNonce)
{
    StdLog("Dispatcher", "Activating fork %s ...", hashFork.GetHex().c_str());
    if (!pBlockChain->Exists(hashFork))
    {
        CForkContext ctxt;
        if (!pBlockChain->GetForkContext(hashFork, ctxt))
        {
            Warn("Failed to find fork context %s", hashFork.GetHex().c_str());
            return;
        }

        if (ctxt.hashParent != 0)
        {
            uint256 hashJointBlock;
            int64 nJointTime;
            if (!pBlockChain->GetLastBlockOfHeight(ctxt.hashParent,
                                                   CBlock::GetBlockHeightByHash(ctxt.hashJoint),
                                                   hashJointBlock, nJointTime)
                || hashJointBlock != ctxt.hashJoint)
            {
                return;
            }
        }

        CTransaction txFork;
        if (!pBlockChain->GetTransaction(pCoreProtocol->GetGenesisBlockHash(), uint256(), ctxt.txidEmbedded, txFork))
        {
            Warn("Failed to find tx fork %s", hashFork.GetHex().c_str());
            return;
        }

        if (!ProcessForkTx(ctxt.txidEmbedded, txFork))
        {
            return;
        }
        StdLog("Dispatcher", "Add origin block in tx (%s), hash=%s", ctxt.txidEmbedded.GetHex().c_str(),
               hashFork.GetHex().c_str());
    }
    pNetChannel->SubscribeFork(hashFork, nNonce);
    StdLog("Dispatcher", "Activated fork %s ...", hashFork.GetHex().c_str());
}

bool CDispatcher::ProcessForkTx(const uint256& txid, const CTransaction& tx)
{
    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        Warn("Invalid tx data in tx (%s)", txid.GetHex().c_str());
        return false;
    }
    CBlock block;
    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        Warn("Invalid orign block found in tx (%s)", txid.GetHex().c_str());
        return false;
    }
    if (!block.IsOrigin() || block.IsPrimary())
    {
        Warn("invalid block, tx (%s)", txid.GetHex().c_str());
        return false;
    }

    Errno err = AddNewBlock(block);
    if (err != OK)
    {
        StdLog("Dispatcher", "Add origin block in tx (%s) failed : %s", txid.GetHex().c_str(),
               ErrorString(err));
        return false;
    }
    return true;
}

void CDispatcher::CheckSubForkLastBlock(const uint256& hashFork)
{
    CBlockChainUpdate updateBlockChain;
    if (pBlockChain->CheckForkValidLast(hashFork, updateBlockChain) && !updateBlockChain.IsNull())
    {
        if (!pTxPool->SynchronizeBlockChain(updateBlockChain))
        {
            StdError("Dispatcher", "CheckSubForkLastBlock: TxPool synchronize block chain fail, last block: %s", updateBlockChain.hashLastBlock.GetHex().c_str());
        }

        for (auto& block : updateBlockChain.vBlockAddNew)
        {
            if (!block.IsOrigin())
            {
                pNetChannel->BroadcastBlockInv(updateBlockChain.hashFork, block.GetHash());
                pDataStat->AddP2pSynSendStatData(updateBlockChain.hashFork, 1, block.vtx.size());
            }
        }

        vector<uint256> vActive, vDeactive;
        pForkManager->ForkUpdate(updateBlockChain, vActive, vDeactive);
        for (const uint256& hashFork : vActive)
        {
            ActivateFork(hashFork, 0);
        }
        for (const uint256& hashFork : vDeactive)
        {
            pNetChannel->UnsubscribeFork(hashFork);
        }
    }
}

} // namespace metabasenet

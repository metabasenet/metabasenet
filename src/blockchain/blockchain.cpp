// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockchain.h"

#include "delegatecomm.h"
#include "delegateverify.h"
#include "param.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/vote.h"

using namespace std;
using namespace hcode;

#define ENROLLED_CACHE_COUNT (120)
#define AGREEMENT_CACHE_COUNT (16)

namespace metabasenet
{

//////////////////////////////
// CBlockChain

CBlockChain::CBlockChain()
  : cacheEnrolled(ENROLLED_CACHE_COUNT), cacheAgreement(AGREEMENT_CACHE_COUNT)
{
    pCoreProtocol = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
}

CBlockChain::~CBlockChain()
{
}

bool CBlockChain::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        StdError("BlockChain", "Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        StdError("BlockChain", "Failed to request txpool");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        StdError("BlockChain", "Failed to request forkmanager");
        return false;
    }

    return true;
}

void CBlockChain::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
}

bool CBlockChain::HandleInvoke()
{
    CBlock blockGenesis;
    pCoreProtocol->GetGenesisBlock(blockGenesis);

    nMaxBlockRewardTxCount = GetBlockInvestRewardTxMaxCount();
    StdLog("BlockChain", "HandleInvoke: nMaxBlockRewardTxCount: %d", nMaxBlockRewardTxCount);

    if (!cntrBlock.Initialize(Config()->pathData, blockGenesis.GetHash(), Config()->fFullDb))
    {
        StdError("BlockChain", "Failed to initialize container");
        return false;
    }

    if (cntrBlock.IsEmpty())
    {
        if (!InsertGenesisBlock(blockGenesis))
        {
            StdError("BlockChain", "Failed to create genesis block");
            return false;
        }
    }

    InitCheckPoints();

    // Check local block compared to checkpoint
    if (Config()->nMagicNum == MAINNET_MAGICNUM)
    {
        std::map<uint256, CForkStatus> mapForkStatus;
        GetForkStatus(mapForkStatus);
        for (const auto& fork : mapForkStatus)
        {
            CBlock block;
            if (!FindPreviousCheckPointBlock(fork.first, block))
            {
                StdError("BlockChain", "Find CheckPoint on fork %s Error when the node starting, you should purge data(metabasenet -purge) to resync blockchain",
                         fork.first.ToString().c_str());
                return false;
            }
        }
    }

    return true;
}

void CBlockChain::HandleHalt()
{
    cntrBlock.Deinitialize();
    cacheEnrolled.Clear();
    cacheAgreement.Clear();
}

void CBlockChain::GetForkStatus(map<uint256, CForkStatus>& mapForkStatus)
{
    mapForkStatus.clear();

    multimap<int, CBlockIndex*> mapForkIndex;
    cntrBlock.ListForkIndex(mapForkIndex);

    for (auto it = mapForkIndex.begin(); it != mapForkIndex.end(); ++it)
    {
        CBlockIndex* pIndex = it->second;
        int nForkHeight = it->first;
        uint256 hashFork = pIndex->GetOriginHash();
        uint256 hashParent = pIndex->GetParentHash();

        if (hashParent != 0)
        {
            bool fHave = false;
            multimap<int, uint256>& mapSubline = mapForkStatus[hashParent].mapSubline;
            auto mt = mapSubline.lower_bound(nForkHeight);
            while (mt != mapSubline.upper_bound(nForkHeight))
            {
                if (mt->second == hashFork)
                {
                    fHave = true;
                    break;
                }
                ++mt;
            }
            if (!fHave)
            {
                mapSubline.insert(make_pair(nForkHeight, hashFork));
            }
        }

        auto mi = mapForkStatus.find(hashFork);
        if (mi == mapForkStatus.end())
        {
            mi = mapForkStatus.insert(make_pair(hashFork, CForkStatus(hashFork, hashParent, nForkHeight))).first;
        }
        else
        {
            mi->second.hashOrigin = hashFork;
            mi->second.hashParent = hashParent;
            mi->second.nOriginHeight = nForkHeight;
        }
        CForkStatus& status = mi->second;
        status.hashPrevBlock = pIndex->GetPrevHash();
        status.hashLastBlock = pIndex->GetBlockHash();
        status.nLastBlockTime = pIndex->GetBlockTime();
        status.nLastBlockHeight = pIndex->GetBlockHeight();
        status.nLastBlockNumber = pIndex->GetBlockNumber();
        status.nMoneySupply = pIndex->GetMoneySupply();
        status.nMoneyDestroy = pIndex->GetMoneyDestroy();
        status.nMintType = pIndex->nMintType;
    }
}

bool CBlockChain::GetForkProfile(const uint256& hashFork, CProfile& profile)
{
    return cntrBlock.RetrieveProfile(hashFork, profile);
}

bool CBlockChain::GetForkContext(const uint256& hashFork, CForkContext& ctxt)
{
    return cntrBlock.RetrieveForkContext(hashFork, ctxt);
}

bool CBlockChain::GetForkAncestry(const uint256& hashFork, vector<pair<uint256, uint256>> vAncestry)
{
    return cntrBlock.RetrieveAncestry(hashFork, vAncestry);
}

int CBlockChain::GetBlockCount(const uint256& hashFork)
{
    int nCount = 0;
    CBlockIndex* pIndex = nullptr;
    if (cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        while (pIndex != nullptr)
        {
            pIndex = pIndex->pPrev;
            ++nCount;
        }
    }
    return nCount;
}

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    hashFork = pIndex->GetOriginHash();
    nHeight = pIndex->GetBlockHeight();
    return true;
}

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight, uint256& hashNext)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    hashFork = pIndex->GetOriginHash();
    nHeight = pIndex->GetBlockHeight();
    if (pIndex->pNext != nullptr)
    {
        hashNext = pIndex->pNext->GetBlockHash();
    }
    else
    {
        hashNext = 0;
    }
    return true;
}

bool CBlockChain::GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() == nHeight && pIndex->IsExtended())
    {
        pIndex = pIndex->pPrev;
    }
    hashBlock = !pIndex ? uint64(0) : pIndex->GetBlockHash();
    return (pIndex != nullptr);
}

bool CBlockChain::GetBlockHash(const uint256& hashFork, int nHeight, vector<uint256>& vBlockHash)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() == nHeight)
    {
        vBlockHash.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }
    std::reverse(vBlockHash.begin(), vBlockHash.end());
    return (!vBlockHash.empty());
}

bool CBlockChain::GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockNumber() < nNumber)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockNumber() > nNumber)
    {
        pIndex = pIndex->pPrev;
    }
    hashBlock = !pIndex ? uint64(0) : pIndex->GetBlockHash();
    return (pIndex != nullptr);
}

bool CBlockChain::GetBlockStatus(const uint256& hashBlock, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }

    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nBlockNumber = pIndex->GetBlockNumber();
    status.nTxCount = pIndex->GetTxCount();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nMintType = pIndex->nMintType;
    status.hashStateRoot = pIndex->GetStateRoot();

    return true;
}

bool CBlockChain::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    if (pIndex == nullptr || pIndex->GetBlockHeight() != nHeight)
    {
        return false;
    }

    hashBlock = pIndex->GetBlockHash();
    nTime = pIndex->GetBlockTime();

    return true;
}

bool CBlockChain::GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }
    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nBlockNumber = pIndex->GetBlockNumber();
    status.nTxCount = pIndex->GetTxCount();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nMintType = pIndex->nMintType;
    status.hashStateRoot = pIndex->GetStateRoot();
    return true;
}

bool CBlockChain::GetLastBlockTime(const uint256& hashFork, int nDepth, vector<int64>& vTime)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }

    vTime.clear();
    vTime.reserve(nDepth);
    while (nDepth > 0 && pIndex != nullptr)
    {
        vTime.push_back(pIndex->GetBlockTime());
        if (!pIndex->IsExtended())
        {
            nDepth--;
        }
        pIndex = pIndex->pPrev;
    }
    return true;
}

bool CBlockChain::GetBlock(const uint256& hashBlock, CBlock& block)
{
    CBlockEx blockex;
    if (!cntrBlock.Retrieve(hashBlock, blockex))
    {
        return false;
    }
    block = static_cast<CBlock>(blockex);
    return true;
}

bool CBlockChain::GetBlockEx(const uint256& hashBlock, CBlockEx& block)
{
    return cntrBlock.Retrieve(hashBlock, block);
}

bool CBlockChain::GetOrigin(const uint256& hashFork, CBlock& block)
{
    return cntrBlock.RetrieveOrigin(hashFork, block);
}

bool CBlockChain::Exists(const uint256& hashBlock)
{
    return cntrBlock.Exists(hashBlock);
}

bool CBlockChain::GetTransaction(const uint256& txid, CTransaction& tx)
{
    return cntrBlock.RetrieveTx(txid, tx);
}

bool CBlockChain::GetTransaction(const uint256& hashFork, const uint256& hashLastBlock, const uint256& txid, CTransaction& tx)
{
    return cntrBlock.RetrieveTx(hashFork, hashLastBlock, txid, tx);
}

bool CBlockChain::GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight)
{
    return cntrBlock.RetrieveTx(txid, tx, hashFork, nHeight);
}

bool CBlockChain::ExistsTx(const uint256& txid)
{
    return cntrBlock.ExistsTx(txid);
}

bool CBlockChain::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return cntrBlock.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockChain::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return cntrBlock.RetrieveForkLast(hashFork, hashLastBlock);
}

int CBlockChain::GetForkCreatedHeight(const uint256& hashFork)
{
    return cntrBlock.GetForkCreatedHeight(hashFork);
}

bool CBlockChain::GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    return cntrBlock.GetForkStorageMaxHeight(hashFork, nMaxHeight);
}

Errno CBlockChain::AddNewBlock(const CBlock& block, CBlockChainUpdate& update)
{
    uint256 hashBlock = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hashBlock))
    {
        StdLog("BlockChain", "Add new block: Already exists, block: %s", hashBlock.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new block: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Add new block: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    uint256 hashFork = pIndexPrev->GetOriginHash();

    uint256 nReward;
    CDelegateAgreement agreement;
    size_t nEnrollTrust = 0;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hashBlock, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new block: Verify block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    std::map<uint256, CAddressContext> mapBlockAddress;
    if (!VerifyBlockAddress(hashFork, block, mapBlockAddress))
    {
        StdError("BlockChain", "Add new block: Verify block address fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    size_t nIgnoreVerifyTx = 0;
    if (!VerifyVoteRewardTx(block, nIgnoreVerifyTx))
    {
        StdError("BlockChain", "Add new block: Verify invest reward tx fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    err = VerifyBlockTx(hashFork, hashBlock, block, nReward, nIgnoreVerifyTx, mapBlockAddress);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new block: Verify Block tx fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
    {
        StdLog("BlockChain", "Add new block: Get block trust fail, block: %s", hashBlock.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }

    CBlockEx blockex(block, nChainTrust);
    if (!cntrBlock.StorageNewBlock(hashFork, hashBlock, blockex, update))
    {
        StdLog("BlockChain", "Add new block: Storage block fail, block: %s", hashBlock.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    StdLog("BlockChain", "Add new block: Add block success, block: %s", hashBlock.ToString().c_str());

    return OK;
}

Errno CBlockChain::AddNewOrigin(const CBlock& block, CBlockChainUpdate& update)
{
    uint256 hashBlock = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hashBlock))
    {
        StdLog("BlockChain", "Add new origin: Already exists, block: %s", hashBlock.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new origin: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Add new origin: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    if (pIndexPrev->IsExtended() || pIndexPrev->IsVacant())
    {
        StdLog("BlockChain", "Add new origin: Prev block should not be extended/vacant block");
        return ERR_BLOCK_TYPE_INVALID;
    }

    uint256 hashBlockRef;
    int64 nTimeRef;
    if (!GetLastBlockOfHeight(pCoreProtocol->GetGenesisBlockHash(), block.GetBlockHeight(), hashBlockRef, nTimeRef))
    {
        StdLog("BlockChain", "Add new origin: Failed to query main chain reference block");
        return ERR_SYS_STORAGE_ERROR;
    }
    if (block.GetBlockTime() != nTimeRef)
    {
        StdLog("BlockChain", "Add new origin: Invalid origin block time");
        return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
    }

    CProfile parent;
    if (!cntrBlock.RetrieveProfile(pIndexPrev->GetOriginHash(), parent))
    {
        StdLog("BlockChain", "Add new origin: Retrieve parent profile fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    CProfile profile;
    err = pCoreProtocol->ValidateOrigin(block, parent, profile);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new origin: Validate origin fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexDuplicated;
    if (cntrBlock.RetrieveFork(profile.strName, &pIndexDuplicated))
    {
        StdLog("BlockChain", "Add new origin: Validate origin fail, err: duplated fork name, block: %s, existed: %s",
               hashBlock.ToString().c_str(), pIndexDuplicated->GetOriginHash().GetHex().c_str());
        return ERR_ALREADY_HAVE;
    }

    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust, pIndexPrev))
    {
        StdLog("BlockChain", "Add new origin: get block trust fail, block: %s", hashBlock.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    CBlockEx blockex(block, nChainTrust);
    if (!cntrBlock.StorageNewBlock(hashBlock, hashBlock, blockex, update))
    {
        StdLog("BlockChain", "Add new origin: Storage block fail, block: %s", hashBlock.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    StdLog("BlockChain", "Add origin block success, block: %s", hashBlock.ToString().c_str());

    return OK;
}

bool CBlockChain::GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits)
{
    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Get ProofOfWork Target : Retrieve Prev Index Error: %s", hashPrev.ToString().c_str());
        return false;
    }
    if (!pIndexPrev->IsPrimary())
    {
        StdLog("BlockChain", "Get ProofOfWork Target : Previous is not primary: %s", hashPrev.ToString().c_str());
        return false;
    }
    if (!pCoreProtocol->GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits))
    {
        StdLog("BlockChain", "Get ProofOfWork Target : Unknown proof-of-work algo: %s", hashPrev.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::GetBlockMintReward(const uint256& hashPrev, uint256& nReward)
{
    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Get block reward: Retrieve Prev Index Error, hashPrev: %s", hashPrev.ToString().c_str());
        return false;
    }

    if (pIndexPrev->IsPrimary())
    {
        nReward = GetPrimaryBlockReward(hashPrev);
        if (nReward < 0)
        {
            StdLog("BlockChain", "Get block reward: Get block invest total reward fail, hashPrev: %s", hashPrev.ToString().c_str());
            return false;
        }
    }
    else
    {
        CProfile profile;
        if (!GetForkProfile(pIndexPrev->GetOriginHash(), profile))
        {
            StdLog("BlockChain", "Get block reward: Get fork profile fail, hashPrev: %s", hashPrev.ToString().c_str());
            return false;
        }
        if (profile.nHalveCycle == 0)
        {
            nReward = profile.nMintReward;
        }
        else
        {
            uint64 u = (uint64)pow(2, (pIndexPrev->GetBlockHeight() + 1 - profile.nJointHeight) / profile.nHalveCycle);
            nReward = profile.nMintReward / uint256(u);
        }
    }
    //nReward = nReward / 2;
    return true;
}

bool CBlockChain::GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep)
{
    return cntrBlock.GetForkBlockLocator(hashFork, locator, hashDepth, nIncStep);
}

bool CBlockChain::GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, vector<uint256>& vBlockHash, size_t nMaxCount)
{
    return cntrBlock.GetForkBlockInv(hashFork, locator, vBlockHash, nMaxCount);
}

bool CBlockChain::GetVotes(const CDestination& destDelegate, uint256& nVotes)
{
    return cntrBlock.GetVotes(pCoreProtocol->GetGenesisBlockHash(), destDelegate, nVotes);
}

bool CBlockChain::ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    return cntrBlock.GetDelegateList(pCoreProtocol->GetGenesisBlockHash(), nCount, mapVotes);
}

bool CBlockChain::VerifyRepeatBlock(const uint256& hashFork, const CBlock& block, const uint256& hashBlockRef)
{
    uint32 nRefTimeStamp = 0;
    if (hashBlockRef != 0 && (block.IsSubsidiary() || block.IsExtended()))
    {
        CBlockIndex* pIndexRef;
        if (!cntrBlock.RetrieveIndex(hashBlockRef, &pIndexRef))
        {
            StdLog("BlockChain", "VerifyRepeatBlock: RetrieveIndex fail, hashBlockRef: %s, block: %s",
                   hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
            return false;
        }
        if (block.IsSubsidiary())
        {
            if (block.GetBlockTime() != pIndexRef->GetBlockTime())
            {
                StdLog("BlockChain", "VerifyRepeatBlock: Subsidiary block time error, block time: %ld, ref block time: %ld, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
                || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING
                || ((block.GetBlockTime() - pIndexRef->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
            {
                StdLog("BlockChain", "VerifyRepeatBlock: Extended block time error, block time: %ld, ref block time: %ld, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
                return false;
            }
        }
        nRefTimeStamp = pIndexRef->nTimeStamp;
    }
    return cntrBlock.VerifyRepeatBlock(hashFork, block.GetBlockHeight(), block.txMint.to, block.nType, block.nTimeStamp, nRefTimeStamp, EXTENDED_BLOCK_SPACING);
}

bool CBlockChain::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, uint256>& mapVote)
{
    return cntrBlock.GetBlockDelegateVote(hashBlock, mapVote);
}

uint256 CBlockChain::GetDelegateMinEnrollAmount(const uint256& hashBlock)
{
    return pCoreProtocol->MinEnrollAmount();
}

bool CBlockChain::GetDelegateCertTxCount(const uint256& hashLastBlock, map<CDestination, int>& mapVoteCert)
{
    CBlockIndex* pLastIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashLastBlock, &pLastIndex))
    {
        StdLog("BlockChain", "Get Delegate Cert Tx Count: RetrieveIndex fail, block: %s", hashLastBlock.GetHex().c_str());
        return false;
    }
    if (pLastIndex->GetBlockHeight() <= 0)
    {
        return true;
    }

    int nMinHeight = pLastIndex->GetBlockHeight() - CONSENSUS_ENROLL_INTERVAL + 2;
    if (nMinHeight < 1)
    {
        nMinHeight = 1;
    }

    CBlockIndex* pIndex = pLastIndex;
    for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL - 1 && pIndex != nullptr; i++)
    {
        std::map<int, std::set<CDestination>> mapEnrollDest;
        if (cntrBlock.GetBlockDelegatedEnrollTx(pIndex->GetBlockHash(), mapEnrollDest))
        {
            for (const auto& t : mapEnrollDest)
            {
                if (t.first >= nMinHeight)
                {
                    for (const auto& m : t.second)
                    {
                        map<CDestination, int>::iterator it = mapVoteCert.find(m);
                        if (it == mapVoteCert.end())
                        {
                            mapVoteCert.insert(make_pair(m, 1));
                        }
                        else
                        {
                            it->second++;
                        }
                    }
                }
            }
        }
        pIndex = pIndex->pPrev;
    }

    int nMaxCertCount = CONSENSUS_ENROLL_INTERVAL * 2; //CONSENSUS_ENROLL_INTERVAL * 4 / 3;
    if (nMaxCertCount > pLastIndex->GetBlockHeight())
    {
        nMaxCertCount = pLastIndex->GetBlockHeight();
    }
    for (auto& v : mapVoteCert)
    {
        v.second = nMaxCertCount - v.second;
        if (v.second < 0)
        {
            v.second = 0;
        }
    }
    return true;
}

bool CBlockChain::GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled)
{
    enrolled.Clear();

    if (cacheEnrolled.Retrieve(hashBlock, enrolled))
    {
        return true;
    }

    CBlockIndex* pIndex;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        StdLog("BlockChain", "GetBlockDelegateEnrolled: Retrieve block index, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    uint256 nMinEnrollAmount = pCoreProtocol->MinEnrollAmount();

    if (pIndex->GetBlockHeight() < CONSENSUS_ENROLL_INTERVAL)
    {
        return true;
    }
    vector<uint256> vBlockRange;
    for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL; i++)
    {
        vBlockRange.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }

    if (!cntrBlock.RetrieveAvailDelegate(hashBlock, pIndex->GetBlockHeight(), vBlockRange, nMinEnrollAmount,
                                         enrolled.mapWeight, enrolled.mapEnrollData, enrolled.vecAmount))
    {
        StdLog("BlockChain", "GetBlockDelegateEnrolled: Retrieve avail delegate fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    cacheEnrolled.AddNew(hashBlock, enrolled);

    return true;
}

bool CBlockChain::GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement)
{
    agreement.Clear();

    if (cacheAgreement.Retrieve(hashBlock, agreement))
    {
        return true;
    }

    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: Retrieve block index fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    // is not primary or PoW, return.
    if (!pIndex->IsPrimary() || pIndex->IsProofOfWork())
    {
        return true;
    }

    CBlockIndex* pIndexRef = pIndex;
    if (pIndex->GetBlockHeight() < CONSENSUS_INTERVAL)
    {
        return true;
    }

    CBlock block;
    if (!cntrBlock.Retrieve(pIndex, block))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: Retrieve block fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL + 1; i++)
    {
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), enrolled))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: Get delegate enrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    if (!verifier.VerifyProof(block.vchProof, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: Invalid block proof, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    size_t nEnrollTrust = 0;
    pCoreProtocol->GetDelegatedBallot(agreement.nAgreement, agreement.nWeight, mapBallot, enrolled.vecAmount,
                                      pIndex->GetMoneySupply(), agreement.vBallot, nEnrollTrust, pIndexRef->GetBlockHeight());

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

uint256 CBlockChain::GetBlockMoneySupply(const uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex) || pIndex == nullptr)
    {
        return -1;
    }
    return pIndex->GetMoneySupply();
}

uint32 CBlockChain::DPoSTimestamp(const uint256& hashPrev)
{
    CBlockIndex* pIndexPrev = nullptr;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev) || pIndexPrev == nullptr)
    {
        return 0;
    }
    return pCoreProtocol->DPoSTimestamp(pIndexPrev);
}

Errno CBlockChain::VerifyPowBlock(const CBlock& block, bool& fLongChain)
{
    uint256 hashBlock = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hashBlock))
    {
        StdLog("BlockChain", "Verify pow block: Already exists, block: %s", hashBlock.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        StdLog("BlockChain", "Verify pow block: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Verify pow block: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    uint256 hashFork = pIndexPrev->GetOriginHash();

    uint256 nReward;
    CDelegateAgreement agreement;
    size_t nEnrollTrust = 0;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hashBlock, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        StdLog("BlockChain", "Verify pow block: Verify block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    std::map<uint256, CAddressContext> mapBlockAddress;
    if (!VerifyBlockAddress(hashFork, block, mapBlockAddress))
    {
        StdError("BlockChain", "Verify pow block: Verify block address fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CBlockEx blockex(block);

    size_t nIgnoreVerifyTx = 0;
    if (!VerifyVoteRewardTx(block, nIgnoreVerifyTx))
    {
        StdError("BlockChain", "Verify pow block: Verify invest reward tx fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    err = VerifyBlockTx(hashFork, hashBlock, block, nReward, nIgnoreVerifyTx, mapBlockAddress);
    if (err != OK)
    {
        StdError("BlockChain", "Verify pow block: Verify block tx fail, block: %s", hashBlock.ToString().c_str());
        return err;
    }

    // Get block trust
    uint256 nNewBlockChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nNewBlockChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
    {
        StdLog("BlockChain", "Verify pow block: get block trust fail, block: %s", hashBlock.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    nNewBlockChainTrust += pIndexPrev->nChainTrust;

    CBlockIndex* pIndexFork = nullptr;
    if (cntrBlock.RetrieveFork(pIndexPrev->GetOriginHash(), &pIndexFork)
        && pIndexFork->nChainTrust > nNewBlockChainTrust)
    {
        StdLog("BlockChain", "Verify pow block: Short chain, new block height: %d, block: %s, fork chain trust: %s, fork last block: %s",
               block.GetBlockHeight(), hashBlock.GetHex().c_str(), pIndexFork->nChainTrust.GetHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str());
        fLongChain = false;
    }
    else
    {
        fLongChain = true;
    }

    return OK;
}

bool CBlockChain::VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt)
{
    return cntrBlock.VerifyBlockForkTx(hashPrev, tx, vForkCtxt);
}

bool CBlockChain::CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update)
{
    CBlockIndex* pValidLastIndex = cntrBlock.GetForkValidLast(pCoreProtocol->GetGenesisBlockHash(), hashFork);
    if (pValidLastIndex == nullptr)
    {
        return false;
    }

    CBlockIndex* pOldLastIndex = nullptr;
    uint256 hashOldLastBlock;
    if (cntrBlock.RetrieveFork(hashFork, &pOldLastIndex))
    {
        hashOldLastBlock = pOldLastIndex->GetBlockHash();
    }

    StdLog("BlockChain", "Check Fork Valid Last: Repair fork last block, old last block: %s, valid last block: %s, valid ref block: %s, fork: %s",
           hashOldLastBlock.ToString().c_str(), pValidLastIndex->GetBlockHash().ToString().c_str(),
           pValidLastIndex->hashRefBlock.ToString().c_str(), hashFork.GetHex().c_str());

    update = CBlockChainUpdate(pValidLastIndex);
    if (!cntrBlock.GetBlockBranchList(pValidLastIndex->GetBlockHash(), update.vBlockRemove, update.vBlockAddNew))
    {
        StdError("BlockChain", "Check Fork Valid Last: get block branch list fail, last block: %s, fork: %s",
                 pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    //view.GetTxUpdated(update.setTxUpdate);
    //view.GetBlockChanges(update.vBlockAddNew, update.vBlockRemove);

    if (!cntrBlock.UpdateForkNext(hashFork, pValidLastIndex))
    {
        StdError("BlockChain", "Check Fork Valid Last: Update fork last fail, last block: %s, fork: %s",
                 pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }

    if (update.IsNull())
    {
        StdLog("BlockChain", "Check Fork Valid Last: update is null, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
    }
    if (update.vBlockAddNew.empty())
    {
        StdLog("BlockChain", "Check Fork Valid Last: vBlockAddNew is empty, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
    }
    return true;
}

bool CBlockChain::VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock)
{
    uint256 hashRefBlock;
    bool fOrigin = false;
    if (!cntrBlock.GetLastRefBlockHash(hashFork, hashForkBlock, hashRefBlock, fOrigin))
    {
        StdLog("BlockChain", "VerifyForkRefLongChain: Get ref block fail, last block: %s, fork: %s",
               hashForkBlock.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!fOrigin)
    {
        if (!cntrBlock.VerifySameChain(hashRefBlock, hashPrimaryBlock))
        {
            StdLog("BlockChain", "VerifyForkRefLongChain: Fork does not refer to long chain, fork last: %s, ref block: %s, primayr block: %s, fork: %s",
                   hashForkBlock.GetHex().c_str(), hashRefBlock.GetHex().c_str(),
                   hashPrimaryBlock.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockChain::GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime)
{
    return cntrBlock.GetPrimaryHeightBlockTime(hashLastBlock, nHeight, hashBlock, nTime);
}

bool CBlockChain::IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block)
{
    int nCreatedHeight = cntrBlock.GetForkCreatedHeight(hashFork);
    if (nCreatedHeight < 0)
    {
        return true;
    }

    int nOriginHeight = CBlock::GetBlockHeightByHash(hashFork);
    int nTargetHeight = block.GetBlockHeight();

    if (nTargetHeight < nCreatedHeight)
    {
        StdLog("BlockChain", "Target Block Is Vacant %s at height %d in range of (%d, %d)", block.IsVacant() ? "true" : "false",
               nTargetHeight, nOriginHeight, nCreatedHeight);
        return block.IsVacant();
    }

    return true;
}

bool CBlockChain::InsertGenesisBlock(CBlock& block)
{
    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust))
    {
        return false;
    }
    return cntrBlock.Initiate(block.GetHash(), block, nChainTrust);
}

bool CBlockChain::GetBlockChanges(const CBlockIndex* pIndexNew, const CBlockIndex* pIndexFork,
                                  vector<CBlockEx>& vBlockAddNew, vector<CBlockEx>& vBlockRemove)
{
    while (pIndexNew != pIndexFork)
    {
        int64 nLastBlockTime = pIndexFork ? pIndexFork->GetBlockTime() : -1;
        if (pIndexNew->GetBlockTime() >= nLastBlockTime)
        {
            CBlockEx block;
            if (!cntrBlock.Retrieve(pIndexNew, block))
            {
                return false;
            }
            vBlockAddNew.push_back(block);
            pIndexNew = pIndexNew->pPrev;
        }
        else
        {
            CBlockEx block;
            if (!cntrBlock.Retrieve(pIndexFork, block))
            {
                return false;
            }
            vBlockRemove.push_back(block);
            pIndexFork = pIndexFork->pPrev;
        }
    }
    return true;
}

bool CBlockChain::GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, const CBlockIndex* pIndexPrev,
                                            CDelegateAgreement& agreement, size_t& nEnrollTrust)
{
    agreement.Clear();

    if (pIndexPrev->GetBlockHeight() < CONSENSUS_INTERVAL - 1)
    {
        return true;
    }

    const CBlockIndex* pIndex = pIndexPrev;
    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL; i++)
    {
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), enrolled))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: GetBlockDelegateEnrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    if (!verifier.VerifyProof(block.vchProof, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        StdLog("BlockChain", "GetBlockDelegateAgreement: Invalid block proof : %s", hashBlock.ToString().c_str());
        return false;
    }

    pCoreProtocol->GetDelegatedBallot(agreement.nAgreement, agreement.nWeight, mapBallot, enrolled.vecAmount,
                                      pIndex->GetMoneySupply(), agreement.vBallot, nEnrollTrust, pIndexPrev->GetBlockHeight() + 1);

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

Errno CBlockChain::VerifyBlock(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev,
                               uint256& nReward, CDelegateAgreement& agreement, size_t& nEnrollTrust, CBlockIndex** ppIndexRef)
{
    nReward = 0;
    if (block.IsOrigin())
    {
        StdLog("BlockChain", "Verify block: Is origin, block: %s", hashBlock.GetHex().c_str());
        return ERR_BLOCK_INVALID_FORK;
    }

    if (block.IsPrimary())
    {
        if (!pIndexPrev->IsPrimary())
        {
            StdLog("BlockChain", "Verify block: Prev block not is primary, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        if (!VerifyBlockCertTx(block))
        {
            StdLog("BlockChain", "Verify block: Verify cert tx fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_CERTTX_OUT_OF_BOUND;
        }

        if (!GetBlockDelegateAgreement(hashBlock, block, pIndexPrev, agreement, nEnrollTrust))
        {
            StdLog("BlockChain", "Verify block: Get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!GetBlockMintReward(block.hashPrev, nReward))
        {
            StdLog("BlockChain", "Verify block: Get mint reward fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_COINBASE_INVALID;
        }

        if (agreement.IsProofOfWork())
        {
            return pCoreProtocol->VerifyProofOfWork(block, pIndexPrev);
        }
        else
        {
            return pCoreProtocol->VerifyDelegatedProofOfStake(block, pIndexPrev, agreement);
        }
    }
    else if (block.IsSubsidiary() || block.IsExtended())
    {
        if (pIndexPrev->IsPrimary())
        {
            StdLog("BlockChain", "Verify block: SubFork prev not is primary, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
        {
            StdLog("BlockChain", "Verify block: SubFork load proof fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        CDelegateAgreement agreement;
        if (!GetBlockDelegateAgreement(proof.hashRefBlock, agreement))
        {
            StdLog("BlockChain", "Verify block: SubFork get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (agreement.nAgreement != proof.nAgreement || agreement.nWeight != proof.nWeight
            || agreement.IsProofOfWork())
        {
            StdLog("BlockChain", "Verify block: SubFork agreement error, ref agreement: %s, block agreement: %s, ref weight: %d, block weight: %d, type: %s, block: %s",
                   agreement.nAgreement.GetHex().c_str(), proof.nAgreement.GetHex().c_str(),
                   agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "pow" : "dpos"),
                   hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!cntrBlock.RetrieveIndex(proof.hashRefBlock, ppIndexRef) || *ppIndexRef == nullptr || !(*ppIndexRef)->IsPrimary())
        {
            StdLog("BlockChain", "Verify block: SubFork retrieve ref index fail, ref block: %s, block: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        CProofOfPiggyback proofPrev;
        if (!pIndexPrev->IsOrigin())
        {
            CBlock blockPrev;
            if (!cntrBlock.Retrieve(pIndexPrev, blockPrev))
            {
                StdLog("BlockChain", "Verify block: SubFork retrieve prev index fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            if (!proofPrev.Load(blockPrev.vchProof) || proofPrev.hashRefBlock == 0)
            {
                StdLog("BlockChain", "Verify block: SubFork load prev proof fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (proof.hashRefBlock != proofPrev.hashRefBlock
                && !cntrBlock.VerifySameChain(proofPrev.hashRefBlock, proof.hashRefBlock))
            {
                StdLog("BlockChain", "Verify block: SubFork verify same chain fail, prev ref: %s, block ref: %s, block: %s",
                       proofPrev.hashRefBlock.GetHex().c_str(), proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        if (block.IsExtended())
        {
            if (pIndexPrev->IsOrigin() || pIndexPrev->IsVacant())
            {
                StdLog("BlockChain", "Verify block: SubFork extended prev is origin or vacant, prev: %s, block: %s",
                       pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            if (proof.nAgreement != proofPrev.nAgreement || proof.nWeight != proofPrev.nWeight)
            {
                StdLog("BlockChain", "Verify block: SubFork extended agreement error, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            nReward = 0;
        }
        else
        {
            if (!GetBlockMintReward(block.hashPrev, nReward))
            {
                StdLog("BlockChain", "Verify block: SubFork get mint reward error, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        return pCoreProtocol->VerifySubsidiary(block, pIndexPrev, *ppIndexRef, agreement);
    }
    else if (block.IsVacant())
    {
        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
        {
            StdLog("BlockChain", "Verify block: Vacant load proof error, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        CDelegateAgreement agreement;
        if (!GetBlockDelegateAgreement(proof.hashRefBlock, agreement))
        {
            StdLog("BlockChain", "Verify block: Vacant get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (agreement.nAgreement != proof.nAgreement || agreement.nWeight != proof.nWeight
            || agreement.IsProofOfWork())
        {
            StdLog("BlockChain", "Verify block: Vacant agreement error, ref agreement: %s, block agreement: %s, ref weight: %d, block weight: %d, type: %s, block: %s",
                   agreement.nAgreement.GetHex().c_str(), proof.nAgreement.GetHex().c_str(),
                   agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "pow" : "dpos"),
                   hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (block.txMint.to != agreement.GetBallot(0))
        {
            StdLog("BlockChain", "Verify block: Vacant to error, to: %s, ballot: %s, block: %s",
                   block.txMint.to.ToString().c_str(),
                   agreement.GetBallot(0).ToString().c_str(),
                   hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (block.txMint.nTimeStamp != block.GetBlockTime())
        {
            StdLog("BlockChain", "Verify block: Vacant txMint timestamp error, mint tx time: %d, block time: %d, block: %s",
                   block.txMint.nTimeStamp, block.GetBlockTime(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!cntrBlock.RetrieveIndex(proof.hashRefBlock, ppIndexRef) || *ppIndexRef == nullptr || !(*ppIndexRef)->IsPrimary())
        {
            StdLog("BlockChain", "Verify block: Vacant retrieve ref index fail, ref: %s, block: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!pIndexPrev->IsOrigin())
        {
            CBlock blockPrev;
            if (!cntrBlock.Retrieve(pIndexPrev, blockPrev))
            {
                StdLog("BlockChain", "Verify block: Vacant retrieve prev index fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            CProofOfPiggyback proofPrev;
            if (!proofPrev.Load(blockPrev.vchProof) || proofPrev.hashRefBlock == 0)
            {
                StdLog("BlockChain", "Verify block: Vacant load prev proof fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (proof.hashRefBlock != proofPrev.hashRefBlock
                && !cntrBlock.VerifySameChain(proofPrev.hashRefBlock, proof.hashRefBlock))
            {
                StdLog("BlockChain", "Verify block: Vacant verify same chain fail, prev ref: %s, block ref: %s, block: %s",
                       proofPrev.hashRefBlock.GetHex().c_str(), proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        if (!cntrBlock.VerifyPrimaryHeightRefBlockTime(block.GetBlockHeight(), block.GetBlockTime()))
        {
            uint256 hashPrimaryBlock;
            int64 nPrimaryTime = 0;
            if (!cntrBlock.GetPrimaryHeightBlockTime((*ppIndexRef)->GetBlockHash(), block.GetBlockHeight(), hashPrimaryBlock, nPrimaryTime))
            {
                StdLog("BlockChain", "Verify block: Vacant get height time, block ref: %s, block: %s",
                       (*ppIndexRef)->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (block.GetBlockTime() != nPrimaryTime)
            {
                StdLog("BlockChain", "Verify block: Vacant time error, block time: %d, primary time: %d, ref block: %s, same height block: %s, block: %s",
                       block.GetBlockTime(), nPrimaryTime, (*ppIndexRef)->GetBlockHash().GetHex().c_str(),
                       hashPrimaryBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        StdLog("BlockChain", "Verify block: block type error, nType: %d, block: %s", block.nType, hashBlock.GetHex().c_str());
        return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
    }

    return OK;
}

bool CBlockChain::VerifyBlockCertTx(const CBlock& block)
{
    map<CDestination, int> mapBlockCert;
    for (const auto& d : block.vtx)
    {
        if (d.nType == CTransaction::TX_CERT)
        {
            ++mapBlockCert[d.to];
        }
    }
    if (!mapBlockCert.empty())
    {
        map<CDestination, uint256> mapVote;
        if (!GetBlockDelegateVote(block.hashPrev, mapVote))
        {
            StdError("BlockChain", "VerifyBlockCertTx: Get block delegate vote fail");
            return false;
        }
        map<CDestination, int> mapVoteCert;
        if (!GetDelegateCertTxCount(block.hashPrev, mapVoteCert))
        {
            StdError("BlockChain", "VerifyBlockCertTx: Get delegate cert tx count fail");
            return false;
        }
        uint256 nMinAmount = pCoreProtocol->MinEnrollAmount();
        for (const auto& d : mapBlockCert)
        {
            const CDestination& dest = d.first;
            map<CDestination, uint256>::iterator mt = mapVote.find(dest);
            if (mt == mapVote.end() || mt->second < nMinAmount)
            {
                StdLog("BlockChain", "VerifyBlockCertTx: not enough votes, votes: %ld, dest: %s",
                       (mt == mapVote.end() ? 0 : mt->second), dest.ToString().c_str());
                return false;
            }
            map<CDestination, int>::iterator it = mapVoteCert.find(dest);
            if (it != mapVoteCert.end() && d.second > it->second)
            {
                StdLog("BlockChain", "VerifyBlockCertTx: more than votes, block cert count: %d, available cert count: %d, dest: %s",
                       d.second, it->second, dest.ToString().c_str());
                return false;
            }
        }
    }
    return true;
}

bool CBlockChain::VerifyBlockAddress(const uint256& hashFork, const CBlock& block, std::map<uint256, CAddressContext>& mapBlockAddress)
{
    auto getAddress = [&](const uint256& dest, CAddressContext& ctxtAddress) -> bool
    {
        auto it = mapBlockAddress.find(dest);
        if (it == mapBlockAddress.end())
        {
            if (!cntrBlock.RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxtAddress))
            {
                return false;
            }
            mapBlockAddress.insert(make_pair(dest, ctxtAddress));
        }
        else
        {
            ctxtAddress = it->second;
        }
        return true;
    };

    auto verifyAddress = [&](const CTransaction& tx) -> bool
    {
        if (!tx.from.IsNull())
        {
            CAddressContext ctxtAddress;
            if (!getAddress(tx.from.data, ctxtAddress))
            {
                StdLog("BlockChain", "Verify Block Address: Get from address fail, from: %s", tx.from.ToString().c_str());
                return false;
            }
            if (tx.from.prefix != ctxtAddress.nType)
            {
                StdLog("BlockChain", "Verify Block Address: Get from prefix error, from: %s", tx.from.ToString().c_str());
                return false;
            }
        }

        if (!tx.to.IsNull())
        {
            CDestination destTo;
            if (tx.to.IsNullContract())
            {
                destTo.SetContractId(tx.from.data, tx.nTxNonce);
            }
            else
            {
                destTo = tx.to;
            }
            CAddressContext ctxtAddress;
            if (!getAddress(destTo.data, ctxtAddress))
            {
                switch (destTo.prefix)
                {
                case CDestination::PREFIX_PUBKEY:
                    ctxtAddress = CAddressContext(block.GetBlockNumber());
                    break;
                case CDestination::PREFIX_TEMPLATE:
                {
                    bytes btTempData;
                    if (!tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
                    {
                        StdLog("BlockChain", "Verify Block Address: To address is template, txdata error, to: %s", tx.to.ToString().c_str());
                        return false;
                    }
                    ctxtAddress = CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), block.GetBlockNumber());
                    break;
                }
                case CDestination::PREFIX_CONTRACT:
                {
                    if (tx.to.IsNullContract())
                    {
                        bytes btTempData;
                        if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
                        {
                            StdLog("BlockChain", "Verify Block Address: Get tx data fail, to: %s", tx.to.ToString().c_str());
                            return false;
                        }
                        CTxContractData txcd;
                        try
                        {
                            hcode::CBufStream ss(btTempData);
                            ss >> txcd;
                        }
                        catch (std::exception& e)
                        {
                            StdLog("BlockChain", "Verify Block Address: Parse data fail, to: %s", tx.to.ToString().c_str());
                            return false;
                        }
                        txcd.UncompressCode();
                        ctxtAddress = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), tx.GetHash(), uint256(), txcd.GetWasmCreateCodeHash(), uint256()), block.GetBlockNumber());
                    }
                    else
                    {
                        StdLog("BlockChain", "Verify Block Address: Contract to address error, to: %s", tx.to.ToString().c_str());
                        return false;
                    }
                    break;
                }
                default:
                    StdLog("BlockChain", "Verify Block Address: To address prefix error, to: %s", tx.to.ToString().c_str());
                    return false;
                }
                mapBlockAddress.insert(make_pair(destTo.data, ctxtAddress));
            }
            else
            {
                if (destTo.prefix != ctxtAddress.nType)
                {
                    StdLog("BlockChain", "Verify Block Address: To address prefix != db prefix, to: %s", tx.to.ToString().c_str());
                    return false;
                }
            }
        }
        return true;
    };

    if (!verifyAddress(block.txMint))
    {
        StdLog("BlockChain", "Verify Block Address: Verify mint address fail, txid: %s", block.txMint.GetHash().GetHex().c_str());
        return false;
    }

    for (const auto& tx : block.vtx)
    {
        if (!verifyAddress(tx))
        {
            StdLog("BlockChain", "Verify Block Address: Verify address fail, txid: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    return true;
}

void CBlockChain::InitCheckPoints(const uint256& hashFork, const map<int, uint256>& mapCheckPointsIn)
{
    MapCheckPointsType& mapCheckPoints = mapForkCheckPoints[hashFork];
    for (const auto& vd : mapCheckPointsIn)
    {
        mapCheckPoints.insert(std::make_pair(vd.first, CCheckPoint(vd.first, vd.second)));
    }
}

void CBlockChain::InitCheckPoints()
{
    if (Config()->nMagicNum == MAINNET_MAGICNUM)
    {
        for (const auto& vd : mapCheckPointsList)
        {
            InitCheckPoints(vd.first, vd.second);
        }
    }
}

bool CBlockChain::HasCheckPoints(const uint256& hashFork) const
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        return iter->second.size() > 0;
    }
    else
    {
        return false;
    }
}

bool CBlockChain::GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point)
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        if (iter->second.count(nHeight) == 0)
        {
            return false;
        }
        else
        {
            point = iter->second[nHeight];
            return true;
        }
    }
    else
    {
        return false;
    }
}

std::vector<IBlockChain::CCheckPoint> CBlockChain::CheckPoints(const uint256& hashFork) const
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        std::vector<IBlockChain::CCheckPoint> points;
        for (const auto& kv : iter->second)
        {
            points.push_back(kv.second);
        }

        return points;
    }

    return std::vector<IBlockChain::CCheckPoint>();
}

IBlockChain::CCheckPoint CBlockChain::LatestCheckPoint(const uint256& hashFork) const
{
    if (!HasCheckPoints(hashFork))
    {
        return IBlockChain::CCheckPoint();
    }
    return mapForkCheckPoints.at(hashFork).rbegin()->second;
}

IBlockChain::CCheckPoint CBlockChain::UpperBoundCheckPoint(const uint256& hashFork, int nHeight) const
{
    if (!HasCheckPoints(hashFork))
    {
        return IBlockChain::CCheckPoint();
    }

    auto& forkCheckPoints = mapForkCheckPoints.at(hashFork);
    auto iter = forkCheckPoints.upper_bound(nHeight);
    return (iter != forkCheckPoints.end()) ? IBlockChain::CCheckPoint(iter->second) : IBlockChain::CCheckPoint();
}

bool CBlockChain::VerifyCheckPoint(const uint256& hashFork, int nHeight, const uint256& nBlockHash)
{
    if (!HasCheckPoints(hashFork))
    {
        return true;
    }

    CCheckPoint point;
    if (!GetCheckPointByHeight(hashFork, nHeight, point))
    {
        return true;
    }

    if (nBlockHash != point.nBlockHash)
    {
        return false;
    }

    StdLog("BlockChain", "HashFork %s Verified checkpoint at height %d block %s", hashFork.ToString().c_str(), point.nHeight, point.nBlockHash.ToString().c_str());

    return true;
}

bool CBlockChain::FindPreviousCheckPointBlock(const uint256& hashFork, CBlock& block)
{
    if (!HasCheckPoints(hashFork))
    {
        return true;
    }

    const auto& points = CheckPoints(hashFork);
    int numCheckpoints = points.size();
    for (int i = numCheckpoints - 1; i >= 0; i--)
    {
        const CCheckPoint& point = points[i];

        uint256 hashBlock;
        if (!GetBlockHash(hashFork, point.nHeight, hashBlock))
        {
            StdTrace("BlockChain", "HashFork %s CheckPoint(%d, %s) doest not exists and continuely try to get previous checkpoint",
                     hashFork.ToString().c_str(), point.nHeight, point.nBlockHash.ToString().c_str());

            continue;
        }

        if (hashBlock != point.nBlockHash)
        {
            StdError("BlockChain", "CheckPoint(%d, %s)  does not match block hash %s",
                     point.nHeight, point.nBlockHash.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }

        return GetBlock(hashBlock, block);
    }

    return true;
}

bool CBlockChain::IsSameBranch(const uint256& hashFork, const CBlock& block)
{
    uint256 bestChainBlockHash;
    if (!GetBlockHash(hashFork, block.GetBlockHeight(), bestChainBlockHash))
    {
        return true;
    }

    return block.GetHash() == bestChainBlockHash;
}

int64 CBlockChain::GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, vector<CTxInfo>& vTx)
{
    return 0;
}

bool CBlockChain::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress)
{
    return cntrBlock.RetrieveAddressContext(hashFork, hashBlock, dest, ctxtAddress);
}

bool CBlockChain::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    return cntrBlock.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockChain::RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode)
{
    return cntrBlock.RetrieveWasmCreateCodeContext(hashFork, hashBlock, hashWasmCreateCode, ctxtCode);
}

bool CBlockChain::GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode)
{
    return cntrBlock.GetBlockSourceCodeData(hashFork, hashBlock, hashSourceCode, txcdCode);
}

bool CBlockChain::GetBlockWasmCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CTxContractData& txcdCode)
{
    return cntrBlock.GetBlockWasmCreateCodeData(hashFork, hashBlock, hashWasmCreateCode, txcdCode);
}

bool CBlockChain::GetContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode)
{
    return cntrBlock.GetWasmCreateCodeContext(hashFork, hashBlock, hashContractCode, ctxtContractCode);
}

bool CBlockChain::ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode)
{
    return cntrBlock.ListWasmCreateCodeContext(hashFork, hashBlock, txid, mapCreateCode);
}

bool CBlockChain::ListAddressTxInfo(const uint256& hashFork, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    uint256 hashLastBlock;
    if (!cntrBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    return cntrBlock.ListAddressTxInfo(hashFork, hashLastBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
}

bool CBlockChain::GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount)
{
    if (!dest.IsTemplate() || dest.GetTemplateId().GetType() != TEMPLATE_FORK)
    {
        StdLog("BlockChain", "Get fork lock amount: Not fork template address, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = nullptr;
    if (!btAddressData.empty())
    {
        ptr = CTemplate::CreateTemplatePtr(dest.GetTemplateId().GetType(), btAddressData);
    }
    if (!ptr)
    {
        CAddressContext ctxtAddress;
        if (!cntrBlock.RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), hashPrevBlock, dest.data, ctxtAddress))
        {
            StdLog("BlockChain", "Get fork lock amount: Retrieve address context fail, dest: %s, prev: %s",
                   dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
            return false;
        }
        CTemplateAddressContext ctxtTemplate;
        if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
        {
            StdLog("BlockChain", "Get fork lock amount: Get template address context fail, dest: %s, prev: %s",
                   dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
            return false;
        }
        ptr = CTemplate::CreateTemplatePtr(dest.GetTemplateId().GetType(), ctxtTemplate.btData);
    }
    if (!ptr)
    {
        StdLog("BlockChain", "Get fork lock amount: Create template fail, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    uint256 hashForkLocked = boost::dynamic_pointer_cast<CTemplateFork>(ptr)->hashFork;
    int nCreateHeight = GetForkCreatedHeight(hashForkLocked);
    if (nCreateHeight < 0)
    {
        nCreateHeight = 0;
    }
    int nForkValidHeight = CBlock::GetBlockHeightByHash(hashPrevBlock) - nCreateHeight;
    if (nForkValidHeight < 0)
    {
        nForkValidHeight = 0;
    }
    nLockedAmount = CTemplateFork::LockedCoin(nForkValidHeight);
    return true;
}

bool CBlockChain::VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock)
{
    CForkContext ctxt;
    if (cntrBlock.RetrieveForkContext(hashFork, ctxt, hashBlock))
    {
        StdLog("BlockChain", "Verify fork name: Fork id existed, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    uint256 hashTempFork;
    if (cntrBlock.GetForkIdByForkName(strForkName, hashTempFork, hashBlock))
    {
        StdLog("BlockChain", "Verify fork name: Fork name existed, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    if (cntrBlock.GetForkIdByForkSn(CTransaction::GetForkSn(hashFork), hashTempFork, hashBlock))
    {
        StdLog("BlockChain", "Verify fork name: Fork sn existed, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CDestination& destParent)
{
    return cntrBlock.RetrieveInviteParent(hashFork, hashBlock, destSub, destParent);
}

bool CBlockChain::ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestination>& mapInviteContext)
{
    return cntrBlock.ListInviteRelation(hashFork, hashBlock, mapInviteContext);
}

bool CBlockChain::RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destRedeem, CVoteRedeemContext& ctxtVoteRedeem)
{
    return cntrBlock.RetrieveDestVoteRedeemContext(hashBlock, destRedeem, ctxtVoteRedeem);
}

bool CBlockChain::CalcBlockVoteRewardTx(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime, vector<CTransaction>& vVoteRewardTx)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwCvrAccess);

    const int nDistributeHeight = VOTE_REWARD_DISTRIBUTE_HEIGHT;
    if (CBlock::GetBlockHeightByHash(hashPrev) < nDistributeHeight
        || nBlockHeight % nDistributeHeight == 0
        || nBlockType == CBlock::BLOCK_GENESIS
        || nBlockType == CBlock::BLOCK_ORIGIN
        || nBlockType == CBlock::BLOCK_VACANT)
    {
        return true;
    }

    CBlockIndex* pPrevIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pPrevIndex) || pPrevIndex == nullptr)
    {
        StdError("BlockChain", "Get distribute vote reward tx: Retrieve prev index fail, hashPrev: %s", hashPrev.GetHex().c_str());
        return false;
    }
    uint256 hashFork = pPrevIndex->GetOriginHash();
    CBlockIndex* pIndex = pPrevIndex;
    while (pIndex && (pIndex->GetBlockHeight() % nDistributeHeight) > 0)
    {
        pIndex = pIndex->pPrev;
    }
    if (pIndex == nullptr)
    {
        StdError("BlockChain", "Get distribute vote reward tx: Find calc index fail, hashPrev: %s", hashPrev.GetHex().c_str());
        return false;
    }
    if (pIndex->GetOriginHash() != hashFork)
    {
        StdLog("BlockChain", "Get distribute vote reward tx: Fork error, calc fork: %s, prev fork: %s, hashPrev: %s",
               pIndex->GetOriginHash().GetHex().c_str(), hashFork.GetHex().c_str(), hashPrev.GetHex().c_str());
        return true;
    }
    uint256 hashCalcEndBlock = pIndex->GetBlockHash();

    auto& mapCacheForkVoteReward = mapCacheDistributeVoteReward[hashFork];
    auto rit = mapCacheForkVoteReward.begin();
    while (mapCacheForkVoteReward.size() > MAX_CACHE_DISTRIBUTE_VOTE_REWARD_BLOCK_COUNT)
    {
        if (rit->first != hashCalcEndBlock)
        {
            mapCacheForkVoteReward.erase(rit++);
        }
        else
        {
            ++rit;
        }
    }

    bool fCalcReward = false;
    if (mapCacheForkVoteReward.find(hashCalcEndBlock) == mapCacheForkVoteReward.end())
    {
        fCalcReward = true;
    }
    vector<vector<CTransaction>>& vRewardList = mapCacheForkVoteReward[hashCalcEndBlock];
    if (fCalcReward)
    {
        StdLog("BlockChain", "Get distribute vote reward tx: Calc vote reward start, calc block: [%d] %s, hashPrev: [%d] %s, fork: %s",
               CBlock::GetBlockHeightByHash(hashCalcEndBlock), hashCalcEndBlock.GetHex().c_str(),
               CBlock::GetBlockHeightByHash(hashPrev), hashPrev.GetHex().c_str(), hashFork.GetHex().c_str());
        if (!CalcEndVoteReward(hashPrev, nBlockType, nBlockHeight, nBlockTime,
                               hashFork, hashCalcEndBlock, vRewardList))
        {
            StdError("BlockChain", "Get distribute vote reward tx: Calc end vote reward fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
            mapCacheForkVoteReward.erase(hashCalcEndBlock);
            return false;
        }
        size_t nRewardTxCount = 0;
        for (const auto& vd : vRewardList)
        {
            nRewardTxCount += vd.size();
        }
        StdLog("BlockChain", "Get distribute vote reward tx: Calc vote reward end, reward tx count: %ld, calc block: [%d] %s, hashPrev: [%d] %s, fork: %s",
               nRewardTxCount, CBlock::GetBlockHeightByHash(hashCalcEndBlock), hashCalcEndBlock.GetHex().c_str(),
               CBlock::GetBlockHeightByHash(hashPrev), hashPrev.GetHex().c_str(), hashFork.GetHex().c_str());
    }

    if (pPrevIndex->IsPrimary())
    {
        int nTxListIndex = CBlock::GetBlockHeightByHash(hashPrev) % nDistributeHeight;
        if (nTxListIndex < vRewardList.size())
        {
            vVoteRewardTx = vRewardList[nTxListIndex];
            for (auto& tx : vVoteRewardTx)
            {
                tx.nTimeStamp = nBlockTime;
            }
        }
    }
    else
    {
        int nDisCount = 0;
        CBlockIndex* pSubIndex = pPrevIndex;
        while (pSubIndex && (pSubIndex->GetBlockHeight() % nDistributeHeight) > 0)
        {
            if (!pSubIndex->IsVacant())
            {
                nDisCount++;
            }
            pSubIndex = pSubIndex->pPrev;
        }
        if (nDisCount < vRewardList.size())
        {
            vVoteRewardTx = vRewardList[nDisCount];
            for (auto& tx : vVoteRewardTx)
            {
                tx.nTimeStamp = nBlockTime;
            }
        }
    }
    return true;
}

uint256 CBlockChain::GetPrimaryBlockReward(const uint256& hashPrev)
{
    uint64 u = (uint64)pow(2, (CBlock::GetBlockHeightByHash(hashPrev) + 1) / BBCP_REWARD_HALVE_CYCLE);
    return (BBCP_REWARD_INIT / uint256(u));
}

bool CBlockChain::CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot, uint256& nBlockGasUsed,
                                       uint2048& nBlockBloom, const bool fCreateBlock, const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut)
{
    uint256 hashPrevStateRoot;
    if (block.hashPrev != 0)
    {
        CBlockIndex* pIndexPrev = nullptr;
        if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev) || pIndexPrev == nullptr)
        {
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
    }
    return cntrBlock.CreateBlockStateRoot(hashFork, block, hashPrevStateRoot, hashStateRoot, hashReceiptRoot, nBlockGasUsed,
                                          nBlockBloom, fCreateBlock, fMintRatio, nTotalMintRewardOut, nBlockMintRewardOut);
}

bool CBlockChain::RetrieveDestState(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CDestState& state)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        StdLog("BlockChain", "Retrieve dest state: Retrieve index fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return cntrBlock.RetrieveDestState(hashFork, pIndex->GetStateRoot(), dest, state);
}

bool CBlockChain::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed)
{
    CBlockIndex* pLastIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pLastIndex))
    {
        StdLog("BlockChain", "Get transaction receipt: Retrieve fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    CTxIndex txIndex;
    CTransaction tx;
    if (!cntrBlock.RetrieveTx(hashFork, pLastIndex->GetBlockHash(), txid, txIndex, tx))
    {
        StdLog("BlockChain", "Get transaction receipt: Retrieve tx fail, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!tx.to.IsContract())
    {
        StdLog("BlockChain", "Get transaction receipt: Tx to not is contract, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    CDestination destTo;
    if (tx.to.IsNullContract())
    {
        destTo.SetContractId(tx.from.data, tx.nTxNonce);
    }
    else
    {
        destTo = tx.to;
    }

    CDestState state;
    if (!cntrBlock.RetrieveDestState(hashFork, pLastIndex->GetStateRoot(), destTo, state))
    {
        StdLog("BlockChain", "Get transaction receipt: Retrieve dest state fail, dest: %s, txid: %s, fork: %s",
               destTo.ToString().c_str(), txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    bytes value;
    if (!cntrBlock.RetrieveWasmState(hashFork, state.hashStorageRoot, txid, value))
    {
        StdLog("BlockChain", "Get transaction receipt: Retrieve wasm state fail, root: %s, txid: %s, fork: %s",
               state.hashStorageRoot.GetHex().c_str(), txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    CBufStream ss;
    ss.Write((char*)value.data(), value.size());
    try
    {
        ss >> receipt;
    }
    catch (const std::exception& e)
    {
        StdLog("BlockChain", "Get transaction receipt: Get receipt fail, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    CBlockIndex* pIndex = pLastIndex;
    while (pIndex != nullptr && pIndex->GetBlockNumber() > txIndex.nBlockNumber)
    {
        pIndex = pIndex->pPrev;
    }

    if (pIndex && pIndex->GetBlockNumber() == txIndex.nBlockNumber)
    {
        hashBlock = pIndex->GetBlockHash();
        nBlockGasUsed = pIndex->GetBlockGasUsed();
    }

    return true;
}

bool CBlockChain::CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult)
{
    CBlockIndex* pLastIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pLastIndex))
    {
        StdLog("BlockChain", "Call contract: Retrieve fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    if (!cntrBlock.CallWasmCode(hashFork, pLastIndex->GetBlockHeight(), CDestination(), MAX_BLOCK_GAS_LIMIT,
                                from, to, nGasPrice, nGas, nAmount,
                                btContractParam, GetNetTime(), pLastIndex->GetBlockHash(), pLastIndex->GetStateRoot(), nStatus, btResult))
    {
        StdLog("BlockChain", "Call contract: Call wasm fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract)
{
    return cntrBlock.VerifyContractAddress(hashFork, hashBlock, destContract);
}

bool CBlockChain::VerifyCreateContractTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    return cntrBlock.VerifyCreateContractTx(hashFork, hashBlock, tx);
}

bool CBlockChain::VerifyVoteRewardTx(const CBlock& block, size_t& nRewardTxCount)
{
    vector<CTransaction> vVoteRewardTx;
    if (!CalcBlockVoteRewardTx(block.hashPrev, block.nType, block.GetBlockHeight(), block.GetBlockTime(), vVoteRewardTx))
    {
        StdLog("BlockChain", "Verify Vote Reward Tx: Calculate vote reward fail, height: %d, hashPrev: %s",
               block.GetBlockHeight(), block.hashPrev.GetHex().c_str());
        return false;
    }
    if (block.vtx.size() < vVoteRewardTx.size())
    {
        StdLog("BlockChain", "Verify Vote Reward Tx: tx count error, vtx size: %ld, vote tx count: %ld, height: %d, hashPrev: %s",
               block.vtx.size(), vVoteRewardTx.size(),
               block.GetBlockHeight(), block.hashPrev.GetHex().c_str());
        return false;
    }
    for (size_t i = 0; i < vVoteRewardTx.size(); i++)
    {
        if (vVoteRewardTx[i] != block.vtx[i])
        {
            StdLog("BlockChain", "Verify Vote Reward Tx: vote reward tx error, index: %ld, vote reward tx: [%s] %s, vote dest: %s, a: %s, block tx: [%s] %s, vote dest: %s, a: %s, height: %d, hashPrev: %s",
                   i, vVoteRewardTx[i].GetTypeString().c_str(), vVoteRewardTx[i].GetHash().GetHex().c_str(), vVoteRewardTx[i].to.ToString().c_str(), CoinToTokenBigFloat(vVoteRewardTx[i].nAmount).c_str(),
                   block.vtx[i].GetTypeString().c_str(), block.vtx[i].GetHash().GetHex().c_str(), block.vtx[i].to.ToString().c_str(), CoinToTokenBigFloat(block.vtx[i].nAmount).c_str(),
                   block.GetBlockHeight(), block.hashPrev.GetHex().c_str());
            return false;
        }
    }
    nRewardTxCount = vVoteRewardTx.size();
    return true;
}

Errno CBlockChain::VerifyBlockTx(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& nReward,
                                 const std::size_t nIgnoreVerifyTx, const std::map<uint256, CAddressContext>& mapBlockAddress)
{
    uint256 nTotalFee;
    std::map<CDestination, CDestState> mapDestState;
    std::size_t nIgnoreTx = nIgnoreVerifyTx;

    // verify tx
    for (const CTransaction& tx : block.vtx)
    {
        if (tx.hashFork != hashFork)
        {
            StdLog("BlockChain", "Verify block tx: Verify tx fork error, block: %s", hashBlock.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
        if (nIgnoreTx > 0)
        {
            nIgnoreTx--;
            if (tx.nAmount > 0)
            {
                auto it = mapDestState.find(tx.to);
                if (it == mapDestState.end())
                {
                    CDestState state;
                    if (!RetrieveDestState(tx.hashFork, block.hashPrev, tx.to, state))
                    {
                        state.SetNull();
                    }
                    it = mapDestState.insert(make_pair(tx.to, state)).first;
                }
                it->second.nBalance += tx.nAmount;
            }
        }
        else
        {
            uint256 txid = tx.GetHash();

            if (tx.IsNoMintRewardTx())
            {
                StdLog("BlockChain", "Verify block tx: Verify vote or defi reward tx error, txid: %s, block: %s",
                       txid.GetHex().c_str(), hashBlock.ToString().c_str());
                return ERR_TRANSACTION_INVALID;
            }
            if (tx.nTimeStamp > block.nTimeStamp)
            {
                StdLog("BlockChain", "Verify block tx: Verify BlockTx time fail: tx time: %d, block time: %d, tx: %s, block: %s",
                       tx.nTimeStamp, block.nTimeStamp, txid.ToString().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }

            if (!tx.from.IsNull())
            {
                auto it = mapDestState.find(tx.from);
                if (it == mapDestState.end())
                {
                    CDestState state;
                    if (!RetrieveDestState(tx.hashFork, block.hashPrev, tx.from, state))
                    {
                        StdLog("BlockChain", "Verify block tx: Retrieve dest state fail, from: %s, txid: %s", tx.from.ToString().c_str(), txid.ToString().c_str());
                        state.SetNull();
                    }
                    it = mapDestState.insert(make_pair(tx.from, state)).first;
                }
                CDestState& stateFrom = it->second;

                Errno err = pCoreProtocol->VerifyTransaction(tx, block.hashPrev, block.GetBlockHeight(), stateFrom, mapBlockAddress);
                if (err != OK)
                {
                    StdLog("BlockChain", "Verify block tx: Verify transaction fail, err: %s, block: %s", ErrorString(err), txid.ToString().c_str());
                    return err;
                }

                stateFrom.nBalance -= (tx.nAmount + tx.GetTxFee());
                stateFrom.nTxNonce++;
            }

            if (tx.nAmount > 0)
            {
                auto it = mapDestState.find(tx.to);
                if (it == mapDestState.end())
                {
                    CDestState state;
                    if (!RetrieveDestState(tx.hashFork, block.hashPrev, tx.to, state))
                    {
                        state.SetNull();
                    }
                    it = mapDestState.insert(make_pair(tx.to, state)).first;
                }
                it->second.nBalance += tx.nAmount;
            }
        }

        nTotalFee += tx.GetTxFee();
    }

    if (block.txMint.nAmount > nTotalFee + nReward)
    {
        StdLog("BlockChain", "Verify block tx: Mint tx amount invalid : (0x%s > 0x%s + 0x%s)",
               block.txMint.nAmount.GetHex().c_str(), nTotalFee.GetHex().c_str(), nReward.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    return OK;
}

uint32 CBlockChain::GetBlockInvestRewardTxMaxCount()
{
    CBlock block;

    CProofOfHashWork proof;
    proof.Save(block.vchProof);
    size_t nMaxTxSize = MAX_BLOCK_SIZE - hcode::GetSerializeSize(block);

    CTransaction txReward;
    txReward.nType = CTransaction::TX_DEFI_REWARD;
    txReward.hashFork = ~uint256();
    txReward.nTimeStamp = 0;
    txReward.nTxNonce = 1;
    txReward.to = CDestination();
    txReward.nAmount = ~uint256();

    hcode::CBufStream ss;
    ss << ~uint256();
    bytes btTempData;
    ss.GetData(btTempData);
    txReward.AddTxData(CTransaction::DF_VOTEREWARD, btTempData);

    uint32 nDistributeVoteTxCount = (uint32)(nMaxTxSize / hcode::GetSerializeSize(txReward));
    if (nDistributeVoteTxCount > 100)
    {
        nDistributeVoteTxCount -= 100;
    }
    return nDistributeVoteTxCount;
    /*
    CBlock block;

    metabasenet::crypto::CPubKey pubkey(uint256("d1e1a33b30ec21b3675608679b7750ef2dd38d9bb8847ddf8a8e5c19273357ac"));
    const CDestination dest = CDestination(metabasenet::crypto::CPubKey(uint256("2ae5c9621cd4ca80653a6fa4438ad8f31b240c29344c9655e6e64c88c213ee10")));

    CTemplatePtr ptrDelegate = CTemplate::CreateTemplatePtr(new CTemplateDelegate(pubkey, dest, 500));
    CTemplatePtr ptrProof = CTemplateMint::CreateTemplatePtr(new CTemplateProof(pubkey, dest));

    size_t nMintDataSize = ptrDelegate->GetTemplateData().size();
    if (nMintDataSize < ptrProof->GetTemplateData().size())
    {
        nMintDataSize = ptrProof->GetTemplateData().size();
    }

    size_t nSigSize = nMintDataSize + 64 + 2;
    size_t nProofSize = 680;
    size_t nMaxTxSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - nSigSize - nProofSize;
    nMaxTxSize = nMaxTxSize * 9 / 10;

    CTransaction txReward;

    hcode::CBufStream ss;
    ss << ~uint256();
    bytes btTempData;
    btTempData.assign(ss.GetData(), ss.GetData() + ss.GetSize());
    txReward.AddTxData(CTransaction::DF_VOTEREWARD, btTempData);

    return (nMaxTxSize / GetSerializeSize(txReward));*/
}

bool CBlockChain::CalcEndVoteReward(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime,
                                    const uint256& hashFork, const uint256& hashCalcEndBlock, vector<vector<CTransaction>>& vRewardList)
{
    uint256 nTotalMintReward;
    map<CDestination, pair<CDestination, uint256>> mapVoteReward;
    if (!CalcDistributeVoteReward(hashCalcEndBlock, nTotalMintReward, mapVoteReward))
    {
        StdError("BlockChain", "Calc end vote reward tx: Calc distribute pledge reward fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    map<CDestination, uint256> mapInviteReward;
    if (!CalcInviteRelationReward(hashFork, hashCalcEndBlock, nTotalMintReward, mapInviteReward))
    {
        StdError("BlockChain", "Calc end vote reward tx: Calc distribute invite reward fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    class CReward
    {
    public:
        CReward() {}

        uint256 nTotalReward;
        uint256 nVoteReward;
    };

    map<CDestination, CReward> mapReward;
    for (const auto& kv : mapVoteReward)
    {
        auto& reward = mapReward[kv.second.first];
        reward.nTotalReward += kv.second.second;
        reward.nVoteReward += kv.second.second;
    }
    for (const auto& kv : mapInviteReward)
    {
        mapReward[kv.first].nTotalReward += kv.second;
    }

    if (!mapReward.empty())
    {
        const uint32 nSingleBlockTxCount = nMaxBlockRewardTxCount;
        if (nSingleBlockTxCount == 0)
        {
            StdError("BlockChain", "Calc end vote reward tx: Calc Single Block Distribute Vote Reward Tx Count fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
            return false;
        }
        uint32 nAddBlockCount = 0;
        if (mapReward.size() > 0)
        {
            nAddBlockCount = mapReward.size() / nSingleBlockTxCount;
            if ((mapReward.size() % nSingleBlockTxCount) > 0)
            {
                nAddBlockCount++;
            }
        }
        StdDebug("BlockChain", "Calc end vote reward tx: Single block tx count: %d, Pledge reward address count: %lu, Add block count: %d",
                 nSingleBlockTxCount, mapReward.size(), nAddBlockCount);

        vRewardList.resize(nAddBlockCount);

        uint32 nAddTxCount = 0;
        for (const auto& kv : mapReward)
        {
            CTransaction txReward;
            txReward.nType = CTransaction::TX_DEFI_REWARD;
            txReward.hashFork = hashFork;
            txReward.nTimeStamp = 0;
            txReward.nTxNonce = nAddTxCount;
            txReward.to = kv.first;
            txReward.nAmount = kv.second.nTotalReward;

            hcode::CBufStream ss;
            ss << kv.second.nVoteReward;
            bytes btTempData;
            ss.GetData(btTempData);
            txReward.AddTxData(CTransaction::DF_VOTEREWARD, btTempData);

            if (txReward.to.IsTemplate())
            {
                if (txReward.to.GetTemplateId().GetType() != TEMPLATE_DELEGATE
                    && txReward.to.GetTemplateId().GetType() != TEMPLATE_VOTE)
                {
                    StdError("BlockChain", "Calc end vote reward tx: To address not is delegate or vote, to: %s, hashPrev: %s",
                             txReward.to.ToString().c_str(), hashPrev.GetHex().c_str());
                    return false;
                }
                CAddressContext ctxtAddress;
                if (!cntrBlock.RetrieveAddressContext(hashFork, hashPrev, txReward.to.data, ctxtAddress))
                {
                    CBlockIndex* pPrevIndex = nullptr;
                    if (!cntrBlock.RetrieveIndex(hashPrev, &pPrevIndex) || pPrevIndex == nullptr)
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Retrieve prev index fail, to: %s, hashPrev: %s",
                                 txReward.to.ToString().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    if (pPrevIndex->IsPrimary())
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Primary not address context fail, to: %s, hashPrev: %s",
                                 txReward.to.ToString().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    if (!cntrBlock.RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), pPrevIndex->GetRefBlock(), txReward.to.data, ctxtAddress))
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Retrieve ref block address context fail, to: %s, ref block: %s, hashPrev: %s",
                                 txReward.to.ToString().c_str(), pPrevIndex->GetRefBlock().GetHex().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    CTemplateAddressContext ctxtTemplate;
                    if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Get template address context fail, to: %s, hashPrev: %s",
                                 txReward.to.ToString().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    txReward.AddTxData(CTransaction::DF_TEMPLATEDATA, ctxtTemplate.btData);
                }
            }

            //StdDebug("BlockChain", "Calc end vote reward tx: distribute reward tx: calc: [%d] %s, dist height: %d, reward amount: %s, dest: %s, hashPrev: %s, hashFork: %s",
            //         CBlock::GetBlockHeightByHash(hashCalcEndBlock), hashCalcEndBlock.GetHex().c_str(), nBlockHeight,
            //         CoinToTokenBigFloat(txReward.nAmount).c_str(), txReward.to.ToString().c_str(), hashPrev.GetHex().c_str(), hashFork.GetHex().c_str());

            vRewardList[nAddTxCount / nSingleBlockTxCount].push_back(txReward);
            nAddTxCount++;
        }
    }
    return true;
}

bool CBlockChain::CalcDistributeVoteReward(const uint256& hashCalcEndBlock, uint256& nTotalMintReward, std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteReward)
{
    // hashCalcEndBlock at 4320 height or multiple
    const int nDistributeHeight = VOTE_REWARD_DISTRIBUTE_HEIGHT;
    if ((CBlock::GetBlockHeightByHash(hashCalcEndBlock) % nDistributeHeight) != 0)
    {
        StdLog("BlockChain", "Calculate distribute vote reward: height error, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }
    mapVoteReward.clear();

    CBlockIndex* pTailIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashCalcEndBlock, &pTailIndex) || pTailIndex == nullptr)
    {
        StdLog("BlockChain", "Calculate distribute vote reward: RetrieveIndex fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    class CCalcBlock
    {
    public:
        CCalcBlock()
          : nRewardRation(0) {}

        uint256 hashPrimaryBlock;
        CDestination destMint;
        uint32 nRewardRation;
        uint256 nRewardAmount;
    };

    // Gets the block data to be calculated
    std::map<uint32, CCalcBlock> mapCalcBlock;
    //int nTailHeight = pTailIndex->GetBlockHeight();                        // 4320
    int nBeginHeight = pTailIndex->GetBlockHeight() - nDistributeHeight + 1; // 1
    CBlockIndex* pIndex = pTailIndex;
    while (pIndex && pIndex->pPrev && !pIndex->IsOrigin() && pIndex->GetBlockHeight() >= nBeginHeight)
    {
        nTotalMintReward += pIndex->GetMintReward();
        if ((pIndex->IsPrimary() && pIndex->nMintType == CTransaction::TX_STAKE)
            || (!pIndex->IsPrimary() && (pIndex->IsSubsidiary() || pIndex->IsExtended())))
        {
            CCalcBlock& calcBlock = mapCalcBlock[pIndex->GetBlockHeight()];
            if (calcBlock.hashPrimaryBlock == 0)
            {
                CBlockIndex* pPrimaryIndex = nullptr;
                if (pIndex->IsPrimary())
                {
                    pPrimaryIndex = pIndex;
                }
                else
                {
                    if (!cntrBlock.RetrieveIndex(pIndex->GetRefBlock(), &pPrimaryIndex) || pPrimaryIndex == nullptr)
                    {
                        StdLog("BlockChain", "Calculate block vote reward: Retrieve ref index fail, ref block: %s, block: %s",
                               pIndex->GetRefBlock().GetHex().c_str(), pIndex->GetBlockHash().GetHex().c_str());
                        return false;
                    }
                }
                calcBlock.hashPrimaryBlock = pPrimaryIndex->GetBlockHash();
                if (!pIndex->IsPrimary() && pPrimaryIndex->destMint != pIndex->destMint)
                {
                    StdLog("BlockChain", "Calculate block vote reward: destMint error, destMint: %s, primary mint: %s, block: %s",
                           pIndex->destMint.ToString().c_str(), pPrimaryIndex->destMint.ToString().c_str(), pIndex->GetBlockHash().GetHex().c_str());
                    return false;
                }
                if (pIndex->destMint.IsTemplate() && pIndex->destMint.GetTemplateId().GetType() == TEMPLATE_DELEGATE)
                {
                    if (!cntrBlock.GetDelegateMintRewardRatio(calcBlock.hashPrimaryBlock, pIndex->destMint, calcBlock.nRewardRation))
                    {
                        StdLog("BlockChain", "Calculate block vote reward: Get delegate mint reward ratio fail, destMint: %s, primary block: %s",
                               pIndex->destMint.ToString().c_str(), calcBlock.hashPrimaryBlock.GetHex().c_str());
                        return false;
                    }
                }
                else
                {
                    StdLog("BlockChain", "Calculate block vote reward: destMint not is delegate, destMint: %s, block: %s",
                           pIndex->destMint.ToString().c_str(), pIndex->GetBlockHash().GetHex().c_str());
                    return false;
                }
                calcBlock.destMint = pIndex->destMint;
            }
            calcBlock.nRewardAmount += pIndex->GetBlockReward();
        }
        pIndex = pIndex->pPrev;
    }

    // Obtain the main chain block hash, which is used to obtain voting data
    uint256 hashPrimaryBeginBlock;
    uint256 hashPrimaryTailBlock;
    {
        CBlockIndex* pPrimaryIndex = nullptr;
        if (pTailIndex->IsPrimary())
        {
            if (pTailIndex->pPrev == nullptr)
            {
                StdLog("BlockChain", "Calculate block vote reward: Primary pPrev is null, tail block: %s",
                       pTailIndex->GetBlockHash().GetHex().c_str());
                return false;
            }
            pPrimaryIndex = pTailIndex->pPrev;
        }
        else
        {
            if (!cntrBlock.RetrieveIndex(pTailIndex->GetRefBlock(), &pPrimaryIndex)
                || pPrimaryIndex == nullptr || pPrimaryIndex->pPrev == nullptr)
            {
                StdLog("BlockChain", "Calculate block vote reward: Retrieve ref index fail, ref block: %s, tail block: %s",
                       pTailIndex->GetRefBlock().GetHex().c_str(), pTailIndex->GetBlockHash().GetHex().c_str());
                return false;
            }
            pPrimaryIndex = pPrimaryIndex->pPrev;
        }
        hashPrimaryTailBlock = pPrimaryIndex->GetBlockHash(); // 4320-1=4319
        while (pPrimaryIndex && pPrimaryIndex->GetBlockHeight() >= nBeginHeight)
        {
            pPrimaryIndex = pPrimaryIndex->pPrev;
        }
        if (pPrimaryIndex == nullptr)
        {
            StdLog("BlockChain", "Calculate block vote reward: Begin primary index is null, tail block: %s",
                   pTailIndex->GetBlockHash().GetHex().c_str());
            return false;
        }
        hashPrimaryBeginBlock = pPrimaryIndex->GetBlockHash(); // 1-1=0
    }

    // Slice row voting data, starting from the bottom height
    class CListDayVoteWalker : public storage::CDayVoteWalker
    {
    public:
        CListDayVoteWalker(const std::map<uint32, CCalcBlock>& mapCalcBlockIn, std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteRewardIn)
          : mapCalcBlock(mapCalcBlockIn), mapVoteReward(mapVoteRewardIn) {}

        // nHeight = (nBeginHeight-1 ~ nTailHeight-1) or (0 ~ 4319)
        bool Walk(const uint32 nHeight, const std::map<CDestination, std::pair<std::map<CDestination, CVoteContext>, uint256>>& mapDelegateVote) override
        {
            auto it = mapCalcBlock.find(nHeight + 1);
            if (it != mapCalcBlock.end())
            {
                const CCalcBlock& calcBlock = it->second;

                auto mt = mapDelegateVote.find(calcBlock.destMint);
                if (mt != mapDelegateVote.end())
                {
                    uint256 nBlockReward = calcBlock.nRewardAmount;
                    if (nBlockReward != 0 && calcBlock.nRewardRation != 0)
                    {
                        nBlockReward -= (nBlockReward * uint256(calcBlock.nRewardRation) / uint256(MINT_REWARD_PER));
                    }
                    const auto& mapDestVote = mt->second.first;
                    const uint256& nDelegateTotalVoteAmount = mt->second.second;

                    // distribute reward
                    uint256 nDistributeTotalReward = 0;
                    for (const auto& kv : mapDestVote)
                    {
                        uint256 nDestReward = nBlockReward * kv.second.nVoteAmount / nDelegateTotalVoteAmount;
                        auto& voteReward = mapVoteReward[kv.first];
                        const CVoteContext& ctxtVote = kv.second;
                        if (ctxtVote.nRewardMode == CVoteContext::REWARD_MODE_VOTE)
                        {
                            voteReward.first = kv.first;
                        }
                        else
                        {
                            voteReward.first = ctxtVote.destOwner;
                        }
                        voteReward.second += nDestReward;
                        nDistributeTotalReward += nDestReward;
                    }
                    if (mapDestVote.size() > 0 && nBlockReward > nDistributeTotalReward)
                    {
                        mapVoteReward[mapDestVote.begin()->first].second += (nBlockReward - nDistributeTotalReward);
                    }
                }
            }
            return true;
        }

    public:
        const std::map<uint32, CCalcBlock>& mapCalcBlock;
        std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteReward;
    };

    CListDayVoteWalker walker(mapCalcBlock, mapVoteReward);
    if (!cntrBlock.WalkThroughDayVote(hashPrimaryBeginBlock, hashPrimaryTailBlock, walker))
    {
        StdLog("BlockChain", "Calculate block vote reward: Walk through day vote fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::CalcInviteRelationReward(const uint256& hashFork, const uint256& hashCalcEndBlock, const uint256& nTotalReward, std::map<CDestination, uint256>& mapInviteReward)
{
    std::map<CDestination, CDestination> mapInviteContext; // key: sub, value: parent
    if (!cntrBlock.ListInviteRelation(hashFork, hashCalcEndBlock, mapInviteContext))
    {
        StdLog("BlockChain", "Calculate block invite reward: List invite relation fail, hashFork: %s, hashCalcEndBlock: %s",
               hashFork.GetHex().c_str(), hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    std::map<CDestination, CDestState> mapBlockState;
    if (!cntrBlock.ListDestState(hashFork, hashCalcEndBlock, mapBlockState))
    {
        StdLog("BlockChain", "Calculate block invite reward: List dest state fail, hashFork: %s, hashCalcEndBlock: %s",
               hashFork.GetHex().c_str(), hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    uint256 nTotalPower;
    const uint64 nBaseBegin = 50;
    const uint64 nBaseEnd = 200;
    std::map<CDestination, uint64> mapCalcDest;
    for (const auto& kv : mapInviteContext)
    {
        uint64 nBalanceSub = 0;
        uint64 nBalanceParent = 0;
        auto it = mapBlockState.find(kv.first);
        if (it != mapBlockState.end())
        {
            nBalanceSub = (it->second.GetBalance() / COIN).Get64();
        }
        auto mt = mapBlockState.find(kv.second);
        if (mt != mapBlockState.end())
        {
            nBalanceParent = (mt->second.GetBalance() / COIN).Get64();
        }
        uint64 nBalance = std::min(nBalanceSub, nBalanceParent);
        if (nBalance > 0)
        {
            uint64 nPower = 0;
            if (nBalance >= nBaseBegin && nBalance <= nBaseEnd)
            {
                nPower = (nBalance * 3 / 2);
            }
            else
            {
                nPower = nBalance;
            }
            mapCalcDest[kv.second] += nPower;
            nTotalPower += nPower;
        }
    }

    uint256 nStatReward;
    CDestination destFirst;
    for (auto& kv : mapCalcDest)
    {
        if (kv.second > 0)
        {
            uint256 nReward = nTotalReward * uint256(kv.second) / nTotalPower;
            mapInviteReward[kv.first] = nReward;
            nStatReward += nReward;
            if (destFirst.IsNull())
            {
                destFirst = kv.first;
            }
        }
    }
    if (destFirst.IsNull() && mapCalcDest.size() > 0 && nTotalReward > 0)
    {
        destFirst = mapCalcDest.begin()->first;
    }
    if (nStatReward < nTotalReward && !destFirst.IsNull())
    {
        mapInviteReward[destFirst] += (nTotalReward - nStatReward);
    }
    return true;
}

} // namespace metabasenet

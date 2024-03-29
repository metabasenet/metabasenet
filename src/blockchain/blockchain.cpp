// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockchain.h"

#include "bloomfilter/bloomfilter.h"
#include "consblockvote.h"
#include "delegatecomm.h"
#include "delegateverify.h"
#include "mevm/evmexec.h"
#include "param.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/vote.h"

using namespace std;
using namespace mtbase;

#define ENROLLED_CACHE_COUNT (120)
#define AGREEMENT_CACHE_COUNT (16)
#define PIGGYBACK_CACHE_COUNT (2560)

namespace metabasenet
{

//////////////////////////////
// CBlockChain

CBlockChain::CBlockChain()
  : cacheEnrolled(ENROLLED_CACHE_COUNT), cacheAgreement(AGREEMENT_CACHE_COUNT), cachePiggyback(PIGGYBACK_CACHE_COUNT), nMaxBlockRewardTxCount(0)
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
    StdLog("BlockChain", "HandleInvoke: Max block reward tx count: %d", nMaxBlockRewardTxCount);

    if (!cntrBlock.Initialize(Config()->pathData, blockGenesis.GetHash(), Config()->fFullDb, Config()->fRewardCheck))
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
    //if (Config()->nMagicNum == MAINNET_MAGICNUM)
    if (!TESTNET_FLAG)
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
    cachePiggyback.Clear();
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

bool CBlockChain::GetForkProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.RetrieveProfile(hashFork, profile, hashMainChainRefBlock);
}

bool CBlockChain::GetForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock);
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

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        StdLog("CBlockChain", "Get Block Location: Retrieve index fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    nChainId = pIndex->nChainId;
    hashFork = pIndex->GetOriginHash();
    nHeight = pIndex->GetBlockHeight();
    return true;
}

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight, uint256& hashNext)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    nChainId = pIndex->nChainId;
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

bool CBlockChain::GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock)
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
        if (pIndex->GetBlockSlot() == nSlot)
        {
            hashBlock = pIndex->GetBlockHash();
            return true;
        }
        pIndex = pIndex->pPrev;
    }
    return false;
}

bool CBlockChain::GetBlockHashList(const uint256& hashFork, const uint32 nHeight, vector<uint256>& vBlockHash)
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
    return cntrBlock.GetBlockHashByNumber(hashFork, nNumber, hashBlock);
}

bool CBlockChain::GetBlockStatus(const uint256& hashBlock, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    status.hashFork = pIndex->GetOriginHash();
    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.hashRefBlock = pIndex->GetRefBlock();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nBlockSlot = pIndex->GetBlockSlot();
    status.nBlockNumber = pIndex->GetBlockNumber();
    status.nTotalTxCount = pIndex->GetTxCount();
    status.nUserTxCount = pIndex->GetUserTxCount();
    status.nRewardTxCount = pIndex->GetRewardTxCount();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nBlockType = pIndex->nType;
    status.nMintType = pIndex->nMintType;
    status.hashStateRoot = pIndex->GetStateRoot();
    status.destMint = pIndex->destMint;
    return true;
}

bool CBlockChain::GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }
    status.hashFork = pIndex->GetOriginHash();
    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.hashRefBlock = pIndex->GetRefBlock();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nBlockSlot = pIndex->GetBlockSlot();
    status.nBlockNumber = pIndex->GetBlockNumber();
    status.nTotalTxCount = pIndex->GetTxCount();
    status.nUserTxCount = pIndex->GetUserTxCount();
    status.nRewardTxCount = pIndex->GetRewardTxCount();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nBlockType = pIndex->nType;
    status.nMintType = pIndex->nMintType;
    status.hashStateRoot = pIndex->GetStateRoot();
    status.destMint = pIndex->destMint;
    return true;
}

bool CBlockChain::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime)
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

bool CBlockChain::GetBlockHashOfRefBlock(const uint256& hashRefBlock, const int nHeight, uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashRefBlock, &pIndex) || pIndex->GetBlockHeight() < nHeight)
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
    return true;
}

bool CBlockChain::GetLastBlockTime(const uint256& hashFork, int nDepth, vector<uint64>& vTime)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }

    vTime.clear();
    if (nDepth > 0)
    {
        vTime.reserve(nDepth);
    }
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
        StdLog("CBlockChain", "Get Block: Retrieve fail, block: %s", hashBlock.GetHex().c_str());
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

bool CBlockChain::GetTransactionAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex)
{
    return cntrBlock.RetrieveTxAndIndex(hashFork, txid, tx, hashAtFork, txIndex);
}

bool CBlockChain::ExistsTx(const uint256& hashFork, const uint256& txid)
{
    return cntrBlock.ExistsTx(hashFork, txid);
}

bool CBlockChain::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return cntrBlock.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockChain::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return cntrBlock.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockChain::GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    return cntrBlock.GetForkStorageMaxHeight(hashFork, nMaxHeight);
}

Errno CBlockChain::AddNewBlock(const uint256& hashBlock, const CBlock& block, uint256& hashFork, CBlockChainUpdate& update)
{
    Errno err = OK;

    if (cntrBlock.Exists(hashBlock))
    {
        StdLog("BlockChain", "Add new block: Already exists, block: %s", hashBlock.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Add new block: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    hashFork = pIndexPrev->GetOriginHash();

    err = pCoreProtocol->ValidateBlock(hashFork, pIndexPrev->GetRefBlock(), block);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new block: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    uint256 nReward;
    CDelegateAgreement agreement;
    uint256 nEnrollTrust;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hashBlock, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new block: Verify block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    std::map<CDestination, CAddressContext> mapBlockAddress;
    if (!cntrBlock.GetBlockAddress(hashFork, hashBlock, block, mapBlockAddress))
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

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Add new origin: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    uint256 hashFork = hashBlock;

    err = pCoreProtocol->ValidateBlock(hashFork, pIndexPrev->GetRefBlock(), block);
    if (err != OK)
    {
        StdLog("BlockChain", "Add new origin: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    if (!pIndexPrev->IsPrimary())
    {
        StdLog("BlockChain", "Add new origin: Prev block must main chain block");
        return ERR_BLOCK_TYPE_INVALID;
    }

    uint256 hashBlockRef;
    uint64 nTimeRef;
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
    if (!cntrBlock.RetrieveProfile(pIndexPrev->GetOriginHash(), parent, hashBlockRef))
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

bool CBlockChain::GetBlockMintReward(const uint256& hashPrev, const bool fPow, uint256& nReward, const uint256& hashMainChainRefBlock)
{
    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Get block reward: Retrieve Prev Index Error, hashPrev: %s", hashPrev.ToString().c_str());
        return false;
    }

    if (pIndexPrev->IsPrimary())
    {
        if (fPow)
        {
            nReward = 0;
        }
        else
        {
            nReward = GetPrimaryBlockReward(hashPrev);
            if (nReward < 0)
            {
                StdLog("BlockChain", "Get block reward: Get block invest total reward fail, hashPrev: %s", hashPrev.ToString().c_str());
                return false;
            }
        }
    }
    else
    {
        CProfile profile;
        if (!GetForkProfile(pIndexPrev->GetOriginHash(), profile, hashMainChainRefBlock))
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
            if (u == 0)
            {
                nReward = 0;
            }
            else
            {
                nReward = profile.nMintReward / uint256(u);
            }
        }
    }
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

bool CBlockChain::GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes)
{
    return cntrBlock.GetDelegateVotes(pCoreProtocol->GetGenesisBlockHash(), hashRefBlock, destDelegate, nVotes);
}

bool CBlockChain::GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight)
{
    CVoteContext ctxVote;
    if (!cntrBlock.RetrieveDestVoteContext(hashRefBlock, destVote, ctxVote))
    {
        return false;
    }
    CAddressContext ctxAddress;
    if (!cntrBlock.RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), hashRefBlock, destVote, ctxAddress))
    {
        return false;
    }
    if (!ctxAddress.IsTemplate())
    {
        return false;
    }
    nTemplateType = ctxAddress.GetTemplateType();
    nVotes = ctxVote.nVoteAmount;
    nUnlockHeight = ctxVote.nFinalHeight;
    return true;
}

bool CBlockChain::ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    return cntrBlock.GetDelegateList(hashRefBlock, nStartIndex, nCount, mapVotes);
}

bool CBlockChain::VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& hashBlockRef)
{
    uint64 nRefTimeStamp = 0;
    if (hashBlockRef != 0 && (block.IsSubsidiary() || block.IsExtended()))
    {
        CBlockIndex* pIndexRef;
        if (!cntrBlock.RetrieveIndex(hashBlockRef, &pIndexRef))
        {
            StdLog("BlockChain", "Verify Repeat Block: RetrieveIndex fail, hashBlockRef: %s, block: %s",
                   hashBlockRef.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
        if (block.IsSubsidiary())
        {
            if (block.GetBlockTime() != pIndexRef->GetBlockTime())
            {
                StdLog("BlockChain", "Verify Repeat Block: Subsidiary block time error, block time: %lu, ref block time: %lu, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
                || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING
                || ((block.GetBlockTime() - pIndexRef->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
            {
                StdLog("BlockChain", "Verify Repeat Block: Extended block time error, block time: %lu, ref block time: %lu, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
        }
        nRefTimeStamp = pIndexRef->GetBlockTime();
    }
    return cntrBlock.VerifyRepeatBlock(hashFork, hashBlock, block.GetBlockHeight(), block.txMint.GetToAddress(), block.nType, block.GetBlockTime(), nRefTimeStamp, EXTENDED_BLOCK_SPACING);
}

bool CBlockChain::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, uint256>& mapVote)
{
    return cntrBlock.GetBlockDelegateVote(hashBlock, mapVote);
}

bool CBlockChain::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount)
{
    return cntrBlock.RetrieveDestDelegateVote(hashBlock, destDelegate, nVoteAmount);
}

bool CBlockChain::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return cntrBlock.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockChain::GetDelegateEnrollByHeight(const uint256& hashRefBlock, const int nEnrollHeight, std::map<CDestination, storage::CDiskPos>& mapEnrollTxPos)
{
    CBlockIndex* pIndex;
    if (!cntrBlock.RetrieveIndex(hashRefBlock, &pIndex))
    {
        StdLog("BlockChain", "Get Delegate Enroll By Height: Retrieve block index, block: %s", hashRefBlock.ToString().c_str());
        return false;
    }
    if (pIndex->GetBlockHeight() >= CONSENSUS_ENROLL_INTERVAL)
    {
        vector<uint256> vBlockRange;
        for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL; i++)
        {
            vBlockRange.push_back(pIndex->GetBlockHash());
            pIndex = pIndex->pPrev;
        }
        if (!cntrBlock.GetRangeDelegateEnroll(nEnrollHeight, vBlockRange, mapEnrollTxPos))
        {
            StdLog("BlockChain", "Get Delegate Enroll By Height: Get delegate enroll fail, enroll height: %d, block: %s", nEnrollHeight, hashRefBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockChain::VerifyDelegateEnroll(const uint256& hashRefBlock, const int nEnrollHeight, const CDestination& destDelegate)
{
    std::map<CDestination, storage::CDiskPos> mapEnrollTxPos;
    if (GetDelegateEnrollByHeight(hashRefBlock, nEnrollHeight, mapEnrollTxPos))
    {
        if (mapEnrollTxPos.find(destDelegate) != mapEnrollTxPos.end())
        {
            return false;
        }
    }
    return true;
}

bool CBlockChain::VerifyDelegateMinVote(const uint256& hashRefBlock, const uint32 nHeight, const CDestination& destDelegate)
{
    if (CBlock::GetBlockHeightByHash(hashRefBlock) < nHeight)
    {
        return false;
    }
    uint256 hashBlock;
    if (!GetBlockHashOfRefBlock(hashRefBlock, nHeight, hashBlock))
    {
        StdLog("BlockChain", "Verify delegate min vote: Get refblock fail, height: %d, refblock: %s", nHeight, hashRefBlock.ToString().c_str());
        return false;
    }
    uint256 nVoteAmount;
    if (!cntrBlock.RetrieveDestDelegateVote(hashBlock, destDelegate, nVoteAmount))
    {
        StdLog("BlockChain", "Verify delegate min vote: Get vote fail, delegate address: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.ToString().c_str());
        return false;
    }
    if (nVoteAmount < DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT)
    {
        StdLog("BlockChain", "Verify delegate min vote: Not enough votes, votes: %s, delegate address: %s, block: %s",
               CoinToTokenBigFloat(nVoteAmount).c_str(), destDelegate.ToString().c_str(), hashBlock.ToString().c_str());
        return false;
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

    if (!cntrBlock.RetrieveAvailDelegate(hashBlock, pIndex->GetBlockHeight(), vBlockRange, DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT,
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
        StdLog("BlockChain", "Get Block Delegate Agreement: Retrieve block index fail, block: %s", hashBlock.ToString().c_str());
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
        StdLog("BlockChain", "Get Block Delegate Agreement: Retrieve block fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL + 1; i++)
    {
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), enrolled))
    {
        StdLog("BlockChain", "Get Block Delegate Agreement: Get delegate enrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    CProofOfDelegate proof;
    if (!block.IsProofOfWork())
    {
        if (!block.GetDelegateProof(proof))
        {
            StdLog("BlockChain", "Get Block Delegate Agreement: Get delegate proof fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    if (!verifier.VerifyProof(proof.nWeight, proof.nAgreement, proof.btDelegateData, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        StdLog("BlockChain", "Get Block Delegate Agreement: Invalid block proof, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    pCoreProtocol->GetDelegatedBallot(pIndexRef->GetBlockHeight(), agreement.nAgreement, agreement.nWeight, mapBallot, enrolled.vecAmount, pIndex->GetMoneySupply(), agreement.vBallot);

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

bool CBlockChain::GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, const CBlockIndex* pIndexPrev,
                                            CDelegateAgreement& agreement, uint256& nEnrollTrust)
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
        StdLog("BlockChain", "Get Block Delegate Agreement: GetBlockDelegateEnrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    CProofOfDelegate proof;
    if (!block.IsProofOfWork())
    {
        if (!block.GetDelegateProof(proof))
        {
            StdLog("BlockChain", "Get Block Delegate Agreement: Get delegate proof fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    if (!verifier.VerifyProof(proof.nWeight, proof.nAgreement, proof.btDelegateData, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        StdLog("BlockChain", "Get Block Delegate Agreement: Invalid block proof : %s", hashBlock.ToString().c_str());
        return false;
    }

    nEnrollTrust = pCoreProtocol->GetDelegatedBallot(pIndexPrev->GetBlockHeight() + 1, agreement.nAgreement, agreement.nWeight, mapBallot,
                                                     enrolled.vecAmount, pIndex->GetMoneySupply(), agreement.vBallot);

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

bool CBlockChain::GetBlockDelegateVoteAddress(const uint256& hashBlock, std::set<CDestination>& setVoteAddress)
{
    std::map<CDestination, uint256> mapVote;
    if (!cntrBlock.GetBlockDelegateVote(hashBlock, mapVote))
    {
        StdLog("BlockChain", "Get block delegate vote address: Get delegate vote fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    std::map<std::pair<uint256, CDestination>, CDestination> mapVoteSort;
    for (auto& kv : mapVote)
    {
        mapVoteSort.insert(std::make_pair(std::make_pair(kv.second, kv.first), kv.first));
    }
    for (auto it = mapVoteSort.rbegin(); it != mapVoteSort.rend() && setVoteAddress.size() < MAX_DELEGATE_BLOCK_VOTE; ++it)
    {
        //StdDebug("BlockChain", "Get block delegate vote address: vote amount: %s, address: %s", CoinToTokenBigFloat(it->first.first).c_str(), it->second.ToString().c_str());
        setVoteAddress.insert(it->second);
    }
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

uint64 CBlockChain::GetNextBlockTimestamp(const uint256& hashPrev)
{
    CBlockIndex* pIndexPrev = nullptr;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev) || pIndexPrev == nullptr)
    {
        return 0;
    }
    return pCoreProtocol->GetNextBlockTimestamp(pIndexPrev->GetBlockTime());
}

Errno CBlockChain::VerifyPowBlock(const CBlock& block, bool& fLongChain)
{
    uint256 hashBlock = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hashBlock))
    {
        StdLog("BlockChain", "Verify poa block: Already exists, block: %s", hashBlock.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        StdLog("BlockChain", "Verify poa block: Retrieve prev index fail, prev block: %s", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    uint256 hashFork = pIndexPrev->GetOriginHash();

    err = pCoreProtocol->ValidateBlock(hashFork, pIndexPrev->GetRefBlock(), block);
    if (err != OK)
    {
        StdLog("BlockChain", "Verify poa block: Validate block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    uint256 nReward;
    CDelegateAgreement agreement;
    uint256 nEnrollTrust;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hashBlock, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        StdLog("BlockChain", "Verify poa block: Verify block fail, err: %s, block: %s", ErrorString(err), hashBlock.ToString().c_str());
        return err;
    }

    std::map<CDestination, CAddressContext> mapBlockAddress;
    if (!cntrBlock.GetBlockAddress(hashFork, hashBlock, block, mapBlockAddress))
    {
        StdError("BlockChain", "Verify poa block: Verify block address fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CBlockEx blockex(block);

    size_t nIgnoreVerifyTx = 0;
    if (!VerifyVoteRewardTx(block, nIgnoreVerifyTx))
    {
        StdError("BlockChain", "Verify poa block: Verify invest reward tx fail, block: %s", hashBlock.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    err = VerifyBlockTx(hashFork, hashBlock, block, nReward, nIgnoreVerifyTx, mapBlockAddress);
    if (err != OK)
    {
        StdError("BlockChain", "Verify poa block: Verify block tx fail, block: %s", hashBlock.ToString().c_str());
        return err;
    }

    // Get block trust
    uint256 nNewBlockChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nNewBlockChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
    {
        StdLog("BlockChain", "Verify poa block: get block trust fail, block: %s", hashBlock.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    nNewBlockChainTrust += pIndexPrev->nChainTrust;

    CBlockIndex* pIndexFork = nullptr;
    if (cntrBlock.RetrieveFork(pIndexPrev->GetOriginHash(), &pIndexFork)
        && pIndexFork->nChainTrust > nNewBlockChainTrust)
    {
        StdLog("BlockChain", "Verify poa block: Short chain, new block height: %d, block: %s, fork chain trust: %s, fork last block: %s",
               block.GetBlockHeight(), hashBlock.GetHex().c_str(), pIndexFork->nChainTrust.GetValueHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str());
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

    update = CBlockChainUpdate(pValidLastIndex, {});
    if (!cntrBlock.GetBlockBranchList(pValidLastIndex->GetBlockHash(), update.vBlockRemove, update.vBlockAddNew))
    {
        StdError("BlockChain", "Check Fork Valid Last: get block branch list fail, last block: %s, fork: %s",
                 pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    //view.GetTxUpdated(update.setTxUpdate);
    //view.GetBlockChanges(update.vBlockAddNew, update.vBlockRemove);

    if (!cntrBlock.UpdateForkNext(hashFork, pValidLastIndex, update.vBlockRemove, update.vBlockAddNew))
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
    uint256 hashPrimaryRefBlock;
    if (block.IsPrimary())
    {
        hashPrimaryRefBlock = block.hashPrev;
    }
    else
    {
        // CBlockIndex* pPrevIndex;
        // if (!cntrBlock.RetrieveIndex(block.hashPrev, &pPrevIndex))
        // {
        //     StdLog("BlockChain", "Check created fork height: Get prev block index fail, prev block: %s", block.hashPrev.ToString().c_str());
        //     return false;
        // }
        // hashPrimaryRefBlock = pPrevIndex->hashRefBlock;
        hashPrimaryRefBlock = block.GetRefBlock();
    }
    int nCreatedHeight = cntrBlock.GetForkCreatedHeight(hashFork, hashPrimaryRefBlock);
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

Errno CBlockChain::VerifyBlock(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev,
                               uint256& nReward, CDelegateAgreement& agreement, uint256& nEnrollTrust, CBlockIndex** ppIndexRef)
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

        if (!VerifyBlockCertTx(hashBlock, block))
        {
            StdLog("BlockChain", "Verify block: Verify cert tx fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_CERTTX_OUT_OF_BOUND;
        }

        if (!GetBlockDelegateAgreement(hashBlock, block, pIndexPrev, agreement, nEnrollTrust))
        {
            StdLog("BlockChain", "Verify block: Get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!GetBlockMintReward(block.hashPrev, agreement.IsProofOfWork(), nReward, block.hashPrev))
        {
            StdLog("BlockChain", "Verify block: Get mint reward fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_COINBASE_INVALID;
        }

        if (!VerifyBlockVoteResult(hashBlock, block))
        {
            StdLog("BlockChain", "Verify block: Verify block vote fail, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        if (!VerifyBlockCrosschainProve(hashBlock, block))
        {
            StdLog("BlockChain", "Verify block: Verify block crosschain prove fail, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
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
        if (!block.GetPiggybackProof(proof) || proof.hashRefBlock == 0)
        {
            StdLog("BlockChain", "Verify block: SubFork load proof fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }
        cachePiggyback.AddNew(hashBlock, proof);

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
                   agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "poa" : "pos"),
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
            if (!cachePiggyback.Retrieve(pIndexPrev->GetBlockHash(), proofPrev))
            {
                CBlock blockPrev;
                if (!cntrBlock.Retrieve(pIndexPrev, blockPrev))
                {
                    StdLog("BlockChain", "Verify block: SubFork retrieve prev index fail, block: %s", hashBlock.GetHex().c_str());
                    return ERR_MISSING_PREV;
                }
                if (!blockPrev.GetPiggybackProof(proofPrev) || proofPrev.hashRefBlock == 0)
                {
                    StdLog("BlockChain", "Verify block: SubFork load prev proof fail, block: %s", hashBlock.GetHex().c_str());
                    return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
                }
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
        {   // block is subsidiary
            if (!GetBlockMintReward(block.hashPrev, false, nReward, pIndexPrev->hashRefBlock))
            {
                StdLog("BlockChain", "Verify block: SubFork get mint reward error, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        if (!VerifyBlockVoteResult(hashBlock, block))
        {
            StdLog("BlockChain", "Verify block: SubFork verify block vote fail, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        if (!VerifyBlockCrosschainProve(hashBlock, block))
        {
            StdLog("BlockChain", "Verify block: SubFork verify block crosschain prove fail, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        return pCoreProtocol->VerifySubsidiary(block, pIndexPrev, *ppIndexRef, agreement);
    }
    else if (block.IsVacant())
    {
        CProofOfPiggyback proof;
        if (!block.GetPiggybackProof(proof) || proof.hashRefBlock == 0)
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
                   agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "poa" : "pos"),
                   hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (block.txMint.GetToAddress() != agreement.GetBallot(0))
        {
            StdLog("BlockChain", "Verify block: Vacant to error, to: %s, ballot: %s, block: %s",
                   block.txMint.GetToAddress().ToString().c_str(),
                   agreement.GetBallot(0).ToString().c_str(),
                   hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        // if (block.txMint.GetTxTime() != block.GetBlockTime())
        // {
        //     StdLog("BlockChain", "Verify block: Vacant txMint timestamp error, mint tx time: %d, block time: %d, block: %s",
        //            block.txMint.GetTxTime(), block.GetBlockTime(), hashBlock.GetHex().c_str());
        //     return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        // }

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
            if (!blockPrev.GetPiggybackProof(proofPrev) || proofPrev.hashRefBlock == 0)
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

        CBlockVoteSig proofVote;
        if (block.GetBlockVoteSig(proofVote))
        {
            StdLog("BlockChain", "Verify block: Vacant block vote not empty, prev: %s, block: %s",
                   pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }
    }
    else
    {
        StdLog("BlockChain", "Verify block: block type error, nType: %d, block: %s", block.nType, hashBlock.GetHex().c_str());
        return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
    }

    return OK;
}

bool CBlockChain::VerifyBlockCertTx(const uint256& hashBlock, const CBlock& block)
{
    set<pair<CDestination, int>> setDelegateEnroll;
    for (const auto& tx : block.vtx)
    {
        if (tx.GetTxType() == CTransaction::TX_CERT)
        {
            if (tx.GetNonce() + CONSENSUS_ENROLL_INTERVAL < CBlock::GetBlockHeightByHash(block.hashPrev) || tx.GetNonce() > CBlock::GetBlockHeightByHash(block.hashPrev))
            {
                StdLog("BlockChain", "Verify block cert tx: Enroll height error, enroll height: %lu, delegate address: %s, txid: %s, prev: [%d] %s",
                       tx.GetNonce(), tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str(), CBlock::GetBlockHeightByHash(block.hashPrev), block.hashPrev.ToString().c_str());
                return false;
            }
            if (!VerifyDelegateMinVote(block.hashPrev, tx.GetNonce(), tx.GetToAddress()))
            {
                StdLog("BlockChain", "Verify block cert tx: Not enough votes, delegate address: %s, txid: %s, prev: %s",
                       tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str(), block.hashPrev.ToString().c_str());
                return false;
            }
            if (!VerifyDelegateEnroll(block.hashPrev, tx.GetNonce(), tx.GetToAddress()))
            {
                StdLog("BlockChain", "Verify block cert tx: Delegate is enrolled, enroll height: %d, delegate address: %s, txid: %s, prev: [%d] %s, block: %s",
                       (int)(tx.GetNonce()), tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str(), CBlock::GetBlockHeightByHash(block.hashPrev),
                       block.hashPrev.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            if (setDelegateEnroll.find(make_pair(tx.GetToAddress(), (int)(tx.GetNonce()))) != setDelegateEnroll.end())
            {
                StdLog("BlockChain", "Verify block cert tx: Duplicate cert tx, enroll height: %d, delegate address: %s, txid: %s, block: %s",
                       (int)(tx.GetNonce()), tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            setDelegateEnroll.insert(make_pair(tx.GetToAddress(), (int)(tx.GetNonce())));
        }
    }
    return true;
}

bool CBlockChain::VerifyBlockVoteResult(const uint256& hashBlock, const CBlock& block)
{
    CBlockVoteSig proofVote;
    if (block.GetBlockVoteSig(proofVote) && !proofVote.IsNull())
    {
        CBlockStatus statusVoteBlock;
        if (!GetBlockStatus(proofVote.hashBlockVote, statusVoteBlock))
        {
            StdLog("BlockChain", "Verify block vote result: Get block status fail, vote block: %s, block: %s",
                   proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
        if (statusVoteBlock.nBlockNumber >= block.GetBlockNumber())
        {
            StdLog("BlockChain", "Verify block vote result: Vote block number error, vote block number: %lu, local block number: %lu, vote block: %s, block: %s",
                   statusVoteBlock.nBlockNumber, block.GetBlockNumber(),
                   proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
        if (block.IsPrimary())
        {
            if (proofVote.hashBlockVote != block.hashPrev)
            {
                StdLog("BlockChain", "Verify block vote result: Primary chain block vote error, vote block: %s, block: %s",
                       proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (proofVote.hashBlockVote != block.hashPrev && !cntrBlock.VerifySameChain(proofVote.hashBlockVote, block.hashPrev))
            {
                StdLog("BlockChain", "Verify block vote result: Vote block not at chain, vote block number: %lu, local block number: %lu, prev block: %s, vote block: %s, block: %s",
                       statusVoteBlock.nBlockNumber, block.GetBlockNumber(), block.hashPrev.ToString().c_str(),
                       proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }

        // bytes btTempBitmap;
        // bytes btTempAggSig;
        // bool fTempAtChain = false;
        // uint256 hashTempAtBlock;
        // if (cntrBlock.RetrieveBlockVoteResult(proofVote.hashBlockVote, btTempBitmap, btTempAggSig, fTempAtChain, hashTempAtBlock) && fTempAtChain)
        // {
        //     StdLog("BlockChain", "Verify block vote result: Vote is already on the chain, vote block: %s, block: %s",
        //            proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
        //     return false;
        // }

        if (!VerifyBlockCommitVoteAggSig(proofVote.hashBlockVote, proofVote.btBlockVoteBitmap, proofVote.btBlockVoteAggSig))
        {
            StdLog("BlockChain", "Verify block vote result: Verify commit vote fail, vote block: %s, block: %s",
                   proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
    }
    else
    {
        if (block.IsPrimary() && block.hashPrev != pCoreProtocol->GetGenesisBlockHash())
        {
            bool fNeedProve = true;
            do
            {
                std::vector<uint384> vCandidatePubkey;
                if (!GetPrevBlockCandidatePubkey(block.hashPrev, vCandidatePubkey))
                {
                    StdLog("BlockChain", "Verify block vote result: Get prev block candidate pubkey fail, prev block: %s", block.hashPrev.GetBhString().c_str());
                    break;
                }
                if (!vCandidatePubkey.empty())
                {
                    break;
                }
                CBlockStatus status;
                if (!GetBlockStatus(block.hashPrev, status))
                {
                    StdLog("BlockChain", "Verify block vote result: Get block status fail, prev block: %s", block.hashPrev.GetBhString().c_str());
                    break;
                }
                if (status.nMintType == CTransaction::TX_WORK)
                {
                    fNeedProve = false;
                }
            } while (0);

            if (fNeedProve)
            {
                StdLog("BlockChain", "Verify block vote result: Primary chain is not block vote sig, prev block: %s, block: %s",
                       block.hashPrev.GetBhString().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
        }
    }
    return true;
}

bool CBlockChain::VerifyBlockCrosschainProve(const uint256& hashBlock, const CBlock& block)
{
    const CChainId nLocalChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    const CChainId nGenesisChainId = CBlock::GetBlockChainIdByHash(pCoreProtocol->GetGenesisBlockHash());

    for (const auto& kv : block.mapProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        if (nPeerChainId == nLocalChainId)
        {
            StdLog("BlockChain", "Verify block crosschain prove: Peer chainid iequal to local chainid, peer chainid: %u, block: %s", nPeerChainId, hashBlock.ToString().c_str());
            return false;
        }
        if (blockProve.hashBlock == 0)
        {
            StdLog("BlockChain", "Verify block crosschain prove: Prove block is empty, local chainid: %d, peer chainid: %d, block: %s", nLocalChainId, nPeerChainId, hashBlock.ToString().c_str());
            return false;
        }
        if (blockProve.btAggSigBitmap.empty())
        {
            StdLog("BlockChain", "Verify block crosschain prove: Prove agg bitmap is empty, block: %s", hashBlock.ToString().c_str());
            return false;
        }
        if (blockProve.btAggSigData.empty())
        {
            StdLog("BlockChain", "Verify block crosschain prove: Prove agg sig is empty, block: %s", hashBlock.ToString().c_str());
            return false;
        }

        uint256 hashRefBlock;
        if (nPeerChainId == nGenesisChainId)
        {
            // if (blockProve.hashRefBlock != 0 || !blockProve.vRefBlockMerkleProve.empty())
            // {
            //     return false;
            // }
            hashRefBlock = blockProve.hashBlock;
        }
        else
        {
            if (blockProve.hashRefBlock == 0 || blockProve.vRefBlockMerkleProve.empty())
            {
                StdLog("BlockChain", "Verify block crosschain prove: Prove ref block or merkle prove is empty, block: %s", hashBlock.ToString().c_str());
                return false;
            }
            if (!CBlock::VerifyBlockMerkleProve(blockProve.hashBlock, blockProve.vRefBlockMerkleProve, blockProve.hashRefBlock))
            {
                StdLog("BlockChain", "Verify block crosschain prove: Verify ref block merkle prove fail, verify block: %s, ref block: %s, block: %s",
                       blockProve.hashBlock.ToString().c_str(), blockProve.hashRefBlock.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            hashRefBlock = blockProve.hashRefBlock;
        }

        if (!VerifyBlockCommitVoteAggSig(blockProve.hashBlock, hashRefBlock, blockProve.btAggSigBitmap, blockProve.btAggSigData))
        {
            StdLog("BlockChain", "Verify block crosschain prove: Verify commit vote fail, vote block: %s, ref block: %s, block: %s",
                   blockProve.hashBlock.ToString().c_str(), hashRefBlock.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }

        if (blockProve.hashPrevBlock != 0 && !blockProve.vPrevBlockMerkleProve.empty())
        {
            if (!CBlock::VerifyBlockMerkleProve(blockProve.hashBlock, blockProve.vPrevBlockMerkleProve, blockProve.hashPrevBlock))
            {
                StdLog("BlockChain", "Verify block crosschain prove: Verify prev block merkle prove fail, prev block: %s, block: %s",
                       blockProve.hashPrevBlock.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }
        if (!blockProve.proveCrosschain.IsNull() && !blockProve.vCrosschainMerkleProve.empty())
        {
            if (!CBlock::VerifyBlockMerkleProve(blockProve.hashBlock, blockProve.vCrosschainMerkleProve, blockProve.proveCrosschain.GetHash()))
            {
                StdLog("BlockChain", "Verify block crosschain prove: Verify crosschain merkle prove fail, block: %s", hashBlock.ToString().c_str());
                return false;
            }
        }

        if (blockProve.hashPrevBlock != 0 && !blockProve.vPrevBlockCcProve.empty())
        {
            uint256 hashAtBlock = blockProve.hashPrevBlock;
            for (const CBlockPrevProve& prevProve : blockProve.vPrevBlockCcProve)
            {
                if (prevProve.hashPrevBlock != 0 && !prevProve.vPrevBlockMerkleProve.empty())
                {
                    if (!CBlock::VerifyBlockMerkleProve(hashAtBlock, prevProve.vPrevBlockMerkleProve, prevProve.hashPrevBlock))
                    {
                        StdLog("BlockChain", "Verify block crosschain prove: Verify prev block merkle prove fail, at block: %s, merkle prove size: %lu, prev block: %s, block: %s",
                               hashAtBlock.GetBhString().c_str(), prevProve.vPrevBlockMerkleProve.size(), prevProve.hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
                        return false;
                    }
                }
                if (!prevProve.proveCrosschain.IsNull() && !prevProve.vCrosschainMerkleProve.empty())
                {
                    if (!CBlock::VerifyBlockMerkleProve(hashAtBlock, prevProve.vCrosschainMerkleProve, prevProve.proveCrosschain.GetHash()))
                    {
                        StdLog("BlockChain", "Verify block crosschain prove: Verify crosschain merkle prove fail, at block: %s, block: %s",
                               hashAtBlock.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                }
                hashAtBlock = prevProve.hashPrevBlock;
            }
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
    //if (Config()->nMagicNum == MAINNET_MAGICNUM)
    if (!TESTNET_FLAG)
    {
        for (const auto& vd : mapCheckPointsList)
        {
            InitCheckPoints(vd.first, vd.second);
        }
    }
}

bool CBlockChain::HasCheckPoints(const uint256& hashFork) const
{
    auto it = mapForkCheckPoints.find(hashFork);
    if (it != mapForkCheckPoints.end())
    {
        return it->second.size() > 0;
    }
    else
    {
        return false;
    }
}

bool CBlockChain::GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point)
{
    auto it = mapForkCheckPoints.find(hashFork);
    if (it != mapForkCheckPoints.end())
    {
        if (it->second.count(nHeight) == 0)
        {
            return false;
        }
        else
        {
            point = it->second[nHeight];
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
    auto it = mapForkCheckPoints.find(hashFork);
    if (it != mapForkCheckPoints.end())
    {
        std::vector<IBlockChain::CCheckPoint> points;
        for (const auto& kv : it->second)
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
    auto it = forkCheckPoints.upper_bound(nHeight);
    return (it != forkCheckPoints.end()) ? IBlockChain::CCheckPoint(it->second) : IBlockChain::CCheckPoint();
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
        if (!GetBlockHashByHeightSlot(hashFork, point.nHeight, 0, hashBlock))
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
    if (!GetBlockHashByHeightSlot(hashFork, block.GetBlockHeight(), block.GetBlockSlot(), bestChainBlockHash))
    {
        return true;
    }

    return block.GetHash() == bestChainBlockHash;
}

int64 CBlockChain::GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, vector<CTxInfo>& vTx)
{
    return 0;
}

bool CBlockChain::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    return cntrBlock.RetrieveAddressContext(hashFork, hashBlock, dest, ctxAddress);
}

bool CBlockChain::GetTxToAddressContext(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx, CAddressContext& ctxAddress)
{
    if (tx.GetToAddress().IsNull())
    {
        StdLog("CBlockChain", "Get Tx ToAddress Context: to is null, block: %s,", hashBlock.ToString().c_str());
        return false;
    }
    if (cntrBlock.RetrieveAddressContext(hashFork, hashBlock, tx.GetToAddress(), ctxAddress))
    {
        return true;
    }
    //StdLog("CBlockChain", "Get Tx ToAddress Context: Retrieve address context fail, fork: %s, block: %s, to: %s", hashFork.ToString().c_str(), hashBlock.ToString().c_str(), tx.GetToAddress().ToString().c_str());
    if (tx.GetToAddressData(ctxAddress))
    {
        return true;
    }
    //StdLog("CBlockChain", "Get Tx ToAddress Context: Get to address data fail, block: %s, to: %s", hashBlock.ToString().c_str(), tx.GetToAddress().ToString().c_str());
    return false;
}

CTemplatePtr CBlockChain::GetTxToAddressTemplatePtr(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    CAddressContext ctxAddress;
    if (!GetTxToAddressContext(hashFork, hashBlock, tx, ctxAddress))
    {
        StdLog("CBlockChain", "Get Tx ToAddress Template Ptr: Get tx to address context fail, block: %s, to: %s", hashBlock.ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return nullptr;
    }
    if (!ctxAddress.IsTemplate())
    {
        StdLog("CBlockChain", "Get Tx ToAddress Template Ptr: Is template fail, block: %s, to: %s", hashBlock.ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return nullptr;
    }
    CTemplateAddressContext ctxTemplate;
    if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
    {
        StdLog("CBlockChain", "Get Tx ToAddress Template Ptr: Get template address context fail, block: %s, to: %s", hashBlock.ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return nullptr;
    }
    return CTemplate::Import(ctxTemplate.btData);
}

bool CBlockChain::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    return cntrBlock.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockChain::RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    return cntrBlock.RetrieveBlsPubkeyContext(hashFork, hashBlock, dest, blsPubkey);
}

bool CBlockChain::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return cntrBlock.GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CBlockChain::RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    return cntrBlock.RetrieveForkContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode);
}

bool CBlockChain::RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork)
{
    return cntrBlock.RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode, fLinkGenesisFork);
}

bool CBlockChain::GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode)
{
    return cntrBlock.GetBlockSourceCodeData(hashFork, hashBlock, hashSourceCode, txcdCode);
}

bool CBlockChain::GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode)
{
    return cntrBlock.GetBlockContractCreateCodeData(hashFork, hashBlock, hashContractCreateCode, txcdCode);
}

bool CBlockChain::GetForkContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode)
{
    return cntrBlock.GetForkContractCreateCodeContext(hashFork, hashBlock, hashContractCode, ctxtContractCode);
}

bool CBlockChain::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode)
{
    return cntrBlock.ListContractCreateCodeContext(hashFork, hashBlock, txid, mapCreateCode);
}

bool CBlockChain::ListAddressTxInfo(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!cntrBlock.RetrieveForkLast(hashFork, hashLastBlock))
        {
            return false;
        }
    }
    return cntrBlock.ListAddressTxInfo(hashFork, hashLastBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
}

bool CBlockChain::GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount)
{
    return cntrBlock.GetCreateForkLockedAmount(dest, hashPrevBlock, btAddressData, nLockedAmount);
}

bool CBlockChain::VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock)
{
    return cntrBlock.VerifyAddressVoteRedeem(dest, hashPrevBlock);
}

bool CBlockChain::GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount)
{
    return cntrBlock.GetVoteRewardLockedAmount(hashFork, hashPrevBlock, dest, nLockedAmount);
}

bool CBlockChain::GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount)
{
    return cntrBlock.GetAddressLockedAmount(hashFork, hashPrevBlock, dest, ctxAddress, nBalance, nLockedAmount);
}

bool CBlockChain::VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainIdIn, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashBlock)
{
    return cntrBlock.VerifyForkFlag(hashNewFork, nChainIdIn, strForkSymbol, strForkName, hashBlock);
}

bool CBlockChain::GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashMainChainRefBlock);
}

bool CBlockChain::GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.GetForkHashByChainId(nChainId, hashFork, hashMainChainRefBlock);
}

bool CBlockChain::ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.ListCoinContext(mapSymbolCoin, hashMainChainRefBlock);
}

bool CBlockChain::GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.GetDexCoinPairBySymbolPair(strSymbol1, strSymbol2, nCoinPair, hashMainChainRefBlock);
}

bool CBlockChain::GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.GetSymbolPairByDexCoinPair(nCoinPair, strSymbol1, strSymbol2, hashMainChainRefBlock);
}

bool CBlockChain::ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock)
{
    return cntrBlock.ListDexCoinPair(nCoinPair, strCoinSymbol, mapDexCoinPair, hashMainChainRefBlock);
}

bool CBlockChain::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value)
{
    CDestState state;
    if (!RetrieveDestState(hashFork, hashBlock, dest, state))
    {
        StdLog("BlockChain", "Retrieve contract kv value: Retrieve dest state fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    CBufStream ss;
    ss.Write((char*)(dest.begin()), dest.size());
    ss.Write((char*)(key.begin()), key.size());
    uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());

    return cntrBlock.RetrieveContractKvValue(hashFork, state.GetStorageRoot(), hash, value);
}

bool CBlockChain::GetBlockReceiptsByBlock(const uint256& hashFork, const uint256& hashFromBlock, const uint256& hashToBlock, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapBlockReceipts)
{
    return cntrBlock.GetBlockReceiptsByBlock(hashFork, hashFromBlock, hashToBlock, mapBlockReceipts);
}

bool CBlockChain::VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock)
{
    return cntrBlock.VerifySameChain(hashPrevBlock, hashAfterBlock);
}

bool CBlockChain::GetPrevBlockHashList(const uint256& hashBlock, const uint32 nGetCount, std::vector<uint256>& vPrevBlockhash)
{
    return cntrBlock.GetPrevBlockHashList(hashBlock, nGetCount, vPrevBlockhash);
}

bool CBlockChain::CalcBlockVoteRewardTx(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime, vector<CTransaction>& vVoteRewardTx)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwCvrAccess);

    const uint32 issueCycle = VOTE_REWARD_DISTRIBUTE_HEIGHT;
    if (CBlock::GetBlockHeightByHash(hashPrev) < issueCycle
        || nBlockHeight % issueCycle == 0
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
    while (pIndex && (pIndex->GetBlockHeight() % issueCycle) > 0)
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
        StdDebug("BlockChain", "Get distribute vote reward tx: Fork error, calc fork: %s, prev fork: %s, hashPrev: %s",
                 pIndex->GetOriginHash().GetHex().c_str(), hashFork.GetHex().c_str(), hashPrev.GetHex().c_str());
        return true;
    }
    const uint256 hashCalcEndBlock = pIndex->GetBlockHash();
    const uint256 hashCalcEndMainChainRefBlock = pIndex->GetRefBlock();

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
                               hashFork, hashCalcEndBlock, hashCalcEndMainChainRefBlock, vRewardList))
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
        int nTxListIndex = CBlock::GetBlockHeightByHash(hashPrev) % issueCycle;
        if (nTxListIndex < vRewardList.size())
        {
            vVoteRewardTx = vRewardList[nTxListIndex];
        }
    }
    else
    {
        int nDisCount = 0;
        CBlockIndex* pSubIndex = pPrevIndex;
        while (pSubIndex && (pSubIndex->GetBlockHeight() % issueCycle) > 0)
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
        }
    }
    return true;
}

int64 CBlockChain::GetBlockInvestRewardTxMaxCount()
{
    CBlock block;

    CProofOfHashWork proof;
    block.AddHashWorkProof(proof);
    size_t nMaxTxSize = MAX_BLOCK_SIZE - mtbase::GetSerializeSize(block);

    CTransaction txReward;

    txReward.SetTxType(CTransaction::TX_VOTE_REWARD);
    txReward.SetChainId(0xFFFFFFFF);
    txReward.SetNonce(~((uint64)0));
    txReward.SetToAddress(CDestination(~uint160()));
    txReward.SetAmount((~uint256()) >> 160);

    uint32 nDistributeTxCount = (uint32)(nMaxTxSize / mtbase::GetSerializeSize(txReward));
    if (nDistributeTxCount > 100)
    {
        nDistributeTxCount -= 100;
    }
    return nDistributeTxCount;
}

uint256 CBlockChain::GetPrimaryBlockReward(const uint256& hashPrev)
{
    uint64 u = (uint64)pow(2, (CBlock::GetBlockHeightByHash(hashPrev) + 1) / BBCP_REWARD_HALVE_CYCLE);
    if (u == 0)
    {
        return 0;
    }
    return (BBCP_REWARD_INIT / uint256(u));
}

bool CBlockChain::CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                       uint256& hashCrosschainMerkleRoot, uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, bool& fMoStatus)
{
    uint256 hashPrevStateRoot;
    uint32 nPrevBlockTime = 0;
    if (block.hashPrev != 0)
    {
        CBlockIndex* pIndexPrev = nullptr;
        if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev) || pIndexPrev == nullptr)
        {
            StdLog("CBlockChain", "Create Block State Root: Retrieve index fail, hashPrev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
        nPrevBlockTime = pIndexPrev->GetBlockTime();
    }

    std::map<CDestination, CAddressContext> mapAddressContext;
    if (!cntrBlock.GetBlockAddress(hashFork, block.GetHash(), block, mapAddressContext))
    {
        StdLog("CBlockChain", "Create Block State Root: Get block adress fail, hashFork: %s", hashFork.GetHex().c_str());
        return false;
    }

    return (cntrBlock.CreateBlockStateRoot(hashFork, block, hashPrevStateRoot, nPrevBlockTime, hashStateRoot, hashReceiptRoot,
                                           hashCrosschainMerkleRoot, nBlockGasUsed, btBloomDataOut, nTotalMintRewardOut, fMoStatus, mapAddressContext)
            != nullptr);
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

bool CBlockChain::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex)
{
    return cntrBlock.GetTransactionReceipt(hashFork, txid, receiptex);
}

bool CBlockChain::CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from,
                               const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                               const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    CBlockIndex* pIndex = nullptr;
    if (hashBlock == 0)
    {
        if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
        {
            StdLog("BlockChain", "Call contract: Retrieve fork fail, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
    }
    else
    {
        if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
        {
            StdLog("BlockChain", "Call contract: Retrieve index fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrimaryBlock;
    if (pIndex->IsPrimary())
    {
        hashPrimaryBlock = pIndex->GetBlockHash();
    }
    else
    {
        hashPrimaryBlock = pIndex->hashRefBlock;
    }

    CForkContext ctxFork;
    if (!cntrBlock.RetrieveForkContext(hashFork, ctxFork, hashPrimaryBlock))
    {
        StdLog("BlockChain", "Call contract: Retrieve forkc ontext fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    uint256 nRunGasLimit;
    uint256 nBaseGas = TX_BASE_GAS + CTransaction::GetTxDataGasStatic(btContractParam.size());
    if (nGas == 0)
    {
        // Gas is 0, Calc gas used
        nRunGasLimit = DEF_TX_GAS_LIMIT;
    }
    else
    {
        if (nGas < nBaseGas)
        {
            StdLog("BlockChain", "Call contract: Gas not enough, base gas: %lu, gas limit: %lu, from: %s, fork: %s",
                   nBaseGas.Get64(), nGas.Get64(), from.ToString().c_str(), hashFork.GetHex().c_str());
            return false;
        }
        nRunGasLimit = nGas - nBaseGas;
    }

    if (!cntrBlock.CallContractCode(fEthCall, hashFork, ctxFork.nChainId, pIndex->GetAgreement(), pIndex->GetBlockHeight(), pIndex->destMint, MAX_BLOCK_GAS_LIMIT,
                                    from, to, nGasPrice, nRunGasLimit, nAmount,
                                    btContractParam, GetNetTime(), pIndex->GetBlockHash(), pIndex->GetStateRoot(), pIndex->GetBlockTime(), nGasLeft, nStatus, btResult))
    {
        StdLog("BlockChain", "Call contract: Call contract fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    if (nGas == 0)
    {
        if (nRunGasLimit.Get64() <= nGasLeft)
        {
            nUsedGas = nBaseGas;
        }
        else
        {
            nUsedGas = nBaseGas + (nRunGasLimit.Get64() - nGasLeft);
        }
    }
    return true;
}

bool CBlockChain::GetContractBalance(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const CDestination& destUser, uint256& nBalance)
{
    const CDestination& from = destUser;
    const CDestination& to = destContract;
    const uint256 nAmount = 0;
    const uint256 nGasPrice = MIN_GAS_PRICE;
    const uint256 nGas = DEF_TX_GAS_LIMIT;
    uint256 nUsedGas;
    uint64 nGasLeft = 0;
    int nStatus = 0;
    bytes btResult;

    //function balanceOf(address tokenOwner) external view returns (uint);

    std::vector<bytes> vParamList;
    vParamList.push_back(destUser.ToHash().GetBytes());

    bytes btContractParam = MakeEthTxCallData("balanceOf(address)", vParamList);

    if (!CallContract(fEthCall, hashFork, hashBlock, from, to, nAmount, nGasPrice,
                      nGas, btContractParam, nUsedGas, nGasLeft, nStatus, btResult))
    {
        StdLog("BlockChain", "Get contract balance: Call contract fail, contract address: %s, user address: %s, fork: %s",
               destContract.ToString().c_str(), destUser.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (btResult.size() != 32)
    {
        StdLog("BlockChain", "Get contract balance: Result error, result: %s, contract address: %s, user address: %s, fork: %s",
               ToHexString(btResult).c_str(), destContract.ToString().c_str(), destUser.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    nBalance.FromBigEndian(btResult.data(), 32);
    return true;
}

bool CBlockChain::VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract)
{
    return cntrBlock.VerifyContractAddress(hashFork, hashBlock, destContract);
}

bool CBlockChain::VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    return cntrBlock.VerifyCreateCodeTx(hashFork, hashBlock, tx);
}

bool CBlockChain::GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner,
                                   const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    return cntrBlock.GetCompleteOrder(hashBlock, destOrder, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, nCompleteAmount, nCompleteOrderCount);
}

bool CBlockChain::GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    return cntrBlock.GetCompleteOrder(hashBlock, hashDexOrder, nCompleteAmount, nCompleteOrderCount);
}

bool CBlockChain::ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                      const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
{
    return cntrBlock.ListAddressDexOrder(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nBeginOrderNumber, nGetCount, mapDexOrder);
}

bool CBlockChain::ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    return cntrBlock.ListMatchDexOrder(hashBlock, strCoinSymbolSell, strCoinSymbolBuy, nGetCount, realDexOrder);
}

bool CBlockChain::GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve)
{
    return cntrBlock.GetCrosschainProveForPrevBlock(nRecvChainId, hashRecvPrevBlock, mapBlockCrosschainProve);
}

bool CBlockChain::AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve)
{
    return cntrBlock.AddRecvCrosschainProve(nRecvChainId, blockProve);
}

bool CBlockChain::GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve)
{
    return cntrBlock.GetRecvCrosschainProve(nRecvChainId, nSendChainId, hashSendProvePrevBlock, blockProve);
}

bool CBlockChain::AddBlacklistAddress(const CDestination& dest)
{
    return cntrBlock.AddBlacklistAddress(dest);
}

void CBlockChain::RemoveBlacklistAddress(const CDestination& dest)
{
    cntrBlock.RemoveBlacklistAddress(dest);
}

bool CBlockChain::IsExistBlacklistAddress(const CDestination& dest)
{
    return cntrBlock.IsExistBlacklistAddress(dest);
}

void CBlockChain::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    cntrBlock.ListBlacklistAddress(setAddressOut);
}

bool CBlockChain::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    return cntrBlock.ListFunctionAddress(hashBlock, mapFunctionAddress);
}

bool CBlockChain::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    return cntrBlock.RetrieveFunctionAddress(hashBlock, nFuncId, ctxFuncAddress);
}

bool CBlockChain::GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction)
{
    return cntrBlock.GetDefaultFunctionAddress(nFuncId, destDefFunction);
}

bool CBlockChain::VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr)
{
    return cntrBlock.VerifyNewFunctionAddress(hashBlock, destFrom, nFuncId, destNewFunction, strErr);
}

bool CBlockChain::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return cntrBlock.UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

uint256 CBlockChain::GetForkMintMinGasPrice(const uint256& hashFork)
{
    return cntrBlock.GetForkMintMinGasPrice(hashFork);
}

bool CBlockChain::GetCandidatePubkey(const uint256& hashPrimaryBlock, std::vector<uint384>& vCandidatePubkey)
{
    boost::unique_lock<boost::mutex> lock(mutexBlsPubkey);
    if (!cacheBlsPubkey.GetBlsPubkey(hashPrimaryBlock, vCandidatePubkey))
    {
        std::set<CDestination> setVoteAddress;
        if (!GetBlockDelegateVoteAddress(hashPrimaryBlock, setVoteAddress))
        {
            StdLog("CBlockChain", "Get candidate pubkey: Get block delegate vote address fail, block: %s", hashPrimaryBlock.GetBhString().c_str());
            return false;
        }
        vCandidatePubkey.clear();
        for (const CDestination& dest : setVoteAddress)
        {
            uint384 pubkey;
            if (cntrBlock.RetrieveBlsPubkeyContext(pCoreProtocol->GetGenesisBlockHash(), hashPrimaryBlock, dest, pubkey))
            {
                vCandidatePubkey.push_back(pubkey);
            }
        }
        StdDebug("CBlockChain", "Get candidate pubkey: Candidate pubkey count: %lu, Vote address count: %lu, block: %s",
                 vCandidatePubkey.size(), setVoteAddress.size(), hashPrimaryBlock.GetBhString().c_str());
        cacheBlsPubkey.AddBlsPubkey(hashPrimaryBlock, vCandidatePubkey);
    }
    return true;
}

bool CBlockChain::GetPrevBlockCandidatePubkey(const uint256& hashBlock, std::vector<uint384>& vCandidatePubkey)
{
    CBlockStatus status;
    if (!GetBlockStatus(hashBlock, status))
    {
        StdLog("CBlockChain", "Get prev block candidate pubkey: Get block status fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    uint256 hashPrevPrimaryBlock;
    if (status.hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        hashPrevPrimaryBlock = status.hashPrevBlock;
    }
    else
    {
        CBlockStatus statusRefBlock;
        if (!GetBlockStatus(status.hashRefBlock, statusRefBlock))
        {
            StdLog("CBlockChain", "Get prev block candidate pubkey: Get ref block status fail, ref block: %s, block: %s", status.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
        hashPrevPrimaryBlock = statusRefBlock.hashPrevBlock;
    }
    if (!GetCandidatePubkey(hashPrevPrimaryBlock, vCandidatePubkey))
    {
        StdLog("CBlockChain", "Get prev block candidate pubkey: Get candidate pubkey fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::VerifyBlockCommitVoteAggSig(const uint256& hashBlock, const bytes& btAggBitmap, const bytes& btAggSig)
{
    vector<uint384> vCandidatePubkeys;
    if (!GetPrevBlockCandidatePubkey(hashBlock, vCandidatePubkeys))
    {
        StdLog("BlockChain", "Verify block commit vote aggsig: Get candidate pubkey fail, vote block: %s", hashBlock.ToString().c_str());
        return false;
    }
    if (!consensus::consblockvote::CConsBlockVote::VerifyCommitVoteAggSig(hashBlock, btAggBitmap, btAggSig, vCandidatePubkeys))
    {
        StdLog("BlockChain", "Verify block commit vote aggsig: Verify commit vote fail, vote block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::VerifyBlockCommitVoteAggSig(const uint256& hashVoteBlock, const uint256& hashRefBlock, const bytes& btAggBitmap, const bytes& btAggSig)
{
    vector<uint384> vCandidatePubkeys;
    if (!GetCandidatePubkey(hashRefBlock, vCandidatePubkeys))
    {
        StdLog("BlockChain", "Verify block commit vote aggsig: Get candidate pubkey fail, ref block: %s", hashRefBlock.ToString().c_str());
        return false;
    }
    if (!consensus::consblockvote::CConsBlockVote::VerifyCommitVoteAggSig(hashVoteBlock, btAggBitmap, btAggSig, vCandidatePubkeys))
    {
        StdLog("BlockChain", "Verify block commit vote aggsig: Verify commit vote fail, vote block: %s", hashVoteBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock)
{
    return cntrBlock.AddBlockVoteResult(hashBlock, fLongChain, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockChain::RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    return cntrBlock.RetrieveBlockVoteResult(hashBlock, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockChain::GetMakerVoteBlock(const uint256& hashPrevBlock, bytes& btBitmap, bytes& btAggSig, uint256& hashVoteBlock)
{
    return cntrBlock.GetMakerVoteBlock(hashPrevBlock, btBitmap, btAggSig, hashVoteBlock);
}

bool CBlockChain::IsBlockConfirm(const uint256& hashBlock)
{
    return cntrBlock.IsBlockConfirm(hashBlock);
}

bool CBlockChain::AddBlockLocalVoteSignFlag(const uint256& hashBlock)
{
    return cntrBlock.AddBlockLocalVoteSignFlag(hashBlock);
}

bool CBlockChain::VerifyPrimaryBlockConfirm(const uint256& hashBlock)
{
    if (hashBlock == pCoreProtocol->GetGenesisBlockHash())
    {
        return true;
    }

    std::vector<uint384> vCandidatePubkey;
    if (!GetPrevBlockCandidatePubkey(hashBlock, vCandidatePubkey))
    {
        StdLog("BlockChain", "Verify primary block confirm: Get prev block candidate pubkey fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (vCandidatePubkey.empty())
    {
        CBlockStatus status;
        if (!GetBlockStatus(hashBlock, status))
        {
            StdLog("BlockChain", "Verify primary block confirm: Get block status fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
        if (status.nMintType == CTransaction::TX_WORK)
        {
            return true;
        }
    }

    bytes btDbBitmap;
    bytes btDbAggSig;
    bool fDbAtChain = false;
    uint256 hashDbAtBlock;
    if (!RetrieveBlockVoteResult(hashBlock, btDbBitmap, btDbAggSig, fDbAtChain, hashDbAtBlock) && !fDbAtChain)
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------
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
            StdLog("BlockChain", "Verify Vote Reward Tx: vote reward tx error, index: %ld, vote reward tx: [%s] %s, vote dest: %s, a: %s, n: 0x%lx, block tx: [%s] %s, vote dest: %s, a: %s, n: 0x%lx, height: %d, hashPrev: %s",
                   i, vVoteRewardTx[i].GetTypeString().c_str(), vVoteRewardTx[i].GetHash().GetHex().c_str(), vVoteRewardTx[i].GetToAddress().ToString().c_str(), CoinToTokenBigFloat(vVoteRewardTx[i].GetAmount()).c_str(), vVoteRewardTx[i].GetNonce(),
                   block.vtx[i].GetTypeString().c_str(), block.vtx[i].GetHash().GetHex().c_str(), block.vtx[i].GetToAddress().ToString().c_str(), CoinToTokenBigFloat(block.vtx[i].GetAmount()).c_str(), block.vtx[i].GetNonce(),
                   block.GetBlockHeight(), block.hashPrev.GetHex().c_str());
            return false;
        }
    }
    nRewardTxCount = vVoteRewardTx.size();
    return true;
}

Errno CBlockChain::VerifyBlockTx(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& nReward,
                                 const std::size_t nIgnoreVerifyTx, const std::map<CDestination, CAddressContext>& mapBlockAddress)
{
    uint256 nTotalFee;
    std::map<CDestination, CDestState> mapDestState;
    std::size_t nIgnoreTx = nIgnoreVerifyTx;

    // verify tx
    for (const CTransaction& tx : block.vtx)
    {
        if (nIgnoreTx > 0)
        {
            nIgnoreTx--;
            if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
            {
                if (tx.GetAmount() > 0)
                {
                    auto it = mapDestState.find(tx.GetToAddress());
                    if (it == mapDestState.end())
                    {
                        CDestState state;
                        if (!RetrieveDestState(hashFork, block.hashPrev, tx.GetToAddress(), state))
                        {
                            state.SetNull();
                        }
                        it = mapDestState.insert(make_pair(tx.GetToAddress(), state)).first;
                    }
                    it->second.IncBalance(tx.GetAmount());
                }
            }
            else
            {
                StdLog("BlockChain", "Verify block tx: Verify tx no vote reward error, txid: %s, block: %s",
                       tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return ERR_TRANSACTION_INVALID;
            }
        }
        else
        {
            uint256 txid = tx.GetHash();

            if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
            {
                StdLog("BlockChain", "Verify block tx: Verify vote reward tx error, txid: %s, block: %s",
                       txid.GetHex().c_str(), hashBlock.ToString().c_str());
                return ERR_TRANSACTION_INVALID;
            }
            // if (tx.GetTxTime() > block.GetBlockTime())
            // {
            //     StdLog("BlockChain", "Verify block tx: Verify BlockTx time fail: tx time: %d, block time: %d, tx: %s, block: %s",
            //            tx.GetTxTime(), block.GetBlockTime(), txid.ToString().c_str(), hashBlock.GetHex().c_str());
            //     return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            // }

            if (!tx.GetFromAddress().IsNull())
            {
                auto it = mapDestState.find(tx.GetFromAddress());
                if (it == mapDestState.end())
                {
                    CDestState state;
                    if (!RetrieveDestState(hashFork, block.hashPrev, tx.GetFromAddress(), state))
                    {
                        StdLog("BlockChain", "Verify block tx: Retrieve dest state fail, from: %s, txid: %s", tx.GetFromAddress().ToString().c_str(), txid.ToString().c_str());
                        state.SetNull();
                    }
                    it = mapDestState.insert(make_pair(tx.GetFromAddress(), state)).first;
                }
                CDestState& stateFrom = it->second;

                Errno err = pCoreProtocol->VerifyTransaction(txid, tx, hashFork, block.hashPrev, block.GetBlockHeight(), stateFrom, mapBlockAddress);
                if (err != OK)
                {
                    StdLog("BlockChain", "Verify block tx: Verify transaction fail, err: %s, txid: %s", ErrorString(err), txid.ToString().c_str());
                    return err;
                }

                stateFrom.DecBalance(tx.GetAmount() + tx.GetTxFee());
                if (tx.GetTxType() != CTransaction::TX_CERT)
                {
                    stateFrom.IncTxNonce();
                }
            }

            if (tx.GetAmount() > 0)
            {
                auto it = mapDestState.find(tx.GetToAddress());
                if (it == mapDestState.end())
                {
                    CDestState state;
                    if (!RetrieveDestState(hashFork, block.hashPrev, tx.GetToAddress(), state))
                    {
                        state.SetNull();
                    }
                    it = mapDestState.insert(make_pair(tx.GetToAddress(), state)).first;
                }
                it->second.IncBalance(tx.GetAmount());
            }
        }

        nTotalFee += tx.GetTxFee();
    }

    if (block.txMint.GetAmount() > nTotalFee + nReward)
    {
        StdLog("BlockChain", "Verify block tx: Mint tx amount invalid : (0x%s > 0x%s + 0x%s)",
               block.txMint.GetAmount().GetHex().c_str(), nTotalFee.GetHex().c_str(), nReward.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    return OK;
}

bool CBlockChain::CalcEndVoteReward(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime,
                                    const uint256& hashFork, const uint256& hashCalcEndBlock, const uint256& hashCalcEndMainChainRefBlock, 
                                    vector<vector<CTransaction>>& vRewardList)
{   // pattern: for delegate, delegate addr/delegate addr; for pledge, pledge addr/pubkey owner addr of pledge addr; for project party/fundation, they are the same.
    map<CDestination, pair<CDestination, uint256>> voteRewards; // key: vote or delegate address, value first: reward address, value second: reward amount
    if (!CalcDistributeVoteReward(hashCalcEndBlock, voteRewards))
    {
        StdError("BlockChain", "Calc end vote reward tx: Calc distribute pledge reward fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }

    CForkContext ctxFork;
    if (!GetForkContext(hashFork, ctxFork, hashCalcEndMainChainRefBlock))
    {
        StdError("BlockChain", "Calc end vote reward tx: Get fork context fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    map<CDestination, pair<uint256, bool>> mapReward; // key: reward address, value first: reward amount, value second: is pubkey address?
    for (const auto& [k, v] : voteRewards)
    {
        auto& destTemp = k;
        auto& rewardTo = v.first;
        auto& amount = v.second;
        uint32 nFuncId = 0;
        if (rewardTo == PLEDGE_SURPLUS_REWARD_ADDRESS)
        {
            nFuncId = FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS;
        }
        else if (rewardTo == PROJECT_PARTY_REWARD_TO_ADDRESS)
        {
            nFuncId = FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS;
        }
        else if (rewardTo == FOUNDATION_REWARD_TO_ADDRESS)
        {
            nFuncId = FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS;
        }
        if (nFuncId != 0)
        {
            CFunctionAddressContext ctxFuncAddress;
            if (!cntrBlock.RetrieveFunctionAddress(hashCalcEndMainChainRefBlock, nFuncId, ctxFuncAddress))
            {
                StdError("BlockChain", "Calc end vote reward tx: Get function address fail, function id: %d, fork: %s", nFuncId, hashFork.ToString().c_str());
                return false;
            }
            auto& p = mapReward[ctxFuncAddress.GetFunctionAddress()];
            p.first += amount;
            p.second = false;
        }
        else
        {
            auto& p = mapReward[rewardTo];
            p.first += amount;
            p.second = (destTemp != rewardTo);
        }
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

        uint32 nCalcHeight = CBlock::GetBlockHeightByHash(hashCalcEndBlock);
        uint32 nAddTxCount = 0;
        for (const auto& kv : mapReward)
        {
            CTransaction txReward;

            txReward.SetTxType(CTransaction::TX_VOTE_REWARD);
            txReward.SetChainId(ctxFork.nChainId);
            txReward.SetNonce(((uint64)nCalcHeight << 32) | nAddTxCount);
            txReward.SetToAddress(kv.first);
            txReward.SetAmount(kv.second.first);

            if (kv.second.second)
            {
                CAddressContext ctxAddress;
                if (!cntrBlock.RetrieveAddressContext(hashFork, hashPrev, txReward.GetToAddress(), ctxAddress))
                {
                    txReward.SetToAddressData(CAddressContext(CPubkeyAddressContext()));
                }
            }
            else
            {
                CAddressContext ctxAddress;
                if (!cntrBlock.RetrieveAddressContext(hashFork, hashPrev, txReward.GetToAddress(), ctxAddress))
                {
                    CBlockIndex* pPrevIndex = nullptr;
                    if (!cntrBlock.RetrieveIndex(hashPrev, &pPrevIndex) || pPrevIndex == nullptr)
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Retrieve prev index fail, to: %s, hashPrev: %s",
                                 txReward.GetToAddress().ToString().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    if (pPrevIndex->IsPrimary())
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Primary not address context fail, to: %s, hashPrev: %s",
                                 txReward.GetToAddress().ToString().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    if (!cntrBlock.RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), pPrevIndex->GetRefBlock(), txReward.GetToAddress(), ctxAddress))
                    {
                        StdError("BlockChain", "Calc end vote reward tx: Retrieve ref block address context fail, to: %s, ref block: %s, hashPrev: %s",
                                 txReward.GetToAddress().ToString().c_str(), pPrevIndex->GetRefBlock().GetHex().c_str(), hashPrev.GetHex().c_str());
                        return false;
                    }
                    txReward.SetToAddressData(ctxAddress);
                }
            }
            StdDebug("BlockChain", "Calc end vote reward tx: distribute reward tx: calc: [%d] %s, dist height: %d, reward amount: %s, dest: %s, hashPrev: %s, hashFork: %s",
                     CBlock::GetBlockHeightByHash(hashCalcEndBlock), hashCalcEndBlock.GetHex().c_str(), nBlockHeight,
                     CoinToTokenBigFloat(txReward.GetAmount()).c_str(), txReward.GetToAddress().ToString().c_str(), hashPrev.GetHex().c_str(), hashFork.GetHex().c_str());

            vRewardList[nAddTxCount / nSingleBlockTxCount].push_back(txReward);
            nAddTxCount++;
        }
    }
    return true;
}

bool CBlockChain::CalcDistributeVoteReward(const uint256& hashCalcEndBlock, std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteReward)
{
    // hashCalcEndBlock at N height or multiple
    const int issueCycle = VOTE_REWARD_DISTRIBUTE_HEIGHT;
    if ((CBlock::GetBlockHeightByHash(hashCalcEndBlock) % issueCycle) != 0)
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
    //int nTailHeight = pTailIndex->GetBlockHeight();                        // N
    int nBeginHeight = pTailIndex->GetBlockHeight() - issueCycle + 1; // 1
    CBlockIndex* pIndex = pTailIndex;
    while (pIndex && pIndex->pPrev && !pIndex->IsOrigin() && pIndex->GetBlockHeight() >= nBeginHeight)
    {
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
                if (!cntrBlock.GetDelegateMintRewardRatio(calcBlock.hashPrimaryBlock, pIndex->destMint, calcBlock.nRewardRation))
                {
                    StdLog("BlockChain", "Calculate block vote reward: Get delegate mint reward ratio fail, destMint: %s, primary block: %s",
                           pIndex->destMint.ToString().c_str(), calcBlock.hashPrimaryBlock.GetHex().c_str());
                    return false;
                }
                calcBlock.destMint = pIndex->destMint;
            }
            calcBlock.nRewardAmount += pIndex->GetBlockReward();
        }
        else
        {
            if (pIndex->IsPrimary() && pIndex->nMintType == CTransaction::TX_WORK && pIndex->GetBlockReward() > 0)
            {
                auto& destroyReward = mapVoteReward[PLEDGE_SURPLUS_REWARD_ADDRESS];
                destroyReward.first = PLEDGE_SURPLUS_REWARD_ADDRESS;
                destroyReward.second += pIndex->GetBlockReward();
            }
        }
        pIndex = pIndex->pPrev;
    }

    // Obtain the main chain block hash, which is used to obtain voting data
    uint256 hashPrimaryBeginBlock;
    uint256 hashPrimaryTailBlock;
    if (pTailIndex->IsPrimary())
    {
        hashPrimaryTailBlock = pTailIndex->pPrev->GetBlockHash();
    }
    else
    {
        if (!GetBlockHashByHeightSlot(pCoreProtocol->GetGenesisBlockHash(), pTailIndex->GetBlockHeight() - 1, 0, hashPrimaryTailBlock))
        {
            StdLog("BlockChain", "Calculate block vote reward: Get primary tail block hash fail, height: %d, tail block:%s",
                   pTailIndex->GetBlockHeight() - 1, pTailIndex->GetBlockHash().GetHex().c_str());
            return false;
        }
    }
    if (!GetBlockHashByHeightSlot(pCoreProtocol->GetGenesisBlockHash(), nBeginHeight - 1, 0, hashPrimaryBeginBlock))
    {
        StdLog("BlockChain", "Calculate block vote reward: Get primary begin block hash fail, height: %d, tail block:%s",
               nBeginHeight - 1, pTailIndex->GetBlockHash().GetHex().c_str());
        return false;
    }

    // Slice row voting data, starting from the bottom height
    class CListDayVoteWalker : public storage::CDayVoteWalker
    {
    public:
        CListDayVoteWalker(const std::map<uint32, CCalcBlock>& mapCalcBlockIn, std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteRewardIn)
          : mapInternalCalcBlock(mapCalcBlockIn), mapInternalVoteReward(mapVoteRewardIn) {}

        // nHeight = (nBeginHeight-1 ~ nTailHeight-1) or (0 ~ 4319)
        bool Walk(const uint32 nHeight, const std::map<CDestination, std::pair<std::map<CDestination, CVoteContext>, uint256>>& mapDelegateVote) override
        {
            //mapDelegateVote key: delegate address, value: map key: vote address, map value: vote context, second: total vote amount
            auto it = mapInternalCalcBlock.find(nHeight + 1);
            if (it != mapInternalCalcBlock.end())
            {
                const CCalcBlock& calcBlock = it->second;
                if (calcBlock.nRewardAmount == 0)
                {
                    return true;
                }
                uint256 nBlockReward = calcBlock.nRewardAmount;
                if (REWARD_DISTRIBUTION_RATIO_ENABLE)
                {
                    uint256 nProjectPartyReward = (nBlockReward * uint256(REWARD_DISTRIBUTION_RATIO_PROJECT_PARTY) / uint256(REWARD_DISTRIBUTION_PER));
                    uint256 nFoundationReward = (nBlockReward * uint256(REWARD_DISTRIBUTION_RATIO_FOUNDATION) / uint256(REWARD_DISTRIBUTION_PER));

                    if (nProjectPartyReward > 0)
                    {
                        auto& voteRewardProjectParty = mapInternalVoteReward[PROJECT_PARTY_REWARD_TO_ADDRESS];
                        voteRewardProjectParty.first = PROJECT_PARTY_REWARD_TO_ADDRESS;
                        voteRewardProjectParty.second += nProjectPartyReward;
                    }

                    if (nFoundationReward > 0)
                    {
                        auto& voteRewardFoundation = mapInternalVoteReward[FOUNDATION_REWARD_TO_ADDRESS];
                        voteRewardFoundation.first = FOUNDATION_REWARD_TO_ADDRESS;
                        voteRewardFoundation.second += nFoundationReward;
                    }

                    nBlockReward -= (nProjectPartyReward + nFoundationReward);
                }
                if (calcBlock.nRewardRation != 0)
                {
                    //nBlockReward -= (nBlockReward * uint256(calcBlock.nRewardRation) / uint256(MINT_REWARD_PER));
                    uint256 nMintCommissionReward = (nBlockReward * uint256(calcBlock.nRewardRation) / uint256(MINT_REWARD_PER));
                    if (nMintCommissionReward > 0)
                    {
                        auto& voteRewardMint = mapInternalVoteReward[calcBlock.destMint];
                        voteRewardMint.first = calcBlock.destMint;
                        voteRewardMint.second += nMintCommissionReward;
                    }
                    nBlockReward -= nMintCommissionReward;
                }
                if (nBlockReward == 0)
                {
                    return true;
                }

                auto mi = mapDelegateVote.find(calcBlock.destMint);
                if (mi != mapDelegateVote.end())
                {
                    const auto& mapDestVote = mi->second.first;
                    const uint256& nDelegateTotalVoteAmount = mi->second.second;
                    if (nDelegateTotalVoteAmount > 0)
                    {
                        // distribute reward
                        uint256 nDistributeTotalReward = 0;
                        for (const auto& [destVote, ctxt] : mapDestVote)
                        {
                            if (ctxt.nVoteAmount > 0)
                            {
                                uint256 nDestReward = nBlockReward * ctxt.nVoteAmount / nDelegateTotalVoteAmount;
                                auto& voteReward = mapInternalVoteReward[destVote];
                                const CVoteContext& ctxtVote = ctxt;
                                if (ctxtVote.nRewardMode == CVoteContext::REWARD_MODE_VOTE)
                                {
                                    voteReward.first = destVote;
                                }
                                else
                                {
                                    voteReward.first = ctxtVote.destOwner;
                                }
                                voteReward.second += nDestReward;
                                nDistributeTotalReward += nDestReward;
                            }
                        }
                        if (mapDestVote.size() > 0 && nBlockReward > nDistributeTotalReward)
                        {   // randomly reward the rest to someone
                            bool fDistribute = false;
                            auto rest = nBlockReward - nDistributeTotalReward;
                            for (const auto& [k, v] : mapDestVote)
                            {
                                auto mi = mapInternalVoteReward.find(k);
                                if (mi != mapInternalVoteReward.end())
                                {
                                    mi->second.second += rest;
                                    fDistribute = true;
                                    break;
                                }
                            }
                            if (!fDistribute)
                            {
                                if (mapInternalVoteReward.empty())
                                {
                                    auto& destVote = mapDestVote.begin()->first;
                                    auto& voteReward = mapInternalVoteReward[destVote];
                                    const CVoteContext& ctxtVote = mapDestVote.begin()->second;
                                    if (ctxtVote.nRewardMode == CVoteContext::REWARD_MODE_VOTE)
                                    {
                                        voteReward.first = destVote;
                                    }
                                    else
                                    {
                                        voteReward.first = ctxtVote.destOwner;
                                    }
                                    voteReward.second += rest;
                                }
                                else
                                {
                                    mapInternalVoteReward.begin()->second.second += rest;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }

    public:
        const std::map<uint32, CCalcBlock>& mapInternalCalcBlock;
        std::map<CDestination, std::pair<CDestination, uint256>>& mapInternalVoteReward; // key: vote address(pledge or delegate addr), value.key: reward address(pubkey owner of pledge addr or delegate self), value.value: reward amount
    };

    CListDayVoteWalker walker(mapCalcBlock, mapVoteReward);
    if (!cntrBlock.WalkThroughDayVote(hashPrimaryBeginBlock, hashPrimaryTailBlock, walker))
    {
        StdLog("BlockChain", "Calculate block vote reward: Walk through day vote fail, hashCalcEndBlock: %s", hashCalcEndBlock.GetHex().c_str());
        return false;
    }
    return true;
}

} // namespace metabasenet

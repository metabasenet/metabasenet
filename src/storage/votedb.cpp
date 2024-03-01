// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "votedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"
#include "param.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

const string DB_VOTE_KEY_ID_TRIEROOT("trieroot");
const string DB_VOTE_KEY_ID_PREVROOT("prevroot");
const string DB_VOTE_KEY_ID_DELEGATEENROLL("delegateenroll");

const uint8 DB_VOTE_ROOT_TYPE_USER_VOTE = 0x10;
const uint8 DB_VOTE_ROOT_TYPE_VOTE_REWARD = 0x20;
const uint8 DB_VOTE_ROOT_TYPE_DELEGATE_VOTE = 0x30;
const uint8 DB_VOTE_ROOT_TYPE_DELEGATE_ENROLL = 0x40;

const uint8 DB_VOTE_KEY_TYPE_USER_VOTE_ADDRESS = DB_VOTE_ROOT_TYPE_USER_VOTE | 0x01;
const uint8 DB_VOTE_KEY_TYPE_USER_VOTE_HEIGHT_ADDRESS = DB_VOTE_ROOT_TYPE_USER_VOTE | 0x02;
const uint8 DB_VOTE_KEY_TYPE_USER_VOTE_FINAL_HEIGHT_ADDRESS = DB_VOTE_ROOT_TYPE_USER_VOTE | 0x03;

const uint8 DB_VOTE_KEY_TYPE_VOTE_REWARD_ADDRESS = DB_VOTE_ROOT_TYPE_VOTE_REWARD | 0x01;

const uint8 DB_VOTE_KEY_TYPE_DELEGATE_VOTE_ADDRESS = DB_VOTE_ROOT_TYPE_DELEGATE_VOTE | 0x01;

//////////////////////////////
// CListAllVoteTrieDBWalker

bool CListAllVoteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        mtbase::StdError("CListAllVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_VOTE_KEY_TYPE_USER_VOTE_ADDRESS)
        {
            mtbase::CBufStream ssValue(btValue);

            CDestination destVote;
            CVoteContext ctxtVote;
            ssKey >> destVote;
            ssValue >> ctxtVote;
            if (ctxtVote.nVoteAmount > 0)
            {
                mapDelegateVote[ctxtVote.destDelegate][destVote] = ctxtVote;
            }
        }
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListMoreIncVoteTrieDBWalker

bool CListMoreIncVoteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        mtbase::StdError("CListMoreIncVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_VOTE_KEY_TYPE_USER_VOTE_HEIGHT_ADDRESS)
        {
            mtbase::CBufStream ssValue(btValue);

            uint32 nBlockHeight;
            CDestination destVote;
            CVoteContext ctxtVote;
            ssKey >> nBlockHeight >> destVote;
            ssValue >> ctxtVote;

            nBlockHeight = BSwap32(nBlockHeight);
            if (nBlockHeight >= nEndHeight)
            {
                mapIncVote[nBlockHeight][destVote] = ctxtVote;
            }
            else
            {
                fWalkOver = true;
            }
        }
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListPledgeFinalHeightTrieDBWalker

bool CListPledgeFinalHeightTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        mtbase::StdError("CListPledgeFinalHeightTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_VOTE_KEY_TYPE_USER_VOTE_FINAL_HEIGHT_ADDRESS)
        {
            mtbase::CBufStream ssValue(btValue);

            uint32 nFinalHeight = 0;
            CDestination destPledge;
            uint32 nPledgeHeight = 0;

            ssKey >> nFinalHeight >> destPledge;
            ssValue >> nPledgeHeight;

            // nPledgeHeight is 0 indicates remove
            if (nPledgeHeight > 0)
            {
                mapPledgeFinalHeight[destPledge] = std::make_pair(BSwap32(nFinalHeight), nPledgeHeight);
            }
        }
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListVoteRewardTrieDBWalker

bool CListVoteRewardTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListVoteRewardTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        uint32 nChainId;
        uint8 nKeyType;
        CDestination dest;
        uint32 nBlockHeight;

        mtbase::CBufStream ssKey(btKey);
        ssKey >> nKeyType;

        if (nKeyType == DB_VOTE_KEY_TYPE_VOTE_REWARD_ADDRESS)
        {
            ssKey >> nChainId;
            if (nChainId == nGetChainId)
            {
                ssKey >> dest >> nBlockHeight;
                nBlockHeight = BSwap32(nBlockHeight);

                uint256 nRewardAmount;
                mtbase::CBufStream ssValue(btValue);
                ssValue >> nRewardAmount;

                vVoteReward.push_back(std::make_pair(nBlockHeight, nRewardAmount));
                if (nGetCount > 0 && vVoteReward.size() >= nGetCount)
                {
                    fWalkOver = true;
                }
            }
        }
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListDelegateVoteTrieDBWalker

bool CListDelegateVoteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListDelegateVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        mtbase::CBufStream ssKey(btKey);

        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_VOTE_KEY_TYPE_DELEGATE_VOTE_ADDRESS)
        {
            mtbase::CBufStream ssValue(btValue);
            CDestination destDelegate;
            uint256 nVoteAmount;

            ssKey >> destDelegate;
            ssValue >> nVoteAmount;

            if (nVoteAmount != 0)
            {
                mapVote.insert(std::make_pair(destDelegate, nVoteAmount));
            }
        }
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CVoteDB

bool CVoteDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData / "vote"))
    {
        return false;
    }
    return true;
}

void CVoteDB::Deinitialize()
{
    mapCacheCalcVote.clear();
    cacheDelegate.Clear();
    dbTrie.Deinitialize();
}

void CVoteDB::Clear()
{
    mapCacheCalcVote.clear();
    cacheDelegate.Clear();
    dbTrie.Clear();
}

bool CVoteDB::AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                           const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight, uint256& hashVoteRoot)
{
    uint256 hashPrevRoot;
    if (hashPrev != 0)
    {
        uint64 nVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashPrev, hashPrevRoot, nVoteCount))
        {
            StdLog("CVoteDB", "Add Block Vote: Read trie root fail, prev: %s", hashPrev.GetHex().c_str());
            return false;
        }
    }

    uint32 nBlockHeight = CBlock::GetBlockHeightByHash(hashBlock);
    nBlockHeight = BSwap32(nBlockHeight);

    bytesmap mapKv;
    for (const auto& kv : mapBlockVote)
    {
        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_VOTE_KEY_TYPE_USER_VOTE_ADDRESS << kv.first;
            ssKey.GetData(btKey);

            ssValue << kv.second;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_VOTE_KEY_TYPE_USER_VOTE_HEIGHT_ADDRESS << nBlockHeight << kv.first;
            ssKey.GetData(btKey);

            ssValue << kv.second;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }
    }
    // key: pledge address, value first: final height, value second: pledge height
    for (const auto& kv : mapAddPledgeFinalHeight)
    {
        const CDestination& destPledge = kv.first;
        const uint32 nFinalHeight = BSwap32(kv.second.first);
        const uint32 nPledgeHeight = kv.second.second;

        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_VOTE_KEY_TYPE_USER_VOTE_FINAL_HEIGHT_ADDRESS << nFinalHeight << destPledge;
        ssKey.GetData(btKey);

        ssValue << nPledgeHeight;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    // key: pledge address, value: old final height
    for (const auto& kv : mapRemovePledgeFinalHeight)
    {
        const CDestination& destPledge = kv.first;
        const uint32 nOldFinalHeight = BSwap32(kv.second);
        const uint32 nPledgeHeight = 0; // set 0, remove final height

        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_VOTE_KEY_TYPE_USER_VOTE_FINAL_HEIGHT_ADDRESS << nOldFinalHeight << destPledge;
        ssKey.GetData(btKey);

        ssValue << nPledgeHeight;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashVoteRoot))
    {
        StdLog("CVoteDB", "Add Block Vote: Add trie fail, prev root: %s, prev block: %s",
               hashPrevRoot.GetHex().c_str(), hashPrev.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashBlock, hashVoteRoot, mapKv.size() - 1))
    {
        StdLog("CVoteDB", "Add Block Vote: Write block root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Retrieve All Delegate Vote: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_TYPE_USER_VOTE_ADDRESS;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListAllVoteTrieDBWalker walker(mapDelegateVote);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CVoteDB", "Retrieve All Delegate Vote: Walk through trie, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Retrieve Dest Vote Context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    mtbase::CBufStream ssKey;
    bytes btKey, btValue;
    ssKey << DB_VOTE_KEY_TYPE_USER_VOTE_ADDRESS << destVote;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashTrieRoot, btKey, btValue))
    {
        StdLog("CVoteDB", "Retrieve Dest Vote Context: Retrieve kv trie, root: %s, block: %s", hashTrieRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        mtbase::CBufStream ssValue(btValue);
        ssValue >> ctxtVote;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CVoteDB::ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight)
{
    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "List Pledge Final Height: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_TYPE_USER_VOTE_FINAL_HEIGHT_ADDRESS << BSwap32(nFinalHeight);
    ssKeyPrefix.GetData(btKeyPrefix);

    CListPledgeFinalHeightTrieDBWalker walker(mapPledgeFinalHeight);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CVoteDB", "List Pledge Final Height: Walk through trie, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::VerifyVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashBlock, hashRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Verify Vote: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CVoteDB", "Verify Vote: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashPrevBlock, hashPrevRoot, nVoteCount))
        {
            StdLog("CVoteDB", "Verify Vote: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CVoteDB", "Verify Vote: Get prev root fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CVoteDB", "Verify Vote: Verify error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, block: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    CWriteLock wlock(rwAccess);

    uint32 nBeginHeight = CBlock::GetBlockHeightByHash(hashBeginBlock);
    uint32 nTailHeight = CBlock::GetBlockHeightByHash(hashTailBlock);
    if (nBeginHeight + VOTE_REWARD_DISTRIBUTE_HEIGHT - 1 != nTailHeight)
    {
        StdLog("CVoteDB", "Walk Through Day Vote: hashBeginBlock or hashTailBlock error, hashBeginBlock: [%d] %s, hashTailBlock: [%d] %s",
               nBeginHeight, hashBeginBlock.GetHex().c_str(), nTailHeight, hashTailBlock.GetHex().c_str());
        return false;
    }

    auto it = mapCacheCalcVote.find(hashTailBlock);
    if (it == mapCacheCalcVote.end())
    {
        if (!GetCalcDayVote(hashBeginBlock, hashTailBlock))
        {
            StdLog("CVoteDB", "Walk Through Day Vote: Get calc day vote fail, hashBeginBlock: [%d] %s, hashTailBlock: [%d] %s",
                   nBeginHeight, hashBeginBlock.GetHex().c_str(), nTailHeight, hashTailBlock.GetHex().c_str());
            return false;
        }
        it = mapCacheCalcVote.find(hashTailBlock);
        if (it == mapCacheCalcVote.end())
        {
            StdLog("CVoteDB", "Walk Through Day Vote: Find day vote fail, hashBeginBlock: [%d] %s, hashTailBlock: [%d] %s",
                   nBeginHeight, hashBeginBlock.GetHex().c_str(), nTailHeight, hashTailBlock.GetHex().c_str());
            return false;
        }
    }
    const CCacheCalcVoteRewardContext& cacheDayVote = it->second;
    std::map<CDestination, std::pair<std::map<CDestination, CVoteContext>, uint256>> mapFullVote; // key: delegate address, value: map key: vote address, map value: vote context, second: total vote amount

    for (const auto& kv : cacheDayVote.mapFirstFullVote)
    {
        auto& delegateVote = mapFullVote[kv.first];
        delegateVote.first = kv.second;
        delegateVote.second = 0;
        for (const auto& vd : kv.second)
        {
            delegateVote.second += vd.second.nVoteAmount;
        }
    }
    if (!walker.Walk(nBeginHeight, mapFullVote))
    {
        StdLog("CVoteDB", "Walk Through Day Vote: Walk first fail, hashBeginBlock: [%d] %s, hashTailBlock: [%d] %s",
               nBeginHeight, hashBeginBlock.GetHex().c_str(), nTailHeight, hashTailBlock.GetHex().c_str());
        return false;
    }

    for (uint32 h = nBeginHeight + 1; h <= nTailHeight; h++)
    {
        auto mt = cacheDayVote.mapIncVote.find(h);
        if (mt != cacheDayVote.mapIncVote.end())
        {
            for (const auto& kv : mt->second)
            {
                const CDestination& destVote = kv.first;
                const CVoteContext& ctxtVote = kv.second;
                auto& voteDelegate = mapFullVote[ctxtVote.destDelegate];
                auto& voteContext = voteDelegate.first[destVote];
                if (voteContext.nVoteAmount <= ctxtVote.nVoteAmount)
                {
                    voteDelegate.second += (ctxtVote.nVoteAmount - voteContext.nVoteAmount); // Calculate total votes
                }
                else
                {
                    voteDelegate.second -= (voteContext.nVoteAmount - ctxtVote.nVoteAmount);
                }
                voteContext = ctxtVote;
            }
        }
        if (!walker.Walk(h, mapFullVote))
        {
            StdLog("CVoteDB", "Walk Through Day Vote: Walk fail, height: %d, hashBeginBlock: [%d] %s, hashTailBlock: [%d] %s",
                   h, nBeginHeight, hashBeginBlock.GetHex().c_str(), nTailHeight, hashTailBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

///////////////////////////////////
bool CVoteDB::WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot, const uint64 nVoteCount)
{
    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_VOTE_KEY_ID_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot << nVoteCount;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CVoteDB::ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot, uint64& nVoteCount)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_VOTE_KEY_ID_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot >> nVoteCount;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CVoteDB::AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << nRootType << DB_VOTE_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CVoteDB::GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << nRootType << DB_VOTE_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashPrevRoot >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CVoteDB::GetMoreIncVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVote)
{
    if (hashBeginBlock == 0 || hashTailBlock == 0)
    {
        StdLog("CVoteDB", "Get More Inc Vote: hashBeginBlock or hashTailBlock is 0, hashBeginBlock: %s, hashTailBlock: %s",
               hashBeginBlock.GetHex().c_str(), hashTailBlock.GetHex().c_str());
        return false;
    }

    uint256 hashTrieRoot;
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_USER_VOTE, hashTailBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Get More Inc Vote: Read trie root fail, hashTailBlock: %s", hashTailBlock.GetHex().c_str());
        return false;
    }

    uint32 nBeginBlockHeight = CBlock::GetBlockHeightByHash(hashBeginBlock);

    bytes btKeyPrefix;
    bytes btBeginKeyTail;

    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_TYPE_USER_VOTE_HEIGHT_ADDRESS;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListMoreIncVoteTrieDBWalker walker(nBeginBlockHeight + 1, mapIncVote);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix, btBeginKeyTail, true))
    {
        StdLog("CVoteDB", "Get More Inc Vote: Walk through trie, hashTailBlock: %s", hashTailBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::GetCalcDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock)
{
    while (mapCacheCalcVote.size() >= VOTE_USER_VOTE_CACHE_SIZE)
    {
        mapCacheCalcVote.erase(mapCacheCalcVote.begin());
    }

    CCacheCalcVoteRewardContext& cacheDayVote = mapCacheCalcVote[hashTailBlock];
    cacheDayVote.Clear();

    if (!RetrieveAllDelegateVote(hashBeginBlock, cacheDayVote.mapFirstFullVote))
    {
        StdLog("CVoteDB", "Get calc day vote: Retrieve all delegate vote fail, begin block: %s", hashBeginBlock.GetHex().c_str());
        return false;
    }

    if (!GetMoreIncVote(hashBeginBlock, hashTailBlock, cacheDayVote.mapIncVote))
    {
        StdLog("CVoteDB", "Get calc day vote: Get more inc vote fail, tail block: %s", hashTailBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
// delegate db

bool CVoteDB::AddNewDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapDelegateVote,
                             const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot)
{
    if (!AddDelegateVote(hashPrevBlock, hashBlock, mapDelegateVote, hashDelegateRoot))
    {
        StdLog("CVoteDB", "Add new delegate: Add delegate vote fail, hashBlock: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!AddDelegateEnroll(hashBlock, mapEnrollTx))
    {
        StdLog("CVoteDB", "Add new delegate: Add delegate enroll fail, hashBlock: %s", hashBlock.GetHex().c_str());
        return false;
    }
    cacheDelegate.AddNew(hashBlock, mapEnrollTx);
    return true;
}

bool CVoteDB::AddDelegateVote(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapDelegateVote, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        uint64 nDelegateVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashPrevBlock, hashPrevRoot, nDelegateVoteCount))
        {
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapDelegateVote)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_VOTE_KEY_TYPE_DELEGATE_VOTE_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CVoteDB", "Add delegate vote: Add new trie fail, hashBlock: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashBlock, hashNewRoot, mapDelegateVote.size()))
    {
        StdLog("CVoteDB", "Add delegate vote: Write trie root fail, hashBlock: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::AddDelegateEnroll(const uint256& hashBlock, const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_VOTE_ROOT_TYPE_DELEGATE_ENROLL << DB_VOTE_KEY_ID_DELEGATEENROLL << hashBlock;
    ssValue << mapEnrollTx;
    if (!dbTrie.WriteExtKv(ssKey, ssValue))
    {
        StdLog("CVoteDB", "Add delegate enroll: Write ext kv fail, hashBlock: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount)
{
    uint256 hashTrieRoot;
    if (hashBlock != 0)
    {
        uint64 nDelegateVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashBlock, hashTrieRoot, nDelegateVoteCount))
        {
            return false;
        }
    }

    mtbase::CBufStream ssKey;
    bytes btKey, btValue;
    ssKey << DB_VOTE_KEY_TYPE_DELEGATE_VOTE_ADDRESS << destDelegate;
    ssKey.GetData(btKey);

    if (!dbTrie.Retrieve(hashTrieRoot, btKey, btValue))
    {
        return false;
    }

    try
    {
        mtbase::CBufStream ssValue(btValue);
        ssValue >> nVoteAmount;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CVoteDB::RetrieveDelegatedVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapDelegateVote)
{
    uint256 hashTrieRoot;
    if (hashBlock != 0)
    {
        uint64 nDelegateVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashBlock, hashTrieRoot, nDelegateVoteCount))
        {
            return false;
        }
    }

    CListDelegateVoteTrieDBWalker walker(mapDelegateVote);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker))
    {
        return false;
    }
    return true;
}

bool CVoteDB::RetrieveDelegatedEnroll(const uint256& hashBlock, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos)
{
    if (cacheDelegate.Retrieve(hashBlock, mapEnrollTxPos))
    {
        return true;
    }

    CBufStream ssKey, ssValue;
    ssKey << DB_VOTE_ROOT_TYPE_DELEGATE_ENROLL << DB_VOTE_KEY_ID_DELEGATEENROLL << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> mapEnrollTxPos;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    cacheDelegate.AddNew(hashBlock, mapEnrollTxPos);
    return true;
}

bool CVoteDB::RetrieveRangeEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    for (const uint256& hashBlock : boost::adaptors::reverse(vBlockRange))
    {
        if (hashBlock == 0)
        {
            continue;
        }

        std::map<int, std::map<CDestination, CDiskPos>> mapGetEnrollTx;
        if (!RetrieveDelegatedEnroll(hashBlock, mapGetEnrollTx))
        {
            StdLog("CVoteDB", "Retrieve range enroll: Retrieve delegated enroll fail, hashBlock: %s", hashBlock.GetHex().c_str());
            return false;
        }

        auto it = mapGetEnrollTx.find(height);
        if (it != mapGetEnrollTx.end())
        {
            mapEnrollTxPos.insert(it->second.begin(), it->second.end());
        }
    }
    return true;
}

bool CVoteDB::VerifyDelegateVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (hashBlock != 0)
    {
        uint64 nDelegateVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashBlock, hashRoot, nDelegateVoteCount))
        {
            StdLog("CVoteDB", "Verify delegate vote: Read trie root fail, hashBlock: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CVoteDB", "Verify delegate vote: Check trie node fail, hashRoot: %s, hashBlock: %s",
                   hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        uint64 nDelegateVoteCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashPrevBlock, hashPrevRoot, nDelegateVoteCount))
        {
            StdLog("CVoteDB", "Verify delegate vote: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_VOTE_ROOT_TYPE_DELEGATE_VOTE, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CVoteDB", "Verify delegate vote: Get prev root fail, hashRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CVoteDB", "Verify delegate vote: Verify prev root fail, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s, hashPrevBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
// vote reward db

bool CVoteDB::AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        uint64 nRewardCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashPrevBlock, hashPrevRoot, nRewardCount))
        {
            StdLog("CVoteDB", "Add vote reward: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapVoteReward)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_VOTE_KEY_TYPE_VOTE_REWARD_ADDRESS << nChainId << kv.first << BSwap32(nBlockHeight);
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CVoteDB", "Add vote reward: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashBlock, hashNewRoot, mapVoteReward.size()))
    {
        StdLog("CVoteDB", "Add vote reward: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    uint256 hashRoot;
    uint64 nRewardCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashBlock, hashRoot, nRewardCount))
    {
        StdLog("CVoteDB", "List vote reward: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_TYPE_VOTE_REWARD_ADDRESS << nChainId << dest;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    bytes btBeginKeyTail;
    CListVoteRewardTrieDBWalker walker(nChainId, nGetCount, vVoteReward);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix, btBeginKeyTail, true))
    {
        StdLog("CVoteDB", "List vote reward: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteDB::VerifyVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    uint64 nRewardCount = 0;
    if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashBlock, hashRoot, nRewardCount))
    {
        StdLog("CVoteDB", "Verify vote reward: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CVoteDB", "Verify vote reward: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        uint64 nPrevRewardCount = 0;
        if (!ReadTrieRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashPrevBlock, hashPrevRoot, nPrevRewardCount))
        {
            StdLog("CVoteDB", "Verify vote reward: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_VOTE_ROOT_TYPE_VOTE_REWARD, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CVoteDB", "Verify vote reward: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CVoteDB", "Verify vote reward: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

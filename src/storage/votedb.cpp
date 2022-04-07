// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "votedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"
#include "param.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

#define DB_VOTE_KEY_NAME_VOTE_DEST string("vdest")
#define DB_VOTE_KEY_NAME_HEIGHT_DEST string("hdest")
#define DB_VOTE_KEY_NAME_TRIEROOT string("trieroot")
#define DB_VOTE_KEY_NAME_PREVROOT string("prevroot")

//////////////////////////////
// CListAllVoteTrieDBWalker

bool CListAllVoteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        hnbase::StdError("CListAllVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        hnbase::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_VOTE_KEY_NAME_VOTE_DEST)
        {
            hnbase::CBufStream ssValue(btValue);

            CDestination destVote;
            CVoteContext ctxtVote;
            ssKey >> destVote;
            ssValue >> ctxtVote;

            mapDelegateVote[ctxtVote.destDelegate][destVote] = ctxtVote;
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
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
        hnbase::StdError("CListMoreIncVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        hnbase::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_VOTE_KEY_NAME_HEIGHT_DEST)
        {
            hnbase::CBufStream ssValue(btValue);

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
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
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
    dbTrie.Deinitialize();
}

void CVoteDB::Clear()
{
    mapCacheCalcVote.clear();
    dbTrie.Clear();
}

bool CVoteDB::AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote, uint256& hashVoteRoot)
{
    uint256 hashPrevRoot;
    uint64 nVoteCount = 0;
    if (hashPrev != 0)
    {
        if (!ReadTrieRoot(hashPrev, hashPrevRoot, nVoteCount))
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
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_VOTE_KEY_NAME_VOTE_DEST << kv.first;
            ssKey.GetData(btKey);

            ssValue << kv.second;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_VOTE_KEY_NAME_HEIGHT_DEST << nBlockHeight << kv.first;
            ssKey.GetData(btKey);

            ssValue << kv.second;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashVoteRoot))
    {
        StdLog("CVoteDB", "Add Block Vote: Add trie fail, prev root: %s, prev block: %s",
               hashPrevRoot.GetHex().c_str(), hashPrev.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashVoteRoot, mapKv.size() - 1))
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
    if (!ReadTrieRoot(hashBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Retrieve All Delegate Vote: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_NAME_VOTE_DEST;
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
    if (!ReadTrieRoot(hashBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Retrieve Dest Vote Context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_VOTE_KEY_NAME_VOTE_DEST << destVote;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashTrieRoot, btKey, btValue))
    {
        StdLog("CVoteDB", "Retrieve Dest Vote Context: Retrieve kv trie, root: %s, block: %s", hashTrieRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtVote;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CVoteDB::VerifyVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    uint64 nVoteCount = 0;
    if (!ReadTrieRoot(hashBlock, hashRoot, nVoteCount))
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
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot, nVoteCount))
        {
            StdLog("CVoteDB", "Verify Vote: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
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
    std::map<CDestination, std::pair<std::map<CDestination, CVoteContext>, uint256>> mapFullVote;

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
bool CVoteDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot, const uint64 nVoteCount)
{
    CBufStream ssKey;
    ssKey << DB_VOTE_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ssKey.GetData(), ssKey.GetSize());

    CBufStream ssValue;
    ssValue << hashTrieRoot << nVoteCount;
    bytes btValue;
    ssValue.GetData(btValue);
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CVoteDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot, uint64& nVoteCount)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ssKey;
    ssKey << DB_VOTE_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ssKey.GetData(), ssKey.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    CBufStream ssValue(btValue);
    ssValue >> hashTrieRoot >> nVoteCount;
    return true;
}

void CVoteDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_VOTE_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CVoteDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_VOTE_KEY_NAME_PREVROOT;
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
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
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
    if (!ReadTrieRoot(hashTailBlock, hashTrieRoot, nVoteCount))
    {
        StdLog("CVoteDB", "Get More Inc Vote: Read trie root fail, hashTailBlock: %s", hashTailBlock.GetHex().c_str());
        return false;
    }

    uint32 nBeginBlockHeight = CBlock::GetBlockHeightByHash(hashBeginBlock);

    bytes btKeyPrefix;
    bytes btBeginKeyTail;

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_KEY_NAME_HEIGHT_DEST;
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
    while (mapCacheCalcVote.size() >= VOTE_CACHE_SIZE)
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

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "voterewarddb.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

#define DB_VOTE_REWARD_KEY_NAME_DEST string("dest")
#define DB_VOTE_REWARD_KEY_NAME_TRIEROOT string("trieroot")
#define DB_VOTE_REWARD_KEY_NAME_PREVROOT string("prevroot")

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
        string strName;
        CDestination dest;
        uint32 nBlockHeight;
        hnbase::CBufStream ssKey(btKey);
        ssKey >> strName >> dest >> nBlockHeight;

        nBlockHeight = BSwap32(nBlockHeight);

        uint256 nRewardAmount;
        hnbase::CBufStream ssValue(btValue);
        ssValue >> nRewardAmount;

        vVoteReward.push_back(std::make_pair(nBlockHeight, nRewardAmount));
        if (nGetCount > 0 && vVoteReward.size() >= nGetCount)
        {
            fWalkOver = true;
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
// CForkVoteRewardDB

CForkVoteRewardDB::CForkVoteRewardDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkVoteRewardDB::~CForkVoteRewardDB()
{
    dbTrie.Deinitialize();
}

bool CForkVoteRewardDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkVoteRewardDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkVoteRewardDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkVoteRewardDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkVoteRewardDB::AddVoteReward(const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkVoteRewardDB", "Add vote reward: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapVoteReward)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_VOTE_REWARD_KEY_NAME_DEST << kv.first << BSwap32(nBlockHeight);
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkVoteRewardDB", "Add vote reward: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkVoteRewardDB", "Add vote reward: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkVoteRewardDB::ListVoteReward(const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkVoteRewardDB", "List vote reward: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_VOTE_REWARD_KEY_NAME_DEST << dest;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    bytes btBeginKeyTail;
    CListVoteRewardTrieDBWalker walker(nGetCount, vVoteReward);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix, btBeginKeyTail, true))
    {
        StdLog("CForkVoteRewardDB", "List vote reward: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkVoteRewardDB::VerifyVoteReward(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkVoteRewardDB", "Verify vote reward: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkVoteRewardDB", "Verify vote reward: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkVoteRewardDB", "Verify vote reward: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkVoteRewardDB", "Verify vote reward: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkVoteRewardDB", "Verify vote reward: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkVoteRewardDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ss;
    ss << DB_VOTE_REWARD_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkVoteRewardDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ss;
    ss << DB_VOTE_REWARD_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkVoteRewardDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_VOTE_REWARD_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkVoteRewardDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_VOTE_REWARD_KEY_NAME_PREVROOT;
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

//////////////////////////////
// CVoteRewardDB

bool CVoteRewardDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "votereward";

    if (!boost::filesystem::exists(pathAddress))
    {
        boost::filesystem::create_directories(pathAddress);
    }

    if (!boost::filesystem::is_directory(pathAddress))
    {
        return false;
    }
    return true;
}

void CVoteRewardDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapVoteRewardDB.clear();
}

bool CVoteRewardDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapVoteRewardDB.find(hashFork) != mapVoteRewardDB.end());
}

bool CVoteRewardDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapVoteRewardDB.find(hashFork);
    if (it != mapVoteRewardDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkVoteRewardDB> spAddress(new CForkVoteRewardDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapVoteRewardDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CVoteRewardDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapVoteRewardDB.find(hashFork);
    if (it != mapVoteRewardDB.end())
    {
        it->second->RemoveAll();
        mapVoteRewardDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CVoteRewardDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CVoteRewardDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapVoteRewardDB.begin();
    while (it != mapVoteRewardDB.end())
    {
        it->second->RemoveAll();
        mapVoteRewardDB.erase(it++);
    }
}

bool CVoteRewardDB::AddVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapVoteRewardDB.find(hashFork);
    if (it != mapVoteRewardDB.end())
    {
        return it->second->AddVoteReward(hashPrevBlock, hashBlock, nBlockHeight, mapVoteReward, hashNewRoot);
    }
    return false;
}

bool CVoteRewardDB::ListVoteReward(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    CReadLock rlock(rwAccess);

    auto it = mapVoteRewardDB.find(hashFork);
    if (it != mapVoteRewardDB.end())
    {
        return it->second->ListVoteReward(hashBlock, dest, nGetCount, vVoteReward);
    }
    return false;
}

bool CVoteRewardDB::VerifyVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapVoteRewardDB.find(hashFork);
    if (it != mapVoteRewardDB.end())
    {
        return it->second->VerifyVoteReward(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

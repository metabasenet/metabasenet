// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txindexdb.h"

#include <boost/bind.hpp>

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

#define DB_TXINDEX_KEY_NAME_TXID string("txid")
#define DB_TXINDEX_KEY_NAME_TRIEROOT string("trieroot")
#define DB_TXINDEX_KEY_NAME_PREVROOT string("prevroot")

//////////////////////////////
// CListTxIndexTrieDBWalker

bool CListTxIndexTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        hcode::StdError("CListTxIndexTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        hcode::CBufStream ssKey;
        ssKey.Write((char*)(btKey.data()), btKey.size());
        string strName;
        ssKey >> strName;
        if (strName == DB_TXINDEX_KEY_NAME_TXID)
        {
            hcode::CBufStream ssValue;
            ssValue.Write((char*)(btValue.data()), btValue.size());
            uint256 txid;
            CTxIndex txIndex;
            ssKey >> txid;
            ssValue >> txIndex;
            mapTxIndex.insert(std::make_pair(txid, txIndex));
        }
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CForkTxIndexDB

CForkTxIndexDB::CForkTxIndexDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkTxIndexDB::~CForkTxIndexDB()
{
    mapCacheTxIndex.clear();
    dbTrie.Deinitialize();
}

bool CForkTxIndexDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkTxIndexDB", "Initialize: Initialize fail, hashFork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkTxIndexDB::Deinitialize()
{
    mapCacheTxIndex.clear();
    dbTrie.Deinitialize();
}

bool CForkTxIndexDB::RemoveAll()
{
    mapCacheTxIndex.clear();
    dbTrie.RemoveAll();
    return true;
}

bool CForkTxIndexDB::AddBlockTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkTxIndexDB", "Add Block Tx Index: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapBlockTxIndex)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_TXINDEX_KEY_NAME_TXID << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkTxIndexDB", "Add Block Tx Index: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkTxIndexDB", "Add Block Tx Index: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkTxIndexDB::RetrieveTxIndex(const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkTxIndexDB", "Retrieve tx index: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_TXINDEX_KEY_NAME_TXID << txid;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        //StdLog("CForkTxIndexDB", "Retrieve tx index: Retrieve kv fail, ssKey: %s, txid: %s, root: %s, block: %s",
        //       ToHexString(btKey).c_str(), txid.GetHex().c_str(),
        //       hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> txIndex;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkTxIndexDB::VerifyTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        return false;
    }
    return true;
}

bool CForkTxIndexDB::ListAllTxIndex(const uint256& hashBlock, std::map<uint256, CTxIndex>& mapTxIndex)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        return false;
    }
    CListTxIndexTrieDBWalker walker(mapTxIndex);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker))
    {
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkTxIndexDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    CBufStream ss;
    ss << DB_TXINDEX_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkTxIndexDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ss;
    ss << DB_TXINDEX_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        StdLog("CForkTxIndexDB", "Read trie root: hashKey: %s, hashBlock: %s",
               hashKey.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkTxIndexDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hcode::CBufStream ssKey;
    bytes btKey, btValue;

    ssKey << DB_TXINDEX_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    hcode::CBufStream ssValue;
    ssValue << hashPrevRoot << hashBlock;
    btValue.assign(ssValue.GetData(), ssValue.GetData() + ssValue.GetSize());

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkTxIndexDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_TXINDEX_KEY_NAME_PREVROOT;
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
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CTxIndexDB

bool CTxIndexDB::Initialize(const boost::filesystem::path& pathData)
{
    pathTxIndex = pathData / "txindex";

    if (!boost::filesystem::exists(pathTxIndex))
    {
        boost::filesystem::create_directories(pathTxIndex);
    }

    if (!boost::filesystem::is_directory(pathTxIndex))
    {
        return false;
    }
    return true;
}

void CTxIndexDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapTxIndexDB.clear();
}

bool CTxIndexDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapTxIndexDB.find(hashFork) != mapTxIndexDB.end());
}

bool CTxIndexDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkTxIndexDB> spTxIndex(new CForkTxIndexDB());
    if (spTxIndex == nullptr)
    {
        return false;
    }
    if (!spTxIndex->Initialize(hashFork, pathTxIndex / hashFork.GetHex()))
    {
        return false;
    }
    mapTxIndexDB.insert(make_pair(hashFork, spTxIndex));
    return true;
}

void CTxIndexDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        (*it).second->RemoveAll();
        mapTxIndexDB.erase(it);
    }

    boost::filesystem::path forkPath = pathTxIndex / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CTxIndexDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CTxIndexDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.begin();
    while (it != mapTxIndexDB.end())
    {
        (*it).second->RemoveAll();
        mapTxIndexDB.erase(it++);
    }
}

bool CTxIndexDB::AddBlockTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->AddBlockTxIndex(hashPrevBlock, hashBlock, mapBlockTxIndex, hashNewRoot);
    }
    StdLog("CTxIndexDB", "Add Block Tx Index: Fork error, fork: %s", hashFork.GetHex().c_str());
    return false;
}

bool CTxIndexDB::RetrieveTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->RetrieveTxIndex(hashBlock, txid, txIndex);
    }
    return false;
}

bool CTxIndexDB::VerifyTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->VerifyTxIndex(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

bool CTxIndexDB::ListAllTxIndex(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CTxIndex>& mapTxIndex)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->ListAllTxIndex(hashBlock, mapTxIndex);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "forkdb.h"

#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

#define DB_FORK_KEY_NAME_CTXT string("ctxt")
#define DB_FORK_KEY_NAME_TRIEROOT string("trieroot")
#define DB_FORK_KEY_NAME_PREVROOT string("prevroot")
#define DB_FORK_KEY_NAME_ACTIVE string("active")
#define DB_FORK_KEY_NAME_FORKNAME string("forkname")
#define DB_FORK_KEY_NAME_FORKSN string("forksn")

//////////////////////////////
// CListForkTrieDBWalker

bool CListForkTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        hcode::StdError("CListForkTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        hcode::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_FORK_KEY_NAME_CTXT)
        {
            hcode::CBufStream ssValue(btValue);
            uint256 hashFork;
            CForkContext ctxtFork;
            ssKey >> hashFork;
            ssValue >> ctxtFork;
            mapForkCtxt.insert(std::make_pair(hashFork, ctxtFork));
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
// CCacheFork

void CCacheFork::AddForkCtxt(const std::map<uint256, CForkContext>& mapForkCtxtIn)
{
    for (const auto& kv : mapForkCtxtIn)
    {
        mapForkContext[kv.first] = kv.second;
        mapForkName[kv.second.strName] = kv.first;
        mapForkSn[CTransaction::GetForkSn(kv.first)] = kv.first;
        mapForkParent.insert(std::make_pair(kv.second.hashParent, kv.first));
        mapForkJoint.insert(std::make_pair(kv.second.hashJoint, kv.first));
    }
}

bool CCacheFork::ExistForkContext(const uint256& hashFork) const
{
    return (mapForkContext.find(hashFork) != mapForkContext.end());
}

uint256 CCacheFork::GetForkIdByName(const std::string& strName) const
{
    auto it = mapForkName.find(strName);
    if (it != mapForkName.end())
    {
        return it->second;
    }
    return 0;
}

uint256 CCacheFork::GetForkIdBySn(const uint16 nForkSn) const
{
    auto it = mapForkSn.find(nForkSn);
    if (it != mapForkSn.end())
    {
        return it->second;
    }
    return 0;
}

//////////////////////////////
// CForkDB

bool CForkDB::Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn)
{
    hashGenesisBlock = hashGenesisBlockIn;
    if (!dbTrie.Initialize(pathData / "fork"))
    {
        StdLog("CForkDB", "Initialize trie fail");
        return false;
    }
    return true;
}

void CForkDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    dbTrie.Deinitialize();
    mapCacheFork.clear();
    mapCacheLast.clear();
    mapCacheRoot.clear();
}

void CForkDB::Clear()
{
    CWriteLock wlock(rwAccess);
    dbTrie.Clear();
    mapCacheFork.clear();
    mapCacheLast.clear();
    mapCacheRoot.clear();
}

bool CForkDB::AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, uint256& hashNewRoot)
{
    CWriteLock wlock(rwAccess);

    uint256 hashPrevRoot;
    if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
    {
        StdLog("CForkDB", "Add fork context: Read trie root fail, pre block: %s", hashPrevBlock.GetHex().c_str());
        return false;
    }

    const CCacheFork* pCacheFork = LoadCacheForkContext(hashPrevBlock);

    std::map<uint256, CForkContext> mapNewForkCtxt;
    bytesmap mapKv;
    for (const auto& kv : mapForkCtxt)
    {
        const uint16 nForkSn = CTransaction::GetForkSn(kv.first);
        if (pCacheFork)
        {
            if (pCacheFork->ExistForkContext(kv.first))
            {
                StdLog("CForkDB", "Add fork context: Fork existed, fork: %s", kv.first.GetHex().c_str());
                continue;
            }
            if (pCacheFork->GetForkIdByName(kv.second.strName) != 0)
            {
                StdLog("CForkDB", "Add fork context: Fork name existed, name: %s, fork: %s", kv.second.strName.c_str(), kv.first.GetHex().c_str());
                continue;
            }
            if (pCacheFork->GetForkIdBySn(nForkSn) != 0)
            {
                StdLog("CForkDB", "Add fork context: Fork sn existed, sn: 0x%4.4x, fork: %s", nForkSn, kv.first.GetHex().c_str());
                continue;
            }
        }

        {
            hcode::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_FORK_KEY_NAME_CTXT << kv.first;
            ssKey.GetData(btKey);

            ssValue << kv.second;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        {
            hcode::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_FORK_KEY_NAME_FORKNAME << kv.second.strName;
            ssKey.GetData(btKey);

            ssValue << kv.first;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        {
            hcode::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_FORK_KEY_NAME_FORKSN << nForkSn;
            ssKey.GetData(btKey);

            ssValue << kv.first;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        mapNewForkCtxt.insert(kv);
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkDB", "Add fork context: Add new trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkDB", "Add fork context: Write trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (!AddCacheForkContext(hashPrevBlock, hashBlock, mapNewForkCtxt))
    {
        StdLog("CForkDB", "Add fork context: Add cache fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkDB::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!GetForkLast(hashGenesisBlock, hashLastBlock))
        {
            hashLastBlock = 0;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    const CCacheFork* ptr = GetCacheForkContext(hashBlock);
    if (ptr)
    {
        mapForkCtxt = ptr->mapForkContext;
        return true;
    }

    if (!ListDbForkContext(hashLastBlock, mapForkCtxt))
    {
        StdLog("CForkDB", "List Fork Context: List db fork fail, block: %s", hashLastBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkDB::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!GetForkLast(hashGenesisBlock, hashLastBlock))
        {
            hashLastBlock = 0;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    const CCacheFork* ptr = GetCacheForkContext(hashBlock);
    if (ptr)
    {
        auto mt = ptr->mapForkContext.find(hashFork);
        if (mt != ptr->mapForkContext.end())
        {
            ctxt = mt->second;
            return true;
        }
    }

    uint256 hashRoot;
    if (!ReadTrieRoot(hashLastBlock, hashRoot))
    {
        return false;
    }

    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_FORK_KEY_NAME_CTXT << hashFork;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxt;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkDB::UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock)
{
    CWriteLock wlock(rwAccess);

    CBufStream ss;
    ss << DB_FORK_KEY_NAME_ACTIVE << hashFork;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashLastBlock.begin(), hashLastBlock.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        StdLog("CForkDB", "Update fork last: Set value node fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    if (hashLastBlock == 0)
    {
        mapCacheLast.erase(hashFork);
    }
    else
    {
        mapCacheLast[hashFork] = hashLastBlock;
    }
    return true;
}

bool CForkDB::RemoveFork(const uint256& hashFork)
{
    return UpdateForkLast(hashFork, uint256());
}

bool CForkDB::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    CReadLock rlock(rwAccess);

    return GetForkLast(hashFork, hashLastBlock);
}

bool CForkDB::GetForkIdByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!GetForkLast(hashGenesisBlock, hashLastBlock))
        {
            hashLastBlock = 0;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    const CCacheFork* ptr = GetCacheForkContext(hashBlock);
    if (ptr)
    {
        hashFork = ptr->GetForkIdByName(strForkName);
        if (hashFork != 0)
        {
            return true;
        }
    }

    uint256 hashRoot;
    if (!ReadTrieRoot(hashLastBlock, hashRoot))
    {
        StdLog("CForkDB", "Get forkid by fork name: Read trie root fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_FORK_KEY_NAME_FORKNAME << strForkName;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashFork;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkDB::GetForkIdByForkSn(const uint16 nForkSn, uint256& hashFork, const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!GetForkLast(hashGenesisBlock, hashLastBlock))
        {
            hashLastBlock = 0;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    const CCacheFork* ptr = GetCacheForkContext(hashBlock);
    if (ptr)
    {
        hashFork = ptr->GetForkIdBySn(nForkSn);
        if (hashFork != 0)
        {
            return true;
        }
    }

    uint256 hashRoot;
    if (!ReadTrieRoot(hashLastBlock, hashRoot))
    {
        StdLog("CForkDB", "Get forkid by fork sn: Read trie root fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_FORK_KEY_NAME_FORKSN << nForkSn;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashFork;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkDB::VerifyForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkDB", "Verify fork context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkDB", "Verify fork context: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkDB", "Verify fork context: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkDB", "Verify fork context: Get prev root fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkDB", "Verify fork context: Verify fail, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, block: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    CBufStream ss;
    ss << DB_FORK_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }

    while (mapCacheRoot.size() >= MAX_CACHE_ROOT_COUNT)
    {
        mapCacheRoot.erase(mapCacheRoot.begin());
    }
    mapCacheRoot[hashBlock] = hashTrieRoot;
    return true;
}

bool CForkDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    auto it = mapCacheRoot.find(hashBlock);
    if (it != mapCacheRoot.end())
    {
        hashTrieRoot = it->second;
        return true;
    }

    CBufStream ss;
    ss << DB_FORK_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);

    while (mapCacheRoot.size() >= MAX_CACHE_ROOT_COUNT)
    {
        mapCacheRoot.erase(mapCacheRoot.begin());
    }
    mapCacheRoot[hashBlock] = hashTrieRoot;
    return true;
}

void CForkDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_FORK_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_FORK_KEY_NAME_PREVROOT;
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

bool CForkDB::GetForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    auto it = mapCacheLast.find(hashFork);
    if (it != mapCacheLast.end())
    {
        hashLastBlock = it->second;
        return true;
    }

    CBufStream ss;
    ss << DB_FORK_KEY_NAME_ACTIVE << hashFork;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashLastBlock = uint256(btValue);

    mapCacheLast[hashFork] = hashLastBlock;
    return true;
}

bool CForkDB::AddCacheForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapNewForkCtxt)
{
    set<uint256> setRemoveFullFork;
    while (mapCacheFork.size() >= MAX_CACHE_CONTEXT_COUNT)
    {
        auto it = mapCacheFork.begin();
        if (it == mapCacheFork.end())
        {
            break;
        }
        if (it->second.hashRef == 0)
        {
            setRemoveFullFork.insert(it->first);
        }
        mapCacheFork.erase(it);
    }
    if (!setRemoveFullFork.empty())
    {
        for (auto it = mapCacheFork.begin(); it != mapCacheFork.end();)
        {
            if (it->second.hashRef != 0 && setRemoveFullFork.count(it->second.hashRef) > 0)
            {
                mapCacheFork.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    bool fAddFull = false;
    if (CBlock::GetBlockHeightByHash(hashBlock) % (MAX_CACHE_CONTEXT_COUNT / 4) == 0)
    {
        fAddFull = true;
    }

    auto it = mapCacheFork.find(hashPrevBlock);
    if (fAddFull || it == mapCacheFork.end())
    {
        std::map<uint256, CForkContext> mapForkCtxt;
        if (!ListDbForkContext(hashPrevBlock, mapForkCtxt))
        {
            StdLog("CForkDB", "Add Cache Fork Context: List db fork context fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
        if (!mapNewForkCtxt.empty())
        {
            for (const auto& kv : mapNewForkCtxt)
            {
                mapForkCtxt[kv.first] = kv.second;
            }
        }
        mapCacheFork[hashBlock] = CCacheFork(mapForkCtxt);
    }
    else
    {
        if (!mapNewForkCtxt.empty())
        {
            std::map<uint256, CForkContext> mapForkCtxt;
            mapForkCtxt = it->second.mapForkContext;
            for (const auto& kv : mapNewForkCtxt)
            {
                mapForkCtxt[kv.first] = kv.second;
            }
            mapCacheFork[hashBlock] = CCacheFork(mapForkCtxt);
        }
        else
        {
            if (it->second.hashRef == 0)
            {
                mapCacheFork[hashBlock] = CCacheFork(hashPrevBlock);
            }
            else
            {
                mapCacheFork[hashBlock] = CCacheFork(it->second.hashRef);
            }
        }
    }
    return true;
}

const CCacheFork* CForkDB::GetCacheForkContext(const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        return nullptr;
    }
    auto it = mapCacheFork.find(hashBlock);
    if (it != mapCacheFork.end())
    {
        if (it->second.hashRef != 0)
        {
            it = mapCacheFork.find(it->second.hashRef);
            if (it == mapCacheFork.end())
            {
                StdLog("CForkDB", "Get Cache Fork Context: Get ref cache fail, block: %s", hashBlock.GetHex().c_str());
                return nullptr;
            }
        }
        return &(it->second);
    }
    return nullptr;
}

const CCacheFork* CForkDB::LoadCacheForkContext(const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        return nullptr;
    }
    const CCacheFork* ptr = GetCacheForkContext(hashBlock);
    if (ptr == nullptr)
    {
        std::map<uint256, CForkContext> mapForkCtxt;
        if (!ListDbForkContext(hashBlock, mapForkCtxt))
        {
            StdLog("CForkDB", "Get Cache Fork Context: List db fork fail, block: %s", hashBlock.GetHex().c_str());
            return nullptr;
        }
        if (mapCacheFork.find(hashBlock) != mapCacheFork.end())
        {
            mapCacheFork.erase(hashBlock);
        }
        auto it = mapCacheFork.insert(make_pair(hashBlock, CCacheFork(mapForkCtxt))).first;
        return &(it->second);
    }
    return ptr;
}

bool CForkDB::ListDbForkContext(const uint256& hashBlock, std::map<uint256, CForkContext>& mapForkCtxt)
{
    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkDB", "List Db Fork Context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hcode::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_FORK_KEY_NAME_CTXT;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListForkTrieDBWalker walker(mapForkCtxt);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkDB", "List Db Fork Context: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "invitedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

#define DB_INVITE_KEY_NAME_DEST string("dest")
#define DB_INVITE_KEY_NAME_TRIEROOT string("trieroot")
#define DB_INVITE_KEY_NAME_PREVROOT string("prevroot")

//////////////////////////////
// CListContractAddressTrieDBWalker

bool CListInviteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListInviteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hcode::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_INVITE_KEY_NAME_DEST)
        {
            CDestination destKey;
            CInviteContext ctxValue;
            hcode::CBufStream ssValue(btValue);
            ssKey >> destKey;
            ssValue >> ctxValue;
            mapInvite.insert(std::make_pair(destKey, ctxValue));
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
// CForkInviteDB

CForkInviteDB::CForkInviteDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkInviteDB::~CForkInviteDB()
{
    dbTrie.Deinitialize();
}

bool CForkInviteDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkInviteDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkInviteDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkInviteDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkInviteDB::AddInviteRelation(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CInviteContext>& mapInviteContext, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkInviteDB", "Add Invite Context: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapInviteContext)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_INVITE_KEY_NAME_DEST << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkInviteDB", "Add Invite Context: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkInviteDB", "Add Invite Context: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkInviteDB::RetrieveInviteParent(const uint256& hashBlock, const CDestination& destSub, CInviteContext& ctxInvite)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkInviteDB", "Retrieve invite parent: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_INVITE_KEY_NAME_DEST << destSub;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        StdLog("CForkInviteDB", "Retrieve invite parent: Trie retrieve kv fail, root: %s, block: %s",
               hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxInvite;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkInviteDB::ListInviteRelation(const uint256& hashBlock, std::map<CDestination, CInviteContext>& mapInviteContext)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkInviteDB", "List ivnite relation: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    CListInviteTrieDBWalker walker(mapInviteContext);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker))
    {
        StdLog("CForkInviteDB", "List ivnite relation: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkInviteDB::CheckInviteContext(const std::vector<uint256>& vCheckBlock)
{
    if (vCheckBlock.size() > 0)
    {
        std::vector<uint256> vCheckRoot;
        vCheckRoot.reserve(vCheckBlock.size());
        for (size_t i = 0; i < vCheckBlock.size(); i++)
        {
            if (!ReadTrieRoot(vCheckBlock[i], vCheckRoot[i]))
            {
                StdLog("CForkInviteDB", "Check invite context: Read trie root fail, block: %s", vCheckBlock[i].GetHex().c_str());
                return false;
            }
        }
        if (!dbTrie.CheckTrie(vCheckRoot))
        {
            StdLog("CForkInviteDB", "Check invite context: Check trie fail");
            return false;
        }
    }
    return true;
}

bool CForkInviteDB::CheckInviteContext(const uint256& hashLastBlock, const uint256& hashLastRoot)
{
    uint256 hashLastRootRead;
    if (!ReadTrieRoot(hashLastBlock, hashLastRootRead))
    {
        return false;
    }
    if (hashLastRootRead != hashLastRoot)
    {
        return false;
    }

    uint256 hashBlockLocal;
    uint256 hashRoot = hashLastRoot;
    std::map<uint256, CTrieValue> mapCacheNode;
    do
    {
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
        uint256 hashRootPrev;
        if (!GetPrevRoot(hashRoot, hashRootPrev, hashBlockLocal))
        {
            return false;
        }
        uint256 hashRootLocal;
        if (!ReadTrieRoot(hashBlockLocal, hashRootLocal))
        {
            return false;
        }
        if (hashRootLocal != hashRoot)
        {
            return false;
        }
        hashRoot = hashRootPrev;
    } while (hashBlockLocal != hashFork);

    return true;
}

bool CForkInviteDB::VerifyInviteContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkInviteDB", "Verify invite context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkInviteDB", "Verify invite context: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkInviteDB", "Verify invite context: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkInviteDB", "Verify invite context: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkInviteDB", "Verify invite context: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkInviteDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hcode::CBufStream ss;
    ss << DB_INVITE_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkInviteDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hcode::CBufStream ss;
    ss << DB_INVITE_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkInviteDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_INVITE_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkInviteDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_INVITE_KEY_NAME_PREVROOT;
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
// CInviteDB

bool CInviteDB::Initialize(const boost::filesystem::path& pathData)
{
    pathStorage = pathData / "invite";

    if (!boost::filesystem::exists(pathStorage))
    {
        boost::filesystem::create_directories(pathStorage);
    }

    if (!boost::filesystem::is_directory(pathStorage))
    {
        return false;
    }
    return true;
}

void CInviteDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapInviteDB.clear();
}

bool CInviteDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapInviteDB.find(hashFork) != mapInviteDB.end());
}

bool CInviteDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkInviteDB> spAddress(new CForkInviteDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathStorage / hashFork.GetHex()))
    {
        return false;
    }
    mapInviteDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CInviteDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        it->second->RemoveAll();
        mapInviteDB.erase(it);
    }

    boost::filesystem::path forkPath = pathStorage / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CInviteDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CInviteDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapInviteDB.begin();
    while (it != mapInviteDB.end())
    {
        it->second->RemoveAll();
        mapInviteDB.erase(it++);
    }
}

bool CInviteDB::AddInviteRelation(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CInviteContext>& mapInviteContext, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->AddInviteRelation(hashPrevBlock, hashBlock, mapInviteContext, hashNewRoot);
    }
    return false;
}

bool CInviteDB::RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CInviteContext& ctxInvite)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->RetrieveInviteParent(hashBlock, destSub, ctxInvite);
    }
    return false;
}

bool CInviteDB::ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CInviteContext>& mapInviteContext)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->ListInviteRelation(hashBlock, mapInviteContext);
    }
    return false;
}

bool CInviteDB::CheckInviteContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->CheckInviteContext(vCheckBlock);
    }
    return false;
}

bool CInviteDB::CheckInviteContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->CheckInviteContext(hashLastBlock, hashLastRoot);
    }
    return false;
}

bool CInviteDB::VerifyInviteContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapInviteDB.find(hashFork);
    if (it != mapInviteDB.end())
    {
        return it->second->VerifyInviteContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

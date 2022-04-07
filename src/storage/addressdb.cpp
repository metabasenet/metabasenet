// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

#define DB_ADDRESS_KEY_NAME_DEST string("dest")
#define DB_ADDRESS_KEY_NAME_TRIEROOT string("trieroot")
#define DB_ADDRESS_KEY_NAME_PREVROOT string("prevroot")

//////////////////////////////
// CListContractAddressTrieDBWalker

bool CListContractAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListContractAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_ADDRESS_KEY_NAME_DEST)
        {
            uint256 dest;
            CAddressContext ctxtAddress;
            hnbase::CBufStream ssValue(btValue);
            ssKey >> dest;
            ssValue >> ctxtAddress;
            if (ctxtAddress.IsContract())
            {
                CContractAddressContext ctxtContract;
                if (!ctxtAddress.GetContractAddressContext(ctxtContract))
                {
                    StdError("CListContractAddressTrieDBWalker", "GetContractAddressContext fail");
                    return false;
                }
                mapContractAddress.insert(std::make_pair(dest, ctxtContract));
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
// CForkAddressDB

CForkAddressDB::CForkAddressDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkAddressDB::~CForkAddressDB()
{
    dbTrie.Deinitialize();
}

bool CForkAddressDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkAddressDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkAddressDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkAddressDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkAddressDB::AddAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CAddressContext>& mapAddress, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressDB", "Add Address Context: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapAddress)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_NAME_DEST << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add Address Context: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkAddressDB", "Add Address Context: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveAddressContext(const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_NAME_DEST << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        StdLog("CForkAddressDB", "Retrieve address context: Trie retrieve kv fail, root: %s, block: %s",
               hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtAddress;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListContractAddress(const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "List contract address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    CListContractAddressTrieDBWalker walker(mapContractAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker))
    {
        StdLog("CForkAddressDB", "List contract address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::CheckAddressContext(const std::vector<uint256>& vCheckBlock)
{
    if (vCheckBlock.size() > 0)
    {
        std::vector<uint256> vCheckRoot;
        vCheckRoot.reserve(vCheckBlock.size());
        for (size_t i = 0; i < vCheckBlock.size(); i++)
        {
            if (!ReadTrieRoot(vCheckBlock[i], vCheckRoot[i]))
            {
                StdLog("CForkAddressDB", "Check address context: Read trie root fail, block: %s", vCheckBlock[i].GetHex().c_str());
                return false;
            }
        }
        if (!dbTrie.CheckTrie(vCheckRoot))
        {
            StdLog("CForkAddressDB", "Check address context: Check trie fail");
            return false;
        }
    }
    return true;
}

bool CForkAddressDB::CheckAddressContext(const uint256& hashLastBlock, const uint256& hashLastRoot)
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

bool CForkAddressDB::VerifyAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Verify address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkAddressDB", "Verify address context: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressDB", "Verify address context: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkAddressDB", "Verify address context: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkAddressDB", "Verify address context: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkAddressDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ss;
    ss << DB_ADDRESS_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkAddressDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ss;
    ss << DB_ADDRESS_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkAddressDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_NAME_PREVROOT;
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
// CAddressDB

bool CAddressDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "address";

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

void CAddressDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapAddressDB.clear();
}

bool CAddressDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapAddressDB.find(hashFork) != mapAddressDB.end());
}

bool CAddressDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkAddressDB> spAddress(new CForkAddressDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapAddressDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CAddressDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        it->second->RemoveAll();
        mapAddressDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CAddressDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CAddressDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressDB.begin();
    while (it != mapAddressDB.end())
    {
        it->second->RemoveAll();
        mapAddressDB.erase(it++);
    }
}

bool CAddressDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CAddressContext>& mapAddress, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->AddAddressContext(hashPrevBlock, hashBlock, mapAddress, hashNewRoot);
    }
    return false;
}

bool CAddressDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveAddressContext(hashBlock, dest, ctxtAddress);
    }
    return false;
}

bool CAddressDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->ListContractAddress(hashBlock, mapContractAddress);
    }
    return false;
}

bool CAddressDB::CheckAddressContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->CheckAddressContext(vCheckBlock);
    }
    return false;
}

bool CAddressDB::CheckAddressContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->CheckAddressContext(hashLastBlock, hashLastRoot);
    }
    return false;
}

bool CAddressDB::VerifyAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->VerifyAddressContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

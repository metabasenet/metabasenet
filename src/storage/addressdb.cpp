// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

const uint8 DB_ADDRESS_KEY_TYPE_ADDRESS = 0x10;
const uint8 DB_ADDRESS_KEY_TYPE_TIMEVAULT = 0x20;
const uint8 DB_ADDRESS_KEY_TYPE_TRIEROOT = 0x30;
const uint8 DB_ADDRESS_KEY_TYPE_PREVROOT = 0x40;
const uint8 DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT = 0x50;
const uint8 DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS = 0x60;
const uint8 DB_ADDRESS_KEY_TYPE_BLSPUBKEY = 0x70;

#define DB_ADDRESS_KEY_ID_PREVROOT string("prevroot")

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
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_ADDRESS)
        {
            CDestination dest;
            CAddressContext ctxAddress;
            mtbase::CBufStream ssValue(btValue);
            ssKey >> dest;
            ssValue >> ctxAddress;
            if (ctxAddress.IsContract())
            {
                CContractAddressContext ctxtContract;
                if (!ctxAddress.GetContractAddressContext(ctxtContract))
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CListFunctionAddressTrieDBWalker

bool CListFunctionAddressTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListFunctionAddressTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS)
        {
            uint32 nFuncId;
            CFunctionAddressContext ctxFuncAddress;
            mtbase::CBufStream ssValue(btValue);
            ssKey >> nFuncId;
            ssValue >> ctxFuncAddress;
            mapFunctionAddress.insert(std::make_pair(nFuncId, ctxFuncAddress));
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

bool CForkAddressDB::AddAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                       const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot)
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
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapTimeVault)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_TIMEVAULT << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapFunctionAddress)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapBlsPubkeyContext)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_ADDRESS_KEY_TYPE_BLSPUBKEY << kv.first;
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

    uint64 nPrevAddressCount = 0;
    if (hashBlock != hashFork)
    {
        uint64 nPrevNewAddressCount = 0;
        if (!GetAddressCount(hashPrevBlock, nPrevAddressCount, nPrevNewAddressCount))
        {
            StdLog("CForkAddressDB", "Add Address Context: Get prev address count fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    if (!AddAddressCount(hashBlock, nPrevAddressCount + nNewAddressCount, nNewAddressCount))
    {
        StdLog("CForkAddressDB", "Add Address Context: Add address count fail, hashPrevBlock: %s, hashBlock: %s",
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

bool CForkAddressDB::RetrieveAddressContext(const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve address context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESS << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        // StdLog("CForkAddressDB", "Retrieve address context: Trie retrieve kv fail, root: %s, dest: %s, block: %s",
        //        hashRoot.GetHex().c_str(), dest.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxAddress;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "List contract address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_ADDRESS;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListContractAddressTrieDBWalker walker(mapContractAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "List contract address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveTimeVault(const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve time vault: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TIMEVAULT << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> tv;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::GetAddressCount(const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    if (hashBlock == 0)
    {
        nAddressCount = 0;
        return true;
    }

    mtbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> nAddressCount >> nNewAddressCount;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "List function address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListFunctionAddressTrieDBWalker walker(mapFunctionAddress);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkAddressDB", "List function address: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve function address: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_FUNCTION_ADDRESS << nFuncId;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxFuncAddress;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressDB::RetrieveBlsPubkeyContext(const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressDB", "Retrieve bls pubkey context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_BLSPUBKEY << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> blsPubkey;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
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
    mtbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkAddressDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    mtbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CForkAddressDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_KEY_TYPE_PREVROOT << DB_ADDRESS_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_KEY_TYPE_PREVROOT << DB_ADDRESS_KEY_ID_PREVROOT;
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

bool CForkAddressDB::AddAddressCount(const uint256& hashBlock, const uint64 nAddressCount, const uint64 nNewAddressCount)
{
    mtbase::CBufStream ssKey, ssValue;
    ssKey << DB_ADDRESS_KEY_TYPE_ADDRESSCOUNT << hashBlock;
    ssValue << nAddressCount << nNewAddressCount;
    return dbTrie.WriteExtKv(ssKey, ssValue);
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

bool CAddressDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                   const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->AddAddressContext(hashPrevBlock, hashBlock, mapAddress, nNewAddressCount, mapTimeVault, mapFunctionAddress, mapBlsPubkeyContext, hashNewRoot);
    }
    return false;
}

bool CAddressDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveAddressContext(hashBlock, dest, ctxAddress);
    }
    return false;
}

bool CAddressDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->ListContractAddress(hashBlock, mapContractAddress);
    }
    return false;
}

bool CAddressDB::RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveTimeVault(hashBlock, dest, tv);
    }
    return false;
}

bool CAddressDB::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->GetAddressCount(hashBlock, nAddressCount, nNewAddressCount);
    }
    return false;
}

bool CAddressDB::ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->ListFunctionAddress(hashBlock, mapFunctionAddress);
    }
    return false;
}

bool CAddressDB::RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveFunctionAddress(hashBlock, nFuncId, ctxFuncAddress);
    }
    return false;
}

bool CAddressDB::RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return it->second->RetrieveBlsPubkeyContext(hashBlock, dest, blsPubkey);
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

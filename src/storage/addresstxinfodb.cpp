// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addresstxinfodb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

#define DB_ADDRESS_TXINFO_KEY_NAME_DEST string("dest")
#define DB_ADDRESS_TXINFO_KEY_NAME_TRIEROOT string("trieroot")
#define DB_ADDRESS_TXINFO_KEY_NAME_PREVROOT string("prevroot")
#define DB_ADDRESS_TXINFO_KEY_NAME_LASTDEST string("lastdest")

//////////////////////////////
// CListAddressTxInfoTrieDBWalker

bool CListAddressTxInfoTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListAddressTxInfoTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        string strName;
        CDestination dest;
        uint64 nTxIndex;
        hnbase::CBufStream ssKey(btKey);
        ssKey >> strName >> dest >> nTxIndex;

        nTxIndex = BSwap64(nTxIndex);

        CDestTxInfo ctxtAddressTxInfo;
        hnbase::CBufStream ssValue(btValue);
        ssValue >> ctxtAddressTxInfo;
        ctxtAddressTxInfo.nTxIndex = nTxIndex;

        vAddressTxInfo.push_back(ctxtAddressTxInfo);
        if (nGetTxCount > 0 && vAddressTxInfo.size() >= nGetTxCount)
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
// CForkAddressTxInfoDB

CForkAddressTxInfoDB::CForkAddressTxInfoDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkAddressTxInfoDB::~CForkAddressTxInfoDB()
{
    dbTrie.Deinitialize();
}

bool CForkAddressTxInfoDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkAddressTxInfoDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkAddressTxInfoDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkAddressTxInfoDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkAddressTxInfoDB::AddAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressTxInfoDB", "Add address tx info: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapAddressTxInfo)
    {
        const CDestination& dest = kv.first;

        uint64 nTxCount = 0;
        if (!ReadAddressLast(hashPrevRoot, dest, nTxCount))
        {
            nTxCount = 0;
        }

        for (const CDestTxInfo& dti : kv.second)
        {
            hnbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_ADDRESS_TXINFO_KEY_NAME_DEST << dest << BSwap64(nTxCount);
            ssKey.GetData(btKey);

            ssValue << dti;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
            nTxCount++;
        }

        WriteAddressLast(dest, nTxCount, mapKv);
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Add address tx info: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashNewRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Add address tx info: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::GetAddressTxCount(const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Get address last info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!ReadAddressLast(hashRoot, dest, nTxCount))
    {
        StdLog("CForkAddressTxInfoDB", "Get address last info: Read address last fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::RetrieveAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Retrieve address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_NAME_DEST << dest << BSwap64(nTxIndex);
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        StdLog("CForkAddressTxInfoDB", "Retrieve address tx info: Trie retrieve kv fail, root: %s, block: %s",
               hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtAddressTxInfo;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::ListAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "List address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hnbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_ADDRESS_TXINFO_KEY_NAME_DEST << dest;
    bytes btKeyPrefix;
    ssKeyPrefix.GetData(btKeyPrefix);

    bytes btBeginKeyTail;
    if (nBeginTxIndex > 0)
    {
        hnbase::CBufStream ss;
        ss << BSwap64(nBeginTxIndex);
        ss.GetData(btBeginKeyTail);
    }

    CListAddressTxInfoTrieDBWalker walker(nGetTxCount, vAddressTxInfo);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix, btBeginKeyTail, fReverse))
    {
        StdLog("CForkAddressTxInfoDB", "List address tx info: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::VerifyAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CForkAddressTxInfoDB", "Verify address tx info: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkAddressTxInfoDB", "Verify address tx info: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CForkAddressTxInfoDB", "Verify address tx info: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CForkAddressTxInfoDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hnbase::CBufStream ss;
    ss << DB_ADDRESS_TXINFO_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkAddressTxInfoDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hnbase::CBufStream ss;
    ss << DB_ADDRESS_TXINFO_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkAddressTxInfoDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_TXINFO_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressTxInfoDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_NAME_PREVROOT;
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

void CForkAddressTxInfoDB::WriteAddressLast(const CDestination& dest, const uint64 nTxCount, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_ADDRESS_TXINFO_KEY_NAME_LASTDEST << dest;
    ssKey.GetData(btKey);

    ssValue << nTxCount;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkAddressTxInfoDB::ReadAddressLast(const uint256& hashRoot, const CDestination& dest, uint64& nTxCount)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_ADDRESS_TXINFO_KEY_NAME_LASTDEST << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nTxCount;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CAddressTxInfoDB

bool CAddressTxInfoDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "addresstxinfo";

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

void CAddressTxInfoDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapAddressTxInfoDB.clear();
}

bool CAddressTxInfoDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapAddressTxInfoDB.find(hashFork) != mapAddressTxInfoDB.end());
}

bool CAddressTxInfoDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkAddressTxInfoDB> spAddress(new CForkAddressTxInfoDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapAddressTxInfoDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CAddressTxInfoDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        it->second->RemoveAll();
        mapAddressTxInfoDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CAddressTxInfoDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CAddressTxInfoDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapAddressTxInfoDB.begin();
    while (it != mapAddressTxInfoDB.end())
    {
        it->second->RemoveAll();
        mapAddressTxInfoDB.erase(it++);
    }
}

bool CAddressTxInfoDB::AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->AddAddressTxInfo(hashPrevBlock, hashBlock, nBlockNumber, mapAddressTxInfo, hashNewRoot);
    }
    return false;
}

bool CAddressTxInfoDB::GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->GetAddressTxCount(hashBlock, dest, nTxCount);
    }
    return false;
}

bool CAddressTxInfoDB::RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->RetrieveAddressTxInfo(hashBlock, dest, nTxIndex, ctxtAddressTxInfo);
    }
    return false;
}

bool CAddressTxInfoDB::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->ListAddressTxInfo(hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
    }
    return false;
}

bool CAddressTxInfoDB::VerifyAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapAddressTxInfoDB.find(hashFork);
    if (it != mapAddressTxInfoDB.end())
    {
        return it->second->VerifyAddressTxInfo(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

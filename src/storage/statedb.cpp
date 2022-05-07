// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "statedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CListStateTrieDBWalker

bool CListStateTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        hcode::StdError("CListStateTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }
    try
    {
        hcode::CBufStream ssKey, ssValue;
        ssKey.Write((char*)(btKey.data()), btKey.size());
        ssValue.Write((char*)(btValue.data()), btValue.size());
        CDestination dest;
        CDestState state;
        ssKey >> dest;
        ssValue >> state;
        mapState.insert(std::make_pair(dest, state));
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CForkStateDB

CForkStateDB::CForkStateDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkStateDB::~CForkStateDB()
{
    mapCacheState.clear();
    dbTrie.Deinitialize();
}

bool CForkStateDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        return false;
    }
    return true;
}

void CForkStateDB::Deinitialize()
{
    mapCacheState.clear();
    dbTrie.Deinitialize();
}

bool CForkStateDB::RemoveAll()
{
    mapCacheState.clear();
    dbTrie.RemoveAll();
    return true;
}

bool CForkStateDB::AddBlockState(const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    if (hashBlockRoot != 0)
    {
        auto it = mapCacheTrie.find(hashBlockRoot);
        if (it != mapCacheTrie.end())
        {
            if (!dbTrie.SaveCacheTrie(it->second))
            {
                return false;
            }
            mapCacheTrie.erase(it);
            return true;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapBlockState)
    {
        hcode::CBufStream ssKey;
        bytes btKey, btValue;

        ssKey << kv.first;
        ssKey.GetData(btKey);

        hcode::CBufStream ssValue;
        ssValue << kv.second;
        btValue.assign(ssValue.GetData(), ssValue.GetData() + ssValue.GetSize());

        mapKv.insert(make_pair(btKey, btValue));
    }

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashBlockRoot))
    {
        return false;
    }
    return true;
}

bool CForkStateDB::CreateCacheStateTrie(const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    bytesmap mapKv;
    for (const auto& kv : mapBlockState)
    {
        hcode::CBufStream ssKey;
        bytes btKey, btValue;

        ssKey << kv.first;
        ssKey.GetData(btKey);

        hcode::CBufStream ssValue;
        ssValue << kv.second;
        btValue.assign(ssValue.GetData(), ssValue.GetData() + ssValue.GetSize());

        mapKv.insert(make_pair(btKey, btValue));
    }

    std::map<uint256, CTrieValue> mapCacheNode;
    if (!dbTrie.CreateCacheTrie(hashPrevRoot, mapKv, hashBlockRoot, mapCacheNode))
    {
        return false;
    }

    auto it = mapCacheTrie.find(hashBlockRoot);
    if (it == mapCacheTrie.end())
    {
        mapCacheTrie.insert(make_pair(hashBlockRoot, mapCacheNode));
    }
    else
    {
        it->second = mapCacheNode;
    }
    return true;
}

bool CForkStateDB::RetrieveDestState(const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << dest;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashBlockRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> state;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkStateDB::ListDestState(const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState)
{
    CListStateTrieDBWalker walker(mapBlockState);
    if (!dbTrie.WalkThroughTrie(hashBlockRoot, walker))
    {
        return false;
    }
    return true;
}

bool CForkStateDB::VerifyState(const uint256& hashRoot, const bool fVerifyAllNode)
{
    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }
    else
    {
        if (!dbTrie.VerifyTrieRootNode(hashRoot))
        {
            return false;
        }
    }
    return true;
}

//////////////////////////////
// CStateDB

bool CStateDB::Initialize(const boost::filesystem::path& pathData)
{
    pathState = pathData / "state";

    if (!boost::filesystem::exists(pathState))
    {
        boost::filesystem::create_directories(pathState);
    }

    if (!boost::filesystem::is_directory(pathState))
    {
        return false;
    }
    return true;
}

void CStateDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapStateDB.clear();
}

bool CStateDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapStateDB.find(hashFork) != mapStateDB.end());
}

bool CStateDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkStateDB> spState(new CForkStateDB());
    if (spState == nullptr)
    {
        return false;
    }
    if (!spState->Initialize(pathState / hashFork.GetHex()))
    {
        return false;
    }
    mapStateDB.insert(make_pair(hashFork, spState));
    return true;
}

void CStateDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        (*it).second->RemoveAll();
        mapStateDB.erase(it);
    }

    boost::filesystem::path forkPath = pathState / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CStateDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CStateDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapStateDB.begin();
    while (it != mapStateDB.end())
    {
        (*it).second->RemoveAll();
        mapStateDB.erase(it++);
    }
}

bool CStateDB::AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    CReadLock rlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return it->second->AddBlockState(hashPrevRoot, hashBlockRoot, mapBlockState);
    }
    return false;
}

bool CStateDB::CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    CReadLock rlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return it->second->CreateCacheStateTrie(hashPrevRoot, hashBlockRoot, mapBlockState);
    }
    return false;
}

bool CStateDB::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    CReadLock rlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return it->second->RetrieveDestState(hashBlockRoot, dest, state);
    }
    return false;
}

bool CStateDB::ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState)
{
    CReadLock rlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return it->second->ListDestState(hashBlockRoot, mapBlockState);
    }
    return false;
}

bool CStateDB::VerifyState(const uint256& hashFork, const uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapStateDB.find(hashFork);
    if (it != mapStateDB.end())
    {
        return it->second->VerifyState(hashRoot, fVerifyAllNode);
    }
    return false;
}

bool CStateDB::CreateStaticStateRoot(const std::map<CDestination, CDestState>& mapBlockState, uint256& hashStateRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapBlockState)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    CTrieDB dbTrieTemp;
    std::map<uint256, CTrieValue> mapCacheNode;
    return dbTrieTemp.CreateCacheTrie(uint256(), mapKv, hashStateRoot, mapCacheNode);
}

} // namespace storage
} // namespace metabasenet

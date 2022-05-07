// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmdb.h"

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
// CForkWasmDB

CForkWasmDB::CForkWasmDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkWasmDB::~CForkWasmDB()
{
    dbTrie.Deinitialize();
}

bool CForkWasmDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        return false;
    }
    return true;
}

void CForkWasmDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkWasmDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkWasmDB::AddBlockWasmState(const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<uint256, bytes>& mapWasmState)
{
    bytesmap mapKv;
    for (const auto& kv : mapWasmState)
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

bool CForkWasmDB::RetrieveWasmState(const uint256& hashWasmRoot, const uint256& key, bytes& value)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << key;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashWasmRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> value;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CWasmDB

bool CWasmDB::Initialize(const boost::filesystem::path& pathData)
{
    pathWasm = pathData / "wasm";

    if (!boost::filesystem::exists(pathWasm))
    {
        boost::filesystem::create_directories(pathWasm);
    }

    if (!boost::filesystem::is_directory(pathWasm))
    {
        return false;
    }
    return true;
}

void CWasmDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapWasmDB.clear();
}

bool CWasmDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapWasmDB.find(hashFork) != mapWasmDB.end());
}

bool CWasmDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapWasmDB.find(hashFork);
    if (it != mapWasmDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkWasmDB> spWasm(new CForkWasmDB());
    if (spWasm == nullptr)
    {
        return false;
    }
    if (!spWasm->Initialize(pathWasm / hashFork.GetHex()))
    {
        return false;
    }
    mapWasmDB.insert(make_pair(hashFork, spWasm));
    return true;
}

void CWasmDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapWasmDB.find(hashFork);
    if (it != mapWasmDB.end())
    {
        (*it).second->RemoveAll();
        mapWasmDB.erase(it);
    }

    boost::filesystem::path forkPath = pathWasm / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CWasmDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CWasmDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapWasmDB.begin();
    while (it != mapWasmDB.end())
    {
        (*it).second->RemoveAll();
        mapWasmDB.erase(it++);
    }
}

bool CWasmDB::AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState)
{
    CReadLock rlock(rwAccess);

    auto it = mapWasmDB.find(hashFork);
    if (it != mapWasmDB.end())
    {
        return it->second->AddBlockWasmState(hashPrevRoot, hashWasmRoot, mapWasmState);
    }
    return false;
}

bool CWasmDB::RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value)
{
    CReadLock rlock(rwAccess);

    auto it = mapWasmDB.find(hashFork);
    if (it != mapWasmDB.end())
    {
        return it->second->RetrieveWasmState(hashWasmRoot, key, value);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "codedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

#define DB_CODE_KEY_NAME_SOURCE_CODE string("sourcecode")
#define DB_CODE_KEY_NAME_WASM_CREATE_CODE string("createcode")
#define DB_CODE_KEY_NAME_WASM_RUN_CODE string("runcode")
#define DB_CODE_KEY_NAME_PREV_ROOT string("prevroot")

//////////////////////////////
// CListCreateCodeTrieDBWalker

bool CListCreateCodeTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListCreateCodeTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hcode::CBufStream ssKey(btKey);
        string strName;
        ssKey >> strName;
        if (strName == DB_CODE_KEY_NAME_WASM_CREATE_CODE)
        {
            uint256 hashCreateCode;
            CWasmCreateCodeContext ctxtCreateCode;
            hcode::CBufStream ssValue(btValue);
            ssKey >> hashCreateCode;
            ssValue >> ctxtCreateCode;
            mapWasmCreateCode.insert(std::make_pair(hashCreateCode, ctxtCreateCode));
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
// CForkCodeDB

CForkCodeDB::CForkCodeDB(const bool fCacheIn)
{
    fCache = fCacheIn;
}

CForkCodeDB::~CForkCodeDB()
{
    dbTrie.Deinitialize();
}

bool CForkCodeDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        StdLog("CForkCodeDB", "Initialize: Initialize fail, fork: %s", hashForkIn.GetHex().c_str());
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkCodeDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkCodeDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkCodeDB::AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                                 const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                                 const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                                 const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                                 uint256& hashCodeRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkCodeDB", "Add Code Context: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapSourceCode)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CODE_KEY_NAME_SOURCE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapWasmCreateCode)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CODE_KEY_NAME_WASM_CREATE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapWasmRunCode)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CODE_KEY_NAME_WASM_RUN_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashCodeRoot))
    {
        StdLog("CForkCodeDB", "Add Code Context: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashCodeRoot))
    {
        StdLog("CForkCodeDB", "Add Code Context: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkCodeDB::RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(hashBlock, hashCodeRoot))
    {
        return false;
    }
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CODE_KEY_NAME_SOURCE_CODE << hashSourceCode;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashCodeRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtCode;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkCodeDB::RetrieveWasmCreateCodeContext(const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(hashBlock, hashCodeRoot))
    {
        return false;
    }
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CODE_KEY_NAME_WASM_CREATE_CODE << hashWasmCreateCode;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashCodeRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtCode;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkCodeDB::RetrieveWasmRunCodeContext(const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(hashBlock, hashCodeRoot))
    {
        return false;
    }
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CODE_KEY_NAME_WASM_RUN_CODE << hashWasmRunCode;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashCodeRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtCode;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkCodeDB::ListWasmCreateCodeContext(const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CForkCodeDB", "List wasm create code: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    hcode::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_CODE_KEY_NAME_WASM_CREATE_CODE;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListCreateCodeTrieDBWalker walker(mapWasmCreateCode);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkCodeDB", "List wasm create code: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkCodeDB::VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
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

///////////////////////////////////
bool CForkCodeDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    hcode::CBufStream ss;
    ss << string("trieroot") << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue(hashTrieRoot.begin(), hashTrieRoot.end());
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CForkCodeDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    hcode::CBufStream ss;
    ss << string("trieroot") << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    hashTrieRoot = uint256(btValue);
    return true;
}

void CForkCodeDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_CODE_KEY_NAME_PREV_ROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkCodeDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CODE_KEY_NAME_PREV_ROOT;
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
// CCodeDB

bool CCodeDB::Initialize(const boost::filesystem::path& pathData)
{
    pathAddress = pathData / "code";

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

void CCodeDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapCodeDB.clear();
}

bool CCodeDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapCodeDB.find(hashFork) != mapCodeDB.end());
}

bool CCodeDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkCodeDB> spAddress(new CForkCodeDB());
    if (spAddress == nullptr)
    {
        return false;
    }
    if (!spAddress->Initialize(hashFork, pathAddress / hashFork.GetHex()))
    {
        return false;
    }
    mapCodeDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CCodeDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        it->second->RemoveAll();
        mapCodeDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CCodeDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CCodeDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapCodeDB.begin();
    while (it != mapCodeDB.end())
    {
        it->second->RemoveAll();
        mapCodeDB.erase(it++);
    }
}

bool CCodeDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                             const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                             const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                             const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                             uint256& hashCodeRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->AddCodeContext(hashPrevBlock, hashBlock, mapSourceCode, mapWasmCreateCode, mapWasmRunCode, hashCodeRoot);
    }
    return false;
}

bool CCodeDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->RetrieveSourceCodeContext(hashBlock, hashSourceCode, ctxtCode);
    }
    return false;
}

bool CCodeDB::RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->RetrieveWasmCreateCodeContext(hashBlock, hashWasmCreateCode, ctxtCode);
    }
    return false;
}

bool CCodeDB::RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->RetrieveWasmRunCodeContext(hashBlock, hashWasmRunCode, ctxtCode);
    }
    return false;
}

bool CCodeDB::ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->ListWasmCreateCodeContext(hashBlock, mapWasmCreateCode);
    }
    return false;
}

bool CCodeDB::VerifyCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapCodeDB.find(hashFork);
    if (it != mapCodeDB.end())
    {
        return it->second->VerifyCodeContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

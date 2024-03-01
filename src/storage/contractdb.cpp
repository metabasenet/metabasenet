// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

const string DB_CONTRACT_KEY_ID_TRIEROOT("trieroot");
const string DB_CONTRACT_KEY_ID_PREVROOT("prevroot");

const uint8 DB_CONTRACT_ROOT_TYPE_CONTRACTKV = 0x10;
const uint8 DB_CONTRACT_ROOT_TYPE_CODE = 0x20;

const uint8 DB_CONTRACT_KEY_TYPE_CONTRACTKV = DB_CONTRACT_ROOT_TYPE_CONTRACTKV | 0x01;

const uint8 DB_CONTRACT_KEY_TYPE_SOURCE_CODE = DB_CONTRACT_ROOT_TYPE_CODE | 0x01;
const uint8 DB_CONTRACT_KEY_TYPE_CONTRACT_CREATE_CODE = DB_CONTRACT_ROOT_TYPE_CODE | 0x02;
const uint8 DB_CONTRACT_KEY_TYPE_CONTRACT_RUN_CODE = DB_CONTRACT_ROOT_TYPE_CODE | 0x03;
const uint8 DB_CONTRACT_KEY_TYPE_CONTRACT_TEMPLATE_DATA = DB_CONTRACT_ROOT_TYPE_CODE | 0x04;

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
        mtbase::CBufStream ssKey(btKey);
        uint8 nKeyType;
        ssKey >> nKeyType;
        if (nKeyType == DB_CONTRACT_KEY_TYPE_CONTRACT_CREATE_CODE)
        {
            uint256 hashCreateCode;
            CContractCreateCodeContext ctxtCreateCode;
            mtbase::CBufStream ssValue(btValue);
            ssKey >> hashCreateCode;
            ssValue >> ctxtCreateCode;
            mapContractCreateCode.insert(std::make_pair(hashCreateCode, ctxtCreateCode));
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
// CForkContractDB

CForkContractDB::CForkContractDB(const uint256& hashForkIn)
  : hashFork(hashForkIn)
{
}

CForkContractDB::~CForkContractDB()
{
    dbTrie.Deinitialize();
}

bool CForkContractDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData))
    {
        return false;
    }
    return true;
}

void CForkContractDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CForkContractDB::RemoveAll()
{
    dbTrie.RemoveAll();
    return true;
}

bool CForkContractDB::AddBlockContractKvValue(const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<uint256, bytes>& mapContractState)
{
    bytesmap mapKv;
    for (const auto& kv : mapContractState)
    {
        mtbase::CBufStream ssKey;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << kv.first;
        ssKey.GetData(btKey);

        mtbase::CBufStream ssValue;
        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashBlockRoot))
    {
        return false;
    }
    return true;
}

bool CForkContractDB::RetrieveContractKvValue(const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << key;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashContractRoot, btKey, btValue))
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// contract code

bool CForkContractDB::AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                                     const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                                     const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                                     const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                                     const std::map<uint256, CTemplateContext>& mapTemplateData,
                                     uint256& hashCodeRoot)
{
    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CForkContractDB", "Add Code Context: Read trie root fail, hashPrevBlock: %s, hashBlock: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapSourceCode)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_SOURCE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapContractCreateCode)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACT_CREATE_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapContractRunCode)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACT_RUN_CODE << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    for (const auto& kv : mapTemplateData)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACT_TEMPLATE_DATA << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashCodeRoot))
    {
        StdLog("CForkContractDB", "Add Code Context: Add new trie fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        StdLog("CForkContractDB", "Add Code Context: Write trie root fail, hashPrevBlock: %s, hashBlock: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkContractDB::RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CONTRACT_KEY_TYPE_SOURCE_CODE << hashSourceCode;
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkContractDB::RetrieveContractCreateCodeContext(const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CONTRACT_KEY_TYPE_CONTRACT_CREATE_CODE << hashContractCreateCode;
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkContractDB::RetrieveContractRunCodeContext(const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    uint256 hashCodeRoot;
    if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashCodeRoot))
    {
        return false;
    }
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_CONTRACT_KEY_TYPE_CONTRACT_RUN_CODE << hashContractRunCode;
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkContractDB::ListContractCreateCodeContext(const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashRoot))
    {
        StdLog("CForkContractDB", "List wasm create code: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_CONTRACT_KEY_TYPE_CONTRACT_CREATE_CODE;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListCreateCodeTrieDBWalker walker(mapContractCreateCode);
    if (!dbTrie.WalkThroughTrie(hashRoot, walker, btKeyPrefix))
    {
        StdLog("CForkContractDB", "List wasm create code: Walk through trie fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CForkContractDB::VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashBlock, hashRoot))
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
        if (!ReadTrieRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashPrevBlock, hashPrevRoot))
        {
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_CONTRACT_ROOT_TYPE_CODE, hashRoot, hashRootPrevDb, hashBlockLocalDb))
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
bool CForkContractDB::WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot)
{
    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_CONTRACT_KEY_ID_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CForkContractDB::ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_CONTRACT_KEY_ID_TRIEROOT << hashBlock;
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

void CForkContractDB::AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << nRootType << DB_CONTRACT_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CForkContractDB::GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << nRootType << DB_CONTRACT_KEY_ID_PREVROOT;
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

//////////////////////////////
// CContractDB

bool CContractDB::Initialize(const boost::filesystem::path& pathData)
{
    pathContract = pathData / "contract";

    if (!boost::filesystem::exists(pathContract))
    {
        boost::filesystem::create_directories(pathContract);
    }

    if (!boost::filesystem::is_directory(pathContract))
    {
        return false;
    }
    return true;
}

void CContractDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapContractDB.clear();
}

bool CContractDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapContractDB.find(hashFork) != mapContractDB.end());
}

bool CContractDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkContractDB> spWasm(new CForkContractDB(hashFork));
    if (spWasm == nullptr)
    {
        return false;
    }
    if (!spWasm->Initialize(pathContract / hashFork.GetHex()))
    {
        return false;
    }
    mapContractDB.insert(make_pair(hashFork, spWasm));
    return true;
}

void CContractDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        (*it).second->RemoveAll();
        mapContractDB.erase(it);
    }

    boost::filesystem::path forkPath = pathContract / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CContractDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CContractDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapContractDB.begin();
    while (it != mapContractDB.end())
    {
        (*it).second->RemoveAll();
        mapContractDB.erase(it++);
    }
}

bool CContractDB::AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->AddBlockContractKvValue(hashPrevRoot, hashContractRoot, mapContractState);
    }
    return false;
}

bool CContractDB::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractKvValue(hashContractRoot, key, value);
    }
    return false;
}

bool CContractDB::CreateStaticContractStateRoot(const std::map<uint256, bytes>& mapContractState, uint256& hashStateRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapContractState)
    {
        mtbase::CBufStream ssKey;
        bytes btKey, btValue;

        ssKey << DB_CONTRACT_KEY_TYPE_CONTRACTKV << kv.first;
        ssKey.GetData(btKey);

        mtbase::CBufStream ssValue;
        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    CTrieDB dbTrieTemp;
    std::map<uint256, CTrieValue> mapCacheNode;
    return dbTrieTemp.CreateCacheTrie(uint256(), mapKv, hashStateRoot, mapCacheNode);
}

////////////////////////////////////////////
// contract code

bool CContractDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                                 const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                                 const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                                 const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                                 const std::map<uint256, CTemplateContext>& mapTemplateData,
                                 uint256& hashCodeRoot)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->AddCodeContext(hashPrevBlock, hashBlock, mapSourceCode, mapContractCreateCode, mapContractRunCode, mapTemplateData, hashCodeRoot);
    }
    return false;
}

bool CContractDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveSourceCodeContext(hashBlock, hashSourceCode, ctxtCode);
    }
    return false;
}

bool CContractDB::RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractCreateCodeContext(hashBlock, hashContractCreateCode, ctxtCode);
    }
    return false;
}

bool CContractDB::RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->RetrieveContractRunCodeContext(hashBlock, hashContractRunCode, ctxtCode);
    }
    return false;
}

bool CContractDB::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->ListContractCreateCodeContext(hashBlock, mapContractCreateCode);
    }
    return false;
}

bool CContractDB::VerifyCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapContractDB.find(hashFork);
    if (it != mapContractDB.end())
    {
        return it->second->VerifyCodeContext(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

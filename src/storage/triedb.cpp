// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "triedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//#define TEST_FLAG
//#define TEST_STAT

const uint8 TDB_KEY_TYPE_TRIE_KEY = 0x10;
const uint8 TDB_KEY_TYPE_EXT_KEY = 0x20;

//////////////////////////////
// CKeyNibble

void CKeyNibble::Byte2Nibble(const bytes& bt, const uint8 flag, bytes& nibble)
{
    for (std::size_t i = 0; i < bt.size(); i++)
    {
        const uint8& d = bt[i];
        nibble.push_back(d >> 4);
        if ((i < bt.size() - 1) || (flag == 0))
        {
            nibble.push_back(d & 0x0F);
        }
    }
}

uint8 CKeyNibble::Nibble2Byte(const bytes& nibble, bytes& bt)
{
    uint8 x = 0;
    int n = 0;
    for (const uint8& d : nibble)
    {
        if (n == 0)
        {
            x |= (d << 4);
        }
        else
        {
            x |= (d & 0x0F);
        }
        if (++n >= 2)
        {
            bt.push_back(x);
            x = 0;
            n = 0;
        }
    }
    uint8 flag;
    if (n == 0)
    {
        flag = 0;
    }
    else
    {
        bt.push_back(x);
        flag = 1;
    }
    return flag;
}

//////////////////////////////
// CTrieBranch

uint256 CTrieBranch::GetNextHash(const uint8 n) const
{
    if (n >= 16)
    {
        return uint256();
    }
    uint8 x = keyIndexNext[n];
    if (x == 0)
    {
        return uint256();
    }
    x--;
    if (x >= branchNext.size())
    {
        return uint256();
    }
    return branchNext[x];
}

void CTrieBranch::SetNextHash(const uint8 n, const uint256& hash)
{
    if (n >= 16)
    {
        return;
    }
    uint8 x = keyIndexNext[n];
    if (x == 0 || x - 1 >= branchNext.size())
    {
        if (hash != 0)
        {
            branchNext.push_back(hash);
            keyIndexNext[n] = branchNext.size();
        }
    }
    else
    {
        branchNext[x - 1] = hash;
        if (hash == 0)
        {
            keyIndexNext[n] = 0;
        }
    }
}

uint256 CTrieBranch::GetValueHash(const uint8 n) const
{
    if (n >= 16)
    {
        return uint256();
    }
    uint8 x = keyIndexValue[n];
    if (x == 0)
    {
        return uint256();
    }
    x--;
    if (x >= branchValue.size())
    {
        return uint256();
    }
    return branchValue[x];
}

void CTrieBranch::SetValueHash(const uint8 n, const uint256& hash)
{
    if (n >= 16)
    {
        return;
    }
    uint8 x = keyIndexValue[n];
    if (x == 0 || x - 1 >= branchValue.size())
    {
        if (hash != 0)
        {
            branchValue.push_back(hash);
            keyIndexValue[n] = branchValue.size();
        }
    }
    else
    {
        branchValue[x - 1] = hash;
        if (hash == 0)
        {
            keyIndexValue[n] = 0;
        }
    }
}

//////////////////////////////
// CTrieExtension

void CTrieExtension::SetKey(const std::vector<uint8>& vKeyNibble)
{
    key.clear();
    uint8 nNibbleFlag = CKeyNibble::Nibble2Byte(vKeyNibble, key);
    flag = ((flag & 0xF0) | nNibbleFlag);
}

void CTrieExtension::GetKey(std::vector<uint8>& vKeyNibble)
{
    CKeyNibble::Byte2Nibble(key, (flag & 0x0F), vKeyNibble);
}

void CTrieExtension::SetNextHash(const uint256& hash)
{
    if ((flag >> 4) == 0)
    {
        if (link.empty())
        {
            link.push_back(hash);
        }
        else
        {
            link[0] = hash;
        }
    }
    else
    {
        if (link.size() < 2)
        {
            if (link.empty())
            {
                flag = (flag & 0x0F);
            }
            link.push_back(hash);
        }
        else
        {
            link[1] = hash;
        }
    }
}

void CTrieExtension::SetValueHash(const uint256& hash)
{
    if ((flag >> 4) == 0)
    {
        if (link.empty())
        {
            link.push_back(hash);
            flag = ((flag & 0x0F) | 0x10);
        }
        else if (link.size() == 1)
        {
            link.push_back(hash);
        }
        else
        {
            link[1] = hash;
        }
    }
    else
    {
        if (link.empty())
        {
            link.push_back(hash);
        }
        else
        {
            link[0] = hash;
        }
    }
}

uint256 CTrieExtension::GetNextHash() const
{
    if ((flag >> 4) == 0)
    {
        if (link.empty())
        {
            return uint256();
        }
        return link[0];
    }
    else
    {
        if (link.size() < 2)
        {
            return uint256();
        }
        return link[1];
    }
}

uint256 CTrieExtension::GetValueHash() const
{
    if ((flag >> 4) == 0)
    {
        if (link.size() < 2)
        {
            return uint256();
        }
        return link[1];
    }
    else
    {
        if (link.empty())
        {
            return uint256();
        }
        return link[0];
    }
}

//////////////////////////////
// CTrieValue

bool CTrieValue::SetStream(CBufStream& ssValue)
{
    try
    {
        ssValue >> type;
        switch (type)
        {
        case TYPE_BRANCH:
            ssValue >> vaBranch;
            return true;
        case TYPE_EXTENSION:
            ssValue >> vaExtension;
            return true;
        case TYPE_VALUE:
            ssValue >> vaValue;
            return true;
        default:
            StdError("CTrieValue", "type error, type: %d", type);
            break;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
    return false;
}

bool CTrieValue::GetStream(CBufStream& ssValue) const
{
    ssValue << type;
    switch (type)
    {
    case TYPE_BRANCH:
        ssValue << vaBranch;
        return true;
    case TYPE_EXTENSION:
        ssValue << vaExtension;
        return true;
    case TYPE_VALUE:
        ssValue << vaValue;
        return true;
    }
    return false;
}

const uint256 CTrieValue::CalcHash()
{
    bool fHasValue = false;
    switch (type)
    {
    case TYPE_BRANCH:
    {
        for (uint8 i = 0; i < 16; i++)
        {
            if (vaBranch.GetNextHash(i) != 0 || vaBranch.GetValueHash(i) != 0)
            {
                fHasValue = true;
                break;
            }
        }
        break;
    }
    case TYPE_EXTENSION:
    {
        if (vaExtension.GetNextHash() != 0 || vaExtension.GetValueHash() != 0)
        {
            fHasValue = true;
            break;
        }
        break;
    }
    case TYPE_VALUE:
    {
        if (vaValue.size() > 0)
        {
            fHasValue = true;
        }
        break;
    }
    }
    if (fHasValue)
    {
        CBufStream ssValue;
        if (!GetStream(ssValue))
        {
            return uint256();
        }
        return crypto::CryptoHash(ssValue.GetData(), ssValue.GetSize());
    }
    return uint256();
}

//////////////////////////////
// CTrieDB

bool CTrieDB::Initialize(const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = pathData.string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);
    if (!Open(engine))
    {
        delete engine;
        return false;
    }
    return true;
}

void CTrieDB::Deinitialize()
{
    Close();
}

void CTrieDB::Clear()
{
    RemoveAll();
}

bool CTrieDB::AddNewTrie(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot)
{
    mtbase::CWriteLock wlock(rwAccess);

    std::map<uint256, CTrieValue> mapCacheNode;
    if (!CreateTrieNodeList(hashPrevRoot, mapKvList, hashNewRoot, mapCacheNode))
    {
        StdLog("CTrieDB", "Add new trie: Create trie node list fail, prev root: %s", hashPrevRoot.GetHex().c_str());
        return false;
    }

    for (auto& kv : mapCacheNode)
    {
        if (!SetDbNodeValue(kv.first, kv.second))
        {
            StdLog("CTrieDB", "Add new trie: Set db node value fail, prev root: %s", hashPrevRoot.GetHex().c_str());
            return false;
        }
    }

#ifdef TEST_STAT
    static int64 nTotalAddNodeCount = 0;
    nTotalAddNodeCount += mapCacheNode.size();
    printf("Add node count: %ld, total count: %ld\n", mapCacheNode.size(), nTotalAddNodeCount);
#endif
    return true;
}

bool CTrieDB::CreateCacheTrie(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot, std::map<uint256, CTrieValue>& mapCacheNode)
{
    mtbase::CReadLock rlock(rwAccess);
    return CreateTrieNodeList(hashPrevRoot, mapKvList, hashNewRoot, mapCacheNode);
}

bool CTrieDB::SaveCacheTrie(std::map<uint256, CTrieValue>& mapCacheNode)
{
    mtbase::CWriteLock wlock(rwAccess);

    for (auto& kv : mapCacheNode)
    {
        if (!SetDbNodeValue(kv.first, kv.second))
        {
            StdLog("CTrieDB", "Save cache trie: Set db node value fail");
            return false;
        }
    }
    return true;
}

bool CTrieDB::Retrieve(const uint256& hashRoot, const bytes& btKey, bytes& btValue)
{
    mtbase::CReadLock rlock(rwAccess);
    bytes nbKeyNibble;
    CKeyNibble::Byte2Nibble(btKey, 0, nbKeyNibble);
    return GetKeyValue(hashRoot, nbKeyNibble, btValue);
}

bool CTrieDB::WalkThroughTrie(const uint256& hashRoot, CTrieDBWalker& walker, const bytes& btKeyPrefix, const bytes& btBeginKeyTail, const bool fReverse)
{
    mtbase::CReadLock rlock(rwAccess);
#ifdef TEST_FLAG
    printf("Walk Through Trie: hashRoot: %s\n", hashRoot.GetHex().c_str());
#endif

    if (hashRoot == 0)
    {
        return true;
    }
    bytes nbKeyPrefix;
    if (!btKeyPrefix.empty())
    {
        CKeyNibble::Byte2Nibble(btKeyPrefix, 0, nbKeyPrefix);
    }
    bytes nbBeginKey;
    if (!btBeginKeyTail.empty())
    {
        bytes nbTail;
        CKeyNibble::Byte2Nibble(btBeginKeyTail, 0, nbTail);
        nbBeginKey = nbKeyPrefix;
        nbBeginKey.insert(nbBeginKey.end(), nbTail.begin(), nbTail.end());
    }
    std::map<uint256, CTrieValue> mapCacheNode;
    bool fWalkOver = false;
    return WalkThroughNode(hashRoot, nbKeyPrefix, nbBeginKey, fReverse, bytes(), walker, mapCacheNode, 0, fWalkOver);
}

bool CTrieDB::WalkThroughAll(CTrieDBWalker& walker)
{
    mtbase::CReadLock rlock(rwAccess);
    try
    {
        if (!WalkThrough(boost::bind(&CTrieDB::WalkerAll, this, _1, _2, boost::ref(walker))))
        {
            return false;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTrieDB::CheckTrie(const std::vector<uint256>& vCheckRoot)
{
    mtbase::CReadLock rlock(rwAccess);

    std::map<uint256, CTrieValue> mapCacheNode;
    for (const uint256& hashRoot : vCheckRoot)
    {
        CTrieListDBWalker walker;
        bytes nbBeginKey;
        bool fWalkOver = false;
        if (!WalkThroughNode(hashRoot, bytes(), nbBeginKey, false, bytes(), walker, mapCacheNode, 0, fWalkOver))
        {
            return false;
        }
    }
    return true;
}

bool CTrieDB::CheckTrieNode(const uint256& hashRoot, std::map<uint256, CTrieValue>& mapCacheNode)
{
    mtbase::CReadLock rlock(rwAccess);

    if (hashRoot != 0)
    {
        CTrieListDBWalker walker;
        bytes nbBeginKey;
        bool fWalkOver = false;
        if (!WalkThroughNode(hashRoot, bytes(), nbBeginKey, false, bytes(), walker, mapCacheNode, 0, fWalkOver))
        {
            return false;
        }
    }
    return true;
}

bool CTrieDB::VerifyTrieRootNode(const uint256& hashRoot)
{
    mtbase::CReadLock rlock(rwAccess);

    if (hashRoot != 0)
    {
        CTrieValue value;
        if (!GetDbNodeValue(hashRoot, value))
        {
            return false;
        }
    }
    return true;
}

bool CTrieDB::WriteExtKv(CBufStream& ssKey, CBufStream& ssValue)
{
    CBufStream ssNewKey;
    ssNewKey << TDB_KEY_TYPE_EXT_KEY;
    ssNewKey.Write(ssKey.GetData(), ssKey.GetSize());
    return Write(ssNewKey, ssValue);
}

bool CTrieDB::ReadExtKv(CBufStream& ssKey, CBufStream& ssValue)
{
    CBufStream ssNewKey;
    ssNewKey << TDB_KEY_TYPE_EXT_KEY;
    ssNewKey.Write(ssKey.GetData(), ssKey.GetSize());
    return Read(ssNewKey, ssValue);
}

bool CTrieDB::RemoveExtKv(CBufStream& ssKey)
{
    CBufStream ssNewKey;
    ssNewKey << TDB_KEY_TYPE_EXT_KEY;
    ssNewKey.Write(ssKey.GetData(), ssKey.GetSize());
    return Erase(ssNewKey);
}

bool CTrieDB::WalkThroughExtKv(CBufStream& ssKeyBegin, CBufStream& ssKeyPrefix, WalkerFunc walkerFunc)
{
    CBufStream ssNewKeyBegin, ssNewKeyPrefix;

    ssNewKeyBegin << TDB_KEY_TYPE_EXT_KEY;
    ssNewKeyBegin.Write(ssKeyBegin.GetData(), ssKeyBegin.GetSize());

    ssNewKeyPrefix << TDB_KEY_TYPE_EXT_KEY;
    ssNewKeyPrefix.Write(ssKeyPrefix.GetData(), ssKeyPrefix.GetSize());

    return WalkThroughOfPrefix(ssNewKeyBegin, ssNewKeyPrefix, walkerFunc);
}

//////////////////////////////////////////
bool CTrieDB::CreateTrieNodeList(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot, std::map<uint256, CTrieValue>& mapCacheNode)
{
    if (mapKvList.empty())
    {
        hashNewRoot = hashPrevRoot;
        return true;
    }
    uint256 hashRoot = hashPrevRoot;
    for (const auto& kv : mapKvList)
    {
        bytes nbKeyNibble;
        CKeyNibble::Byte2Nibble(kv.first, 0, nbKeyNibble);
        if (!AddNode(hashRoot, nbKeyNibble, kv.second, mapCacheNode))
        {
            StdLog("CTrieDB", "Create trie node list: Add node fail, prev root: %s", hashPrevRoot.GetHex().c_str());
            return false;
        }
    }
    hashNewRoot = hashRoot;
    return true;
}

bool CTrieDB::AddNode(uint256& hashRoot, const bytes& nbKeyNibble, const bytes& btValue, std::map<uint256, CTrieValue>& mapCacheNode)
{
    TRIE_NODE_PATH path;
    std::vector<uint256> vRemove;
    if (!GetNodePath(hashRoot, nbKeyNibble, path, vRemove, mapCacheNode))
    {
        StdLog("CTrieDB", "Add node: Get node path fail, root hash: %s", hashRoot.GetHex().c_str());
        return false;
    }

    CTrieKeyValue kvValue;
    kvValue.nKvType = CTrieKeyValue::KV_TYPE_NEW;
    kvValue.value.type = CTrieValue::TYPE_VALUE;
    kvValue.value.vaValue = btValue;
    kvValue.CalcValueHash();
    path.push_back(kvValue);

    uint256 hashPrevOld;
    for (int64 i = path.size() - 2; i >= 0; i--)
    {
        CTrieKeyValue& kv = path[i];
        switch (kv.value.type)
        {
        case CTrieValue::TYPE_BRANCH:
            if (kv.nKvType == CTrieKeyValue::KV_TYPE_SHARE || kv.nKvType == CTrieKeyValue::KV_TYPE_OLD)
            {
                if (hashPrevOld != 0 && kv.keyOld.size() > 0)
                {
                    kv.value.vaBranch.SetNextHash(kv.keyOld[0], hashPrevOld);
                }
                kv.CalcValueHash();
                hashPrevOld = kv.hashValue;
            }
            break;
        case CTrieValue::TYPE_EXTENSION:
            if (kv.nKvType == CTrieKeyValue::KV_TYPE_SHARE || kv.nKvType == CTrieKeyValue::KV_TYPE_OLD)
            {
                if (hashPrevOld != 0)
                {
                    kv.value.vaExtension.SetNextHash(hashPrevOld);
                }
                kv.CalcValueHash();
                hashPrevOld = kv.hashValue;
            }
            break;
        case CTrieValue::TYPE_VALUE:
        default:
            StdLog("CTrieDB", "Add node: Old node type error, type: %d, hash: %s", kv.value.type, kv.hashValue.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrev = kvValue.hashValue;
    bool fIsValue = true;

    for (int64 i = path.size() - 2; i >= 0; i--)
    {
        CTrieKeyValue& kv = path[i];
        switch (kv.value.type)
        {
        case CTrieValue::TYPE_BRANCH:
            if (kv.nKvType == CTrieKeyValue::KV_TYPE_SHARE || kv.nKvType == CTrieKeyValue::KV_TYPE_NEW)
            {
                if (kv.keyNew.size() > 0)
                {
                    if (fIsValue)
                    {
                        if (hashPrev != 0 && kv.value.vaBranch.GetValueHash(kv.keyNew[0]) == hashPrev)
                        {
#ifdef TEST_FLAG
                            std::string strValue(btValue.begin(), btValue.end());
                            printf("Add node: Branch value same, value: %s, vaue hash: %s\n", strValue.c_str(), hashPrev.GetHex().c_str());
#endif
                            return true;
                        }
                        kv.value.vaBranch.SetValueHash(kv.keyNew[0], hashPrev);
                        fIsValue = false;
                    }
                    else
                    {
                        kv.value.vaBranch.SetNextHash(kv.keyNew[0], hashPrev);
                    }
                }
                kv.CalcValueHash();
                hashPrev = kv.hashValue;
            }
            break;
        case CTrieValue::TYPE_EXTENSION:
            if (kv.nKvType == CTrieKeyValue::KV_TYPE_SHARE || kv.nKvType == CTrieKeyValue::KV_TYPE_NEW)
            {
                if (fIsValue)
                {
                    if (hashPrev != 0 && kv.value.vaExtension.GetValueHash() == hashPrev)
                    {
#ifdef TEST_FLAG
                        std::string strValue(btValue.begin(), btValue.end());
                        printf("Add node: Extension value same, value: %s, vaue hash: %s\n", strValue.c_str(), hashPrev.GetHex().c_str());
#endif
                        return true;
                    }
                    kv.value.vaExtension.SetValueHash(hashPrev);
                    fIsValue = false;
                }
                else
                {
                    kv.value.vaExtension.SetNextHash(hashPrev);
                }
                kv.CalcValueHash();
                hashPrev = kv.hashValue;
            }
            break;
        case CTrieValue::TYPE_VALUE:
        default:
            StdLog("CTrieDB", "Add node: New node type error, type: %d, hash: %s", kv.value.type, kv.hashValue.GetHex().c_str());
            return false;
        }
    }
    hashRoot = hashPrev;

    for (const uint256& hash : vRemove)
    {
        mapCacheNode.erase(hash);
    }

    for (std::size_t i = 0; i < path.size(); i++)
    {
        const CTrieKeyValue& kv = path[i];
        if (kv.hashValue != 0)
        {
            mapCacheNode[kv.hashValue] = kv.value;
        }
    }
    return true;
}

bool CTrieDB::GetNodeValue(const uint256& hash, CTrieValue& value, bool& fCache, std::map<uint256, CTrieValue>& mapCacheNode)
{
    if (hash == 0)
    {
        return false;
    }
    auto it = mapCacheNode.find(hash);
    if (it != mapCacheNode.end())
    {
        value = it->second;
        fCache = true;
    }
    else
    {
        if (!GetDbNodeValue(hash, value))
        {
            return false;
        }
        fCache = false;
    }
    return true;
}

bool CTrieDB::SetDbNodeValue(const uint256& hash, const CTrieValue& value)
{
    CBufStream ssKey, ssValue;
    ssKey << TDB_KEY_TYPE_TRIE_KEY << hash;
    if (!value.GetStream(ssValue))
    {
        StdLog("CTrieDB", "Set db node value: Get stream fail, hash: %s", hash.GetHex().c_str());
        return false;
    }
    if (!Write(ssKey, ssValue))
    {
        StdLog("CTrieDB", "Set db node value: Write fail, hash: %s", hash.GetHex().c_str());
        return false;
    }
    return true;
}

bool CTrieDB::GetDbNodeValue(const uint256& hash, CTrieValue& value)
{
    CBufStream ssKey, ssValue;
    ssKey << TDB_KEY_TYPE_TRIE_KEY << hash;
    if (!Read(ssKey, ssValue))
    {
        StdLog("CTrieDB", "Get db node value: Read fail, hash: %s", hash.GetHex().c_str());
        return false;
    }
    if (!value.SetStream(ssValue))
    {
        StdLog("CTrieDB", "Get db node value: Set stream fail, hash: %s", hash.GetHex().c_str());
        return false;
    }
    return true;
}

bool CTrieDB::RemoveDbNodeValue(const uint256& hash)
{
    CBufStream ssKey;
    ssKey << TDB_KEY_TYPE_TRIE_KEY << hash;
    return Erase(ssKey);
}

bool CTrieDB::GetNodePath(const uint256& hashRoot, const bytes& nbKeyNibble, TRIE_NODE_PATH& path, std::vector<uint256>& vRemove, std::map<uint256, CTrieValue>& mapCacheNode)
{
    uint256 hash = hashRoot;
    bytes syKey = nbKeyNibble;
    while (!syKey.empty())
    {
        CTrieValue value;
        bool fCache = false;
        if (!GetNodeValue(hash, value, fCache, mapCacheNode))
        {
            if (hash == hashRoot)
            {
                if (syKey.size() == 1)
                {
                    CTrieKeyValue kvBranch;
                    kvBranch.nKvType = CTrieKeyValue::KV_TYPE_NEW;
                    kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
                    kvBranch.value.type = CTrieValue::TYPE_BRANCH;
                    path.push_back(kvBranch);
                }
                else
                {
                    CTrieKeyValue kvExtension;
                    kvExtension.nKvType = CTrieKeyValue::KV_TYPE_NEW;
                    kvExtension.keyNew.assign(syKey.begin(), syKey.end());
                    kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
                    kvExtension.value.vaExtension.SetKey(kvExtension.keyNew);
                    path.push_back(kvExtension);
                }
                return true;
            }
            StdLog("CTrieDB", "Get node path: Get node fail, hash: %s", hash.GetHex().c_str());
            return false;
        }

        switch (value.type)
        {
        case CTrieValue::TYPE_BRANCH:
        {
            if (fCache)
            {
                vRemove.push_back(hash);
            }
            if (!GetBranchPath(syKey, hash, value, path))
            {
                return true;
            }
            break;
        }
        case CTrieValue::TYPE_EXTENSION:
        {
            if (fCache)
            {
                vRemove.push_back(hash);
            }
            if (!GetExtensionPath(syKey, hash, value, path))
            {
                return true;
            }
            break;
        }
        case CTrieValue::TYPE_VALUE:
        default:
            StdLog("CTrieDB", "Get node path: node type error, type: %d, hash: %s", value.type, hash.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CTrieDB::GetBranchPath(bytes& syKey, uint256& hash, CTrieValue& value, TRIE_NODE_PATH& path)
{
    if (syKey.size() > 1)
    {
        hash = value.vaBranch.GetNextHash(syKey[0]);
        if (hash != 0)
        {
            CTrieKeyValue kvBranch(value);
            kvBranch.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
            kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
            path.push_back(kvBranch);

            syKey.erase(syKey.begin(), syKey.begin() + 1);
            return true;
        }
    }

    CTrieKeyValue kvBranch(value);
    kvBranch.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
    kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
    path.push_back(kvBranch);

    syKey.erase(syKey.begin(), syKey.begin() + 1);
    if (syKey.size() == 1)
    {
        CTrieKeyValue kvBranchEx;
        kvBranchEx.nKvType = CTrieKeyValue::KV_TYPE_NEW;
        kvBranchEx.keyNew.assign(syKey.begin(), syKey.begin() + 1);
        kvBranchEx.value.type = CTrieValue::TYPE_BRANCH;
        path.push_back(kvBranchEx);
    }
    else if (syKey.size() > 1)
    {
        CTrieKeyValue kvExtension;
        kvExtension.nKvType = CTrieKeyValue::KV_TYPE_NEW;
        kvExtension.keyNew.assign(syKey.begin(), syKey.end());
        kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
        kvExtension.value.vaExtension.SetKey(kvExtension.keyNew);
        path.push_back(kvExtension);
    }
    return false;
}

bool CTrieDB::GetExtensionPath(bytes& syKey, uint256& hash, CTrieValue& value, TRIE_NODE_PATH& path)
{
    bytes btNodeKey;
    value.vaExtension.GetKey(btNodeKey);
    if (syKey.size() >= btNodeKey.size())
    {
        bytes btFindKey(syKey.begin(), syKey.begin() + btNodeKey.size());
        if (btNodeKey == btFindKey)
        {
            CTrieKeyValue kvExtension(value);
            kvExtension.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
            kvExtension.keyOld.assign(btNodeKey.begin(), btNodeKey.end());
            kvExtension.keyNew.assign(btNodeKey.begin(), btNodeKey.end());
            path.push_back(kvExtension);

            hash = value.vaExtension.GetNextHash();
            syKey.erase(syKey.begin(), syKey.begin() + btNodeKey.size());

            if (hash == 0)
            {
                if (syKey.size() == 1)
                {
                    CTrieKeyValue kvBranch;
                    kvBranch.nKvType = CTrieKeyValue::KV_TYPE_NEW;
                    kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
                    kvBranch.value.type = CTrieValue::TYPE_BRANCH;
                    path.push_back(kvBranch);
                }
                else if (syKey.size() > 1)
                {
                    CTrieKeyValue kvExtension;
                    kvExtension.nKvType = CTrieKeyValue::KV_TYPE_NEW;
                    kvExtension.keyNew.assign(syKey.begin(), syKey.end());
                    kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
                    kvExtension.value.vaExtension.SetKey(kvExtension.keyNew);
                    path.push_back(kvExtension);
                }
                return false;
            }
            return true;
        }
    }

    std::size_t nMinLen = std::min(btNodeKey.size(), syKey.size());
    std::size_t nSameLen = 0;
    for (std::size_t i = 0; i < nMinLen; i++)
    {
        if (syKey[i] != btNodeKey[i])
        {
            break;
        }
        nSameLen++;
    }

    if (nSameLen == 0)
    {
        CTrieKeyValue kvBranch;
        kvBranch.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
        kvBranch.keyOld.assign(btNodeKey.begin(), btNodeKey.begin() + 1);
        kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
        kvBranch.value.type = CTrieValue::TYPE_BRANCH;
        if (btNodeKey.size() == 1)
        {
            kvBranch.value.vaBranch.SetNextHash(kvBranch.keyOld[0], value.vaExtension.GetNextHash());
            kvBranch.value.vaBranch.SetValueHash(kvBranch.keyOld[0], value.vaExtension.GetValueHash());
        }
        path.push_back(kvBranch);

        btNodeKey.erase(btNodeKey.begin(), btNodeKey.begin() + 1);
        syKey.erase(syKey.begin(), syKey.begin() + 1);
    }
    else if (nSameLen >= 1)
    {
        if (nSameLen == 1)
        {
            CTrieKeyValue kvBranch;
            kvBranch.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
            kvBranch.keyOld.assign(btNodeKey.begin(), btNodeKey.begin() + 1);
            kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
            kvBranch.value.type = CTrieValue::TYPE_BRANCH;
            if (btNodeKey.size() == 1)
            {
                kvBranch.value.vaBranch.SetNextHash(kvBranch.keyOld[0], value.vaExtension.GetNextHash());
                kvBranch.value.vaBranch.SetValueHash(kvBranch.keyOld[0], value.vaExtension.GetValueHash());
            }
            path.push_back(kvBranch);
        }
        else
        {
            CTrieKeyValue kvExtension;
            kvExtension.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
            kvExtension.keyOld.assign(btNodeKey.begin(), btNodeKey.begin() + nSameLen);
            kvExtension.keyNew.assign(syKey.begin(), syKey.begin() + nSameLen);
            kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
            kvExtension.value.vaExtension.SetKey(kvExtension.keyOld);
            if (btNodeKey.size() == nSameLen)
            {
                kvExtension.value.vaExtension.SetNextHash(value.vaExtension.GetNextHash());
                kvExtension.value.vaExtension.SetValueHash(value.vaExtension.GetValueHash());
            }
            path.push_back(kvExtension);
        }

        btNodeKey.erase(btNodeKey.begin(), btNodeKey.begin() + nSameLen);
        syKey.erase(syKey.begin(), syKey.begin() + nSameLen);

        if (btNodeKey.size() > 0 && syKey.size() > 0)
        {
            CTrieKeyValue kvBranch;
            kvBranch.nKvType = CTrieKeyValue::KV_TYPE_SHARE;
            kvBranch.keyOld.assign(btNodeKey.begin(), btNodeKey.begin() + 1);
            kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
            kvBranch.value.type = CTrieValue::TYPE_BRANCH;
            if (btNodeKey.size() == 1)
            {
                kvBranch.value.vaBranch.SetNextHash(kvBranch.keyOld[0], value.vaExtension.GetNextHash());
                kvBranch.value.vaBranch.SetValueHash(kvBranch.keyOld[0], value.vaExtension.GetValueHash());
            }
            path.push_back(kvBranch);

            btNodeKey.erase(btNodeKey.begin(), btNodeKey.begin() + 1);
            syKey.erase(syKey.begin(), syKey.begin() + 1);
        }
    }

    if (btNodeKey.size() == 1)
    {
        CTrieKeyValue kvBranch;
        kvBranch.nKvType = CTrieKeyValue::KV_TYPE_OLD;
        kvBranch.keyOld.assign(btNodeKey.begin(), btNodeKey.begin() + 1);
        kvBranch.value.type = CTrieValue::TYPE_BRANCH;
        kvBranch.value.vaBranch.SetNextHash(kvBranch.keyOld[0], value.vaExtension.GetNextHash());
        kvBranch.value.vaBranch.SetValueHash(kvBranch.keyOld[0], value.vaExtension.GetValueHash());
        path.push_back(kvBranch);
    }
    else if (btNodeKey.size() > 1)
    {
        CTrieKeyValue kvExtension;
        kvExtension.nKvType = CTrieKeyValue::KV_TYPE_OLD;
        kvExtension.keyOld.assign(btNodeKey.begin(), btNodeKey.end());
        kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
        kvExtension.value.vaExtension.SetKey(kvExtension.keyOld);
        kvExtension.value.vaExtension.SetNextHash(value.vaExtension.GetNextHash());
        kvExtension.value.vaExtension.SetValueHash(value.vaExtension.GetValueHash());
        path.push_back(kvExtension);
    }

    if (syKey.size() == 1)
    {
        CTrieKeyValue kvBranch;
        kvBranch.nKvType = CTrieKeyValue::KV_TYPE_NEW;
        kvBranch.keyNew.assign(syKey.begin(), syKey.begin() + 1);
        kvBranch.value.type = CTrieValue::TYPE_BRANCH;
        path.push_back(kvBranch);
    }
    else if (syKey.size() > 1)
    {
        CTrieKeyValue kvExtension;
        kvExtension.nKvType = CTrieKeyValue::KV_TYPE_NEW;
        kvExtension.keyNew.assign(syKey.begin(), syKey.end());
        kvExtension.value.type = CTrieValue::TYPE_EXTENSION;
        kvExtension.value.vaExtension.SetKey(kvExtension.keyNew);
        path.push_back(kvExtension);
    }
    return false;
}

bool CTrieDB::GetKeyValue(const uint256& hashRoot, const bytes& nbKey, bytes& btValue)
{
    if (hashRoot == 0)
    {
        return false;
    }
    uint256 hash = hashRoot;
    bytes syKey = nbKey;
    while (!syKey.empty())
    {
        CTrieValue value;
        if (!GetDbNodeValue(hash, value))
        {
            StdLog("CTrieDB", "Get key value: Get db node fail, hash: %s", hash.GetHex().c_str());
            return false;
        }
        std::size_t nKeyLen = 0;
        switch (value.type)
        {
        case CTrieValue::TYPE_BRANCH:
        {
            if (syKey.size() == 1)
            {
                hash = value.vaBranch.GetValueHash(syKey[0]);
            }
            else
            {
                hash = value.vaBranch.GetNextHash(syKey[0]);
            }
            if (hash == 0)
            {
                return false;
            }
            nKeyLen = 1;
            break;
        }
        case CTrieValue::TYPE_EXTENSION:
        {
            bytes nbNodeKey;
            value.vaExtension.GetKey(nbNodeKey);
            if (syKey.size() >= nbNodeKey.size())
            {
                bytes nbFindKey(syKey.begin(), syKey.begin() + nbNodeKey.size());
                if (nbNodeKey == nbFindKey)
                {
                    if (syKey.size() == nbNodeKey.size())
                    {
                        hash = value.vaExtension.GetValueHash();
                    }
                    else
                    {
                        hash = value.vaExtension.GetNextHash();
                    }
                    if (hash == 0)
                    {
                        return false;
                    }
                    nKeyLen = nbNodeKey.size();
                    break;
                }
            }
            return false;
        }
        case CTrieValue::TYPE_VALUE:
        default:
            StdLog("CTrieDB", "Get key value: node type error, type: %d", value.type);
            return false;
        }
        syKey.erase(syKey.begin(), syKey.begin() + nKeyLen);
    }

    CTrieValue value;
    if (!GetDbNodeValue(hash, value))
    {
        StdLog("CTrieDB", "Get key value: Get db value fail, hash: %s", hash.GetHex().c_str());
        return false;
    }
    if (value.type != CTrieValue::TYPE_VALUE)
    {
        StdLog("CTrieDB", "Get key value: Db value type error, type: %d, hash: %s", value.type, hash.GetHex().c_str());
        return false;
    }
    btValue = value.vaValue;
    return true;
}

bool CTrieDB::WalkerAll(CBufStream& ssKey, CBufStream& ssValue, CTrieDBWalker& walker)
{
    bytes key(ssKey.GetData(), ssKey.GetData() + ssKey.GetSize());
    bytes value(ssValue.GetData(), ssValue.GetData() + ssValue.GetSize());
    bool fWalkOver = false;
    return walker.Walk(key, value, 0, fWalkOver);
}

bool CTrieDB::WalkThroughNode(const uint256& hashNode, const bytes& nbKeyPrefix, bytes& nbBeginKey, const bool fReverse, const bytes& nbPrevKey,
                              CTrieDBWalker& walker, std::map<uint256, CTrieValue>& mapCacheNode, const uint32 nDepth, bool& fWalkOver)
{
    if (fWalkOver)
    {
        return true;
    }

    CTrieValue value;
    auto it = mapCacheNode.find(hashNode);
    if (it != mapCacheNode.end())
    {
        value = it->second;
    }
    else
    {
        if (!GetDbNodeValue(hashNode, value))
        {
            StdLog("CTrieDB", "Walk Through Node: Get Db Node Value fail, hash: %s", hashNode.GetHex().c_str());
#ifdef TEST_FLAG
            printf("Walk Through Node: GetDbNodeValue fail, hash: %s\n", hashNode.GetHex().c_str());
#endif
            return false;
        }
        if (hashNode != value.CalcHash())
        {
            StdLog("CTrieDB", "Walk Through Node: Value hash error, hash: %s", hashNode.GetHex().c_str());
            return false;
        }
        mapCacheNode.insert(make_pair(hashNode, value));
    }

    if (!nbBeginKey.empty() && nbPrevKey.size() > 0)
    {
        size_t nMinSize = min(nbBeginKey.size(), nbPrevKey.size());
        bytes nbCompFindKey(nbBeginKey.begin(), nbBeginKey.begin() + nMinSize);
        bytes nbCompNewKey(nbPrevKey.begin(), nbPrevKey.begin() + nMinSize);
        if (nbCompFindKey != nbCompNewKey)
        {
            return true;
        }
        if (nbBeginKey.size() == nbCompNewKey.size())
        {
            nbBeginKey.clear();
        }
    }

    switch (value.type)
    {
    case CTrieValue::TYPE_BRANCH:
    {
#ifdef TEST_FLAG
        printf("Walk Through Node: Branch node hash: %s\n", hashNode.GetHex().c_str());
#endif
        bytes nbNextKeyPrefix;
        if (nbKeyPrefix.size() > 1)
        {
            nbNextKeyPrefix.assign(nbKeyPrefix.begin() + 1, nbKeyPrefix.end());
        }
        int nBranchKey = 0;
        int nBranchEnd = 16;
        if (fReverse)
        {
            nBranchKey = 15;
            nBranchEnd = -1;
        }
        while (nBranchKey != nBranchEnd && !fWalkOver)
        {
            if (!nbKeyPrefix.empty() && nbKeyPrefix[0] != (uint8)nBranchKey)
            {
                if (fReverse)
                    nBranchKey--;
                else
                    nBranchKey++;
                continue;
            }

            bytes nbNextKey = nbPrevKey;
            nbNextKey.insert(nbNextKey.end(), (uint8)nBranchKey);

            uint256 hashValue = value.vaBranch.GetValueHash(nBranchKey);
            if (hashValue != 0)
            {
#ifdef TEST_FLAG
                string strValue;
                strValue.assign(btValue.begin(), btValue.end());
                printf("Walk Through Node: Branch node: key: %x, value: %s\n", nBranchKey, strValue.c_str());
#endif
                if (!WalkThroughNode(hashValue, nbNextKeyPrefix, nbBeginKey, fReverse, nbNextKey, walker, mapCacheNode, nDepth + 1, fWalkOver))
                {
                    return false;
                }
            }
            uint256 hashSub = value.vaBranch.GetNextHash(nBranchKey);
            if (hashSub != 0)
            {
#ifdef TEST_FLAG
                printf("Walk Through Node: Branch node: key: %x, next hash: %s\n", nBranchKey, hashSub.GetHex().c_str());
#endif
                if (!WalkThroughNode(hashSub, nbNextKeyPrefix, nbBeginKey, fReverse, nbNextKey, walker, mapCacheNode, nDepth + 1, fWalkOver))
                {
                    return false;
                }
            }
            if (fReverse)
                nBranchKey--;
            else
                nBranchKey++;
        }
        break;
    }
    case CTrieValue::TYPE_EXTENSION:
    {
#ifdef TEST_FLAG
        printf("Walk Through Node: Extension node hash: %s\n", hashNode.GetHex().c_str());
#endif

        bytes key;
        value.vaExtension.GetKey(key);
        bytes nbKey = nbPrevKey;
        nbKey.insert(nbKey.end(), key.begin(), key.end());

        bytes nbNextKeyPrefix;
        if (!nbKeyPrefix.empty())
        {
            size_t nMinSize = min(nbKeyPrefix.size(), key.size());
            bytes nbCompFindKey(nbKeyPrefix.begin(), nbKeyPrefix.begin() + nMinSize);
            bytes nbCompNewKey(key.begin(), key.begin() + nMinSize);
            if (nbCompFindKey != nbCompNewKey)
            {
                break;
            }
            if (nbKeyPrefix.size() > nMinSize)
            {
                nbNextKeyPrefix.assign(nbKeyPrefix.begin() + nMinSize, nbKeyPrefix.end());
            }
        }

#ifdef TEST_FLAG
        string strKey;
        for (std::size_t i = 0; i < key.size(); i++)
        {
            char buf[8] = { 0 };
            sprintf(buf, "%x-", key[i]);
            strKey += buf;
        }
#endif

        uint256 hashValue = value.vaExtension.GetValueHash();
        if (hashValue != 0)
        {
#ifdef TEST_FLAG
            string strValue;
            strValue.assign(btValue.begin(), btValue.end());
            printf("Walk Through Node: Extension node: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
#endif
            if (!WalkThroughNode(hashValue, nbNextKeyPrefix, nbBeginKey, fReverse, nbKey, walker, mapCacheNode, nDepth + 1, fWalkOver))
            {
                return false;
            }
        }
        uint256 hashSub = value.vaExtension.GetNextHash();
        if (hashSub != 0)
        {
#ifdef TEST_FLAG
            printf("Walk Through Node: Extension node: key: %s, next hash: %s\n", strKey.c_str(), hashSub.GetHex().c_str());
#endif
            if (!WalkThroughNode(hashSub, nbNextKeyPrefix, nbBeginKey, fReverse, nbKey, walker, mapCacheNode, nDepth + 1, fWalkOver))
            {
                return false;
            }
        }
        break;
    }
    case CTrieValue::TYPE_VALUE:
    {
        bytes btByteKey;
        CKeyNibble::Nibble2Byte(nbPrevKey, btByteKey);
        if (!walker.Walk(btByteKey, value.vaValue, nDepth, fWalkOver))
        {
            return false;
        }
        break;
    }
    default:
        StdLog("CTrieDB", "Walk Through Node: type error, type: %d", value.type);
#ifdef TEST_FLAG
        printf("Walk Through Node: type error, type: %d\n", value.type);
#endif
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

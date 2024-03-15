// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TRIEDB_H
#define STORAGE_TRIEDB_H

#include <map>

#include "destination.h"
#include "mtbase.h"
#include "timeseries.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

//////////////////////////////////////////////////////////////

class CKeyNibble
{
public:
    static void Byte2Nibble(const bytes& bt, const uint8 flag, bytes& nibble);
    static uint8 Nibble2Byte(const bytes& nibble, bytes& bt);
};

class CTrieBranch
{
    friend class mtbase::CStream;

public:
    CTrieBranch()
    {
        keyIndexNext.resize(16);
        keyIndexValue.resize(16);
    }

    uint256 GetNextHash(const uint8 n) const;
    void SetNextHash(const uint8 n, const uint256& hash);

    uint256 GetValueHash(const uint8 n) const;
    void SetValueHash(const uint8 n, const uint256& hash);

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(keyIndexNext, opt);
        s.Serialize(keyIndexValue, opt);
        s.Serialize(branchNext, opt);
        s.Serialize(branchValue, opt);
    }

protected:
    std::vector<uint8> keyIndexNext;
    std::vector<uint8> keyIndexValue;
    std::vector<uint256> branchNext;
    std::vector<uint256> branchValue;
};

class CTrieExtension
{
    friend class mtbase::CStream;

public:
    CTrieExtension()
      : flag(0) {}

    void SetKey(const std::vector<uint8>& vKeyNibble);
    void GetKey(std::vector<uint8>& vKeyNibble) const;
    void SetNextHash(const uint256& hash);
    uint256 GetNextHash() const;
    void SetValueHash(const uint256& hash);
    uint256 GetValueHash() const;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(flag, opt);
        s.Serialize(key, opt);
        s.Serialize(link, opt);
    }

protected:
    uint8 flag;
    std::vector<uint8> key;     // length of this field: even or odd, decided by the low 4-bits of field 'flag', 0 - even, 1 - odd
    std::vector<uint256> link;  // next or value, determined by the high 4-bits of field 'flag', 0 - next, 1 - value. its length is 2 at most, one for next and another for value
};

class CTrieValue
{
public:
    enum
    {
        TYPE_BRANCH = 1,
        TYPE_EXTENSION = 2,
        TYPE_VALUE = 3
    };

    CTrieValue()
      : type(0) {}

    bool SetStream(mtbase::CBufStream& ssValue);
    bool GetStream(mtbase::CBufStream& ssValue) const;
    const uint256 CalcHash();

public:
    uint8 type;
    CTrieBranch vaBranch;
    CTrieExtension vaExtension;
    bytes vaValue;
};

class CTrieKeyValue
{
public:
    CTrieKeyValue()
      : nKvType(KV_TYPE_SHARE) {}
    CTrieKeyValue(const CTrieValue& v)
      : nKvType(KV_TYPE_SHARE), value(v) {}

    void CalcValueHash()
    {
        hashValue = value.CalcHash();
    }

    enum
    {
        KV_TYPE_SHARE,
        KV_TYPE_OLD,
        KV_TYPE_NEW,
    };

public:
    int nKvType;
    bytes keyOld;
    bytes keyNew;
    CTrieValue value;
    uint256 hashValue;
};

typedef std::vector<CTrieKeyValue> TRIE_NODE_PATH;

//////////////////////////////////////////////////////////////
// CTrieDBWalker

class CTrieDBWalker
{
public:
    virtual bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) = 0;
};

class CTrieListDBWalker : public CTrieDBWalker
{
public:
    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
    {
        return true;
    }
};

//////////////////////////////////////////////////////////////
// CTrieDB

class CTrieDB : public mtbase::CKVDB
{
public:
    CTrieDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddNewTrie(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot);
    bool CreateCacheTrie(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot, std::map<uint256, CTrieValue>& mapCacheNode);
    bool SaveCacheTrie(std::map<uint256, CTrieValue>& mapCacheNode);
    bool Retrieve(const uint256& hashRoot, const bytes& btKey, bytes& btValue);
    bool WalkThroughTrie(const uint256& hashRoot, CTrieDBWalker& walker, const bytes& btKeyPrefix = bytes(), const bytes& btBeginKeyTail = bytes(), const bool fReverse = false);
    bool WalkThroughAll(CTrieDBWalker& walker);
    bool CheckTrie(const std::vector<uint256>& vCheckRoot);
    bool CheckTrieNode(const uint256& hashRoot, std::map<uint256, CTrieValue>& mapCacheNode);
    bool VerifyTrieRootNode(const uint256& hashRoot);

    bool WriteExtKv(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue);
    bool ReadExtKv(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue);
    bool RemoveExtKv(mtbase::CBufStream& ssKey);
    bool WalkThroughExtKv(mtbase::CBufStream& ssKeyBegin, mtbase::CBufStream& ssKeyPrefix, WalkerFunc walkerFunc);

protected:
    bool CreateTrieNodeList(const uint256& hashPrevRoot, const bytesmap& mapKvList, uint256& hashNewRoot, std::map<uint256, CTrieValue>& mapCacheNode);
    bool AddNode(uint256& hashRoot, const bytes& nbKeyNibble, const bytes& btValue, std::map<uint256, CTrieValue>& mapCacheNode);
    bool GetNodeValue(const uint256& hash, CTrieValue& value, bool& fCache, std::map<uint256, CTrieValue>& mapCacheNode);
    bool SetDbNodeValue(const uint256& hash, const CTrieValue& value);
    bool GetDbNodeValue(const uint256& hash, CTrieValue& value);
    bool RemoveDbNodeValue(const uint256& hash);
    bool GetNodePath(const uint256& hashRoot, const bytes& nbKeyNibble, TRIE_NODE_PATH& path, std::vector<uint256>& vRemove, std::map<uint256, CTrieValue>& mapCacheNode);
    bool GetBranchPath(bytes& syKey, uint256& hash, CTrieValue& value, TRIE_NODE_PATH& path);
    bool GetExtensionPath(bytes& syKey, uint256& hash, CTrieValue& value, TRIE_NODE_PATH& path);
    bool GetKeyValue(const uint256& hashRoot, const bytes& nbKey, bytes& btValue);
    bool WalkerAll(mtbase::CBufStream& ssKey, mtbase::CBufStream& ssValue, CTrieDBWalker& walker);
    bool WalkThroughNode(const uint256& hashNode, const bytes& nbKeyPrefix, bytes& nbBeginKey, const bool fReverse, const bytes& nbPrevKey,
                         CTrieDBWalker& walker, std::map<uint256, CTrieValue>& mapCacheNode, const uint32 nDepth, bool& fWalkOver);

protected:
    mtbase::CRWAccess rwAccess;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_TRIEDB_H

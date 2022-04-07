// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TXINDEXDB_H
#define STORAGE_TXINDEXDB_H

#include <boost/thread/thread.hpp>

#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"

namespace metabasenet
{
namespace storage
{

class CListTxIndexTrieDBWalker : public CTrieDBWalker
{
public:
    CListTxIndexTrieDBWalker(std::map<uint256, CTxIndex>& mapTxIndexIn)
      : mapTxIndex(mapTxIndexIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint256, CTxIndex>& mapTxIndex;
};

class CForkTxIndexDB
{
public:
    CForkTxIndexDB(const bool fCacheIn = true);
    ~CForkTxIndexDB();
    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();
    bool AddBlockTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot);
    bool RetrieveTxIndex(const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex);
    bool VerifyTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);
    bool ListAllTxIndex(const uint256& hashBlock, std::map<uint256, CTxIndex>& mapTxIndex);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    uint256 hashFork;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::map<uint256, CTxIndex>> mapCacheTxIndex;
    CTrieDB dbTrie;
};

class CTxIndexDB
{
public:
    CTxIndexDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot);
    bool RetrieveTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex);
    bool VerifyTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, bool fVerifyAllNode = true);
    bool ListAllTxIndex(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CTxIndex>& mapTxIndex);

protected:
    bool fCache;
    boost::filesystem::path pathTxIndex;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkTxIndexDB>> mapTxIndexDB;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_TXINDEXDB_H

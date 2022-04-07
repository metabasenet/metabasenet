// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VOTEREWARDDB_H
#define STORAGE_VOTEREWARDDB_H

#include <map>

#include "destination.h"
#include "hnbase.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CListVoteRewardTrieDBWalker : public CTrieDBWalker
{
public:
    CListVoteRewardTrieDBWalker(const uint32 nGetCountIn, std::vector<std::pair<uint32, uint256>>& vVoteRewardIn)
      : nGetCount(nGetCountIn), vVoteReward(vVoteRewardIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

protected:
    const uint32 nGetCount;
    std::vector<std::pair<uint32, uint256>>& vVoteReward;
};

class CForkVoteRewardDB
{
public:
    CForkVoteRewardDB(const bool fCacheIn = true);
    ~CForkVoteRewardDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddVoteReward(const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot);
    bool ListVoteReward(const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward);

    bool VerifyVoteReward(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

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
    CTrieDB dbTrie;
};

class CVoteRewardDB
{
public:
    CVoteRewardDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot);
    bool ListVoteReward(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward);

    bool VerifyVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkVoteRewardDB>> mapVoteRewardDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_VOTEREWARDDB_H

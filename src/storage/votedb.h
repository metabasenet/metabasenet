// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VOTEDB_H
#define STORAGE_VOTEDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hnbase.h"
#include "timeseries.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CCacheCalcVoteRewardContext
{
public:
    CCacheCalcVoteRewardContext() {}

    void Clear()
    {
        mapFirstFullVote.clear();
        mapIncVote.clear();
    }

public:
    std::map<CDestination, std::map<CDestination, CVoteContext>> mapFirstFullVote; // Full vote for first block of day
    std::map<uint32, std::map<CDestination, CVoteContext>> mapIncVote;             // key: height, value: a height of increased vote
};

class CListAllVoteTrieDBWalker : public CTrieDBWalker
{
public:
    CListAllVoteTrieDBWalker(std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVoteIn)
      : mapDelegateVote(mapDelegateVoteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote;
};

class CListMoreIncVoteTrieDBWalker : public CTrieDBWalker
{
public:
    CListMoreIncVoteTrieDBWalker(const uint32 nEndHeightIn, std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVoteIn)
      : nEndHeight(nEndHeightIn), mapIncVote(mapIncVoteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    const uint32 nEndHeight;
    std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVote;
};

//////////////////////////////////////////////////////////////
// CDayVoteWalker

class CDayVoteWalker
{
public:
    virtual bool Walk(const uint32 nHeight, const std::map<CDestination, std::pair<std::map<CDestination, CVoteContext>, uint256>>& mapDelegateVote) = 0;
};

////////////////////////////////////////////
// CVoteDB
class CVoteDB
{
public:
    CVoteDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();
    bool AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote, uint256& hashVoteRoot);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool VerifyVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot, const uint64 nVoteCount);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot, uint64& nVoteCount);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool GetMoreIncVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVote);
    bool GetCalcDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock);

protected:
    enum
    {
        VOTE_CACHE_SIZE = 8
    };
    hnbase::CRWAccess rwAccess;
    std::map<uint256, CCacheCalcVoteRewardContext> mapCacheCalcVote; // key: day end block hash, value: a day of voting data
    CTrieDB dbTrie;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_VOTEDB_H

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VOTEDB_H
#define STORAGE_VOTEDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "mtbase.h"
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
    std::map<CDestination, std::map<CDestination, CVoteContext>> mapFirstFullVote; // Full vote for first block of day, key: delegate address, value: map key: vote address, map value: vote context
    std::map<uint32, std::map<CDestination, CVoteContext>> mapIncVote;             // key: height, value: a height of increased vote, map key: vote address, map value: vote context
};

class CListAllVoteTrieDBWalker : public CTrieDBWalker
{
public:
    CListAllVoteTrieDBWalker(std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVoteIn)
      : mapDelegateVote(mapDelegateVoteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote; // key: delegate address, value: map key: vote address, map value: vote context
};

class CListMoreIncVoteTrieDBWalker : public CTrieDBWalker
{
public:
    CListMoreIncVoteTrieDBWalker(const uint32 nEndHeightIn, std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVoteIn)
      : nEndHeight(nEndHeightIn), mapIncVote(mapIncVoteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    const uint32 nEndHeight;
    std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVote; // key: height, value: map key: vote address, map value: vote context
};

class CListPledgeFinalHeightTrieDBWalker : public CTrieDBWalker
{
public:
    CListPledgeFinalHeightTrieDBWalker(std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeightIn)
      : mapPledgeFinalHeight(mapPledgeFinalHeightIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight; // key: pledge address, value first: final height, value second: pledge height
};

class CListDelegateVoteTrieDBWalker : public CTrieDBWalker
{
public:
    CListDelegateVoteTrieDBWalker(std::map<CDestination, uint256>& mapVoteIn)
      : mapVote(mapVoteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, uint256>& mapVote;
};

class CListVoteRewardTrieDBWalker : public CTrieDBWalker
{
public:
    CListVoteRewardTrieDBWalker(const uint32 nGetChainIdIn, const uint32 nGetCountIn, std::vector<std::pair<uint32, uint256>>& vVoteRewardIn)
      : nGetChainId(nGetChainIdIn), nGetCount(nGetCountIn), vVoteReward(vVoteRewardIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

protected:
    const uint32 nGetChainId;
    const uint32 nGetCount;
    std::vector<std::pair<uint32, uint256>>& vVoteReward;
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
    CVoteDB()
      : cacheDelegate(MAX_DELEGATE_ENROLL_CACHE_COUNT) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                      const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight, uint256& hashVoteRoot);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight);
    bool VerifyVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);

    bool AddNewDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapDelegateVote,
                        const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot);
    bool AddDelegateVote(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapDelegateVote, uint256& hashNewRoot);
    bool AddDelegateEnroll(const uint256& hashBlock, const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount);
    bool RetrieveDelegatedVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapDelegateVote);
    bool RetrieveDelegatedEnroll(const uint256& hashBlock, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos);
    bool RetrieveRangeEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool VerifyDelegateVote(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

    bool AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot);
    bool ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward);
    bool VerifyVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot, const uint64 nVoteCount);
    bool ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot, uint64& nVoteCount);
    void AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool GetMoreIncVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, std::map<uint32, std::map<CDestination, CVoteContext>>& mapIncVote);
    bool GetCalcDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock);

protected:
    enum
    {
        VOTE_USER_VOTE_CACHE_SIZE = 8,
        MAX_DELEGATE_ENROLL_CACHE_COUNT = 64,
    };

    mtbase::CRWAccess rwAccess;
    std::map<uint256, CCacheCalcVoteRewardContext> mapCacheCalcVote; // key: day end block hash, value: a day of voting data
    CTrieDB dbTrie;

    mtbase::CCache<uint256, std::map<int, std::map<CDestination, CDiskPos>>> cacheDelegate;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_VOTEDB_H

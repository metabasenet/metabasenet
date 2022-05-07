// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DELEGATEDB_H
#define STORAGE_DELEGATEDB_H

#include <map>

#include "destination.h"
#include "hcode.h"
#include "timeseries.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CDelegateContext
{
    friend class hcode::CStream;

public:
    CDelegateContext() {}

    uint256 hashVoteTrieRoot;
    std::map<int, std::map<CDestination, CDiskPos>> mapEnrollTx;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(hashVoteTrieRoot, opt);
        s.Serialize(mapEnrollTx, opt);
    }
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

class CDelegateDB : public hcode::CKVDB
{
public:
    CDelegateDB()
      : cacheDelegate(MAX_CACHE_COUNT) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();
    bool AddNewDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                        const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot);
    bool Remove(const uint256& hashBlock);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote);
    bool RetrieveDelegatedVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote);
    bool RetrieveDelegatedEnrollTx(const uint256& hashBlock, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos);
    bool RetrieveEnrollTx(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);

    bool VerifyDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool Retrieve(const uint256& hashBlock, CDelegateContext& ctxtDelegate);
    bool GetDestVote(const uint256& hashTrieRoot, const CDestination& dest, uint256& nVote);
    bool AddDelegateVote(const uint256& hashPrevRoot, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote, uint256& hashNewRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    enum
    {
        MAX_CACHE_COUNT = 64,
    };
    hcode::CCache<uint256, CDelegateContext> cacheDelegate;
    CTrieDB dbVoteTrie;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_DELEGATEDB_H

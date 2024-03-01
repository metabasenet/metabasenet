// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_STATEDB_H
#define STORAGE_STATEDB_H

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

class CListStateTrieDBWalker : public CTrieDBWalker
{
public:
    CListStateTrieDBWalker(std::map<CDestination, CDestState>& mapStateIn)
      : mapState(mapStateIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CDestState>& mapState;
};

class CForkStateDB
{
public:
    CForkStateDB(const bool fCacheIn = true);
    ~CForkStateDB();
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();
    bool AddBlockState(const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool VerifyState(const uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    void AddPrevRoot(const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, CBlockRootStatus& statusBlockRoot);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    mtbase::CRWAccess rwAccess;
    std::map<uint256, std::map<CDestination, CDestState>> mapCacheState;
    std::map<uint256, std::map<uint256, CTrieValue>> mapCacheTrie;
    std::map<uint64, uint256> mapCacheIndex;
    CTrieDB dbTrie;
};

class CStateDB
{
public:
    CStateDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool VerifyState(const uint256& hashFork, const uint256& hashRoot, const bool fVerifyAllNode = true);

    static bool CreateStaticStateRoot(const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashStateRoot);

protected:
    bool fCache;
    boost::filesystem::path pathState;
    mtbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkStateDB>> mapStateDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_STATEDB_H

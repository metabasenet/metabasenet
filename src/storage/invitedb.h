// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_INVITEDB_H
#define STORAGE_INVITEDB_H

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

class CListInviteTrieDBWalker : public CTrieDBWalker
{
public:
    CListInviteTrieDBWalker(std::map<CDestination, CDestination>& mapInviteIn)
      : mapInvite(mapInviteIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CDestination>& mapInvite; // key: sub, value: parent
};

class CForkInviteDB
{
public:
    CForkInviteDB(const bool fCacheIn = true);
    ~CForkInviteDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddInviteRelation(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CDestination>& mapInviteContext, uint256& hashNewRoot);
    bool RetrieveInviteParent(const uint256& hashBlock, const CDestination& destSub, CDestination& destParent);
    bool ListInviteRelation(const uint256& hashBlock, std::map<CDestination, CDestination>& mapInviteContext);

    bool CheckInviteContext(const std::vector<uint256>& vCheckBlock);
    bool CheckInviteContext(const uint256& hashLastBlock, const uint256& hashLastRoot);
    bool VerifyInviteContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    bool fCache;
    uint256 hashFork;
    CTrieDB dbTrie;
};

class CInviteDB
{
public:
    CInviteDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddInviteRelation(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CDestination>& mapInviteContext, uint256& hashNewRoot);
    bool RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CDestination& destParent);
    bool ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestination>& mapInviteContext);

    bool CheckInviteContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock);
    bool CheckInviteContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot);
    bool VerifyInviteContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathStorage;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkInviteDB>> mapInviteDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_INVITEDB_H

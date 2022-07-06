// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VOTEREDEEMDB_H
#define STORAGE_VOTEREDEEMDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "hcode.h"
#include "timeseries.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

////////////////////////////////////////////
// CVoteRedeemDB

class CVoteRedeemDB
{
public:
    CVoteRedeemDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();
    bool AddBlockVoteRedeem(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteRedeemContext>& mapBlockVoteRedeem, uint256& hashVoteRedeemRoot);
    bool RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destRedeem, CVoteRedeemContext& ctxtVoteRedeem);
    bool VerifyVoteRedeem(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    CTrieDB dbTrie;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_VOTEREDEEMDB_H

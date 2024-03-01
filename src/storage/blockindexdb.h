// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKINDEXDB_H
#define STORAGE_BLOCKINDEXDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "mtbase.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CBlockDBWalker
{
public:
    virtual bool Walk(CBlockOutline& outline) = 0;
};

class CBlockIndexDB
{
public:
    CBlockIndexDB();
    ~CBlockIndexDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool Clear();

    bool AddNewBlockIndex(const CBlockOutline& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);

    bool AddBlockNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint64 nBlockNumber, const uint256& hashBlock, uint256& hashNewRoot);
    bool RetrieveBlockHashByNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashLastBlock, const uint64 nBlockNumber, uint256& hashBlock);
    bool VerifyBlockNumberContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    CTrieDB dbTrie;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_BLOCKINDEXDB_H

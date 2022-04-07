// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKINDEXDB_H
#define STORAGE_BLOCKINDEXDB_H

#include "block.h"
#include "hnbase.h"

namespace metabasenet
{
namespace storage
{

class CBlockDBWalker
{
public:
    virtual bool Walk(CBlockOutline& outline) = 0;
};

class CBlockIndexDB : public hnbase::CKVDB
{
public:
    CBlockIndexDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddNewBlockIndex(const CBlockOutline& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);

protected:
    bool LoadBlockWalker(hnbase::CBufStream& ssKey, hnbase::CBufStream& ssValue, CBlockDBWalker& walker);
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_BLOCKINDEXDB_H

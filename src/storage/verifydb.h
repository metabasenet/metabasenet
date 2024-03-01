// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_VERIFYDB_H
#define STORAGE_VERIFYDB_H

#include "block.h"
#include "mtbase.h"
#include "timeseries.h"

namespace metabasenet
{
namespace storage
{

class CVerifyDB
{
public:
    CVerifyDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddBlockVerify(const CBlockOutline& outline, const uint32 nRootCrc);
    bool RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock);
    std::size_t GetBlockVerifyCount();
    bool GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock);

protected:
    bool LoadVerifyDB();

protected:
    CTimeSeriesCached tsVerify;

    mutable boost::shared_mutex rwAccess;
    std::map<uint256, std::size_t> mapVerifyIndex;
    std::vector<CBlockVerify> vVerifyIndex;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_VERIFYDB_H

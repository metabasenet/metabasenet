// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_WASMDB_H
#define STORAGE_WASMDB_H

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

class CForkWasmDB
{
public:
    CForkWasmDB(const bool fCacheIn = true);
    ~CForkWasmDB();
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();
    bool AddBlockWasmState(const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState);
    bool RetrieveWasmState(const uint256& hashWasmRoot, const uint256& key, bytes& value);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    hcode::CRWAccess rwAccess;
    CTrieDB dbTrie;
};

class CWasmDB
{
public:
    CWasmDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState);
    bool RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value);

protected:
    bool fCache;
    boost::filesystem::path pathWasm;
    hcode::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkWasmDB>> mapWasmDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_WASMDB_H

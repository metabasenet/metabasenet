// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_CODEDB_H
#define STORAGE_CODEDB_H

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

class CListCreateCodeTrieDBWalker : public CTrieDBWalker
{
public:
    CListCreateCodeTrieDBWalker(std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCodeIn)
      : mapWasmCreateCode(mapWasmCreateCodeIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode;
};

class CForkCodeDB
{
public:
    CForkCodeDB(const bool fCacheIn = true);
    ~CForkCodeDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                        const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode);
    bool RetrieveWasmCreateCodeContext(const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode);
    bool RetrieveWasmRunCodeContext(const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode);
    bool ListWasmCreateCodeContext(const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode);

    bool VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    uint256 hashFork;
    CTrieDB dbTrie;
};

class CCodeDB
{
public:
    CCodeDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                        const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode);
    bool RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode);
    bool RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode);
    bool ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode);

    bool VerifyCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    hnbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkCodeDB>> mapCodeDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_CODEDB_H

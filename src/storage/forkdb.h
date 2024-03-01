// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_FORKDB_H
#define STORAGE_FORKDB_H

#include <map>

#include "block.h"
#include "forkcontext.h"
#include "mtbase.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CListForkTrieDBWalker : public CTrieDBWalker
{
public:
    CListForkTrieDBWalker(std::map<uint256, CForkContext>& mapForkCtxtIn)
      : mapForkCtxt(mapForkCtxtIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint256, CForkContext>& mapForkCtxt;
};

class CCacheFork
{
public:
    CCacheFork() {}
    CCacheFork(const std::map<uint256, CForkContext>& mapForkCtxtIn)
    {
        hashRef = 0;
        AddForkCtxt(mapForkCtxtIn);
    }
    CCacheFork(const uint256& hashRefIn)
      : hashRef(hashRefIn)
    {
    }
    void AddForkCtxt(const std::map<uint256, CForkContext>& mapForkCtxtIn);
    bool ExistForkContext(const uint256& hashFork) const;
    uint256 GetForkHashByName(const std::string& strName) const;
    uint256 GetForkHashByChainId(const CChainId nChainId) const;

public:
    uint256 hashRef;
    std::map<uint256, CForkContext> mapForkContext;
    std::map<std::string, uint256> mapForkName;
    std::map<CChainId, uint256> mapChainId;
    std::multimap<uint256, uint256> mapForkParent;
    std::multimap<uint256, uint256> mapForkJoint;
};

class CForkDB
{
public:
    CForkDB() {}
    bool Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn);
    void Deinitialize();
    void Clear();

    bool AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, uint256& hashNewRoot);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256());

    bool UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock);
    bool RemoveFork(const uint256& hashFork);
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);

    bool GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());

    bool VerifyForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool GetForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool AddCacheForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapNewForkCtxt);
    const CCacheFork* GetCacheForkContext(const uint256& hashBlock);
    const CCacheFork* LoadCacheForkContext(const uint256& hashBlock);
    bool ListDbForkContext(const uint256& hashBlock, std::map<uint256, CForkContext>& mapForkCtxt);

protected:
    enum
    {
        MAX_CACHE_CONTEXT_COUNT = 10240,
        MAX_CACHE_ROOT_COUNT = 10240
    };

    uint256 hashGenesisBlock;
    CTrieDB dbTrie;

    mtbase::CRWAccess rwAccess;
    std::map<uint256, CCacheFork> mapCacheFork;
    std::map<uint256, uint256> mapCacheLast;
    std::map<uint256, uint256> mapCacheRoot;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_FORKDB_H

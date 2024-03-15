// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSDB_H
#define STORAGE_ADDRESSDB_H

#include <map>

#include "destination.h"
#include "mtbase.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

class CListContractAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListContractAddressTrieDBWalker(std::map<CDestination, CContractAddressContext>& mapContractAddressIn)
      : mapContractAddress(mapContractAddressIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<CDestination, CContractAddressContext>& mapContractAddress;
};

class CListFunctionAddressTrieDBWalker : public CTrieDBWalker
{
public:
    CListFunctionAddressTrieDBWalker(std::map<uint32, CFunctionAddressContext>& mapFunctionAddressIn)
      : mapFunctionAddress(mapFunctionAddressIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint32, CFunctionAddressContext>& mapFunctionAddress;
};

class CForkAddressDB
{
public:
    CForkAddressDB(const bool fCacheIn = true);
    ~CForkAddressDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot);
    bool RetrieveAddressContext(const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool GetAddressCount(const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool RetrieveBlsPubkeyContext(const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey);

    bool CheckAddressContext(const std::vector<uint256>& vCheckBlock);
    bool CheckAddressContext(const uint256& hashLastBlock, const uint256& hashLastRoot);
    bool VerifyAddressContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    bool AddAddressCount(const uint256& hashBlock, const uint64 nAddressCount, const uint64 nNewAddressCount);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    uint256 hashFork;
    CTrieDB dbTrie;
};

class CAddressDB
{
public:
    CAddressDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, const std::map<CDestination, uint384>& mapBlsPubkeyContext, uint256& hashNewRoot);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey);

    bool CheckAddressContext(const uint256& hashFork, const std::vector<uint256>& vCheckBlock);
    bool CheckAddressContext(const uint256& hashFork, const uint256& hashLastBlock, const uint256& hashLastRoot);
    bool VerifyAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    mtbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressDB>> mapAddressDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_ADDRESSDB_H

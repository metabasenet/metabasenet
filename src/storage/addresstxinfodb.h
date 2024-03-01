// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSTXINFODB_H
#define STORAGE_ADDRESSTXINFODB_H

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

class CListAddressTxInfoTrieDBWalker : public CTrieDBWalker
{
public:
    CListAddressTxInfoTrieDBWalker(const uint64 nGetTxCountIn, std::vector<CDestTxInfo>& vAddressTxInfoIn)
      : nGetTxCount(nGetTxCountIn), vAddressTxInfo(vAddressTxInfoIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

protected:
    const uint64 nGetTxCount;
    std::vector<CDestTxInfo>& vAddressTxInfo;
};

class CForkAddressTxInfoDB
{
public:
    CForkAddressTxInfoDB(const bool fCacheIn = true);
    ~CForkAddressTxInfoDB();

    bool Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool VerifyAddressTxInfo(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);
    void WriteAddressLast(const CDestination& dest, const uint64 nTxCount, bytesmap& mapKv);
    bool ReadAddressLast(const uint256& hashRoot, const CDestination& dest, uint64& nTxCount);

protected:
    enum
    {
        MAX_CACHE_COUNT = 16
    };
    bool fCache;
    uint256 hashFork;
    CTrieDB dbTrie;
};

class CAddressTxInfoDB
{
public:
    CAddressTxInfoDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool VerifyAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool fCache;
    boost::filesystem::path pathAddress;
    mtbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressTxInfoDB>> mapAddressTxInfoDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_ADDRESSTXINFODB_H

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_CONTRACTDB_H
#define STORAGE_CONTRACTDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "mtbase.h"
#include "timeseries.h"
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
    CListCreateCodeTrieDBWalker(std::map<uint256, CContractCreateCodeContext>& mapWasmCreateCodeIn)
      : mapContractCreateCode(mapWasmCreateCodeIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override;

public:
    std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode;
};

class CForkContractDB
{
public:
    CForkContractDB(const uint256& hashForkIn);
    ~CForkContractDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool RemoveAll();

    bool AddBlockContractKvValue(const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState);
    bool RetrieveContractKvValue(const uint256& hashContractRoot, const uint256& key, bytes& value);

    bool AddCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                        const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                        const std::map<uint256, CTemplateContext>& mapTemplateData,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode);
    bool RetrieveContractCreateCodeContext(const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveContractRunCodeContext(const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode);
    bool ListContractCreateCodeContext(const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode);
    bool VerifyCodeContext(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

protected:
    const uint256 hashFork;
    CTrieDB dbTrie;
};

class CContractDB
{
public:
    CContractDB() {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();

    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();

    bool AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    static bool CreateStaticContractStateRoot(const std::map<uint256, bytes>& mapContractState, uint256& hashStateRoot);

    bool AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                        const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                        const std::map<uint256, CTemplateContext>& mapTemplateData,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode);
    bool RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode);
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode);
    bool VerifyCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    boost::filesystem::path pathContract;
    mtbase::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkContractDB>> mapContractDB;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_CONTRACTDB_H

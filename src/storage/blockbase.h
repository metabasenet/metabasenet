// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKBASE_H
#define STORAGE_BLOCKBASE_H

#include <boost/range/adaptor/reversed.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <map>
#include <numeric>

#include "block.h"
#include "blockdb.h"
#include "forkcontext.h"
#include "hcode.h"
#include "param.h"
#include "profile.h"
#include "timeseries.h"
#include "wasmbase.h"

namespace metabasenet
{
namespace storage
{

class CBlockBase;

class CBlockHeightIndex
{
public:
    CBlockHeightIndex()
      : nTimeStamp(0) {}
    CBlockHeightIndex(uint32 nTimeStampIn, CDestination destMintIn, const uint256& hashRefBlockIn)
      : nTimeStamp(nTimeStampIn), destMint(destMintIn), hashRefBlock(hashRefBlockIn) {}

public:
    uint32 nTimeStamp;
    CDestination destMint;
    uint256 hashRefBlock;
};

class CForkHeightIndex
{
public:
    CForkHeightIndex() {}

    void AddHeightIndex(uint32 nHeight, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock);
    void RemoveHeightIndex(uint32 nHeight, const uint256& hashBlock);
    void UpdateBlockRef(int nHeight, const uint256& hashBlock, const uint256& hashRefBlock);
    std::map<uint256, CBlockHeightIndex>* GetBlockMintList(uint32 nHeight);
    bool GetMaxHeight(uint32& nMaxHeight);

protected:
    std::map<uint32, std::map<uint256, CBlockHeightIndex>> mapHeightIndex;
};

class CCacheBlockState
{
public:
    CCacheBlockState() {}
    CCacheBlockState(const uint256& hashPrevRootIn, const uint256& hashBlockRootIn,
                     const std::map<CDestination, CDestState>& mapAccStateIn,
                     const std::map<uint256, CAddressContext>& mapBlockStateAddressContextIn,
                     const std::map<uint256, CWasmRunCodeContext>& mapBlockStateWasmRunCodeContextIn,
                     const std::map<uint256, std::vector<CContractTransfer>>& mapBlockStateContractTransferIn,
                     const std::map<uint256, uint256>& mapBlockStateTxFeeUsedIn,
                     const std::map<uint256, std::map<CDestination, uint256>>& mapBlockStateCodeDestFeeUsedIn)
      : hashPrevRoot(hashPrevRootIn), hashBlockRoot(hashBlockRootIn), mapAccState(mapAccStateIn),
        mapBlockStateAddressContext(mapBlockStateAddressContextIn), mapBlockStateWasmRunCodeContext(mapBlockStateWasmRunCodeContextIn),
        mapBlockStateContractTransfer(mapBlockStateContractTransferIn), mapBlockStateTxFeeUsed(mapBlockStateTxFeeUsedIn), mapBlockStateCodeDestFeeUsed(mapBlockStateCodeDestFeeUsedIn) {}

public:
    uint256 hashPrevRoot;
    uint256 hashBlockRoot;
    std::map<CDestination, CDestState> mapAccState;
    std::map<uint256, CAddressContext> mapBlockStateAddressContext;
    std::map<uint256, CWasmRunCodeContext> mapBlockStateWasmRunCodeContext;
    std::map<uint256, std::vector<CContractTransfer>> mapBlockStateContractTransfer;
    std::map<uint256, uint256> mapBlockStateTxFeeUsed;
    std::map<uint256, std::map<CDestination, uint256>> mapBlockStateCodeDestFeeUsed;
};

class CBlockBase;

class CBlockState
{
public:
    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CBlock& block, const uint256& hashPrevStateRootIn)
      : dbBlockBase(dbBlockBaseIn), hashFork(hashForkIn), hashBlock(block.GetHash()), hashPrevBlock(block.hashPrev),
        hashPrevStateRoot(hashPrevStateRootIn), nOriBlockGasLimit(block.nGasLimit.Get64()), nSurplusBlockGasLimit(block.nGasLimit.Get64()), hashMintDest(block.txMint.to.data),
        nBlockTimestamp(block.nTimeStamp), nBlockHeight(block.GetBlockHeight()), nBlockNumber(block.GetBlockNumber()), destMint(block.txMint.to),
        nMintReward(block.txMint.nAmount), fPrimaryBlock(block.IsPrimary()), btBlockProof(block.vchProof) {}

    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const uint256& hashPrevBlockIn, const uint256& hashPrevStateRootIn)
      : dbBlockBase(dbBlockBaseIn), hashFork(hashForkIn), hashPrevBlock(hashPrevBlockIn), hashPrevStateRoot(hashPrevStateRootIn),
        nOriBlockGasLimit(0), nSurplusBlockGasLimit(0), nBlockTimestamp(0), nBlockHeight(0), nBlockNumber(0), fPrimaryBlock(false) {}

    bool AddTxState(const CTransaction& tx, const int nTxIndex);
    bool DoBlockState(uint256& hashReceiptRoot, uint256& nBlockGasUsed, uint2048& nBlockBloomOut, const bool fCreateBlock,
                      const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut);

    bool GetDestState(const CDestination& dest, CDestState& stateDest);
    void SetDestState(const CDestination& dest, const CDestState& stateDest);
    void SetCacheDestState(const CDestination& dest, const CDestState& stateDest);
    bool GetDestKvData(const CDestination& dest, const uint256& key, bytes& value);
    bool RetrieveAddressContext(const uint256& dest, CAddressContext& ctxtAddress);
    bool GetWasmRunCode(const CDestination& destWasm, uint256& hashWasmCreateCode, CDestination& destCodeOwner, bytes& btWasmRunCode);
    bool GetWasmCreateCode(const CDestination& destWasm, CTxContractData& txcd);

    bool SaveWasmRunCode(const CDestination& destWasm, const bytes& btWasmRunCode, const CTxContractData& txcd);
    void SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed);
    void SaveRunResult(const CDestination& destWasm, const CTransactionLogs& logs, const std::map<uint256, bytes>& mapCacheKv);
    bool ContractTransfer(const uint256& from, const uint256& to, const uint256& amount);

protected:
    bool GetDestWasmCode(const CTransaction& tx, CDestination& destWasm, bytes& btWasmCode, bytes& btRunParam, uint256& hashWasmCreateCode, CDestination& destCodeOwner, CTxContractData& txcd, bool& fCall);

    bool AddWasmState(const uint256& txid, const CTransaction& tx, const int nTxIndex, bool& fCallResult);
    bool DoRunResult(const uint256& txid, const CTransaction& tx, const int nTxIndex, const CDestination& destWasm, const uint256& hashWasmCreateCode,
                     const uint64 nGasLeftIn, const int nStatusCode, const bytes& vResult, const CTransactionLogs& logs);

protected:
    CBlockBase& dbBlockBase;
    const uint256 hashFork;
    const uint256 hashBlock;
    const uint256 hashPrevBlock;
    const uint256 hashPrevStateRoot;
    const uint64 nOriBlockGasLimit;
    const uint256 hashMintDest;
    const uint32 nBlockTimestamp;
    const int nBlockHeight;
    const uint64 nBlockNumber;
    const CDestination destMint;
    const uint256 nMintReward;
    const bool fPrimaryBlock;
    const bytes btBlockProof;

    class CCacheWasm
    {
    public:
        CCacheWasm() {}

    public:
        CDestState cacheDestState;
        std::vector<CTransactionLogs> cacheWasmLogs;
        std::map<uint256, bytes> cacheWasmKv;
    };
    std::map<CDestination, CCacheWasm> mapCacheWasmData;
    std::map<uint256, CAddressContext> mapCacheAddressContext;
    std::map<uint256, CWasmRunCodeContext> mapCacheWasmRunCodeContext;
    std::vector<CContractTransfer> vCacheContractTransfer;
    std::map<CDestination, uint64> mapCacheCodeDestGasUsed;

    uint64 nSurplusBlockGasLimit;

public:
    std::map<CDestination, CDestState> mapBlockState;
    std::map<CDestination, std::map<uint256, bytes>> mapWasmKvState;
    std::map<uint256, CAddressContext> mapBlockAddressContext;
    std::map<uint256, CWasmRunCodeContext> mapBlockWasmRunCodeContext;
    std::map<uint256, std::vector<CContractTransfer>> mapBlockContractTransfer; // key is txid
    std::map<uint256, uint256> mapBlockTxFeeUsed;
    std::map<uint256, std::map<CDestination, uint256>> mapBlockCodeDestFeeUsed; // key is txid
    std::vector<uint256> vReceiptHash;
    uint2048 nBlockBloom;
    uint256 nBlockFeeLeft;
};

class CWasmHostDB : public CHostBaseDB
{
public:
    CWasmHostDB(CBlockState& blockStateIn, const CDestination& destWasmIn)
      : blockState(blockStateIn), destWasm(destWasmIn) {}

    virtual bool Get(const uint256& key, bytes& value) const;
    virtual bool GetBalance(const uint256& addr, uint256& balance) const;
    virtual bool Transfer(const uint256& from, const uint256& to, const uint256& amount);

    virtual bool GetWasmRunCode(const uint256& hashWasm, uint256& hashWasmCreateCode, CDestination& destCodeOwner, bytes& btWasmRunCode);
    virtual bool GetWasmCreateCode(const uint256& hashWasm, CTxContractData& txcd);
    virtual CHostBaseDBPtr CloneHostDB(const uint256& hashWasm);
    virtual void SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed);
    virtual void SaveRunResult(const uint256& hashWasm, const CTransactionLogs& logs, const std::map<uint256, bytes>& mapCacheKv);
    virtual bool SaveWasmRunCode(const uint256& hashWasm, const bytes& btWasmRunCode, const CTxContractData& txcd);

protected:
    CBlockState& blockState;
    CDestination destWasm;
};

class CBlockBase
{
public:
    CBlockBase();
    ~CBlockBase();
    bool Initialize(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fRenewDB = false);
    void Deinitialize();
    void Clear();
    bool IsEmpty() const;
    bool Exists(const uint256& hash) const;
    bool ExistsTx(const uint256& txid);
    bool Initiate(const uint256& hashGenesis, const CBlock& blockGenesis, const uint256& nChainTrust);
    bool StorageNewBlock(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, CBlockChainUpdate& update);
    bool SaveBlock(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, CBlockIndex** ppIndexNew, CBlockRoot& blockRoot, const bool fRepair);
    bool CheckForkLongChain(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const CBlockIndex* pIndexNew);
    bool Retrieve(const CBlockIndex* pIndex, CBlock& block);
    bool Retrieve(const uint256& hash, CBlockEx& block);
    bool Retrieve(const CBlockIndex* pIndex, CBlockEx& block);
    bool RetrieveIndex(const uint256& hashBlock, CBlockIndex** ppIndex);
    bool RetrieveFork(const uint256& hashFork, CBlockIndex** ppIndex);
    bool RetrieveFork(const std::string& strName, CBlockIndex** ppIndex);
    bool RetrieveProfile(const uint256& hashFork, CProfile& profile);
    bool RetrieveAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry);
    bool RetrieveOrigin(const uint256& hashFork, CBlock& block);
    bool RetrieveTx(const uint256& txid, CTransaction& tx);
    bool RetrieveTx(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight);
    bool RetrieveTx(const uint256& hashFork, const uint256& txid, CTxIndex& txIndex, CTransaction& tx);
    bool RetrieveTx(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTransaction& tx);
    bool RetrieveTx(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex, CTransaction& tx);
    bool RetrieveTxLocation(const uint256& txid, uint256& hashFork, int& nHeight);
    bool RetrieveAvailDelegate(const uint256& hash, int height, const std::vector<uint256>& vBlockRange,
                               const uint256& nMinEnrollAmount,
                               std::map<CDestination, std::size_t>& mapWeight,
                               std::map<CDestination, std::vector<unsigned char>>& mapEnrollData,
                               std::vector<std::pair<CDestination, uint256>>& vecAmount);
    void ListForkIndex(std::map<uint256, CBlockIndex*>& mapForkIndex);
    void ListForkIndex(std::multimap<int, CBlockIndex*>& mapForkIndex);
    bool GetBlockBranchList(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool GetBlockBranchListNolock(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool LoadIndex(CBlockOutline& diskIndex);
    bool LoadTx(const uint32 nTxFile, const uint32 nTxOffset, CTransaction& tx);
    bool AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, uint256& hashNewRoot);
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt);
    bool VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashBlock = uint256());
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkIdByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashBlock = uint256());
    bool GetForkIdByForkSn(const uint16 nForkSn, uint256& hashFork, const uint256& hashBlock = uint256());
    int GetForkCreatedHeight(const uint256& hashFork);
    bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight);

    bool GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep);
    bool GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, size_t nMaxCount);

    bool GetVotes(const uint256& hashGenesis, const CDestination& destDelegate, uint256& nVotes);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destRedeem, CVoteRedeemContext& ctxtVoteRedeem);
    bool GetDelegateList(const uint256& hashGenesis, uint32 nCount, std::multimap<uint256, CDestination>& mapVotes);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation);
    bool VerifyRepeatBlock(const uint256& hashFork, uint32 height, const CDestination& destMint, uint16 nBlockType,
                           uint32 nBlockTimeStamp, uint32 nRefBlockTimeStamp, uint32 nExtendedBlockSpacing);
    bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote);
    bool GetDelegateEnrollTx(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool GetBlockDelegatedEnrollTx(const uint256& hashBlock, std::map<int, std::set<CDestination>>& mapEnrollDest);
    bool VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock);
    CBlockIndex* GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork);
    bool VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock);
    bool GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin);
    bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime);
    bool VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime);
    bool UpdateForkNext(const uint256& hashFork, CBlockIndex* pIndexLast);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, const uint256& hashPrevStateRoot, uint256& hashStateRoot, uint256& hashReceiptRoot, uint256& nBlockGasUsed,
                              uint2048& nBlockBloom, const bool fCreateBlock, const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut);
    bool UpdateBlockState(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewStateRoot,
                          std::map<uint256, CAddressContext>& mapAddressContextOut, std::map<uint256, CWasmRunCodeContext>& mapWasmRunCodeContextOut,
                          std::map<uint256, std::vector<CContractTransfer>>& mapBlockContractTransferOut, std::map<uint256, uint256>& mapBlockTxFeeUsedOut,
                          std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedOut);
    bool UpdateBlockTxIndex(const uint256& hashFork, const CBlockEx& block, const uint32 nFile, const uint32 nOffset, uint256& hashNewRoot);
    bool UpdateBlockAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block,
                                  const std::map<uint256, std::vector<CContractTransfer>>& mapContractTransferIn,
                                  const std::map<uint256, uint256>& mapBlockTxFeeUsedIn, const std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedIn,
                                  uint256& hashNewRoot);
    bool UpdateBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<uint256, CAddressContext>& mapAddressContextIn, uint256& hashNewRoot);
    bool UpdateBlockCode(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nBlockOffset,
                         const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCodeContextIn, uint256& hashCodeRoot);
    bool UpdateBlockInvite(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewRoot);
    bool RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value);
    bool AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress);
    bool CallWasmCode(const uint256& hashFork, const uint32 nHeight, const CDestination& destMint, const uint256& nBlockGasLimit,
                      const CDestination& from, const CDestination& to, const uint256& nGasPrice, const uint256& nGasLimit, const uint256& nAmount,
                      const bytes& data, const uint32 nTimeStamp, const uint256& hashPrevBlock, const uint256& hashPrevStateRoot, int& nStatus, bytes& btResult);
    bool GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate);
    bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode);
    bool GetBlockWasmCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CTxContractData& txcdCode);
    bool RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode);
    bool RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode);
    bool GetWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CContractCodeContext& ctxtContractCode);
    bool ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode);
    bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract);
    bool VerifyCreateContractTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestState>& mapBlockState);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);
    bool RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CDestination& destParent);
    bool ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestination>& mapInviteContext);

protected:
    CBlockIndex* GetIndex(const uint256& hash) const;
    CBlockIndex* GetForkLastIndex(const uint256& hashFork);
    CBlockIndex* GetOrCreateIndex(const uint256& hash);
    CBlockIndex* GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, std::vector<CBlockIndex*>& vPath);
    CBlockIndex* GetOriginIndex(const uint256& txidMint);
    void UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock);
    void RemoveBlockIndex(const uint256& hashFork, const uint256& hashBlock);
    void UpdateBlockRef(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock);
    void UpdateBlockNext(CBlockIndex* pIndexLast);
    CBlockIndex* AddNewIndex(const uint256& hash, const CBlock& block, const uint32 nFile, const uint32 nOffset, const uint32 nCrc, const uint256& nChainTrust, const uint256& hashNewStateRoot);
    bool LoadForkProfile(const CBlockIndex* pIndexOrigin, CProfile& profile);
    bool UpdateDelegate(const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                        const uint256& nMinEnrollAmount, uint256& hashDelegateRoot);
    bool UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, uint256& hashVoteRoot);
    bool UpdateVoteRedeem(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, uint256& hashVoteRedeemRoot);
    bool IsValidBlock(CBlockIndex* pForkLast, const uint256& hashBlock);
    bool VerifyValidBlock(CBlockIndex* pIndexGenesisLast, const CBlockIndex* pIndex);
    CBlockIndex* GetLongChainLastBlock(const uint256& hashFork, int nStartHeight, CBlockIndex* pIndexGenesisLast, const std::set<uint256>& setInvalidHash);
    bool GetBlockDelegateAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CDestination>& mapDelegateAddress);
    bool GetTxIndex(const uint256& txid, CTxIndex& txIndex, uint256& hashAtFork);
    bool GetTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex);
    void ClearCache();
    bool LoadDB();
    bool VerifyDB();
    bool VerifyBlockDB(const CBlockVerify& verifyBlock, CBlockOutline& outline, CBlockRoot& blockRoot, const bool fVerify);
    bool RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockIndex** ppIndexNew);
    bool LoadBlockIndex(CBlockOutline& outline, CBlockIndex** ppIndexNew);

protected:
    enum
    {
        MAX_CACHE_BLOCK_STATE = 64
    };

    mutable hcode::CRWAccess rwAccess;
    bool fCfgFullDb;
    uint256 hashGenesisBlock;
    CBlockDB dbBlock;
    CTimeSeriesCached tsBlock;
    std::map<uint256, CBlockIndex*> mapIndex;
    std::map<uint256, CForkHeightIndex> mapForkHeightIndex;
    std::map<uint256, CCacheBlockState> mapCacheBlockState;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_BLOCKBASE_H

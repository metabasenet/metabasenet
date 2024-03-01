// Copyright (c) 2022-2024 The MetabaseNet developers
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

#include "../mvm/vface/vmhostface.h"
#include "block.h"
#include "blockdb.h"
#include "forkcontext.h"
#include "mtbase.h"
#include "param.h"
#include "profile.h"
#include "timeseries.h"

using namespace metabasenet::mvm;

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
    CBlockHeightIndex(const uint64 nTimeStampIn, const CDestination& destMintIn, const uint256& hashRefBlockIn)
      : nTimeStamp(nTimeStampIn), destMint(destMintIn), hashRefBlock(hashRefBlockIn) {}

public:
    uint64 nTimeStamp;
    CDestination destMint;
    uint256 hashRefBlock;
};

class CForkHeightIndex
{
public:
    CForkHeightIndex() {}

    void AddHeightIndex(const uint32 nHeight, const uint256& hashBlock, const uint64 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock);
    void RemoveHeightIndex(const uint32 nHeight, const uint256& hashBlock);
    void UpdateBlockRef(const uint32 nHeight, const uint256& hashBlock, const uint256& hashRefBlock);
    std::map<uint256, CBlockHeightIndex>* GetBlockMintList(const uint32 nHeight);
    bool GetMaxHeight(uint32& nMaxHeight);

protected:
    std::map<uint32, std::map<uint256, CBlockHeightIndex>> mapHeightIndex;
};

//////////////////////////////
// CBlockLogsFilter

class CBlockLogsFilter
{
public:
    CBlockLogsFilter(const uint256& hashForkIn, const CLogsFilter& logFilterIn)
      : hashFork(hashForkIn), logFilter(logFilterIn), nLogsCount(0), nHisLogsCount(0), nPrevGetChangesTime(mtbase::GetTime()) {}

    bool isTimeout();

    bool AddTxReceipt(const uint256& hashForkIn, const uint64 nBlockNumber, const uint256& hashBlock, const uint256& txid, const CTransactionReceipt& receipt);
    void GetTxReceiptLogs(const bool fAll, ReceiptLogsVec& receiptLogs);

protected:
    const uint256 hashFork;
    const CLogsFilter logFilter;

    int64 nPrevGetChangesTime;
    uint64 nLogsCount;
    uint64 nHisLogsCount;

    ReceiptLogsVec vReceiptLogs;
    ReceiptLogsVec vHisReceiptLogs;
};

//////////////////////////////
// CBlockMakerFilter

class CBlockMakerFilter
{
public:
    CBlockMakerFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(mtbase::GetTime()) {}

    bool isTimeout();
    bool AddBlockHash(const uint256& hashForkIn, const uint256& hashBlock, const uint256& hashPrev);
    bool GetFilterBlockHashs(const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockhash);

protected:
    const uint256 hashFork;
    int64 nPrevGetChangesTime;

    std::map<uint256, uint256> mapBlockHash;
    std::map<uint256, uint256> mapHisBlockHash;
};

//////////////////////////////
// CBlockPendingTxFilter

class CBlockPendingTxFilter
{
public:
    CBlockPendingTxFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(mtbase::GetTime()), nSeqCreate(0) {}

    bool isTimeout();
    bool AddPendingTx(const uint256& hashForkIn, const uint256& txid);
    bool GetFilterTxids(const uint256& hashForkIn, const bool fAll, std::vector<uint256>& vTxid);

protected:
    const uint256 hashFork;
    int64 nPrevGetChangesTime;
    uint64 nSeqCreate;

    std::set<uint256> setTxid;
    std::set<uint256> setHisTxid;
    std::map<uint64, uint256> mapHisSeq;
};

//////////////////////////////
// CBlockFilter

class CBlockFilter
{
public:
    CBlockFilter() {}

    void RemoveFilter(const uint256& nFilterId);

    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn);
    void AddTxReceipt(const uint256& hashForkIn, const uint64 nBlockNumber, const uint256& hashBlock, const uint256& txid, const CTransactionReceipt& receipt);
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs);

    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork);
    void AddNewBlockHash(const uint256& hashForkIn, const uint256& hashBlock, const uint256& hashPrev);
    bool GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash);

    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork);
    void AddPendingTx(const uint256& hashFork, const uint256& txid);
    bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid);

protected:
    boost::shared_mutex mutexFilter;
    CFilterId createFilterId;

    std::map<uint256, CBlockLogsFilter> mapLogFilter;
    std::map<uint256, CBlockMakerFilter> mapBlockFilter;
    std::map<uint256, CBlockPendingTxFilter> mapTxFilter;
};

//////////////////////////////
// CBlockState

class CBlockBase;

class CBlockState
{
public:
    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn, const std::map<CDestination, CAddressContext>& mapAddressContext)
      : dbBlockBase(dbBlockBaseIn), nBlockType(block.nType), hashFork(hashForkIn), hashPrevBlock(block.hashPrev),
        hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn), nOriBlockGasLimit(block.nGasLimit.Get64()), nSurplusBlockGasLimit(block.nGasLimit.Get64()),
        nBlockTimestamp(block.GetBlockTime()), nBlockHeight(block.GetBlockHeight()), nBlockNumber(block.GetBlockNumber()), destMint(block.txMint.GetToAddress()),
        mintTx(block.txMint), txidMint(block.txMint.GetHash()), fPrimaryBlock(block.IsPrimary()), mapBlockAddressContext(mapAddressContext)
    {
        if (fPrimaryBlock)
        {
            hashRefBlock = 0;
            if (block.IsProofOfWork())
            {
                CProofOfHashWork proof;
                if (block.GetHashWorkProof(proof))
                {
                    uint256 hashTarget = (~uint256(uint64(0)) >> proof.nBits);
                    nAgreement = hashTarget + proof.nNonce;
                }
            }
            else
            {
                CProofOfDelegate proof;
                if (block.GetDelegateProof(proof))
                {
                    nAgreement = proof.nAgreement;
                }
            }
        }
        else
        {
            hashRefBlock = 0;
            CProofOfPiggyback proof;
            if (block.GetPiggybackProof(proof))
            {
                hashRefBlock = proof.hashRefBlock;
                nAgreement = proof.nAgreement;
            }
        }
        for (auto& tx : block.vtx)
        {
            setBlockBloomData.insert(tx.GetHash().GetBytes());
        }
        uint256 nMintCoint;
        if (!block.GetMintCoinProof(nMintCoint))
        {
            nMintCoint = 0;
        }
        uint256 nTotalTxFee;
        for (auto& tx : block.vtx)
        {
            nTotalTxFee += tx.GetTxFee();
        }
        nOriginalBlockMintReward = nMintCoint + nTotalTxFee;
    }

    CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const uint256& hashPrevBlockIn, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn)
      : dbBlockBase(dbBlockBaseIn), nBlockType(0), hashFork(hashForkIn), hashPrevBlock(hashPrevBlockIn), hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn),
        nOriBlockGasLimit(0), nSurplusBlockGasLimit(0), nBlockTimestamp(0), nBlockHeight(0), nBlockNumber(0), fPrimaryBlock(false) {}

    bool AddTxState(const CTransaction& tx, const int nTxIndex);
    bool DoBlockState(uint256& hashReceiptRoot, uint256& nBlockGasUsed, bytes& btBlockBloomDataOut, uint256& nTotalMintRewardOut);

    bool GetDestState(const CDestination& dest, CDestState& stateDest);
    void SetDestState(const CDestination& dest, const CDestState& stateDest);
    void SetCacheDestState(const CDestination& dest, const CDestState& stateDest);
    bool GetDestKvData(const CDestination& dest, const uint256& key, bytes& value);
    bool GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress);
    bool IsContractAddress(const CDestination& addr);
    bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy);
    bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd);
    bool GetBlockHashByNumber(const uint64 nBlockNumberIn, uint256& hashBlockOut);
    void GetBlockBloomData(bytes& btBloomDataOut);
    bool GetDestBalance(const CDestination& dest, uint256& nBalance);

    bool SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd, const uint256& txidCreate);
    bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult);
    bool Selfdestruct(const CDestination& destContractIn, const CDestination& destBeneficiaryIn);
    void SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed);
    void SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv);
    bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft, const CAddressContext& ctxToAddress, const uint8 nTransferType);
    bool IsContractDestroy(const CDestination& destContractIn);

    bool DoFunctionContractTx(const uint256& txid, const CTransaction& tx, const int nTxIndex, const uint64 nRunGasLimit, const uint256& nTvGasUsedIn, CTransactionReceipt& receipt);
    bool DoFuncContractCall(const CDestination& destFrom, const CDestination& destTo, const bytes& btFuncSign, const bytes& btTxParam,
                            const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

protected:
    void CreateFunctionContractData();
    bool GetDestContractCode(const CTransaction& tx, CDestination& destContract, bytes& btContractCode, bytes& btRunParam, uint256& hashContractCreateCode,
                             CDestination& destCodeOwner, CTxContractData& txcd, bool& fCall, bool& fDestroy);

    bool AddContractState(const uint256& txid, const CTransaction& tx, const int nTxIndex, const uint64 nRunGasLimit, const uint256& nTvGasUsedIn, bool& fCallResult, CTransactionReceipt& receipt);
    bool DoRunResult(const uint256& txid, const CTransaction& tx, const int nTxIndex, const CDestination& destContractIn,
                     const uint256& hashContractCreateCode, const uint64 nGasLeftIn, const uint256& nTvGasUsedIn,
                     const int nStatusCode, const bytes& vResult, CTransactionReceipt& receipt);

    bool GetDestLockedAmount(const CDestination& dest, uint256& nLockedAmount);
    bool VerifyFunctionAddressRepeat(const CDestination& destNewFunction);
    bool VerifyFunctionAddressDisable(const uint32 nFuncId);

    bool DoFuncTxDelegateVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxDelegateRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetDelegateVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

    bool DoFuncTxUserVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxUserRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetUserVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

    bool DoFuncTxPledgeVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxPledgeReqRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetPledgeVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetPledgeUnlockHeight(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

    bool DoFuncTxGetPageSize(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetDelegateCount(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetDelegateAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetDelegateTotalVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetVoteUnlockHeight(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

    bool DoFuncTxSetFunctionAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);
    bool DoFuncTxGetFunctionAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult);

protected:
    CBlockBase& dbBlockBase;
    const uint16 nBlockType;
    const uint256 hashFork;
    const uint256 hashPrevBlock;
    const uint256 hashPrevStateRoot;
    const uint32 nPrevBlockTime;
    const uint64 nOriBlockGasLimit;
    const uint64 nBlockTimestamp;
    const int nBlockHeight;
    const uint64 nBlockNumber;
    const CDestination destMint;
    const CTransaction mintTx;
    const uint256 txidMint;
    const bool fPrimaryBlock;

    uint256 nOriginalBlockMintReward;
    uint256 hashRefBlock;
    uint256 nAgreement;

    class CCacheContractData
    {
    public:
        CCacheContractData() {}

    public:
        CDestState cacheDestState;
        std::vector<CTransactionLogs> cacheContractLogs;
        std::map<uint256, bytes> cacheContractKv;
    };
    std::map<CDestination, CCacheContractData> mapCacheContractData;
    std::map<CDestination, CAddressContext> mapCacheAddressContext;
    std::map<uint256, CContractCreateCodeContext> mapCacheContractCreateCodeContext;
    std::map<uint256, CContractRunCodeContext> mapCacheContractRunCodeContext;
    std::vector<CContractTransfer> vCacheContractTransfer;
    std::map<CDestination, uint64> mapCacheCodeDestGasUsed;
    std::map<CDestination, std::pair<uint32, uint32>> mapCacheModifyPledgeFinalHeight;
    std::map<uint32, CFunctionAddressContext> mapCacheFunctionAddress;

    uint64 nSurplusBlockGasLimit;

    std::map<CDestination, uint256> mapBlockRewardLocked;

public:
    std::map<CDestination, CDestState> mapBlockState;
    std::map<CDestination, std::map<uint256, bytes>> mapContractKvState;
    std::map<CDestination, CAddressContext> mapBlockAddressContext;
    std::map<CDestination, uint256> mapBlockPayTvFee;
    std::map<uint256, CContractCreateCodeContext> mapBlockContractCreateCodeContext;
    std::map<uint256, CContractRunCodeContext> mapBlockContractRunCodeContext;
    std::map<uint256, std::vector<CContractTransfer>> mapBlockContractTransfer; // key is txid
    std::map<uint256, uint256> mapBlockTxFeeUsed;
    std::map<uint256, std::map<CDestination, uint256>> mapBlockCodeDestFeeUsed; // key is txid
    std::map<CDestination, std::pair<uint32, uint32>> mapBlockModifyPledgeFinalHeight;
    std::vector<uint256> vReceiptHash;
    std::map<uint256, CTransactionReceipt> mapBlockTxReceipts; // key is txid
    std::map<uint32, CFunctionAddressContext> mapBlockFunctionAddress;
    //uint2048 nBlockBloom;
    uint256 nBlockFeeLeft;
    std::set<bytes> setBlockBloomData;
};

typedef std::shared_ptr<CBlockState> SHP_BLOCK_STATE;

class CContractHostDB : public CVmHostFaceDB
{
public:
    CContractHostDB(CBlockState& blockStateIn, const CDestination& destContractIn, const CDestination& destCodeOwnerIn, const uint256& txidIn, const uint64 nTxNonceIn)
      : blockState(blockStateIn), destContract(destContractIn), destCodeOwner(destCodeOwnerIn), txid(txidIn), nTxNonce(nTxNonceIn) {}

    virtual bool Get(const uint256& key, bytes& value) const;
    virtual uint256 GetTxid() const;
    virtual uint256 GetTxNonce() const;
    virtual CDestination GetContractAddress() const;
    virtual CDestination GetCodeOwnerAddress() const;
    virtual bool GetBalance(const CDestination& addr, uint256& balance) const;
    virtual bool ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft);
    virtual uint256 GetBlockHash(const uint64 nBlockNumber);
    virtual bool IsContractAddress(const CDestination& addr);

    virtual bool GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy);
    virtual bool GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd);
    virtual CVmHostFaceDBPtr CloneHostDB(const CDestination& destContractIn);
    virtual void SaveGasUsed(const CDestination& destCodeOwnerIn, const uint64 nGasUsed);
    virtual void SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv);
    virtual bool SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd);
    virtual bool ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult);
    virtual bool Selfdestruct(const CDestination& destBeneficiaryIn);

protected:
    CBlockState& blockState;
    CDestination destContract;
    CDestination destCodeOwner;
    uint256 txid;
    uint64 nTxNonce;
};

class CBlockBase
{
public:
    CBlockBase();
    ~CBlockBase();
    bool Initialize(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fRewardCheckIn, const bool fRenewDB = false);
    void Deinitialize();
    void Clear();
    bool IsEmpty() const;
    const uint256& GetGenesisBlockHash() const;
    bool Exists(const uint256& hash) const;
    bool ExistsTx(const uint256& hashFork, const uint256& txid);
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
    bool RetrieveProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock);
    bool RetrieveAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry);
    bool RetrieveOrigin(const uint256& hashFork, CBlock& block);
    bool RetrieveTxAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex);
    bool RetrieveAvailDelegate(const uint256& hash, int height, const std::vector<uint256>& vBlockRange,
                               const uint256& nMinEnrollAmount,
                               std::map<CDestination, std::size_t>& mapWeight,
                               std::map<CDestination, std::vector<unsigned char>>& mapEnrollData,
                               std::vector<std::pair<CDestination, uint256>>& vecAmount);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount);
    void ListForkIndex(std::map<uint256, CBlockIndex*>& mapForkIndex);
    void ListForkIndex(std::multimap<int, CBlockIndex*>& mapForkIndex);
    bool GetBlockBranchList(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool GetBlockBranchListNolock(const uint256& hashBlock, const uint256& hashForkLastBlock, const CBlockEx* pBlockex, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder);
    bool LoadIndex(CBlockOutline& diskIndex);
    bool LoadTx(const uint32 nTxFile, const uint32 nTxOffset, CTransaction& tx);
    bool AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashNewRoot);
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt);
    bool VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256());
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    int GetForkCreatedHeight(const uint256& hashFork, const uint256& hashRefBlock);
    bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight);
    bool GetBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock);

    bool GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep);
    bool GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, size_t nMaxCount);

    bool GetDelegateVotes(const uint256& hashGenesis, const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool GetDelegateList(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation);
    bool VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const uint32 height, const CDestination& destMint, const uint16 nBlockType,
                           const uint64 nBlockTimeStamp, const uint64 nRefBlockTimeStamp, const uint32 nExtendedBlockSpacing);
    bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote);
    bool GetRangeDelegateEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock);
    CBlockIndex* GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork);
    bool VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock);
    bool GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin);
    bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime);
    bool VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime);
    bool UpdateForkNext(const uint256& hashFork, CBlockIndex* pIndexLast, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    SHP_BLOCK_STATE CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, const uint256& hashPrevStateRoot, const uint32 nPrevBlockTime, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                         uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, const std::map<CDestination, CAddressContext>& mapAddressContext);
    bool UpdateBlockState(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashNewStateRoot, SHP_BLOCK_STATE& ptrBlockStateOut);
    bool UpdateBlockTxIndex(const uint256& hashFork, const CBlockEx& block, const uint32 nFile, const uint32 nOffset, const std::map<uint256, CTransactionReceipt>& mapBlockTxReceipts, uint256& hashNewRoot);
    bool UpdateBlockAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block,
                                  const std::map<uint256, std::vector<CContractTransfer>>& mapContractTransferIn,
                                  const std::map<uint256, uint256>& mapBlockTxFeeUsedIn, const std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedIn,
                                  uint256& hashNewRoot);
    bool UpdateBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<CDestination, CAddressContext>& mapAddressContextIn, const std::map<CDestination, CDestState>& mapBlockStateIn,
                            const std::map<CDestination, uint256>& mapBlockPayTvFeeIn, const std::map<uint32, CFunctionAddressContext>& mapBlockFunctionAddressIn, uint256& hashNewRoot);
    bool UpdateBlockCode(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nBlockOffset, const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCodeContextIn,
                         const std::map<uint256, CContractRunCodeContext>& mapContractRunCodeContextIn, const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashCodeRoot);
    bool UpdateBlockVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewRoot);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction);
    bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr);
    bool CallContractCode(const bool fEthCall, const uint256& hashFork, const CChainId& chainId, const uint256& nAgreement, const uint32 nHeight, const CDestination& destMint, const uint256& nBlockGasLimit,
                          const CDestination& from, const CDestination& to, const uint256& nGasPrice, const uint256& nGasLimit, const uint256& nAmount,
                          const bytes& data, const uint64 nTimeStamp, const uint256& hashPrevBlock, const uint256& hashPrevStateRoot, const uint64 nPrevBlockTime, uint64& nGasLeft, int& nStatus, bytes& btResult);
    bool GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate);
    bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode);
    bool GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode);
    bool RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork);
    bool RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode);
    bool GetForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCodeContext& ctxtContractCode);
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode);
    bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract);
    bool VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);
    bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount);
    bool GetBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CAddressContext>& mapBlockAddress);
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& txReceiptex);
    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter);
    void RemoveFilter(const uint256& nFilterId);
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs);
    bool GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs);
    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork);
    bool GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash);
    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork);
    void AddPendingTx(const uint256& hashFork, const uint256& txid);
    bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid);
    bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount);
    bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock);
    bool GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount);

    bool AddBlacklistAddress(const CDestination& dest);
    void RemoveBlacklistAddress(const CDestination& dest);
    bool IsExistBlacklistAddress(const CDestination& dest);
    void ListBlacklistAddress(set<CDestination>& setAddressOut);

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice);
    uint256 GetForkMintMinGasPrice(const uint256& hashFork);

protected:
    CBlockIndex* GetIndex(const uint256& hash) const;
    CBlockIndex* GetForkLastIndex(const uint256& hashFork);
    CBlockIndex* GetOrCreateIndex(const uint256& hash);
    CBlockIndex* GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, std::vector<CBlockIndex*>& vPath);
    CBlockIndex* GetOriginIndex(const uint256& txidMint);
    void UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, const uint64 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock);
    void RemoveBlockIndex(const uint256& hashFork, const uint256& hashBlock);
    void UpdateBlockRef(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock);
    bool UpdateBlockLongChain(const uint256& hashFork, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew);
    void UpdateBlockNext(CBlockIndex* pIndexLast);
    CBlockIndex* AddNewIndex(const uint256& hash, const CBlock& block, const uint32 nFile, const uint32 nOffset, const uint32 nCrc, const uint256& nChainTrust, const uint256& hashNewStateRoot);
    bool LoadForkProfile(const CBlockIndex* pIndexOrigin, CProfile& profile);
    bool UpdateDelegate(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                        const uint256& nMinEnrollAmount, const std::map<CDestination, CAddressContext>& mapAddressContext,
                        const std::map<CDestination, CDestState>& mapAccStateIn, uint256& hashDelegateRoot);
    bool UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block,
                    const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<CDestination, CDestState>& mapAccStateIn,
                    const std::map<CDestination, std::pair<uint32, uint32>>& mapBlockModifyPledgeFinalHeightIn, uint256& hashVoteRoot);
    bool IsValidBlock(CBlockIndex* pForkLast, const uint256& hashBlock);
    bool VerifyValidBlock(CBlockIndex* pIndexGenesisLast, const CBlockIndex* pIndex);
    CBlockIndex* GetLongChainLastBlock(const uint256& hashFork, int nStartHeight, CBlockIndex* pIndexGenesisLast, const std::set<uint256>& setInvalidHash);
    bool GetTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashAtFork, CTxIndex& txIndex);
    void ClearCache();
    bool LoadDB();
    bool VerifyDB();
    bool VerifyBlockDB(const CBlockVerify& verifyBlock, CBlockOutline& outline, CBlockRoot& blockRoot, const bool fVerify);
    bool RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockEx& block, CBlockIndex** ppIndexNew);
    bool LoadBlockIndex(CBlockOutline& outline, CBlockIndex** ppIndexNew);

protected:
    enum
    {
        MAX_CACHE_BLOCK_STATE = 64
    };

    mutable mtbase::CRWAccess rwAccess;
    bool fCfgFullDb;
    bool fCfgRewardCheck;
    uint256 hashGenesisBlock;
    CBlockDB dbBlock;
    CTimeSeriesCached tsBlock;
    std::map<uint256, CBlockIndex*> mapIndex;
    std::map<uint256, CForkHeightIndex> mapForkHeightIndex;
    CBlockFilter blockFilter;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_BLOCKBASE_H

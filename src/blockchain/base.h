// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_BASE_H
#define METABASENET_BASE_H

#include <boost/optional.hpp>
#include <mtbase.h>
#include <map>
#include <set>

#include "block.h"
#include "config.h"
#include "crypto.h"
#include "destination.h"
#include "error.h"
#include "forkcontext.h"
#include "key.h"
#include "param.h"
#include "peer.h"
#include "profile.h"
#include "struct.h"
#include "template/mint.h"
#include "template/template.h"
#include "timeseries.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{

class ICoreProtocol : public mtbase::IBase
{
public:
    ICoreProtocol()
      : IBase("coreprotocol") {}
    virtual void InitializeGenesisBlock() = 0;
    virtual const uint256& GetGenesisBlockHash() = 0;
    virtual CChainId GetGenesisChainId() const = 0;
    virtual void GetGenesisBlock(CBlock& block) = 0;
    virtual Errno ValidateTransaction(const uint256& hashTxAtFork, const uint256& hashMainChainRefBlock, const CTransaction& tx) = 0;
    virtual Errno ValidateBlock(const uint256& hashFork, const uint256& hashMainChainRefBlock, const CBlock& block) = 0;
    virtual Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) = 0;
    virtual Errno VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev) = 0;
    virtual Errno VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                              const CDelegateAgreement& agreement)
        = 0;
    virtual Errno VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                   const CDelegateAgreement& agreement)
        = 0;
    virtual Errno VerifyTransaction(const uint256& txid, const CTransaction& tx, const uint256& hashFork, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<CDestination, CAddressContext>& mapBlockAddress) = 0;
    virtual bool GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, const uint256& nEnrollTrust = uint256()) = 0;
    virtual bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits) = 0;
    virtual uint256 GetDelegatedBallot(const int nBlockHeight, const uint256& nAgreement, const std::size_t& nWeight, const std::map<CDestination, std::size_t>& mapBallot,
                                       const std::vector<std::pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, std::vector<CDestination>& vBallot)
        = 0;
    virtual uint64 GetNextBlockTimestamp(const uint64 nPrevTimeStamp) = 0;
};

class IBlockChain : public mtbase::IBase
{
public:
    class CCheckPoint
    {
    public:
        CCheckPoint()
          : nHeight(-1)
        {
        }
        CCheckPoint(int nHeightIn, const uint256& nBlockHashIn)
          : nHeight(nHeightIn), nBlockHash(nBlockHashIn)
        {
        }
        CCheckPoint(const CCheckPoint& point)
          : nHeight(point.nHeight), nBlockHash(point.nBlockHash)
        {
        }
        CCheckPoint& operator=(const CCheckPoint& point)
        {
            nHeight = point.nHeight;
            nBlockHash = point.nBlockHash;
            return *this;
        }
        bool IsNull() const
        {
            return (nHeight == -1 || !nBlockHash);
        }

    public:
        int nHeight;
        uint256 nBlockHash;
    };

public:
    IBlockChain()
      : IBase("blockchain") {}
    virtual void GetForkStatus(std::map<uint256, CForkStatus>& mapForkStatus) = 0;
    virtual bool GetForkProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetForkAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry) = 0;
    virtual int GetBlockCount(const uint256& hashFork) = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight, uint256& hashNext) = 0;
    virtual bool GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock) = 0;
    virtual bool GetBlockHashList(const uint256& hashFork, const uint32 nHeight, std::vector<uint256>& vBlockHash) = 0;
    virtual bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) = 0;
    virtual bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) = 0;
    virtual bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime) = 0;
    virtual bool GetBlockHashOfRefBlock(const uint256& hashRefBlock, const int nHeight, uint256& hashBlock) = 0;
    virtual bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) = 0;
    virtual bool GetLastBlockTime(const uint256& hashFork, int nDepth, std::vector<uint64>& vTime) = 0;
    virtual bool GetBlock(const uint256& hashBlock, CBlock& block) = 0;
    virtual bool GetBlockEx(const uint256& hashBlock, CBlockEx& block) = 0;
    virtual bool GetOrigin(const uint256& hashFork, CBlock& block) = 0;
    virtual bool Exists(const uint256& hashBlock) = 0;
    virtual bool GetTransactionAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex) = 0;
    virtual bool ExistsTx(const uint256& hashFork, const uint256& txid) = 0;
    virtual bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256()) = 0;
    virtual bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock) = 0;
    virtual bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight) = 0;
    virtual Errno AddNewBlock(const uint256& hashBlock, const CBlock& block, uint256& hashFork, CBlockChainUpdate& update) = 0;
    virtual Errno AddNewOrigin(const CBlock& block, CBlockChainUpdate& update) = 0;
    virtual bool GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits) = 0;
    virtual bool GetBlockMintReward(const uint256& hashPrev, const bool fPow, uint256& nReward, const uint256& hashMainChainRefBlock) = 0;
    virtual bool GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep) = 0;
    virtual bool GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, std::size_t nMaxCount) = 0;
    virtual int64 GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::vector<CTxInfo>& vTx) = 0;
    virtual bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress) = 0;
    virtual bool GetTxToAddressContext(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx, CAddressContext& ctxAddress) = 0;
    virtual CTemplatePtr GetTxToAddressTemplatePtr(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) = 0;
    virtual bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress) = 0;
    virtual bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv) = 0;
    virtual bool RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey) = 0;
    virtual bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount) = 0;
    virtual bool RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode) = 0;
    virtual bool RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork) = 0;
    virtual bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode) = 0;
    virtual bool GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode) = 0;
    virtual bool GetForkContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) = 0;
    virtual bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode) = 0;
    virtual bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo) = 0;
    virtual bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount) = 0;
    virtual bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock) = 0;
    virtual bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount) = 0;
    virtual bool GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount) = 0;
    virtual bool VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainIdIn, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashBlock = uint256()) = 0;
    virtual bool GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value) = 0;
    virtual bool GetBlockReceiptsByBlock(const uint256& hashFork, const uint256& hashFromBlock, const uint256& hashToBlock, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapBlockReceipts) = 0;
    virtual bool VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock) = 0;
    virtual bool GetPrevBlockHashList(const uint256& hashBlock, const uint32 nGetCount, std::vector<uint256>& vPrevBlockhash) = 0;

    /////////////    CheckPoints    /////////////////////
    virtual bool HasCheckPoints(const uint256& hashFork) const = 0;
    virtual bool GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point) = 0;
    virtual std::vector<CCheckPoint> CheckPoints(const uint256& hashFork) const = 0;
    virtual CCheckPoint LatestCheckPoint(const uint256& hashFork) const = 0;
    virtual CCheckPoint UpperBoundCheckPoint(const uint256& hashFork, int nHeight) const = 0;
    virtual bool VerifyCheckPoint(const uint256& hashFork, int nHeight, const uint256& nBlockHash) = 0;
    virtual bool FindPreviousCheckPointBlock(const uint256& hashFork, CBlock& block) = 0;
    virtual bool IsSameBranch(const uint256& hashFork, const CBlock& block) = 0;

    virtual bool GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes) = 0;
    virtual bool GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight) = 0;
    virtual bool ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) = 0;
    virtual bool VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& hashBlockRef) = 0;
    virtual bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote) = 0;
    virtual bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount) = 0;
    virtual bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote) = 0;
    virtual bool GetDelegateEnrollByHeight(const uint256& hashRefBlock, const int nEnrollHeight, std::map<CDestination, storage::CDiskPos>& mapEnrollTxPos) = 0;
    virtual bool VerifyDelegateEnroll(const uint256& hashRefBlock, const int nEnrollHeight, const CDestination& destDelegate) = 0;
    virtual bool GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled) = 0;
    virtual bool GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement) = 0;
    virtual uint256 GetBlockMoneySupply(const uint256& hashBlock) = 0;
    virtual bool GetBlockDelegateVoteAddress(const uint256& hashBlock, std::set<CDestination>& setVoteAddress) = 0;
    virtual uint64 GetNextBlockTimestamp(const uint256& hashPrev) = 0;
    virtual Errno VerifyPowBlock(const CBlock& block, bool& fLongChain) = 0;
    virtual bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt) = 0;
    virtual bool CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update) = 0;
    virtual bool VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock) = 0;
    virtual bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime) = 0;
    virtual bool IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block) = 0;

    virtual bool CalcBlockVoteRewardTx(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime, vector<CTransaction>& vVoteRewardTx) = 0;
    virtual uint256 GetPrimaryBlockReward(const uint256& hashPrev) = 0;
    virtual bool CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                      uint256& hashCrosschainMerkleRoot, uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, bool& fMoStatus)
        = 0;
    virtual bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CDestState& state) = 0;
    virtual bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex) = 0;
    virtual bool CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                              const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult)
        = 0;
    virtual bool GetContractBalance(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const CDestination& destUser, uint256& nBalance) = 0;
    virtual bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract) = 0;
    virtual bool VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) = 0;
    virtual bool VerifyDelegateMinVote(const uint256& hashRefBlock, const uint32 nHeight, const CDestination& destDelegate) = 0;

    virtual bool GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner,
                                  const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
        = 0;
    virtual bool GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount) = 0;
    virtual bool ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                     const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
        = 0;
    virtual bool ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder) = 0;
    virtual bool GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve) = 0;
    virtual bool AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve) = 0;
    virtual bool GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve) = 0;

    virtual bool AddBlacklistAddress(const CDestination& dest) = 0;
    virtual void RemoveBlacklistAddress(const CDestination& dest) = 0;
    virtual bool IsExistBlacklistAddress(const CDestination& dest) = 0;
    virtual void ListBlacklistAddress(set<CDestination>& setAddressOut) = 0;
    virtual bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress) = 0;
    virtual bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress) = 0;
    virtual bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction) = 0;
    virtual bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr) = 0;

    virtual bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice) = 0;
    virtual uint256 GetForkMintMinGasPrice(const uint256& hashFork) = 0;

    virtual bool GetCandidatePubkey(const uint256& hashPrimaryBlock, std::vector<uint384>& vCandidatePubkey) = 0;
    virtual bool GetPrevBlockCandidatePubkey(const uint256& hashBlock, std::vector<uint384>& vCandidatePubkey) = 0;
    virtual bool VerifyBlockCommitVoteAggSig(const uint256& hashBlock, const bytes& btAggBitmap, const bytes& btAggSig) = 0;
    virtual bool VerifyBlockCommitVoteAggSig(const uint256& hashVoteBlock, const uint256& hashRefBlock, const bytes& btAggBitmap, const bytes& btAggSig) = 0;

    virtual bool AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock) = 0;
    virtual bool RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock) = 0;
    virtual bool GetMakerVoteBlock(const uint256& hashPrevBlock, bytes& btBitmap, bytes& btAggSig, uint256& hashVoteBlock) = 0;
    virtual bool IsBlockConfirm(const uint256& hashBlock) = 0;
    virtual bool AddBlockLocalVoteSignFlag(const uint256& hashBlock) = 0;
    virtual bool VerifyPrimaryBlockConfirm(const uint256& hashBlock) = 0;

    const CBasicConfig* Config()
    {
        return dynamic_cast<const CBasicConfig*>(mtbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(mtbase::IBase::Config());
    }
};

class ITxPool : public mtbase::IBase
{
public:
    ITxPool()
      : IBase("txpool") {}
    virtual void ClearTxPool(const uint256& hashFork) = 0;
    virtual bool Exists(const uint256& hashFork, const uint256& txid) = 0;
    virtual bool CheckTxNonce(const uint256& hashFork, const CDestination& destFrom, const uint64 nTxNonce) = 0;
    virtual std::size_t Count(const uint256& hashFork) const = 0;
    virtual Errno Push(const uint256& hashFork, const CTransaction& tx) = 0;
    virtual bool Get(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork) const = 0;
    virtual void ListTx(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) = 0;
    virtual void ListTx(const uint256& hashFork, std::vector<uint256>& vTxPool) = 0;
    virtual bool ListTx(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) = 0;
    virtual bool FetchArrangeBlockTx(const uint256& hashFork, const uint256& hashPrev, const int64 nBlockTime,
                                     const std::size_t nMaxSize, std::vector<CTransaction>& vtx, uint256& nTotalTxFee)
        = 0;
    virtual bool SynchronizeBlockChain(const CBlockChainUpdate& update) = 0;
    virtual void GetDestBalance(const uint256& hashFork, const CDestination& dest, uint8& nDestType, uint8& nTemplateType, uint64& nNonce, uint256& nAvail,
                                uint256& nUnconfirmedIn, uint256& nUnconfirmedOut, CAddressContext& ctxAddress, const uint256& hashBlock = uint256())
        = 0;
    virtual uint64 GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest) = 0;
    virtual bool GetAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashRefBlock = uint256()) = 0;
    virtual bool PushCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx) = 0;
    virtual bool PushUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx) = 0;

    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(mtbase::IBase::Config());
    }
};

class IForkManager : public mtbase::IBase
{
public:
    IForkManager()
      : IBase("forkmanager") {}
    virtual bool GetActiveFork(std::vector<uint256>& vActive) = 0;
    virtual bool IsAllowed(const uint256& hashFork) const = 0;
    virtual void ForkUpdate(const CBlockChainUpdate& update, std::vector<uint256>& vActive, std::vector<uint256>& vDeactive) = 0;
    const CForkConfig* ForkConfig()
    {
        return dynamic_cast<const CForkConfig*>(mtbase::IBase::Config());
    }
};

class IConsensus : public mtbase::IBase
{
public:
    IConsensus()
      : IBase("consensus") {}
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(mtbase::IBase::Config());
    }
    virtual void PrimaryUpdate(const CBlockChainUpdate& update, CDelegateRoutine& routine) = 0;
    virtual bool AddNewDistribute(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchDistribute) = 0;
    virtual bool AddNewPublish(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchPublish) = 0;
    virtual void GetAgreement(int nTargetHeight, uint256& nAgreement, std::size_t& nWeight, std::vector<CDestination>& vBallot) = 0;
    virtual void GetProof(int nTargetHeight, std::vector<unsigned char>& vchDelegateData) = 0;
    virtual bool GetNextConsensus(CAgreementBlock& consParam) = 0;
    virtual bool LoadConsensusData(int& nStartHeight, CDelegateRoutine& routine) = 0;
};

class IBlockMaker : public mtbase::CEventProc
{
public:
    IBlockMaker()
      : CEventProc("blockmaker") {}
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(mtbase::IBase::Config());
    }
    virtual bool GetMiningAddressList(std::vector<CDestination>& vMintAddressList) = 0;
};

class IWallet : public mtbase::IBase
{
public:
    IWallet()
      : IBase("wallet") {}
    /* Key store */
    virtual boost::optional<std::string> AddKey(const crypto::CKey& key) = 0;
    virtual boost::optional<std::string> RemoveKey(const CDestination& dest) = 0;
    virtual bool GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const = 0;
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const = 0;
    virtual bool HaveKey(const CDestination& dest, const int32 nVersion) const = 0;
    virtual bool Export(const CDestination& dest, std::vector<unsigned char>& vchKey) const = 0;
    virtual bool Import(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) = 0;
    virtual bool Encrypt(const CDestination& dest, const crypto::CCryptoString& strPassphrase, const crypto::CCryptoString& strCurrentPassphrase)
        = 0;
    virtual bool GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const = 0;
    virtual bool IsLocked(const CDestination& dest) const = 0;
    virtual bool Lock(const CDestination& dest) = 0;
    virtual bool Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout) = 0;
    virtual bool Sign(const CDestination& dest, const uint256& hash, std::vector<uint8>& vchSig) = 0;
    virtual bool SignEthTx(const CDestination& dest, const CEthTxSkeleton& ets, uint256& hashTx, bytes& btEthTxData) = 0;
    /* Template */
    virtual void GetTemplateIds(std::set<pair<uint8, CTemplateId>>& setTemplateId, const uint64 nPos, const uint64 nCount) const = 0;
    virtual bool HaveTemplate(const CDestination& dest) const = 0;
    virtual bool AddTemplate(CTemplatePtr& ptr) = 0;
    virtual CTemplatePtr GetTemplate(const CDestination& dest) const = 0;
    virtual bool RemoveTemplate(const CDestination& dest) = 0;
    /* Destination */
    virtual void GetDestinations(std::set<CDestination>& setDest) = 0;
    /* Wallet Tx */
    virtual bool SignTransaction(const uint256& hashFork, const uint256& hashSignRefBlock, CTransaction& tx) = 0;

    const CBasicConfig* Config()
    {
        return dynamic_cast<const CBasicConfig*>(mtbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(mtbase::IBase::Config());
    }
};

class IDispatcher : public mtbase::IBase
{
public:
    IDispatcher()
      : IBase("dispatcher") {}
    virtual Errno AddNewBlock(const CBlock& block, uint64 nNonce = 0) = 0;
    virtual Errno AddNewTx(const uint256& hashFork, const CTransaction& tx, uint64 nNonce = 0) = 0;
    virtual bool AddNewDistribute(const uint256& hashAnchor, const CDestination& dest,
                                  const std::vector<unsigned char>& vchDistribute)
        = 0;
    virtual bool AddNewPublish(const uint256& hashAnchor, const CDestination& dest,
                               const std::vector<unsigned char>& vchPublish)
        = 0;
    virtual void SetConsensus(const CAgreementBlock& agreeBlock) = 0;
    virtual void CheckAllSubForkLastBlock() = 0;
    virtual void NotifyBlockVoteChnNewBlock(const uint256& hashBlock, const uint64 nNonce = 0) = 0;
};

class IService : public mtbase::IBase
{
public:
    IService()
      : IBase("service") {}
    /* System */
    virtual void Stop() = 0;
    /* Network */
    virtual CChainId GetChainId() = 0;
    virtual int GetPeerCount() = 0;
    virtual void GetPeers(std::vector<network::CBbPeerInfo>& vPeerInfo) = 0;
    virtual bool AddNode(const mtbase::CNetHost& node) = 0;
    virtual bool RemoveNode(const mtbase::CNetHost& node) = 0;
    virtual bool GetForkRpcPort(const uint256& hashFork, uint16& nRpcPort) = 0;
    /* Blockchain & Tx Pool*/
    virtual int GetForkCount() = 0;
    virtual bool HaveFork(const uint256& hashFork) = 0;
    virtual int GetForkHeight(const uint256& hashFork) = 0;
    virtual bool GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock) = 0;
    virtual void ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll = false) = 0;
    virtual bool GetForkContext(const uint256& hashFork, CForkContext& ctxtFork, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainIdIn, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashBlock = uint256()) = 0;
    virtual bool GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetForkHashByChainId(const CChainId nChainIdIn, uint256& hashFork) = 0;
    virtual bool GetCoinContext(const std::string& strCoinSymbol, CCoinContext& ctxCoin, const uint256& hashLastBlock = uint256()) = 0;
    virtual bool ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashLastBlock = uint256()) = 0;
    virtual bool GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock = uint256()) = 0;
    virtual bool GetForkGenealogy(const uint256& hashFork, std::vector<std::pair<uint256, int>>& vAncestry, std::vector<std::pair<int, uint256>>& vSubline) = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight) = 0;
    virtual int GetBlockCount(const uint256& hashFork) = 0;
    virtual bool GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock) = 0;
    virtual bool GetBlockHashList(const uint256& hashFork, const uint32 nHeight, std::vector<uint256>& vBlockHash) = 0;
    virtual bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) = 0;
    virtual bool GetBlock(const uint256& hashBlock, CBlock& block, CChainId& nChainId, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetBlockEx(const uint256& hashBlock, CBlockEx& block, CChainId& nChainId, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime) = 0;
    virtual bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) = 0;
    virtual bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) = 0;
    virtual bool IsBlockConfirm(const uint256& hashBlock) = 0;
    virtual void GetTxPool(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) = 0;
    virtual void ListTxPool(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) = 0;
    virtual bool GetTransactionAndPosition(const uint256& hashRefFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, uint256& hashAtBlock, uint64& nBlockNumber, uint16& nTxSeq) = 0;
    virtual bool GetTransactionByIndex(const uint256& hashBlock, const uint64 nTxIndex, CTransaction& tx, uint64& nBlockNumber) = 0;
    virtual Errno SendTransaction(const uint256& hashFork, CTransaction& tx) = 0;
    //virtual bool RemovePendingTx(const uint256& txid) = 0;
    virtual bool GetVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes, string& strFailCause) = 0;
    virtual bool ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) = 0;
    virtual bool GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes) = 0;
    virtual bool GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight) = 0;
    virtual bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex) = 0;
    virtual bool CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                              const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult)
        = 0;
    virtual bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value) = 0;
    virtual uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter) = 0;
    virtual void RemoveFilter(const uint256& nFilterId) = 0;
    virtual bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs) = 0;
    virtual bool GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs) = 0;
    virtual uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork) = 0;
    virtual bool GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash) = 0;
    virtual uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork) = 0;
    virtual bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid) = 0;

    /* Wallet */
    virtual bool HaveKey(const CDestination& dest, const int32 nVersion = -1) = 0;
    virtual bool GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const = 0;
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) = 0;
    virtual bool GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) = 0;
    virtual boost::optional<std::string> MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey) = 0;
    virtual boost::optional<std::string> AddKey(const crypto::CKey& key) = 0;
    virtual boost::optional<std::string> RemoveKey(const CDestination& dest) = 0;
    virtual bool ImportKey(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) = 0;
    virtual bool ExportKey(const CDestination& dest, std::vector<unsigned char>& vchKey) = 0;
    virtual bool EncryptKey(const CDestination& dest, const crypto::CCryptoString& strPassphrase,
                            const crypto::CCryptoString& strCurrentPassphrase)
        = 0;
    virtual bool Lock(const CDestination& dest) = 0;
    virtual bool Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout) = 0;
    virtual bool GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, const CCoinContext& ctxCoin, CWalletBalance& balance) = 0;
    virtual bool SignSignature(const CDestination& dest, const uint256& hash, std::vector<unsigned char>& vchSig) = 0;
    virtual bool SignTransaction(const uint256& hashFork, CTransaction& tx) = 0;
    virtual bool HaveTemplate(const CDestination& dest) = 0;
    virtual void GetTemplateIds(std::set<pair<uint8, CTemplateId>>& setTid, const uint64 nPos, const uint64 nCount) = 0;
    virtual bool AddTemplate(CTemplatePtr& ptr) = 0;
    virtual CTemplatePtr GetTemplate(const CDestination& dest) = 0;
    virtual bool RemoveTemplate(const CDestination& dest) = 0;
    virtual bool ListTransaction(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, std::vector<CDestTxInfo>& vTx) = 0;
    virtual boost::optional<std::string> CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData,
                                                           const uint256& nAmount, const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData,
                                                           const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew)
        = 0;
    virtual boost::optional<std::string> SignEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                                                            const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid, bytes& btSignTxData)
        = 0;
    virtual bool SendEthRawTransaction(const bytes& btRawTxData, uint256& txid) = 0;
    virtual bool SendEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                                    const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid)
        = 0;
    virtual void GetWalletDestinations(std::set<CDestination>& setDest) = 0;
    virtual bool GetForkContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) = 0;
    virtual bool ListContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode) = 0;
    virtual bool ListContractAddress(const uint256& hashFork, const uint256& hashRefBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress) = 0;
    virtual bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv) = 0;
    virtual bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount) = 0;
    virtual bool GeDestContractContext(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, CContractAddressContext& ctxtContract) = 0;
    virtual bool GetContractSource(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashSource, bytes& btSource) = 0;
    virtual bool GetContractCode(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashCode, bytes& btCode) = 0;
    virtual bool GetDestTemplateData(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, bytes& btTemplateData) = 0;
    virtual bool RetrieveAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashBlock = uint256()) = 0;
    virtual uint64 GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest) = 0;

    virtual bool GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner,
                                  const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
        = 0;
    virtual bool GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount) = 0;
    virtual bool ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                     const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
        = 0;
    virtual bool ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder) = 0;

    virtual bool AddBlacklistAddress(const CDestination& dest) = 0;
    virtual void RemoveBlacklistAddress(const CDestination& dest) = 0;
    virtual void ListBlacklistAddress(set<CDestination>& setAddressOut) = 0;
    virtual bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress) = 0;
    virtual bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress) = 0;
    virtual bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction) = 0;
    virtual bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr) = 0;

    virtual bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice) = 0;
    virtual uint256 GetForkMintMinGasPrice(const uint256& hashFork) = 0;

    /* Mint */
    virtual bool GetWork(std::vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                         uint256& hashPrev, int& nAlgo, int& nBits,
                         const CTemplateMintPtr& templMint)
        = 0;
    virtual Errno SubmitWork(const std::vector<unsigned char>& vchWorkData, const CTemplateMintPtr& templMint,
                             crypto::CKey& keyMint, uint256& hashBlock)
        = 0;
};

class IWsService : public mtbase::CEventProc
{
public:
    IWsService()
      : CEventProc("wsservice") {}

    virtual void AddNewBlockSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) = 0;
    virtual void AddLogsSubscribe(const CChainId nChainId, const uint64 nClientConnId, const std::set<CDestination>& setSubsAddress, const std::set<uint256>& setSubsTopics, uint128& nSubsId) = 0;
    virtual void AddNewPendingTxSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) = 0;
    virtual void AddSyncingSubscribe(const CChainId nChainId, const uint64 nClientConnId, uint128& nSubsId) = 0;
    virtual bool RemoveSubscribe(const CChainId nChainId, const uint64 nClientConnId, const uint128& nSubsId) = 0;

    virtual void SendWsMsg(const CChainId nChainId, const uint64 nNonce, const std::string& strMsg) = 0;
};

class IDataStat : public mtbase::IIOModule
{
public:
    IDataStat()
      : IIOModule("datastat") {}
    virtual bool AddBlockMakerStatData(const uint256& hashFork, bool fPOW, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynRecvStatData(const uint256& hashFork, uint64 nBlockCountIn, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynSendStatData(const uint256& hashFork, uint64 nBlockCountIn, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynTxSynStatData(const uint256& hashFork, const uint64 nTxCount, const bool fRecv) = 0;
    virtual bool GetBlockMakerStatData(const uint256& hashFork, uint32 nBeginTime, uint32 nGetCount, std::vector<CStatItemBlockMaker>& vStatData) = 0;
    virtual bool GetP2pSynStatData(const uint256& hashFork, uint32 nBeginTime, uint32 nGetCount, std::vector<CStatItemP2pSyn>& vStatData) = 0;
};

class IRecovery : public mtbase::IBase
{
public:
    IRecovery()
      : IBase("recovery") {}
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(mtbase::IBase::Config());
    }
};

class IBlockFilter : public mtbase::IBase
{
public:
    IBlockFilter()
      : IBase("blockfilter") {}

    virtual void RemoveFilter(const uint256& nFilterId) = 0;

    virtual uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn, const std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapHisBlockReceiptsIn) = 0;
    virtual void AddTxReceipt(const uint256& hashForkIn, const uint256& hashBlock, const CTransactionReceipt& receipt) = 0;
    virtual bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs) = 0;

    virtual uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork) = 0;
    virtual void AddNewBlockInfo(const uint256& hashForkIn, const uint256& hashBlock, const CBlock& block) = 0;
    virtual bool GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash) = 0;

    virtual uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork) = 0;
    virtual void AddPendingTx(const uint256& hashFork, const uint256& txid) = 0;
    virtual bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid) = 0;

    virtual void AddMaxPeerBlockNumber(const uint256& hashFork, const uint64 nMaxPeerBlockNumber) = 0;
};

} // namespace metabasenet

#endif //METABASENET_BASE_H

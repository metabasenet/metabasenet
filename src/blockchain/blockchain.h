// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_BLOCKCHAIN_H
#define METABASENET_BLOCKCHAIN_H

#include <map>
#include <uint256.h>
#include <utility>

#include "base.h"
#include "blockbase.h"

namespace metabasenet
{

class CBlockChain : public IBlockChain
{
public:
    CBlockChain();
    ~CBlockChain();
    void GetForkStatus(std::map<uint256, CForkStatus>& mapForkStatus) override;
    bool GetForkProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetForkAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry) override;
    int GetBlockCount(const uint256& hashFork) override;
    bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight) override;
    bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight, uint256& hashNext) override;
    bool GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock) override;
    bool GetBlockHashList(const uint256& hashFork, const uint32 nHeight, std::vector<uint256>& vBlockHash) override;
    bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) override;
    bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) override;
    bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) override;
    bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime) override;
    bool GetBlockHashOfRefBlock(const uint256& hashRefBlock, const int nHeight, uint256& hashBlock) override;
    bool GetLastBlockTime(const uint256& hashFork, int nDepth, std::vector<uint64>& vTime) override;
    bool GetBlock(const uint256& hashBlock, CBlock& block) override;
    bool GetBlockEx(const uint256& hashBlock, CBlockEx& block) override;
    bool GetOrigin(const uint256& hashFork, CBlock& block) override;
    bool Exists(const uint256& hashBlock) override;
    bool GetTransactionAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex) override;
    bool ExistsTx(const uint256& hashFork, const uint256& txid) override;
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256()) override;
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock) override;
    bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight) override;
    Errno AddNewBlock(const uint256& hashBlock, const CBlock& block, uint256& hashFork, CBlockChainUpdate& update) override;
    Errno AddNewOrigin(const CBlock& block, CBlockChainUpdate& update) override;
    bool GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits) override;
    bool GetBlockMintReward(const uint256& hashPrev, const bool fPow, uint256& nReward, const uint256& hashMainChainRefBlock) override;
    bool GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep) override;
    bool GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, std::size_t nMaxCount) override;
    bool GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled) override;
    bool GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement) override;
    bool GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes) override;
    bool GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight) override;
    bool ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) override;
    bool VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& hashBlockRef) override;
    bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote) override;
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount) override;
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote) override;
    bool GetDelegateEnrollByHeight(const uint256& hashRefBlock, const int nEnrollHeight, std::map<CDestination, storage::CDiskPos>& mapEnrollTxPos) override;
    bool VerifyDelegateEnroll(const uint256& hashRefBlock, const int nEnrollHeight, const CDestination& destDelegate) override;
    bool VerifyDelegateMinVote(const uint256& hashRefBlock, const uint32 nHeight, const CDestination& destDelegate) override;
    uint256 GetBlockMoneySupply(const uint256& hashBlock) override;
    bool GetBlockDelegateVoteAddress(const uint256& hashBlock, std::set<CDestination>& setVoteAddress) override;
    uint64 GetNextBlockTimestamp(const uint256& hashPrev) override;
    Errno VerifyPowBlock(const CBlock& block, bool& fLongChain) override;
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt) override;
    bool CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update) override;
    bool VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock) override;
    bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime) override;
    bool IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block) override;
    int64 GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::vector<CTxInfo>& vTx) override;
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress) override;
    bool GetTxToAddressContext(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx, CAddressContext& ctxAddress) override;
    CTemplatePtr GetTxToAddressTemplatePtr(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) override;
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress) override;
    bool RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey) override;
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount) override;
    bool RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode) override;
    bool RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork) override;
    bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode) override;
    bool GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode) override;
    bool GetForkContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) override;
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode) override;
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo) override;
    bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount) override;
    bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock) override;
    bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount) override;
    bool GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount) override;
    bool VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainIdIn, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashBlock = uint256()) override;
    bool GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256()) override;
    bool ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock = uint256()) override;
    bool ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock = uint256()) override;
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value) override;
    bool GetBlockReceiptsByBlock(const uint256& hashFork, const uint256& hashFromBlock, const uint256& hashToBlock, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapBlockReceipts) override;
    bool VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock) override;
    bool GetPrevBlockHashList(const uint256& hashBlock, const uint32 nGetCount, std::vector<uint256>& vPrevBlockhash) override;

    typedef std::map<int, CCheckPoint> MapCheckPointsType;

    bool HasCheckPoints(const uint256& hashFork) const override;
    bool GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point) override;
    std::vector<IBlockChain::CCheckPoint> CheckPoints(const uint256& hashFork) const override;
    CCheckPoint LatestCheckPoint(const uint256& hashFork) const override;
    CCheckPoint UpperBoundCheckPoint(const uint256& hashFork, int nHeight) const override;
    bool VerifyCheckPoint(const uint256& hashFork, int nHeight, const uint256& nBlockHash) override;
    bool FindPreviousCheckPointBlock(const uint256& hashFork, CBlock& block) override;
    bool IsSameBranch(const uint256& hashFork, const CBlock& block) override;

    bool CalcBlockVoteRewardTx(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime, std::vector<CTransaction>& vVoteRewardTx) override;
    uint256 GetPrimaryBlockReward(const uint256& hashPrev) override;
    bool CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot,
                              uint256& hashCrosschainMerkleRoot, uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, bool& fMoStatus) override;
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CDestState& state) override;
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex) override;
    bool CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                      const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult) override;
    bool GetContractBalance(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract, const CDestination& destUser, uint256& nBalance) override;
    bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract) override;
    bool VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) override;

    bool GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner,
                          const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount) override;
    bool GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount) override;
    bool ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                             const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder) override;
    bool ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder) override;
    bool GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve) override;
    bool AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve) override;
    bool GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve) override;

    bool AddBlacklistAddress(const CDestination& dest) override;
    void RemoveBlacklistAddress(const CDestination& dest) override;
    bool IsExistBlacklistAddress(const CDestination& dest) override;
    void ListBlacklistAddress(set<CDestination>& setAddressOut) override;
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress) override;
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress) override;
    bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction) override;
    bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr) override;

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice) override;
    uint256 GetForkMintMinGasPrice(const uint256& hashFork) override;

    bool GetCandidatePubkey(const uint256& hashPrimaryBlock, std::vector<uint384>& vCandidatePubkey) override;
    bool GetPrevBlockCandidatePubkey(const uint256& hashBlock, std::vector<uint384>& vCandidatePubkey) override;
    bool VerifyBlockCommitVoteAggSig(const uint256& hashBlock, const bytes& btAggBitmap, const bytes& btAggSig) override;
    bool VerifyBlockCommitVoteAggSig(const uint256& hashVoteBlock, const uint256& hashRefBlock, const bytes& btAggBitmap, const bytes& btAggSig) override;

    bool AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock) override;
    bool RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock) override;
    bool GetMakerVoteBlock(const uint256& hashPrevBlock, bytes& btBitmap, bytes& btAggSig, uint256& hashVoteBlock) override;
    bool IsBlockConfirm(const uint256& hashBlock) override;
    bool AddBlockLocalVoteSignFlag(const uint256& hashBlock) override;
    bool VerifyPrimaryBlockConfirm(const uint256& hashBlock) override;

public:
    static int64 GetBlockInvestRewardTxMaxCount();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool InsertGenesisBlock(CBlock& block);
    bool GetBlockChanges(const CBlockIndex* pIndexNew, const CBlockIndex* pIndexFork,
                         std::vector<CBlockEx>& vBlockAddNew, std::vector<CBlockEx>& vBlockRemove);
    bool GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, const CBlockIndex* pIndexPrev,
                                   CDelegateAgreement& agreement, uint256& nEnrollTrust);
    Errno VerifyBlock(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev,
                      uint256& nReward, CDelegateAgreement& agreement, uint256& nEnrollTrust, CBlockIndex** ppIndexRef);
    bool VerifyBlockCertTx(const uint256& hashBlock, const CBlock& block);
    bool VerifyBlockVoteResult(const uint256& hashBlock, const CBlock& block);
    bool VerifyBlockCrosschainProve(const uint256& hashBlock, const CBlock& block);

    void InitCheckPoints();
    void InitCheckPoints(const uint256& hashFork, const std::map<int, uint256>& mapCheckPointsIn);

    bool VerifyVoteRewardTx(const CBlock& block, std::size_t& nRewardTxCount);
    Errno VerifyBlockTx(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& nReward,
                        const std::size_t nIgnoreVerifyTx, const std::map<CDestination, CAddressContext>& mapBlockAddress);
    bool CalcEndVoteReward(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime,
                           const uint256& hashFork, const uint256& hashCalcEndBlock, const uint256& hashCalcEndMainChainRefBlock, std::vector<std::vector<CTransaction>>& vRewardList);
    bool CalcDistributeVoteReward(const uint256& hashCalcEndBlock, std::map<CDestination, std::pair<CDestination, uint256>>& mapVoteReward);

protected:
    enum
    {
        MAX_CACHE_DISTRIBUTE_VOTE_REWARD_BLOCK_COUNT = 8
    };
    ICoreProtocol* pCoreProtocol;
    ITxPool* pTxPool;
    IForkManager* pForkManager;

    storage::CBlockBase cntrBlock;
    mtbase::CCache<uint256, CDelegateEnrolled> cacheEnrolled;
    mtbase::CCache<uint256, CDelegateAgreement> cacheAgreement;
    mtbase::CCache<uint256, CProofOfPiggyback> cachePiggyback;

    boost::mutex mutexBlsPubkey;
    CCacheBlsPubkey cacheBlsPubkey;

    std::map<uint256, MapCheckPointsType> mapForkCheckPoints;
    uint32 nMaxBlockRewardTxCount;

    boost::shared_mutex rwCvrAccess;
    std::map<uint256, std::map<uint256, std::vector<std::vector<CTransaction>>, CustomBlockHashCompare>> mapCacheDistributeVoteReward;  // key: fork, value.key: block. max size of value: MAX_CACHE_DISTRIBUTE_VOTE_REWARD_BLOCK_COUNT (means 'days')
};

} // namespace metabasenet

#endif //METABASENET_BLOCKCHAIN_H

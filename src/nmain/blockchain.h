// Copyright (c) 2021-2022 The MetabaseNet developers
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
    bool GetForkProfile(const uint256& hashFork, CProfile& profile) override;
    bool GetForkContext(const uint256& hashFork, CForkContext& ctxt) override;
    bool GetForkAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry) override;
    int GetBlockCount(const uint256& hashFork) override;
    bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight) override;
    bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight, uint256& hashNext) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, std::vector<uint256>& vBlockHash) override;
    bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) override;
    bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) override;
    bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime) override;
    bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) override;
    bool GetLastBlockTime(const uint256& hashFork, int nDepth, std::vector<int64>& vTime) override;
    bool GetBlock(const uint256& hashBlock, CBlock& block) override;
    bool GetBlockEx(const uint256& hashBlock, CBlockEx& block) override;
    bool GetOrigin(const uint256& hashFork, CBlock& block) override;
    bool Exists(const uint256& hashBlock) override;
    bool GetTransaction(const uint256& txid, CTransaction& tx) override;
    bool GetTransaction(const uint256& hashFork, const uint256& hashLastBlock, const uint256& txid, CTransaction& tx) override;
    bool GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight) override;
    bool ExistsTx(const uint256& txid) override;
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256()) override;
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock) override;
    int GetForkCreatedHeight(const uint256& hashFork) override;
    bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight) override;
    Errno AddNewBlock(const CBlock& block, CBlockChainUpdate& update) override;
    Errno AddNewOrigin(const CBlock& block, CBlockChainUpdate& update) override;
    bool GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits) override;
    bool GetBlockMintReward(const uint256& hashPrev, uint256& nReward) override;
    bool GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep) override;
    bool GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, std::size_t nMaxCount) override;
    bool GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled) override;
    bool GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement) override;
    bool GetVotes(const CDestination& destDelegate, uint256& nVotes) override;
    bool ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) override;
    bool VerifyRepeatBlock(const uint256& hashFork, const CBlock& block, const uint256& hashBlockRef) override;
    bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote) override;
    uint256 GetDelegateMinEnrollAmount(const uint256& hashBlock) override;
    bool GetDelegateCertTxCount(const uint256& hashLastBlock, std::map<CDestination, int>& mapVoteCert) override;
    uint256 GetBlockMoneySupply(const uint256& hashBlock) override;
    uint32 DPoSTimestamp(const uint256& hashPrev) override;
    Errno VerifyPowBlock(const CBlock& block, bool& fLongChain) override;
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt) override;
    bool CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update) override;
    bool VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock) override;
    bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime) override;
    bool IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block) override;
    int64 GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::vector<CTxInfo>& vTx) override;
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress) override;
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress) override;
    bool RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode) override;
    bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode) override;
    bool GetBlockWasmCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CTxContractData& txcdCode) override;
    bool GetContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) override;
    bool ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode) override;
    bool ListAddressTxInfo(const uint256& hashFork, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo) override;
    bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount) override;
    bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock) override;
    bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount) override;
    bool VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock = uint256()) override;

    /////////////    CheckPoints    /////////////////////
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
    bool CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot, uint256& nBlockGasUsed,
                              uint2048& nBlockBloom, const bool fCreateBlock, const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut) override;
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CDestState& state) override;
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed) override;
    bool CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult) override;
    bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract) override;
    bool VerifyCreateContractTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) override;

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
                                   CDelegateAgreement& agreement, std::size_t& nEnrollTrust);
    Errno VerifyBlock(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev,
                      uint256& nReward, CDelegateAgreement& agreement, std::size_t& nEnrollTrust, CBlockIndex** ppIndexRef);
    bool VerifyBlockCertTx(const CBlock& block);
    bool VerifyBlockAddress(const uint256& hashFork, const CBlock& block, std::map<uint256, CAddressContext>& mapBlockAddress);

    void InitCheckPoints();
    void InitCheckPoints(const uint256& hashFork, const std::map<int, uint256>& mapCheckPointsIn);

    bool VerifyVoteRewardTx(const CBlock& block, std::size_t& nRewardTxCount);
    Errno VerifyBlockTx(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint256& nReward,
                        const std::size_t nIgnoreVerifyTx, const std::map<uint256, CAddressContext>& mapBlockAddress);
    bool CalcEndVoteReward(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime,
                           const uint256& hashFork, const uint256& hashCalcEndBlock, std::vector<std::vector<CTransaction>>& vRewardList);
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
    hnbase::CCache<uint256, CDelegateEnrolled> cacheEnrolled;
    hnbase::CCache<uint256, CDelegateAgreement> cacheAgreement;

    std::map<uint256, MapCheckPointsType> mapForkCheckPoints;
    uint32 nMaxBlockRewardTxCount;

    boost::shared_mutex rwCvrAccess;
    std::map<uint256, std::map<uint256, std::vector<std::vector<CTransaction>>>> mapCacheDistributeVoteReward;
};

} // namespace metabasenet

#endif //METABASENET_BLOCKCHAIN_H

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CORE_H
#define METABASENET_CORE_H

#include "base.h"

namespace metabasenet
{

class CCoreProtocol : public ICoreProtocol
{
public:
    CCoreProtocol();
    virtual ~CCoreProtocol();
    virtual void InitializeGenesisBlock() override;
    virtual const uint256& GetGenesisBlockHash() override;
    virtual CChainId GetGenesisChainId() const override;
    static uint256 CreateGenesisStateRoot(const uint16 nBlockType, const uint64 nBlockTimeStamp, const CDestination& destOwner, const uint256& nAmount);
    static void CreateGenesisBlock(const bool fMainnet, const CChainId nChainIdIn, const string& strOwnerAddress, CBlock& block);
    virtual void GetGenesisBlock(CBlock& block) override;
    virtual Errno ValidateTransaction(const uint256& hashTxAtFork, const uint256& hashMainChainRefBlock, const CTransaction& tx) override;
    virtual Errno ValidateBlock(const uint256& hashFork, const uint256& hashMainChainRefBlock, const CBlock& block) override;
    virtual Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) override;

    virtual Errno VerifyTransaction(const uint256& txid, const CTransaction& tx, const uint256& hashFork, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<CDestination, CAddressContext>& mapBlockAddress) override;

    virtual Errno VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev) override;
    virtual Errno VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev, const CDelegateAgreement& agreement) override;
    virtual Errno VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef, const CDelegateAgreement& agreement) override;
    virtual bool GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, const uint256& nEnrollTrust = uint256()) override;
    virtual bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits) override;
    virtual uint256 GetDelegatedBallot(const int nBlockHeight, const uint256& nAgreement, const std::size_t& nWeight, const std::map<CDestination, std::size_t>& mapBallot,
                                       const std::vector<std::pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, std::vector<CDestination>& vBallot) override;
    virtual uint64 GetNextBlockTimestamp(const uint64 nPrevTimeStamp) override;

protected:
    bool HandleInitialize() override;
    Errno Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...);
    bool CheckBlockSignature(const uint256& hashFork, const CBlock& block);
    Errno ValidateVacantBlock(const CBlock& block);
    Errno VerifyCertTx(const uint256& hashFork, const CTransaction& tx, const std::map<CDestination, CAddressContext>& mapBlockAddress);
    Errno VerifyVoteTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CTemplateAddressContext& ctxToTemplate);
    Errno VerifyPledgeTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CTemplateAddressContext& ctxToTemplate);
    Errno VerifyVoteRedeemTx(const uint256& hashFork, const CTransaction& tx, const uint16 nFromTemplateType, const uint256& hashPrev);
    Errno VerifyVoteRewardLockTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CDestState& state);
    Errno VerifyActivateCodeTx(const uint256& hashFork, const uint256& hashPrev, const CTransaction& tx);
    Errno VerifyForkCreateTx(const uint256& hashTxAtFork, const CTransaction& tx, const int nHeight, const uint256& hashMainChainPrevBlock, const bytes& btToAddressData);
    Errno VerifyForkRedeemTx(const uint256& hashTxAtFork, const CTransaction& tx, const uint256& hashPrevBlock, const CDestState& state, const bytes& btFromAddressData);

protected:
    uint256 hashGenesisBlock;
    IBlockChain* pBlockChain;
    IForkManager* pForkManager;
};

class CTestNetCoreProtocol : public CCoreProtocol
{
public:
    CTestNetCoreProtocol();
    void GetGenesisBlock(CBlock& block) override;
};

class CProofOfWorkParam
{
public:
    CProofOfWorkParam(const bool fTestnetIn);
    ~CProofOfWorkParam();

public:
    bool fTestnet;
    uint256 nDelegateProofOfStakeEnrollMinimumAmount;
    uint256 nDelegateProofOfStakeEnrollMaximumAmount;
    uint256 hashGenesisBlock;

protected:
    ICoreProtocol* pCoreProtocol;

public:
    uint64 GetNextBlockTimestamp(const uint64 nPrevTimeStamp);
    Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const;
};

} // namespace metabasenet

#endif //METABASENET_BASE_H

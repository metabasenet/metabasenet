// Copyright (c) 2021-2022 The MetabaseNet developers
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
    virtual void GetGenesisBlock(CBlock& block) override;
    virtual Errno ValidateTransaction(const CTransaction& tx, int nHeight) override;
    virtual Errno ValidateBlock(const CBlock& block) override;
    virtual Errno VerifyForkCreateTx(const CTransaction& tx, const int nHeight, const bytes& btToAddressData) override;
    virtual Errno VerifyForkRedeemTx(const CTransaction& tx, const uint256& hashPrevBlock, const CDestState& state, const bytes& btFromAddressData) override;
    virtual Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) override;

    virtual Errno VerifyTransaction(const CTransaction& tx, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<uint256, CAddressContext>& mapBlockAddress) override;

    virtual Errno VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev) override;
    virtual Errno VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                              const CDelegateAgreement& agreement) override;
    virtual Errno VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                   const CDelegateAgreement& agreement) override;
    virtual bool GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, std::size_t nEnrollTrust = 0) override;
    virtual bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits) override;
    virtual void GetDelegatedBallot(const uint256& nAgreement, const std::size_t& nWeight, const std::map<CDestination, std::size_t>& mapBallot,
                                    const std::vector<std::pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, std::vector<CDestination>& vBallot, std::size_t& nEnrollTrust, int nBlockHeight) override;
    virtual uint256 MinEnrollAmount() override;
    virtual uint32 DPoSTimestamp(const CBlockIndex* pIndexPrev) override;
    virtual uint32 GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType) override;
    virtual uint32 CalcSingleBlockDistributeVoteRewardTxCount() override;

protected:
    bool HandleInitialize() override;
    Errno Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...);
    bool CheckBlockSignature(const CBlock& block);
    Errno ValidateVacantBlock(const CBlock& block);
    Errno VerifyCertTx(const CTransaction& tx);
    Errno VerifyVoteTx(const CTransaction& tx, const uint256& hashPrev);
    Errno VerifyVoteRedeemTx(const CTransaction& tx, const uint256& hashPrev);
    Errno VerifyVoteRewardLockTx(const CTransaction& tx, const uint256& hashPrev, const CDestState& state);
    Errno VerifyActivateCodeTx(const uint256& hashFork, const uint256& hashPrev, const CTransaction& tx);
    Errno VerifyDefiRelationTx(const uint256& hashPrev, const CTransaction& tx);

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
    uint32 GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType);
    Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const;
};

} // namespace metabasenet

#endif //METABASENET_BASE_H

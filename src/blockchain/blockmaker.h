// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_BLOCKMAKER_H
#define METABASENET_BLOCKMAKER_H

#include <atomic>

#include "base.h"
#include "event.h"
#include "key.h"

namespace metabasenet
{

class CBlockMakerHashAlgo
{
public:
    CBlockMakerHashAlgo(const std::string& strAlgoIn, int64 nHashRateIn)
      : strAlgo(strAlgoIn), nHashRate(nHashRateIn) {}
    virtual ~CBlockMakerHashAlgo() {}
    virtual uint256 Hash(const std::vector<unsigned char>& vchData) = 0;

public:
    const std::string strAlgo;
    int64 nHashRate;
};

class CBlockMakerProfile
{
public:
    CBlockMakerProfile() {}
    CBlockMakerProfile(int nAlgoIn, const CDestination& destOwnerIn, const uint256& nPrivKey, const uint32 nRewardRationIn)
      : nAlgo(nAlgoIn), destOwner(destOwnerIn), nRewardRation(nRewardRationIn)
    {
        keyMint.SetSecret(crypto::CCryptoKeyData(nPrivKey.begin(), nPrivKey.end()));
        BuildTemplate();
    }

    bool IsValid() const
    {
        return (templMint != nullptr);
    }
    bool BuildTemplate();
    std::size_t GetSignatureSize() const
    {
        std::size_t size = templMint->Export().size() + crypto::SIGN_DATA_SIZE;
        mtbase::CVarInt var(size);
        return (size + mtbase::GetSerializeSize(var));
    }
    const CDestination GetDestination() const
    {
        return (CDestination(templMint->GetTemplateId()));
    }
    bool GetTemplateData(uint8& nTemplateType, bytes& btTemplateData) const
    {
        if (templMint == nullptr)
        {
            return false;
        }
        nTemplateType = templMint->GetTemplateType();
        btTemplateData = templMint->Export();
        return true;
    }

public:
    int nAlgo;
    CDestination destOwner;
    crypto::CKey keyMint;
    CTemplateMintPtr templMint;
    uint32 nRewardRation;
};

class CBlockMaker : public IBlockMaker, virtual public CBlockMakerEventListener
{
public:
    CBlockMaker();
    ~CBlockMaker();
    bool HandleEvent(CEventBlockMakerUpdate& eventUpdate) override;

    bool GetMiningAddressList(std::vector<CDestination>& vMintAddressList) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool InterruptedPoW(const uint256& hashPrimary);
    bool WaitExit(const int64 nSeconds);
    bool WaitUpdateEvent(const int64 nSeconds);
    void PrepareBlock(CBlock& block, const uint256& hashPrev, const uint64 nPrevTime,
                      const uint32 nPrevHeight, const uint32 nPrevNumber, const CDelegateAgreement& agreement);
    bool ArrangeBlockTx(CBlock& block, const uint256& hashFork, const CBlockMakerProfile& profile);
    bool SignBlock(CBlock& block, const CBlockMakerProfile& profile);
    bool DispatchBlock(const uint256& hashFork, const CBlock& block);
    void ProcessDelegatedProofOfStake(const CAgreementBlock& consParam);
    void ProcessSubFork(const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                        const uint256& hashRefBlock, const uint64 nRefBlockTime, const int32 nPrevHeight, const uint16 nPrevMintType);
    bool CreateDelegatedBlock(CBlock& block, const uint256& hashFork, const CChainId nChainId, const uint256& hashMainChainRefBlock, const CBlockMakerProfile& profile);
    bool CreateProofOfWork();
    void PreparePiggyback(CBlock& block, const CDelegateAgreement& agreement, const uint256& hashRefBlock,
                          const uint64 nRefBlockTime, const int32 nPrevHeight, const CForkStatus& status, const uint16 nPrevMintType);
    void CreateExtended(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement, const uint256& hashRefBlock, const uint256& hashFork,
                        const CChainId nChainId, const uint256& hashPrevBlock, const uint64 nPrevTime, const uint64 nExtendedPrevNumber, const uint16 nExtendedPrevSlot);
    bool CreateVacant(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                      const uint256& hashRefBlock, const uint256& hashFork, const CChainId nChainId, const uint256& hashPrevBlock,
                      const uint256& hashPrevStateRoot, const uint64 nTime, const uint64 nBlockNumber);
    bool ReplenishSubForkVacant(const uint256& hashFork, const CChainId nChainId, int nLastBlockHeight, uint256& hashLastBlock, const uint256& hashLastStateRoot, uint64& nLastBlockNumber,
                                const CBlockMakerProfile& profile, const CDelegateAgreement& agreement, const uint256& hashRefBlock, const int32 nPrevHeight);

private:
    void BlockMakerThreadFunc();
    void PowThreadFunc();

protected:
    mtbase::CThread thrMaker;
    mtbase::CThread thrPow;
    boost::mutex mutex;
    boost::condition_variable condExit;
    boost::condition_variable condBlock;
    std::atomic<bool> fExit;
    CBlockStatus lastStatus;
    std::map<int, CBlockMakerHashAlgo*> mapHashAlgo;
    std::map<int, CBlockMakerProfile> mapWorkProfile;
    std::map<CDestination, CBlockMakerProfile> mapDelegatedProfile;
    std::map<uint256, std::pair<int, int>> mapForkLastStat;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    IForkManager* pForkManager;
    ITxPool* pTxPool;
    IDispatcher* pDispatcher;
    IConsensus* pConsensus;
    IService* pService;
};

} // namespace metabasenet

#endif //METABASENET_BLOCKMAKER_H

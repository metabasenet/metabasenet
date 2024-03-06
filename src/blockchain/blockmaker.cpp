// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockmaker.h"

#include <thread>

#include "template/delegate.h"
#include "template/mint.h"
#include "template/proof.h"
#include "util.h"

using namespace std;
using namespace mtbase;

#define INITIAL_HASH_RATE (8)
#define WAIT_AGREEMENT_TIME_OFFSET -5
#define WAIT_NEWBLOCK_TIME (BLOCK_TARGET_SPACING + 5)
#define WAIT_LAST_EXTENDED_TIME 0 //(BLOCK_TARGET_SPACING - 10)
#define FORK_LAST_BLOCK_COUNT 30
#define FORK_WAIT_BLOCK_COUNT 60
#define FORK_WAIT_BLOCK_SECT 100000

namespace metabasenet
{

//////////////////////////////
// CBlockMakerHashAlgo

class CHashAlgo_Cryptonight : public metabasenet::CBlockMakerHashAlgo
{
public:
    CHashAlgo_Cryptonight(int64 nHashRateIn)
      : CBlockMakerHashAlgo("Cryptonight", nHashRateIn) {}
    uint256 Hash(const std::vector<unsigned char>& vchData)
    {
        return crypto::CryptoPowHash(&vchData[0], vchData.size());
    }
};

//////////////////////////////
// CBlockMakerProfile
bool CBlockMakerProfile::BuildTemplate()
{
    if (destOwner == CDestination(keyMint.GetPubKey()))
    {
        return false;
    }
    if (nAlgo == CM_MPVSS)
    {
        templMint = CTemplateMint::CreateTemplatePtr(new CTemplateDelegate(CDestination(keyMint.GetPubKey()), destOwner, nRewardRation));
    }
    else if (nAlgo < CM_MAX)
    {
        templMint = CTemplateMint::CreateTemplatePtr(new CTemplateProof(CDestination(keyMint.GetPubKey()), destOwner));
    }
    return (templMint != nullptr);
}

//////////////////////////////
// CBlockMaker

CBlockMaker::CBlockMaker()
  : thrMaker("blockmaker", boost::bind(&CBlockMaker::BlockMakerThreadFunc, this)),
    thrPow("powmaker", boost::bind(&CBlockMaker::PowThreadFunc, this))
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pForkManager = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pConsensus = nullptr;
    pService = nullptr;
    mapHashAlgo[CM_CRYPTONIGHT] = new CHashAlgo_Cryptonight(INITIAL_HASH_RATE);
}

CBlockMaker::~CBlockMaker()
{
    for (auto it = mapHashAlgo.begin(); it != mapHashAlgo.end(); ++it)
    {
        delete ((*it).second);
    }
    mapHashAlgo.clear();
}

bool CBlockMaker::GetMiningAddressList(std::vector<CDestination>& vMintAddressList)
{
    for (auto& kv : mapDelegatedProfile)
    {
        vMintAddressList.push_back(kv.first);
    }
    return true;
}

bool CBlockMaker::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        StdError("blockmaker", "Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        StdError("blockmaker", "Failed to request blockchain");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        StdError("blockmaker", "Failed to request forkmanager");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        StdError("blockmaker", "Failed to request txpool");
        return false;
    }

    if (!GetObject("dispatcher", pDispatcher))
    {
        StdError("blockmaker", "Failed to request dispatcher");
        return false;
    }

    if (!GetObject("consensus", pConsensus))
    {
        StdError("blockmaker", "Failed to request consensus");
        return false;
    }

    if (!GetObject("service", pService))
    {
        StdError("blockmaker", "Failed to request service");
        return false;
    }

    for (auto& kv : MintConfig()->mapMint)
    {
        const uint256& keyMint = kv.first;
        const CDestination& destOwner = kv.second.first;
        const uint32 nRewardRatio = kv.second.second;
        CBlockMakerProfile profileDelegate(CM_MPVSS, destOwner, keyMint, nRewardRatio);
        if (profileDelegate.IsValid())
        {
            mapDelegatedProfile.insert(make_pair(profileDelegate.GetDestination(), profileDelegate));
        }
        if (mapWorkProfile.empty())
        {
            CBlockMakerProfile profilePow(CM_CRYPTONIGHT, destOwner, keyMint, 0);
            if (profilePow.IsValid())
            {
                mapWorkProfile.insert(make_pair(CM_CRYPTONIGHT, profilePow));
            }
        }
    }

    // print log
    const char* ConsensusMethodName[CM_MAX] = { "mpvss", "cryptonight" };
    StdLog("blockmaker", "Block maker started");
    for (map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.begin();
         it != mapDelegatedProfile.end(); ++it)
    {
        CBlockMakerProfile& profile = (*it).second;
        StdLog("blockmaker", "Profile [%s] : dest=%s,pubkey=%s,pos=%s",
               ConsensusMethodName[CM_MPVSS],
               profile.destOwner.ToString().c_str(),
               profile.keyMint.GetPubKey().GetHex().c_str(),
               CDestination(profile.templMint->GetTemplateId()).ToString().c_str());
    }
    for (map<int, CBlockMakerProfile>::iterator it = mapWorkProfile.begin(); it != mapWorkProfile.end(); ++it)
    {
        CBlockMakerProfile& profile = (*it).second;
        StdLog("blockmaker", "Profile [%s] : dest=%s,pubkey=%s,poa=%s",
               ConsensusMethodName[(*it).first],
               profile.destOwner.ToString().c_str(),
               profile.keyMint.GetPubKey().GetHex().c_str(),
               CDestination(profile.templMint->GetTemplateId()).ToString().c_str());
    }

    return true;
}

void CBlockMaker::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pForkManager = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pConsensus = nullptr;
    pService = nullptr;

    mapWorkProfile.clear();
    mapDelegatedProfile.clear();
}

bool CBlockMaker::HandleInvoke()
{
    if (!IBlockMaker::HandleInvoke())
    {
        return false;
    }

    fExit = false;
    if (!ThreadDelayStart(thrMaker))
    {
        return false;
    }

    if (!mapWorkProfile.empty())
    {
        if (!ThreadDelayStart(thrPow))
        {
            return false;
        }
    }

    if (!fExit)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), lastStatus);
    }
    return true;
}

void CBlockMaker::HandleHalt()
{
    fExit = true;
    condExit.notify_all();
    condBlock.notify_all();

    thrMaker.Interrupt();
    ThreadExit(thrMaker);

    thrPow.Interrupt();
    ThreadExit(thrPow);

    IBlockMaker::HandleHalt();
}

bool CBlockMaker::HandleEvent(CEventBlockMakerUpdate& eventUpdate)
{
    if (fExit)
    {
        return true;
    }

    {
        uint256 nAgreement;
        size_t nWeight;
        vector<CDestination> vBallot;
        pConsensus->GetAgreement(eventUpdate.data.nBlockHeight, nAgreement, nWeight, vBallot);

        string strMintType = "pos";
        if (eventUpdate.data.nMintType == CTransaction::TX_WORK)
        {
            strMintType = "poa";
        }

        if (vBallot.size() == 0)
        {
            StdLog("blockmaker", "MakerUpdate: height: %d, consensus: poa, block type: %s, block: %s",
                   eventUpdate.data.nBlockHeight, strMintType.c_str(), eventUpdate.data.hashBlock.GetHex().c_str());
        }
        else
        {
            StdLog("blockmaker", "MakerUpdate: height: %d, consensus: pos, block type: %s, block: %s, ballot address: %s",
                   eventUpdate.data.nBlockHeight, strMintType.c_str(), eventUpdate.data.hashBlock.GetHex().c_str(), vBallot[0].ToString().c_str());
        }
    }

    {
        boost::unique_lock<boost::mutex> lock(mutex);

        CBlockMakerUpdate& data = eventUpdate.data;
        lastStatus.hashPrevBlock = data.hashPrevBlock;
        lastStatus.hashBlock = data.hashBlock;
        lastStatus.nBlockTime = data.nBlockTime;
        lastStatus.nBlockHeight = data.nBlockHeight;
        lastStatus.nBlockNumber = data.nBlockNumber;
        lastStatus.nMintType = data.nMintType;

        condBlock.notify_all();
    }
    return true;
}

bool CBlockMaker::InterruptedPoW(const uint256& hashPrimary)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    return fExit || (hashPrimary != lastStatus.hashBlock);
}

bool CBlockMaker::WaitExit(const int64 nSeconds)
{
    if (nSeconds <= 0)
    {
        return !fExit;
    }
    boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
    boost::unique_lock<boost::mutex> lock(mutex);
    while (!fExit)
    {
        if (!condExit.timed_wait(lock, timeout))
        {
            break;
        }
    }
    return !fExit;
}

bool CBlockMaker::WaitUpdateEvent(const int64 nSeconds)
{
    if (nSeconds > 0)
    {
        boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
        boost::unique_lock<boost::mutex> lock(mutex);
        while (!fExit)
        {
            if (!condBlock.timed_wait(lock, timeout))
            {
                break;
            }
        }
    }
    return !fExit;
}

bool CBlockMaker::PrepareBlock(CBlock& block, const uint256& hashPrev, const uint64 nPrevTime,
                               const uint32 nPrevHeight, const uint32 nPrevNumber, const CDelegateAgreement& agreement)
{
    block.SetNull();
    block.nType = CBlock::BLOCK_PRIMARY;
    block.hashPrev = hashPrev;
    block.nNumber = nPrevNumber + 1;
    block.nHeight = nPrevHeight + 1;
    block.nSlot = 0;

    CProofOfDelegate proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;

    if (agreement.nAgreement != 0)
    {
        pConsensus->GetProof(nPrevHeight + 1, proof.btDelegateData);
    }
    block.AddDelegateProof(proof);

    if (!pBlockChain->VerifyPrimaryBlockConfirm(hashPrev))
    {
        pDispatcher->NotifyBlockVoteChnNewBlock(hashPrev);
        return false;
    }

    bytes btDbBitmap;
    bytes btDbAggSig;
    bool fDbAtChain = false;
    uint256 hashDbAtBlock;
    if (pBlockChain->RetrieveBlockVoteResult(hashPrev, btDbBitmap, btDbAggSig, fDbAtChain, hashDbAtBlock) && !fDbAtChain)
    {
        block.AddBlockVoteSig(CBlockVoteSig(hashPrev, btDbBitmap, btDbAggSig));
    }

    std::map<CChainId, CBlockProve> mapCrosschainProve;
    if (pBlockChain->GetCrosschainProveForPrevBlock(block.GetChainId(), block.hashPrev, mapCrosschainProve))
    {
        block.mapProve = mapCrosschainProve;
    }
    return true;
}

bool CBlockMaker::ArrangeBlockTx(CBlock& block, const uint256& hashFork, const CBlockMakerProfile& profile, bool& fMoStatus)
{
    size_t nRestOfSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - profile.GetSignatureSize() - 32 * 5;
    size_t rewardTxSize = 0;
    uint256 nRewardTxTotalFee;
    vector<CTransaction> vVoteRewardTx;
    if (!pBlockChain->CalcBlockVoteRewardTx(block.hashPrev, block.nType, block.GetBlockHeight(), block.GetBlockTime(), vVoteRewardTx))
    {
        StdError("blockmaker", "Arrange Block Tx: Calc Block Vote Reward Tx error, prev block: %s, fork: %s", block.hashPrev.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    for (const CTransaction& tx : vVoteRewardTx)
    {
        size_t nTxSize = GetSerializeSize(tx);
        if (rewardTxSize + nTxSize > nRestOfSize)
        {
            StdError("blockmaker", "Arrange Block Tx: Reward tx size error, prev block: %s, fork: %s", block.hashPrev.ToString().c_str(), hashFork.ToString().c_str());
            return false;
        }
        block.vtx.push_back(tx);
        nRewardTxTotalFee += tx.GetTxFee();
        rewardTxSize += nTxSize;
    }

    size_t nMaxTxSize = nRestOfSize - rewardTxSize;
    uint256 nTotalTxFee;
    if (!pTxPool->FetchArrangeBlockTx(hashFork, block.hashPrev, block.GetBlockTime(), nMaxTxSize, block.vtx, nTotalTxFee))
    {
        StdError("blockmaker", "Arrange Block Tx: Fetch arrange block tx error, prev block: %s, fork: %s", block.hashPrev.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    //uint256 nMintCoin(block.txMint.GetAmount());
    //block.txMint.SetAmount(block.txMint.GetAmount() + nTotalTxFee + nRewardTxTotalFee);
    if (!block.IsExtended())
    {
        block.AddMintCoinProof(block.txMint.GetAmount());
    }
    block.txMint.SetAmount(0);
    block.nGasLimit = MAX_BLOCK_GAS_LIMIT;

    uint256 nTotalMintReward;
    if (!pBlockChain->CreateBlockStateRoot(hashFork, block, block.hashStateRoot, block.hashReceiptsRoot,
                                           block.hashCrosschainMerkleRoot, block.nGasUsed, block.btBloomData, nTotalMintReward, fMoStatus))
    {
        StdError("blockmaker", "Arrange Block Tx: Get block state root fail, prev block: %s, fork: %s", block.hashPrev.ToString().c_str(), hashFork.ToString().c_str());
        pTxPool->ClearTxPool(hashFork);
        return false;
    }
    //block.txMint.SetAmount(0);

    // if (!block.IsExtended())
    // {
    //     block.AddMintCoinProof(nMintCoin);
    // }
    block.AddMintRewardProof(nTotalMintReward);
    block.UpdateMerkleRoot();
    return true;
}

bool CBlockMaker::SignBlock(CBlock& block, const CBlockMakerProfile& profile)
{
    const uint256 hashBlock = block.GetHash();
    bytes btSigData;
    if (!profile.keyMint.Sign(hashBlock, btSigData))
    {
        StdTrace("blockmaker", "keyMint Sign failed. block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    block.SetSignData(btSigData);
    return true;
}

bool CBlockMaker::DispatchBlock(const uint256& hashFork, const CBlock& block)
{
    const uint256 hashBlock = block.GetHash();
    StdDebug("blockmaker", "Dispatching block: %s, type: %u", hashBlock.ToString().c_str(), block.nType);
    int64 nWait = block.GetBlockTime() - GetNetTime();
    if (!WaitExit(nWait))
    {
        return false;
    }

    if (!block.IsExtended() && !pBlockChain->VerifyCheckPoint(hashFork, block.GetBlockHeight(), hashBlock))
    {
        StdError("blockmaker", "Fork %s block at height %d does not match checkpoint hash", hashFork.ToString().c_str(), block.GetBlockHeight());
        return false;
    }

    Errno err = pDispatcher->AddNewBlock(block);
    if (err != OK)
    {
        if (err != ERR_ALREADY_HAVE)
        {
            StdError("blockmaker", "Dispatch new block failed (%d) : %s", err, ErrorString(err));
            pTxPool->ClearTxPool(hashFork);
            return false;
        }
        StdDebug("blockmaker", "Dispatching block: %s already have, type: %u", hashBlock.ToString().c_str(), block.nType);
    }
    return true;
}

bool CBlockMaker::ProcessDelegatedProofOfStake(const CAgreementBlock& consParam)
{
    map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.find(consParam.agreement.vBallot[0]);
    if (it != mapDelegatedProfile.end())
    {
        CBlockMakerProfile& profile = (*it).second;

        CBlock block;
        if (!PrepareBlock(block, consParam.hashPrev, consParam.nPrevTime, consParam.nPrevHeight, consParam.nPrevNumber, consParam.agreement))
        {
            StdDebug("blockmaker", "Process delegated proof of stake: Prepare block fail, prev block: %s", consParam.hashPrev.ToString().c_str());
            return false;
        }

        // get block time
        uint64 nBlockTime = pBlockChain->GetNextBlockTimestamp(block.hashPrev);
        if (nBlockTime == 0)
        {
            StdError("blockmaker", "Process delegated proof of stake: Get next block timestamp error, hashPrev: %s", block.hashPrev.ToString().c_str());
            return false;
        }
        uint64 nCurrTime = GetNetTime();
        if (nCurrTime > nBlockTime)
        {
            nBlockTime += (((nCurrTime - nBlockTime) / BLOCK_TARGET_SPACING) * BLOCK_TARGET_SPACING);
        }
        block.SetBlockTime(nBlockTime);

        // create PoS primary block
        if (!CreateDelegatedBlock(block, pCoreProtocol->GetGenesisBlockHash(), pCoreProtocol->GetGenesisChainId(), block.hashPrev, profile))
        {
            StdError("blockmaker", "Process delegated proof of stake: Create delegated block error, hashPrev: %s", block.hashPrev.ToString().c_str());
            return false;
        }

        // dispatch PoS primary block
        if (DispatchBlock(pCoreProtocol->GetGenesisBlockHash(), block))
        {
            pDispatcher->SetConsensus(consParam);

            pDispatcher->CheckAllSubForkLastBlock();

            // create sub fork blocks
            ProcessSubFork(profile, consParam.agreement, block.GetHash(), block.GetBlockTime(), consParam.nPrevHeight, consParam.nPrevMintType);
        }
    }
    return true;
}

void CBlockMaker::ProcessSubFork(const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                                 const uint256& hashRefBlock, const uint64 nRefBlockTime, const int32 nPrevHeight, const uint16 nPrevMintType)
{
    map<uint256, CForkStatus> mapForkStatus;
    pBlockChain->GetForkStatus(mapForkStatus);

    // create subsidiary task
    multimap<uint64, pair<uint256, CBlock>> mapBlocks;
    map<uint256, CChainId> mapChainId;
    for (map<uint256, CForkStatus>::iterator it = mapForkStatus.begin(); it != mapForkStatus.end(); ++it)
    {
        const uint256& hashFork = it->first;
        const CForkStatus& status = it->second;
        if (hashFork != pCoreProtocol->GetGenesisBlockHash() && pForkManager->IsAllowed(hashFork))
        {
            CBlock block;
            PreparePiggyback(block, agreement, hashRefBlock, nRefBlockTime, nPrevHeight, status, nPrevMintType);
            mapBlocks.insert(make_pair(nRefBlockTime, make_pair(hashFork, block)));

            CForkContext ctxSubFork;
            if (!pBlockChain->GetForkContext(hashFork, ctxSubFork, hashRefBlock))
            {
                StdError("blockmaker", "Process sub fork: Get fork context fail, fork: %s", hashFork.ToString().c_str());
                return;
            }
            mapChainId[hashFork] = ctxSubFork.nChainId;
        }
    }

    while (!mapBlocks.empty())
    {
        auto it = mapBlocks.begin();
        int64 nSeconds = it->first - GetNetTime();
        const uint256 hashFork = it->second.first;
        CBlock block = it->second.second;
        mapBlocks.erase(it);

        if (!WaitExit(nSeconds))
        {
            break;
        }

        bool fCreateExtendedTask = false;
        uint256 hashExtendedPrevBlock;
        uint64 nExtendedPrevTime = 0;
        uint64 nExtendedPrevNumber = 0;
        uint16 nExtendedPrevSlot = 0;

        if (block.IsSubsidiary())
        {
            // query previous last extended block
            if (block.hashPrev == 0)
            {
                CBlockStatus status;
                if (pBlockChain->GetLastBlockStatus(hashFork, status))
                {
                    if (!pBlockChain->VerifyForkRefLongChain(hashFork, status.hashBlock, hashRefBlock))
                    {
                        StdError("blockmaker", "Process sub fork: fork does not refer to long chain, fork: %s", hashFork.ToString().c_str());
                    }
                    else if (status.nBlockHeight > nPrevHeight)
                    {
                        if (pBlockChain->GetLastBlockOfHeight(hashFork, nPrevHeight, status.hashBlock, status.nBlockTime))
                        {
                            block.hashPrev = status.hashBlock;
                        }
                        else
                        {
                            StdError("blockmaker", "Process sub fork: get last block error, fork: %s", hashFork.ToString().c_str());
                        }
                    }
                    else if (status.nBlockHeight == nPrevHeight)
                    {
                        if (nPrevMintType != CTransaction::TX_STAKE
                            || status.nBlockTime + EXTENDED_BLOCK_SPACING == nRefBlockTime
                            || GetNetTime() >= WAIT_LAST_EXTENDED_TIME + nRefBlockTime)
                        {
                            block.hashPrev = status.hashBlock;
                        }
                        else
                        {
                            mapBlocks.insert(make_pair(GetNetTime() + 1, make_pair(hashFork, block)));
                        }
                    }
                    else
                    {
                        if (status.nBlockHeight != CBlock::GetBlockHeightByHash(hashFork)
                            && status.nBlockHeight < nPrevHeight + 1 - FORK_LAST_BLOCK_COUNT)
                        {
                            auto& forkStat = mapForkLastStat[hashFork];
                            if (forkStat.first != status.nBlockHeight)
                            {
                                forkStat.first = status.nBlockHeight;
                                forkStat.second = nPrevHeight + 1;
                            }
                            if (nPrevHeight + 1 - forkStat.second >= FORK_WAIT_BLOCK_COUNT * ((nPrevHeight + 1 - status.nBlockHeight) / FORK_WAIT_BLOCK_SECT + 1))
                            {
                                if (ReplenishSubForkVacant(hashFork, mapChainId[hashFork], status.nBlockHeight, status.hashBlock, status.hashStateRoot, status.nBlockNumber, profile, agreement, hashRefBlock, nPrevHeight))
                                {
                                    block.hashPrev = status.hashBlock;
                                    block.nNumber = status.nBlockNumber + 1;
                                }
                                else
                                {
                                    StdError("blockmaker", "Process sub fork: replenish vacant error, fork: %s", hashFork.ToString().c_str());
                                }
                            }
                            else
                            {
                                StdError("blockmaker", "Process sub fork: fork is missing too many blocks, primary height: %d, fork height: %d, fork: %s",
                                         nPrevHeight + 1, status.nBlockHeight, hashFork.ToString().c_str());
                            }
                        }
                        else
                        {
                            if (nPrevMintType != CTransaction::TX_STAKE || GetNetTime() >= WAIT_LAST_EXTENDED_TIME + nRefBlockTime)
                            {
                                if (ReplenishSubForkVacant(hashFork, mapChainId[hashFork], status.nBlockHeight, status.hashBlock, status.hashStateRoot, status.nBlockNumber, profile, agreement, hashRefBlock, nPrevHeight))
                                {
                                    block.hashPrev = status.hashBlock;
                                    block.nNumber = status.nBlockNumber + 1;
                                }
                                else
                                {
                                    StdError("blockmaker", "Process sub fork: replenish vacant error, fork: %s", hashFork.ToString().c_str());
                                }
                            }
                            else
                            {
                                mapBlocks.insert(make_pair(GetNetTime() + 1, make_pair(hashFork, block)));
                            }
                        }
                    }
                }
                else
                {
                    StdError("blockmaker", "Process sub fork: GetLastBlock fail, fork: %s", hashFork.ToString().c_str());
                }
            }

            // make subsidiary block
            if (block.hashPrev != 0)
            {
                if (CreateDelegatedBlock(block, hashFork, mapChainId[hashFork], hashRefBlock, profile))
                {
                    if (DispatchBlock(hashFork, block))
                    {
                        fCreateExtendedTask = true;
                        hashExtendedPrevBlock = block.GetHash();
                        nExtendedPrevTime = block.GetBlockTime();
                        nExtendedPrevNumber = block.GetBlockNumber();
                        nExtendedPrevSlot = block.GetBlockSlot();
                    }
                    else
                    {
                        StdError("blockmaker", "Process sub fork: dispatch subsidiary block error, fork: %s, block: %s", hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                    }
                }
                else
                {
                    StdError("blockmaker", "Process sub fork: Create Delegated Block error, fork: %s, block: %s", hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                }
            }
        }
        else
        {
            // make extended block
            bool fContinue = false;
            do
            {
                bool fMoStatus = false;
                if (!ArrangeBlockTx(block, hashFork, profile, fMoStatus))
                {
                    StdError("blockmaker", "Process sub fork: Arrange Block Tx fail, fork: %s, block: %s",
                             hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                    fContinue = true;
                    break;
                }
                if (block.vtx.size() == 0 && block.mapProve.size() == 0 && !fMoStatus)
                {
                    fContinue = true;
                    break;
                }
                if (!SignBlock(block, profile))
                {
                    StdError("blockmaker", "Process sub fork: extended block sign error, fork: %s, block: %s, seq: %lu",
                             hashFork.ToString().c_str(), block.GetHash().ToString().c_str(),
                             (block.GetBlockTime() - nRefBlockTime) / EXTENDED_BLOCK_SPACING);
                    fContinue = true;
                    break;
                }
                if (!DispatchBlock(hashFork, block))
                {
                    StdError("blockmaker", "Process sub fork: dispatch subsidiary block error, fork: %s, block: %s",
                             hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                    fContinue = true;
                    break;
                }

                fCreateExtendedTask = true;
                hashExtendedPrevBlock = block.GetHash();
                nExtendedPrevTime = block.GetBlockTime();
                nExtendedPrevNumber = block.GetBlockNumber();
                nExtendedPrevSlot = block.GetBlockSlot();
            } while (0);

            if (fContinue)
            {
                fCreateExtendedTask = true;
                hashExtendedPrevBlock = block.hashPrev;
                nExtendedPrevTime = block.GetBlockTime();
                nExtendedPrevNumber = block.GetBlockNumber() - 1;
                nExtendedPrevSlot = block.GetBlockSlot();
            }
        }

        // create next extended task
        if (fCreateExtendedTask && nExtendedPrevTime + EXTENDED_BLOCK_SPACING < nRefBlockTime + BLOCK_TARGET_SPACING && GetNetTime() < nRefBlockTime + BLOCK_TARGET_SPACING)
        {
            CBlock extended;
            CreateExtended(extended, profile, agreement, hashRefBlock, hashFork, mapChainId[hashFork], hashExtendedPrevBlock, nExtendedPrevTime, nExtendedPrevNumber, nExtendedPrevSlot);
            mapBlocks.insert(make_pair(extended.GetBlockTime(), make_pair(hashFork, extended)));
        }
    }
}

bool CBlockMaker::CreateDelegatedBlock(CBlock& block, const uint256& hashFork, const CChainId nChainId, const uint256& hashMainChainRefBlock, const CBlockMakerProfile& profile)
{
    CDestination destSendTo = profile.GetDestination();

    uint256 nReward;
    if (!pBlockChain->GetBlockMintReward(block.hashPrev, false, nReward, hashMainChainRefBlock))
    {
        StdError("blockmaker", "Create delegate block: Get block mint reward error, hashPrev: %s", block.hashPrev.ToString().c_str());
        return false;
    }

    CTransaction& txMint = block.txMint;

    txMint.SetTxType(CTransaction::TX_STAKE);
    txMint.SetNonce(block.nNumber);
    txMint.SetChainId(nChainId);
    txMint.SetToAddress(destSendTo);
    txMint.SetAmount(nReward);
    txMint.SetGasPrice(0);
    txMint.SetGasLimit(0);

    bytes btBitmap;
    bytes btAggSig;
    uint256 hashVoteBlock;
    if (pBlockChain->GetMakerVoteBlock(block.hashPrev, btBitmap, btAggSig, hashVoteBlock))
    {
        block.AddBlockVoteSig(CBlockVoteSig(hashVoteBlock, btBitmap, btAggSig));
    }

    std::map<CChainId, CBlockProve> mapCrosschainProve;
    if (pBlockChain->GetCrosschainProveForPrevBlock(block.GetChainId(), block.hashPrev, mapCrosschainProve))
    {
        block.mapProve = mapCrosschainProve;
    }

    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, block.hashPrev, destSendTo, ctxAddress))
    {
        uint8 nTemplateType = 0;
        bytes btTempData;
        if (!profile.GetTemplateData(nTemplateType, btTempData))
        {
            StdError("blockmaker", "Create delegate block: Get template data fail, hashPrev: %s", block.hashPrev.ToString().c_str());
            return false;
        }
        CTemplateAddressContext ctxTemplate(string(), string(), nTemplateType, btTempData);
        txMint.SetToAddressData(CAddressContext(ctxTemplate));
    }

    bool fMoStatus = false;
    if (!ArrangeBlockTx(block, hashFork, profile, fMoStatus))
    {
        StdError("blockmaker", "Create delegate block: Arrange block tx fail, hashPrev: %s", block.hashPrev.ToString().c_str());
        return false;
    }

    return SignBlock(block, profile);
}

void CBlockMaker::PreparePiggyback(CBlock& block, const CDelegateAgreement& agreement, const uint256& hashRefBlock,
                                   const uint64 nRefBlockTime, const int32 nPrevHeight, const CForkStatus& status, const uint16 nPrevMintType)
{
    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_SUBSIDIARY;
    block.SetBlockTime(nRefBlockTime);
    block.nNumber = status.nLastBlockNumber + 1;
    block.nHeight = CBlock::GetBlockHeightByHash(hashRefBlock);
    block.nSlot = 0;

    block.AddPiggybackProof(proof);
    /*if (status.nLastBlockHeight == nPrevHeight && status.nLastBlockTime < nRefBlockTime)
    {
        // last is PoW or last extended or timeouot
        if (nPrevMintType != CTransaction::TX_STAKE
            || status.nLastBlockTime + EXTENDED_BLOCK_SPACING == nRefBlockTime
            || GetNetTime() - nRefBlockTime >= WAIT_LAST_EXTENDED_TIME)
        {
            block.hashPrev = status.hashLastBlock;
        }
    }*/
}

void CBlockMaker::CreateExtended(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement, const uint256& hashRefBlock, const uint256& hashFork,
                                 const CChainId nChainId, const uint256& hashPrevBlock, const uint64 nPrevTime, const uint64 nExtendedPrevNumber, const uint16 nExtendedPrevSlot)
{
    CDestination destSendTo = profile.GetDestination();

    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_EXTENDED;
    block.SetBlockTime(nPrevTime + EXTENDED_BLOCK_SPACING);
    block.nNumber = nExtendedPrevNumber + 1;
    block.nHeight = CBlock::GetBlockHeightByHash(hashRefBlock);
    block.nSlot = nExtendedPrevSlot + 1;
    block.hashPrev = hashPrevBlock;

    block.AddPiggybackProof(proof);

    bytes btBitmap;
    bytes btAggSig;
    uint256 hashVoteBlock;
    if (pBlockChain->GetMakerVoteBlock(block.hashPrev, btBitmap, btAggSig, hashVoteBlock))
    {
        block.AddBlockVoteSig(CBlockVoteSig(hashVoteBlock, btBitmap, btAggSig));
    }

    std::map<CChainId, CBlockProve> mapCrosschainProve;
    if (pBlockChain->GetCrosschainProveForPrevBlock(block.GetChainId(), block.hashPrev, mapCrosschainProve))
    {
        block.mapProve = mapCrosschainProve;
    }

    CTransaction& txMint = block.txMint;

    txMint.SetTxType(CTransaction::TX_STAKE);
    txMint.SetNonce(block.nNumber);
    txMint.SetChainId(nChainId);
    txMint.SetToAddress(destSendTo);
    txMint.SetAmount(0);
    txMint.SetGasPrice(0);
    txMint.SetGasLimit(0);

    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, block.hashPrev, destSendTo, ctxAddress))
    {
        uint8 nTemplateType = 0;
        bytes btTempData;
        if (!profile.GetTemplateData(nTemplateType, btTempData))
        {
            StdError("blockmaker", "Get template data fail, hashPrev: %s", block.hashPrev.ToString().c_str());
            return;
        }
        CTemplateAddressContext ctxTemplate(string(), string(), nTemplateType, btTempData);
        txMint.SetToAddressData(CAddressContext(ctxTemplate));
    }
}

bool CBlockMaker::CreateVacant(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                               const uint256& hashRefBlock, const uint256& hashFork, const CChainId nChainId, const uint256& hashPrevBlock,
                               const uint256& hashPrevStateRoot, const uint64 nTime, const uint64 nBlockNumber, const uint32 nBlockHeight)
{
    CDestination destSendTo = profile.GetDestination();

    block.SetNull();

    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_VACANT;
    block.SetBlockTime(nTime);
    block.nNumber = nBlockNumber;
    block.nHeight = nBlockHeight;
    block.nSlot = 0;
    block.hashPrev = hashPrevBlock;

    block.AddPiggybackProof(proof);

    CTransaction& txMint = block.txMint;

    txMint.SetTxType(CTransaction::TX_STAKE);
    txMint.SetNonce(block.nNumber);
    txMint.SetChainId(nChainId);
    txMint.SetToAddress(destSendTo);
    txMint.SetAmount(0);
    txMint.SetGasPrice(0);
    txMint.SetGasLimit(0);

    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, block.hashPrev, destSendTo, ctxAddress))
    {
        uint8 nTemplateType = 0;
        bytes btTempData;
        if (!profile.GetTemplateData(nTemplateType, btTempData))
        {
            StdError("blockmaker", "Get template data fail, hashPrev: %s", block.hashPrev.ToString().c_str());
            return false;
        }
        CTemplateAddressContext ctxTemplate(string(), string(), nTemplateType, btTempData);
        txMint.SetToAddressData(CAddressContext(ctxTemplate));
    }

    uint256 nTotalMintReward;
    bool fMoStatus = false;
    if (!pBlockChain->CreateBlockStateRoot(hashFork, block, block.hashStateRoot, block.hashReceiptsRoot,
                                           block.hashCrosschainMerkleRoot, block.nGasUsed, block.btBloomData, nTotalMintReward, fMoStatus))
    {
        StdError("blockmaker", "Create vacant: Get block state root fail, prev block: %s", hashPrevBlock.ToString().c_str());
        return false;
    }
    block.UpdateMerkleRoot();

    return SignBlock(block, profile);
}

bool CBlockMaker::ReplenishSubForkVacant(const uint256& hashFork, const CChainId nChainId, int nLastBlockHeight, uint256& hashLastBlock, const uint256& hashLastStateRoot, uint64& nLastBlockNumber,
                                         const CBlockMakerProfile& profile, const CDelegateAgreement& agreement, const uint256& hashRefBlock, const int32 nPrevHeight)
{
    vector<uint64> vTime;
    int nPrimaryHeight = nPrevHeight + 1;
    int nNextHeight = nLastBlockHeight + 1;
    int nDepth = nPrimaryHeight - nLastBlockHeight;
    if (!pBlockChain->GetLastBlockTime(pCoreProtocol->GetGenesisBlockHash(), nDepth, vTime))
    {
        StdError("blockmaker", "Replenish vacant: Get last block time fail");
        return false;
    }
    while (nNextHeight < nPrimaryHeight)
    {
        CBlock block;
        if (!CreateVacant(block, profile, agreement, hashRefBlock, hashFork, nChainId, hashLastBlock, hashLastStateRoot, vTime[nPrimaryHeight - nNextHeight], nLastBlockNumber + 1, nNextHeight))
        {
            StdError("blockmaker", "Replenish vacant: create vacane fail");
            return false;
        }
        if (!DispatchBlock(hashFork, block))
        {
            StdError("blockmaker", "Replenish vacant: dispatch block fail");
            return false;
        }
        hashLastBlock = block.GetHash();
        nNextHeight++;
        nLastBlockNumber++;
        StdTrace("blockmaker", "Replenish vacant: height: %d, block: %s, fork: %s",
                 block.GetBlockHeight(), hashLastBlock.GetHex().c_str(), hashFork.GetHex().c_str());
    }
    return true;
}

//---------------------------------------------------------------
bool CBlockMaker::CreateProofOfWork()
{
    int nConsensus = CM_CRYPTONIGHT;
    map<int, CBlockMakerProfile>::iterator it = mapWorkProfile.find(nConsensus);
    if (it == mapWorkProfile.end())
    {
        StdError("blockmaker", "did not find Work profile");
        return false;
    }
    CBlockMakerProfile& profile = (*it).second;
    CBlockMakerHashAlgo* pHashAlgo = mapHashAlgo[profile.nAlgo];
    if (pHashAlgo == nullptr)
    {
        StdError("blockmaker", "pHashAlgo is null");
        return false;
    }

    vector<unsigned char> vchWorkData;
    int nPrevBlockHeight = 0;
    uint256 hashPrev;
    int nAlgo = 0, nBits = 0;
    if (!pService->GetWork(vchWorkData, nPrevBlockHeight, hashPrev, nAlgo, nBits, profile.templMint))
    {
        return false;
    }

    uint32& nTime = *((uint32*)&vchWorkData[4]);
    uint64_t& nNonce = *((uint64_t*)&vchWorkData[vchWorkData.size() - sizeof(uint64_t)]);
    nNonce = (GetTime() % 0xFFFFFF) << 40;

    int64& nHashRate = pHashAlgo->nHashRate;
    int64 nHashComputeCount = 0;
    int64 nHashComputeBeginTime = GetTime();
    int64 nTimeDiff = (int64)nTime - GetNetTime();

    StdLog("blockmaker", "Proof-of-work: start hash compute, target height: %d, difficulty bits: (%d)", nPrevBlockHeight + 1, nBits);

    uint256 hashTarget = (~uint256(uint64(0)) >> nBits);
    while (!InterruptedPoW(hashPrev))
    {
        if (nHashRate == 0)
        {
            nHashRate = 1;
        }
        for (int i = 0; i < nHashRate; i++)
        {
            uint256 hash = pHashAlgo->Hash(vchWorkData);
            nHashComputeCount++;
            if (hash <= hashTarget)
            {
                int64 nDuration = GetTime() - nHashComputeBeginTime;
                int nCompHashRate = ((nDuration <= 0) ? 0 : (nHashComputeCount / nDuration));

                StdLog("blockmaker", "Proof-of-work: block found (%s), target height: %d, compute: (rate:%ld, count:%ld, duration:%lds, hashrate:%ld), difficulty bits: (%d)\nhash :   %s\ntarget : %s",
                       pHashAlgo->strAlgo.c_str(), nPrevBlockHeight + 1, nHashRate, nHashComputeCount, nDuration, nCompHashRate, nBits,
                       hash.GetHex().c_str(), hashTarget.GetHex().c_str());

                uint256 hashBlock;
                Errno err = pService->SubmitWork(vchWorkData, profile.templMint, profile.keyMint, hashBlock);
                if (err != OK)
                {
                    return false;
                }
                return true;
            }
            nNonce++;
        }

        int64 nNetTime = GetNetTime() + nTimeDiff;
        if (nTime + 1 < nNetTime)
        {
            nHashRate /= (nNetTime - nTime);
            nTime = nNetTime;
        }
        else if (nTime == nNetTime)
        {
            nHashRate *= 2;
        }
    }
    StdLog("blockmaker", "Proof-of-work: target height: %d, compute interrupted.", nPrevBlockHeight + 1);
    return false;
}

void CBlockMaker::BlockMakerThreadFunc()
{
    uint256 hashLastBlock;
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        hashLastBlock = lastStatus.hashBlock;
    }
    bool fCachePow = false;
    int64 nWaitTime = 1;
    while (!fExit)
    {
        // if (nWaitTime < 1)
        // {
        //     nWaitTime = 1;
        // }
        if (nWaitTime > 0 && !WaitUpdateEvent(nWaitTime))
        {
            break;
        }

        switch (MintConfig()->nPeerType)
        {
        case NODE_TYPE_COMMON:
        case NODE_TYPE_SUPER:
        {
            CAgreementBlock consParam;
            if (!pConsensus->GetNextConsensus(consParam))
            {
                StdDebug("blockmaker", "Block Maker Thread Func: Wait next time, target height: %d, wait time: %ld, last height: %d, prev block: %s",
                         consParam.nPrevHeight + 1, consParam.nWaitTime, lastStatus.nBlockHeight, consParam.hashPrev.GetHex().c_str());
                nWaitTime = consParam.nWaitTime;
                if (nWaitTime < 1)
                {
                    nWaitTime = 1;
                }
                break;
            }
            nWaitTime = consParam.nWaitTime;

            if (hashLastBlock != consParam.hashPrev || fCachePow != consParam.agreement.IsProofOfWork())
            {
                hashLastBlock = consParam.hashPrev;
                fCachePow = consParam.agreement.IsProofOfWork();
                if (consParam.agreement.IsProofOfWork())
                {
                    StdLog("blockmaker", "GetAgreement: height: %d, consensus: poa", consParam.nPrevHeight + 1);
                }
                else
                {
                    StdLog("blockmaker", "GetAgreement: height: %d, consensus: pos, ballot address: %s", consParam.nPrevHeight + 1, consParam.agreement.vBallot[0].ToString().c_str());
                }
            }

            try
            {
                if (!consParam.agreement.IsProofOfWork())
                {
                    if (!ProcessDelegatedProofOfStake(consParam))
                    {
                        nWaitTime = 1;
                    }
                }
                else
                {
                    if (nWaitTime < 1)
                    {
                        nWaitTime = 1;
                    }
                }
                pDispatcher->SetConsensus(consParam);
            }
            catch (exception& e)
            {
                StdError("blockmaker", "Block maker error: %s", e.what());
                if (nWaitTime < 1)
                {
                    nWaitTime = 1;
                }
            }
            break;
        }
        case NODE_TYPE_FORK:
        {
            uint256 hashPrevBlock;
            uint64 nLastTime;
            // wait for new main fork block
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                // no new block
                if (hashLastBlock == lastStatus.hashBlock)
                {
                    continue;
                }
                // too early block
                else if (lastStatus.nBlockTime < GetNetTime() - BLOCK_TARGET_SPACING)
                {
                    continue;
                }
                else
                {
                    hashLastBlock = lastStatus.hashBlock;
                    hashPrevBlock = lastStatus.hashPrevBlock;
                    nLastTime = lastStatus.nBlockTime;
                }
            }

            CDelegateAgreement agreement;
            if (pBlockChain->GetBlockDelegateAgreement(hashLastBlock, agreement) && !agreement.IsProofOfWork())
            {
                map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.find(agreement.vBallot[0]);
                if (it != mapDelegatedProfile.end())
                {
                    CBlockStatus status;
                    if (pBlockChain->GetBlockStatus(hashPrevBlock, status))
                    {
                        // create sub fork blocks
                        ProcessSubFork(it->second, agreement, hashLastBlock, nLastTime, status.nBlockHeight, status.nMintType);
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    StdLog("blockmaker", "Block maker exited");
}

void CBlockMaker::PowThreadFunc()
{
    if (!WaitExit(5))
    {
        StdLog("blockmaker", "Pow exited non");
        return;
    }
    int64 nPrevBlockTime = 0;
    int64 nSynBeginTime = 0;
    while (WaitExit(1))
    {
        bool fWork = false;
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            if (lastStatus.nBlockHeight == 0)
            {
                fWork = true;
            }
            else
            {
                int64 nCurTime = GetNetTime();
                if (nSynBeginTime == 0 || nPrevBlockTime != lastStatus.nBlockTime)
                {
                    nPrevBlockTime = lastStatus.nBlockTime;
                    nSynBeginTime = nCurTime;
                }
                int64 nStakeTime = 0;
                if (lastStatus.nMintType == CTransaction::TX_STAKE)
                {
                    nStakeTime = BLOCK_TARGET_SPACING;
                }
                if (nCurTime >= lastStatus.nBlockTime + BLOCK_TARGET_SPACING + nStakeTime && nCurTime - nSynBeginTime >= BLOCK_TARGET_SPACING / 2)
                {
                    fWork = true;
                }
            }
        }
        if (fWork)
        {
            CreateProofOfWork();
        }
    }
    StdLog("blockmaker", "Pow exited");
}

} // namespace metabasenet

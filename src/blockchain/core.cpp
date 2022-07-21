// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include "bloomfilter/bloomfilter.h"
#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/mint.h"
#include "template/proof.h"
#include "template/vote.h"
#include "wallet.h"

using namespace std;
using namespace hcode;

#define DEBUG(err, ...) Debug((err), __FUNCTION__, __VA_ARGS__)

namespace metabasenet
{
///////////////////////////////
// CCoreProtocol

CCoreProtocol::CCoreProtocol()
{
    pBlockChain = nullptr;
    pForkManager = nullptr;
}

CCoreProtocol::~CCoreProtocol()
{
}

bool CCoreProtocol::HandleInitialize()
{
    InitializeGenesisBlock();
    if (!GetObject("blockchain", pBlockChain))
    {
        return false;
    }
    if (!GetObject("forkmanager", pForkManager))
    {
        return false;
    }
    return true;
}

Errno CCoreProtocol::Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...)
{
    string strFormat(pszFunc);
    strFormat += string(", ") + string(ErrorString(err)) + string(" : ") + string(pszFormat);
    va_list ap;
    va_start(ap, pszFormat);
    VDebug(strFormat.c_str(), ap);
    va_end(ap);
    return err;
}

void CCoreProtocol::InitializeGenesisBlock()
{
    CBlock block;
    GetGenesisBlock(block);
    hashGenesisBlock = block.GetHash();
}

const uint256& CCoreProtocol::GetGenesisBlockHash()
{
    return hashGenesisBlock;
}

/*
Address: 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws
PubKey : ac9a2f4b438a270fcdfe33305db1da885dc53de8e4299bbba765c4207338c310
Secret : 9ae89671cc1a74e9e404a16982ae48d21c56d4ad8278bc9755235a68fc841271
*/
void CCoreProtocol::GetGenesisBlock(CBlock& block)
{
    const CDestination destOwner = CDestination(metabasenet::crypto::CPubKey(uint256("ac9a2f4b438a270fcdfe33305db1da885dc53de8e4299bbba765c4207338c310")));

    block.SetNull();

    block.nVersion = 1;
    block.nType = CBlock::BLOCK_GENESIS;
    block.nTimeStamp = 1658224800;
    block.hashPrev = 0;

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.to = destOwner;
    tx.nAmount = BBCP_TOKEN_INIT; // initial number of token

    {
        hcode::CBufStream ss;
        ss << tx.nAmount;
        bytes btTempData;
        ss.GetData(btTempData);
        tx.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
    }

    CProfile profile;
    profile.strName = "MNT Core";
    profile.strSymbol = "MNT";
    profile.destOwner = destOwner;
    profile.nAmount = tx.nAmount;
    profile.nMintReward = BBCP_REWARD_INIT;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = 0;
    profile.SetFlag(true, false, false);

    profile.Save(block.vchProof);

    std::map<CDestination, CDestState> mapBlockState;
    mapBlockState[destOwner] = CDestState(tx.nAmount);
    if (!storage::CStateDB::CreateStaticStateRoot(mapBlockState, block.hashStateRoot))
    {
        block.SetNull();
        return;
    }

    CBloomFilter<2048> bf;
    if (!block.txMint.to.IsNull())
    {
        bf.Add(block.txMint.to.data.begin(), block.txMint.to.data.size());
    }
    bytes btBloom;
    bf.GetData(btBloom);
    block.nBloom |= uint2048(btBloom);
}

Errno CCoreProtocol::ValidateTransaction(const CTransaction& tx, int nHeight)
{
    if (tx.nType == CTransaction::TX_WORK)
    {
        if (tx.GetTxDataCount() > 0)
        {
            CTransaction txTemp;
            metabasenet::crypto::CPubKey pubkey(~uint256());
            const CDestination dest = CDestination(metabasenet::crypto::CPubKey(~uint256()));
            CTemplatePtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(pubkey, dest));
            txTemp.AddTxData(CTransaction::DF_TEMPLATEDATA, ptr->GetTemplateData());

            {
                hcode::CBufStream ss;
                ss << ~uint256();
                bytes btTempData;
                ss.GetData(btTempData);
                txTemp.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
            }

            if (tx.GetTxDataCount() > 2 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
            }
        }
    }
    else if (tx.nType == CTransaction::TX_STAKE)
    {
        if (tx.GetTxDataCount() > 0)
        {
            CTransaction txTemp;

            {
                hcode::CBufStream ss;
                ss << ~uint256();
                bytes btTempData;
                ss.GetData(btTempData);
                txTemp.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
            }

            {
                hcode::CBufStream ss;
                ss << ~uint256();
                bytes btTempData;
                ss.GetData(btTempData);
                txTemp.AddTxData(CTransaction::DF_MINTREWARD, btTempData);
            }

            metabasenet::crypto::CPubKey pubkey(~uint256());
            const CDestination dest = CDestination(metabasenet::crypto::CPubKey(~uint256()));
            CTemplatePtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateDelegate(pubkey, dest, 500));
            txTemp.AddTxData(CTransaction::DF_TEMPLATEDATA, ptr->GetTemplateData());

            if (tx.GetTxDataCount() > 3 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
            }
        }
    }
    else if (tx.nType == CTransaction::TX_DEFI_REWARD)
    {
        CTransaction txTemp;

        hcode::CBufStream ss;
        ss << ~uint256();
        bytes btTempData;
        ss.GetData(btTempData);
        txTemp.AddTxData(CTransaction::DF_VOTEREWARD, btTempData);

        const CDestination destDelegate = CDestination(CTemplateId(TEMPLATE_DELEGATE, ~uint256()));
        const CDestination destOwner = CDestination(metabasenet::crypto::CPubKey(~uint256()));
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(new CTemplateVote(destDelegate, destOwner, 0));
        txTemp.AddTxData(CTransaction::DF_TEMPLATEDATA, ptr->GetTemplateData());

        if (tx.GetTxDataCount() > 2 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
        }
    }
    else if (tx.nType == CTransaction::TX_DEFI_RELATION)
    {
        CTransaction txTemp;
        bytes btTempData;
        btTempData.resize(160);
        txTemp.AddTxData(CTransaction::DF_COMMON, btTempData);

        if (tx.GetTxDataCount() > 1 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
        }
    }
    if (!tx.vchSig.empty() && tx.IsRewardTx())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid signature");
    }
    if (tx.to.IsNull())
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "send to null address");
    }
    //if (!MoneyRange(tx.nAmount))
    //{
    //    return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "amount overflow, amount: 0x%s", tx.nAmount.GetHex().c_str());
    //}
    //if (!MoneyRange(tx.GetTxFee()))
    //{
    //    return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee overflow, fee: 0x%s", tx.GetTxFee().GetHex().c_str());
    //}
    if (CTransaction::IsUserTx(tx.nType))
    {
        if (tx.nGasPrice < MIN_GAS_PRICE)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "gas price invalid, tx gas price: 0x%s, min gas price: 0x%s",
                         tx.nGasPrice.GetHex().c_str(), MIN_GAS_PRICE.GetHex().c_str());
        }
        uint256 nNeedGas = TX_BASE_GAS + tx.GetTxDataGas();
        if (tx.nGasLimit < nNeedGas)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "gas invalid, tx gas: 0x%s, need gas: 0x%s",
                         tx.nGasLimit.GetHex().c_str(), nNeedGas.GetHex().c_str());
        }
    }
    else
    {
        if (tx.GetTxFee() != 0)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "non token tx, txfee nonzero, fee: 0x%s", tx.GetTxFee().GetHex().c_str());
        }
    }

    if (GetSerializeSize(tx) > MAX_TX_SIZE)
    {
        return DEBUG(ERR_TRANSACTION_OVERSIZE, "tx size: %u", GetSerializeSize(tx));
    }

    return OK;
}

Errno CCoreProtocol::ValidateBlock(const CBlock& block)
{
    // These are checks that are independent of context
    // Only allow CBlock::BLOCK_PRIMARY type in v1.0.0
    /*if (block.nType != CBlock::BLOCK_PRIMARY)
    {
        return DEBUG(ERR_BLOCK_TYPE_INVALID, "Block type error");
    }*/
    // Check timestamp
    if (block.GetBlockTime() > GetNetTime() + MAX_CLOCK_DRIFT)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "%ld", block.GetBlockTime());
    }

    // validate vacant block
    if (block.nType == CBlock::BLOCK_VACANT)
    {
        if (block.txMint.nAmount != 0 || block.txMint.GetTxFee() != 0 || block.txMint.nType != CTransaction::TX_STAKE
            || block.txMint.nTimeStamp == 0 || block.txMint.to.IsNull())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, nAmount: %lu, tx fee: %lu, nType: %d, nTimeStamp: %d, to: %s",
                         block.txMint.nAmount, block.txMint.GetTxFee(), block.txMint.nType, block.txMint.nTimeStamp,
                         (block.txMint.to.IsNull() ? "null" : block.txMint.to.ToString().c_str()));
        }
        if (block.hashMerkleRoot != 0 || !block.vtx.empty())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block vtx is not empty");
        }
    }

    // Validate mint tx
    if (!block.txMint.IsMintTx() || ValidateTransaction(block.txMint, block.GetBlockHeight()) != OK)
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, tx type: %d", block.txMint.nType);
    }

    size_t nBlockSize = GetSerializeSize(block);
    if (nBlockSize > MAX_BLOCK_SIZE)
    {
        return DEBUG(ERR_BLOCK_OVERSIZE, "size overflow size=%u vtx=%u", nBlockSize, block.vtx.size());
    }

    if (block.nType == CBlock::BLOCK_ORIGIN && !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "origin block vtx is not empty");
    }

    vector<uint256> vMerkleTree;
    if (block.hashMerkleRoot != block.BuildMerkleTree(vMerkleTree))
    {
        return DEBUG(ERR_BLOCK_TXHASH_MISMATCH, "tx merkeroot mismatched");
    }

    set<uint256> setTx;
    setTx.insert(vMerkleTree.begin(), vMerkleTree.begin() + block.vtx.size());
    if (setTx.size() != block.vtx.size())
    {
        return DEBUG(ERR_BLOCK_DUPLICATED_TRANSACTION, "duplicate tx");
    }

    for (const CTransaction& tx : block.vtx)
    {
        if (tx.IsMintTx() || ValidateTransaction(tx, block.GetBlockHeight()) != OK)
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid tx %s", tx.GetHash().GetHex().c_str());
        }
    }

    if (!CheckBlockSignature(block))
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "Check block signature fail");
    }
    return OK;
}

Errno CCoreProtocol::VerifyForkCreateTx(const CTransaction& tx, const int nHeight, const bytes& btToAddressData)
{
    if (tx.hashFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork");
    }
    if (tx.to == tx.from)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }
    if (tx.nAmount < MORTGAGE_BASE)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid nAmount");
    }

    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid tx data");
    }
    CBlock block;
    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork data");
    }
    if (!block.IsOrigin() || block.IsPrimary())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
    }
    CProfile profile;
    if (!profile.Load(block.vchProof))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }

    if (profile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    //if (!MoneyRange(profile.nAmount))
    //{
    //    return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    //}
    //if (!RewardRange(profile.nMintReward))
    //{
    //    return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    //}
    if (block.txMint.to != profile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork to");
    }

    if (ValidateBlock(block) != OK)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
    }

    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_FORK, btToAddressData);
    if (!ptr)
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid vchSig");
    }
    if (ptr->GetTemplateId() != tx.to.GetTemplateId())
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid template id");
    }
    if (boost::dynamic_pointer_cast<CTemplateFork>(ptr)->hashFork != block.GetHash())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid forkid");
    }
    return OK;
}

Errno CCoreProtocol::VerifyForkRedeemTx(const CTransaction& tx, const uint256& hashPrevBlock, const CDestState& state, const bytes& btFromAddressData)
{
    if (tx.hashFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid fork");
    }
    if (tx.to == tx.from)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }

    uint256 nLockedAmount;
    if (!pBlockChain->GetCreateForkLockedAmount(tx.from, hashPrevBlock, btFromAddressData, nLockedAmount))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid locked coin template destination");
    }

    // locked coin template: nValueIn >= tx.nAmount + tx.GetTxFee() + nLockedAmount
    if (state.nBalance < tx.nAmount + tx.GetTxFee() + nLockedAmount)
    {
        return DEBUG(ERR_TRANSACTION_IS_LOCKED, "balance is not enough to locked coin, balance: %s, use: %s",
                     CoinToTokenBigFloat(state.nBalance).c_str(), CoinToTokenBigFloat(tx.nAmount + tx.GetTxFee() + nLockedAmount).c_str());
    }
    return OK;
}

Errno CCoreProtocol::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile)
{
    if (!forkProfile.Load(block.vchProof))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "load profile error");
    }
    if (forkProfile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    //if (!MoneyRange(forkProfile.nAmount))
    //{
    //    return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    //}
    //if (!RewardRange(forkProfile.nMintReward))
    //{
    //    return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    //}
    if (block.txMint.to != forkProfile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork to");
    }
    if (parentProfile.IsPrivate())
    {
        if (!forkProfile.IsPrivate() || parentProfile.destOwner != forkProfile.destOwner)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "permission denied");
        }
    }
    return OK;
}

Errno CCoreProtocol::VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev)
{
    uint32 nNextTimestamp = GetNextBlockTimeStamp(pIndexPrev->nMintType, pIndexPrev->GetBlockTime(), block.txMint.nType);
    if (block.GetBlockTime() < nNextTimestamp)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Verify proof work: Timestamp out of range 2, height: %d, block time: %d, next time: %d, prev minttype: 0x%x, prev time: %d, block: %s.",
                     block.GetBlockHeight(), block.GetBlockTime(), nNextTimestamp,
                     pIndexPrev->nMintType, pIndexPrev->GetBlockTime(), block.GetHash().GetHex().c_str());
    }

    CProofOfHashWork proof;
    if (!proof.Load(block.vchProof))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "load proof fail.");
    }

    int nBits = 0;
    if (!GetProofOfWorkTarget(pIndexPrev, proof.nAlgo, nBits))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "get target fail.");
    }

    if (nBits != proof.nBits || proof.nAlgo != CM_CRYPTONIGHT)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "algo or bits error, nAlgo: %d, nBits: %d, vchProof size: %ld.", proof.nAlgo, proof.nBits, block.vchProof.size());
    }
    if (proof.destMint != block.txMint.to)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "destMint error, destMint: %s.", proof.destMint.ToString().c_str());
    }

    uint256 hashTarget = (~uint256(uint64(0)) >> nBits);

    vector<unsigned char> vchProofOfWork;
    block.GetSerializedProofOfWorkData(vchProofOfWork);
    uint256 hash = crypto::CryptoPowHash(&vchProofOfWork[0], vchProofOfWork.size());

    if (hash > hashTarget)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "hash error: proof[%s] vs. target[%s] with bits[%d]",
                     hash.ToString().c_str(), hashTarget.ToString().c_str(), nBits);
    }

    return OK;
}

Errno CCoreProtocol::VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                                 const CDelegateAgreement& agreement)
{
    uint32 nTime = DPoSTimestamp(pIndexPrev);
    if (block.GetBlockTime() != nTime)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range. block time %d is not equal %u", block.GetBlockTime(), nTime);
    }
    if (block.txMint.to != agreement.vBallot[0])
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint to error.");
    }
    if (block.txMint.nTimeStamp != block.GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    }
    return OK;
}

Errno CCoreProtocol::VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                      const CDelegateAgreement& agreement)
{
    if (block.GetBlockTime() <= pIndexPrev->GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range.");
    }

    if (block.IsSubsidiary())
    {
        if (block.GetBlockTime() != pIndexRef->GetBlockTime())
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Subsidiary timestamp out of range.");
        }
    }
    else
    {
        if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
            || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING
            /*|| block.GetBlockTime() != pIndexPrev->GetBlockTime() + EXTENDED_BLOCK_SPACING*/)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp out of range.");
        }
        if (((block.GetBlockTime() - pIndexPrev->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp error.");
        }
    }

    if (block.txMint.to != agreement.GetBallot(0))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint to error.");
    }

    if (block.txMint.nTimeStamp != block.GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    }
    return OK;
}

Errno CCoreProtocol::VerifyTransaction(const CTransaction& tx, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<uint256, CAddressContext>& mapBlockAddress)
{
    Errno err = OK;
    if (stateFrom.nBalance < tx.nAmount + tx.GetTxFee())
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "balance is not enough, balance: %s, tx amount: %s",
                     stateFrom.nBalance.GetHex().c_str(), (tx.nAmount + tx.GetTxFee()).GetHex().c_str());
    }
    if (!tx.from.IsNull() && !stateFrom.VerifyTxNonce(tx.nTxNonce))
    {
        return DEBUG(ERR_MISSING_PREV, "invalid nonce, tx nonce: %ld, state nonce: %ld, from: %s",
                     tx.nTxNonce, stateFrom.nTxNonce, tx.from.ToString().c_str());
    }

    if (tx.nType == CTransaction::TX_CERT)
    {
        if (VerifyCertTx(tx) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid cert tx");
        }
    }
    else if (tx.nType == CTransaction::TX_DEFI_RELATION)
    {
        err = VerifyDefiRelationTx(hashPrevBlock, tx);
        if (err != OK)
        {
            return err;
        }
    }

    uint16 nFromTemplateType = 0;
    uint16 nToTemplateType = 0;
    CTemplateId tid;
    if (tx.from.GetTemplateId(tid))
    {
        nFromTemplateType = tid.GetType();
    }
    if (tx.to.GetTemplateId(tid))
    {
        nToTemplateType = tid.GetType();
    }

    if (nToTemplateType == TEMPLATE_FORK
        || nToTemplateType == TEMPLATE_VOTE
        || nToTemplateType == TEMPLATE_REDEEM)
    {
        if (tx.hashFork != GetGenesisBlockHash())
        {
            StdLog("CoreProtocol", "Template address tx is not on the main chain, to: %s, txid: %s, fork: %s",
                   tx.to.ToString().c_str(), tx.GetHash().GetHex().c_str(), tx.hashFork.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
    }
    if (nFromTemplateType == TEMPLATE_VOTE && nToTemplateType != TEMPLATE_REDEEM)
    {
        StdLog("CoreProtocol", "Template address tx is not on the main chain, to: %s, txid: %s, fork: %s",
               tx.to.ToString().c_str(), tx.GetHash().GetHex().c_str(), tx.hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (nToTemplateType == TEMPLATE_VOTE)
    {
        err = VerifyVoteTx(tx, hashPrevBlock);
        if (err != OK)
        {
            return DEBUG(err, "invalid vote tx");
        }
    }

    if (nFromTemplateType == TEMPLATE_FORK)
    {
        auto it = mapBlockAddress.find(tx.from.data);
        if (it == mapBlockAddress.end())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid from address, no address data");
        }
        CTemplateAddressContext ctxtTemplate;
        if (!it->second.GetTemplateAddressContext(ctxtTemplate))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid from address, no address data");
        }
        err = VerifyForkRedeemTx(tx, tx.hashFork, stateFrom, ctxtTemplate.btData);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork redeem fail");
        }
    }

    if (nToTemplateType == TEMPLATE_FORK)
    {
        auto it = mapBlockAddress.find(tx.to.data);
        if (it == mapBlockAddress.end())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, no address data");
        }
        CTemplateAddressContext ctxtTemplate;
        if (!it->second.GetTemplateAddressContext(ctxtTemplate))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, no address data");
        }
        err = VerifyForkCreateTx(tx, nAtHeight, ctxtTemplate.btData);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork tx fail");
        }
    }

    if (tx.to.IsContract())
    {
        if (tx.to.IsNullContract())
        {
            if (!pBlockChain->VerifyCreateContractTx(tx.hashFork, hashPrevBlock, tx))
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "Verify create contract tx fail");
            }
        }
        else
        {
            if (!pBlockChain->VerifyContractAddress(tx.hashFork, hashPrevBlock, tx.to))
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "Verify contract address fail");
            }
        }
    }

    if (tx.to.IsTemplate())
    {
        auto it = mapBlockAddress.find(tx.to.data);
        if (it == mapBlockAddress.end())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, no address data");
        }
        CTemplateAddressContext ctxtTemplate;
        if (!it->second.GetTemplateAddressContext(ctxtTemplate))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, get address data fail");
        }
        const bytes& btToTemplateData = ctxtTemplate.btData;
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(tx.to.GetTemplateId().GetType(), btToTemplateData);
        if (!ptr)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, address data error");
        }
        if (ptr->GetTemplateId() != tx.to.GetTemplateId())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid to address, template id error");
        }
    }

    bytes btFromTemplateData;
    if (tx.from.IsTemplate())
    {
        auto fit = mapBlockAddress.find(tx.from.data);
        if (fit == mapBlockAddress.end())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid from address, no address data");
        }
        CTemplateAddressContext ctxtTemplate;
        if (!fit->second.GetTemplateAddressContext(ctxtTemplate))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid from address, no address data");
        }
        btFromTemplateData = ctxtTemplate.btData;
    }
    if (!tx.from.VerifyTxSignature(tx.GetSignatureHash(), tx.nType, tx.to, btFromTemplateData, tx.vchSig, nAtHeight))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid signature");
    }
    return OK;
}

bool CCoreProtocol::GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev, const CDelegateAgreement& agreement, const CBlockIndex* pIndexRef, size_t nEnrollTrust)
{
    //int32 nHeight = block.GetBlockHeight();
    if (block.IsGenesis())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsVacant())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsPrimary())
    {
        if (block.IsProofOfWork())
        {
            // PoW difficulty = 2 ^ nBits
            CProofOfHashWork proof;
            if (!proof.Load(block.vchProof))
            {
                StdError("CoreProtocol", "Proof load fail, vchProof size: %lu", block.vchProof.size());
                return false;
            }
            uint256 v(1);
            nChainTrust = v << proof.nBits;
        }
        else if (pIndexPrev != nullptr)
        {
            // Get the last PoW block nAlgo
            int nAlgo;
            const CBlockIndex* pIndex = pIndexPrev;
            while (!pIndex->IsProofOfWork() && (pIndex->pPrev != nullptr))
            {
                pIndex = pIndex->pPrev;
            }
            if (!pIndex->IsProofOfWork())
            {
                nAlgo = CM_CRYPTONIGHT;
            }
            else
            {
                nAlgo = pIndex->nProofAlgo;
            }

            int nBits;
            if (GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits))
            {
                if (agreement.nWeight == 0 || nBits <= 0)
                {
                    StdError("CoreProtocol", "GetBlockTrust: nWeight or nBits error, nWeight: %lu, nBits: %d", agreement.nWeight, nBits);
                    return false;
                }
                if (nEnrollTrust <= 0)
                {
                    StdError("CoreProtocol", "GetBlockTrust: nEnrollTrust error, nEnrollTrust: %lu", nEnrollTrust);
                    return false;
                }

                // DPoS difficulty = 2 ^ (nBits + weight)
                nChainTrust = uint256(1) << (int(nEnrollTrust) + nBits + 20);
            }
            else
            {
                StdError("CoreProtocol", "GetBlockTrust: Get ProofOfWork Target fail");
                return false;
            }
        }
        else
        {
            StdError("CoreProtocol", "GetBlockTrust: Primary pIndexPrev is null");
            return false;
        }
    }
    else if (block.IsOrigin())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsSubsidiary() || block.IsExtended())
    {
        if (pIndexRef == nullptr)
        {
            StdError("CoreProtocol", "GetBlockTrust: pIndexRef is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        if (pIndexRef->pPrev == nullptr)
        {
            StdError("CoreProtocol", "GetBlockTrust: Subsidiary or Extended block pPrev is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        nChainTrust = pIndexRef->nChainTrust - pIndexRef->pPrev->nChainTrust;
    }
    else
    {
        StdError("CoreProtocol", "GetBlockTrust: block type error");
        return false;
    }
    return true;
}

bool CCoreProtocol::GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits)
{
    nBits = 8;
    return true;
}

void CCoreProtocol::GetDelegatedBallot(const uint256& nAgreement, const size_t& nWeight, const map<CDestination, size_t>& mapBallot,
                                       const vector<pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, vector<CDestination>& vBallot, size_t& nEnrollTrust, int nBlockHeight)
{
    vBallot.clear();
    if (nAgreement == 0 || mapBallot.size() == 0)
    {
        StdTrace("CoreProtocol", "Get delegated ballot: height: %d, nAgreement: %s, mapBallot.size: %ld", nBlockHeight, nAgreement.GetHex().c_str(), mapBallot.size());
        return;
    }
    if (nMoneySupply == 0)
    {
        StdTrace("CoreProtocol", "Get delegated ballot: nMoneySupply == 0");
        return;
    }
    if (vecAmount.size() != mapBallot.size())
    {
        StdError("CoreProtocol", "Get delegated ballot: dest ballot size %llu is not equal amount size %llu", mapBallot.size(), vecAmount.size());
    }

    uint64 nSelected = 0;
    for (int i = 0; i < nAgreement.size() / sizeof(int); i++)
    {
        nSelected += nAgreement[i];
    }

    map<CDestination, uint64> mapSelectBallot;
    uint64 nEnrollWeight = 0;
    uint256 nTotalEnrollTrust;
    for (auto& amount : vecAmount)
    {
        StdTrace("CoreProtocol", "Get delegated ballot: height: %d, vote dest: %s, amount: %s",
                 nBlockHeight, amount.first.ToString().c_str(), CoinToTokenBigFloat(amount.second).c_str());
        if (mapBallot.find(amount.first) != mapBallot.end())
        {
            uint256 nMinAmount;
            if (amount.second < DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT)
            {
                nMinAmount = amount.second;
            }
            else
            {
                nMinAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT;
            }
            size_t nDestWeight = (nMinAmount / DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT).Get64();
            mapSelectBallot[amount.first] = nDestWeight;
            nEnrollWeight += nDestWeight;
            nTotalEnrollTrust += nMinAmount;
            StdTrace("CoreProtocol", "Get delegated ballot: height: %d, ballot dest: %s, weight: %lld",
                     nBlockHeight, amount.first.ToString().c_str(), nDestWeight);
        }
    }
    if (nEnrollWeight == 0)
    {
        StdError("CoreProtocol", "Get delegated ballot: nEnrollWeight error, dest ballot size %llu is not equal amount size %llu", mapBallot.size(), vecAmount.size());
        return;
    }
    nEnrollTrust = (nTotalEnrollTrust / DELEGATE_PROOF_OF_STAKE_ENROLL_TRUST).Get64() + 1;
    if (nEnrollTrust > 100)
    {
        nEnrollTrust = 100;
    }
    StdTrace("CoreProtocol", "Get delegated ballot: trust height: %d, ballot dest count is %llu, enroll trust: %llu",
             nBlockHeight, mapSelectBallot.size(), nEnrollTrust);

    uint64 n = (nSelected * DELEGATE_PROOF_OF_STAKE_MAXIMUM_TIMES) % nEnrollWeight;
    for (auto it = mapSelectBallot.begin(); it != mapSelectBallot.end(); ++it)
    {
        if (n < it->second)
        {
            vBallot.push_back(it->first);
            break;
        }
        n -= it->second;
    }

    StdTrace("CoreProtocol", "Get delegated ballot: consensus: %s, height: %d, ballot dest: %s",
             (vBallot.size() > 0 ? "dpos" : "pow"), nBlockHeight, (vBallot.size() > 0 ? vBallot[0].ToString().c_str() : ""));
}

uint256 CCoreProtocol::MinEnrollAmount()
{
    return DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
}

uint32 CCoreProtocol::DPoSTimestamp(const CBlockIndex* pIndexPrev)
{
    if (pIndexPrev == nullptr || !pIndexPrev->IsPrimary())
    {
        return 0;
    }
    return pIndexPrev->GetBlockTime() + BLOCK_TARGET_SPACING;
}

uint32 CCoreProtocol::GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType)
{
    if (nPrevMintType == CTransaction::TX_WORK || nPrevMintType == CTransaction::TX_GENESIS)
    {
        if (nTargetMintType == CTransaction::TX_STAKE)
        {
            return nPrevTimeStamp + BLOCK_TARGET_SPACING;
        }
        return nPrevTimeStamp + PROOF_OF_WORK_BLOCK_SPACING;
    }
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

bool CCoreProtocol::CheckBlockSignature(const CBlock& block)
{
    if (block.GetHash() != GetGenesisBlockHash())
    {
        bytes btAddressData;
        if (block.txMint.to.IsTemplate())
        {
            CAddressContext ctxtAddress;
            if (!pBlockChain->RetrieveAddressContext(block.txMint.hashFork, block.hashPrev, block.txMint.to.data, ctxtAddress))
            {
                if (!block.txMint.GetTxData(CTransaction::DF_TEMPLATEDATA, btAddressData))
                {
                    StdLog("CoreProtocol", "Check Block Signature: Get tx data fail, block: %s", block.GetHash().GetHex().c_str());
                    return false;
                }
            }
            else
            {
                CTemplateAddressContext ctxtTemplate;
                if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
                {
                    StdLog("CoreProtocol", "Check Block Signature: Get template address context fail, block: %s", block.GetHash().GetHex().c_str());
                    return false;
                }
                btAddressData = ctxtTemplate.btData;
            }
        }
        return block.txMint.to.VerifyBlockSignature(block.GetHash(), btAddressData, block.vchSig);
    }
    return true;
}

Errno CCoreProtocol::ValidateVacantBlock(const CBlock& block)
{
    if (block.hashMerkleRoot != 0 || block.txMint != CTransaction() || !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block tx is not empty.");
    }

    if (!block.vchProof.empty() || !block.vchSig.empty())
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "vacant block proof or signature is not empty.");
    }

    return OK;
}

Errno CCoreProtocol::VerifyCertTx(const CTransaction& tx)
{
    // CERT transaction must be on the main chain
    if (tx.hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify Cert Tx CERT tx is not on the main chain, fork: %s", tx.hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // the from address must be equal to the to address of cert tx
    if (tx.from != tx.to)
    {
        StdLog("CoreProtocol", "Verify Cert Tx the from address is not equal the to address of CERT tx, from: %s, to: %s",
               tx.from.ToString().c_str(), tx.to.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // the to address must be delegate template address
    if (!tx.to.IsTemplate() || tx.to.GetTemplateId().GetType() != TEMPLATE_DELEGATE)
    {
        StdLog("CoreProtocol", "Verify Cert Tx the to address of CERT tx is not a delegate template address, to: %s", tx.to.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    return OK;
}

Errno CCoreProtocol::VerifyVoteTx(const CTransaction& tx, const uint256& hashPrev)
{
    if (tx.to == tx.from)
    {
        StdLog("CoreProtocol", "Verify Vote Tx: From and to addresses cannot be the same, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    if (tx.nAmount == 0)
    {
        StdLog("CoreProtocol", "Verify Vote Tx: Transfer quantity is 0, nAmount: 0, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    return OK;
}

Errno CCoreProtocol::VerifyDefiRelationTx(const uint256& hashPrev, const CTransaction& tx)
{
    if (tx.hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify defi relation tx: Tx is not on the main chain, fork: %s", tx.hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.from == tx.to)
    {
        StdLog("CoreProtocol", "Verify defi relation tx: DeFi relation tx from address must be not equal to to address, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (!tx.from.IsPubKey() || !tx.to.IsPubKey())
    {
        StdLog("CoreProtocol", "Verify defi relation tx: DeFi tx to Address and from must be public key address, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    bytes btDefiData;
    if (!tx.GetTxData(CTransaction::DF_COMMON, btDefiData))
    {
        StdLog("CoreProtocol", "Verify defi relation tx: Get data fail, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // vchData: shared_pubkey + sub_sig + parent_sig
    if (btDefiData.size() != 160)
    {
        StdLog("CoreProtocol", "Verify defi relation tx: DeFi tx length of vchData is not 160, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    uint256 sharedPubKey(vector<uint8>(btDefiData.begin(), btDefiData.begin() + 32));
    vector<uint8> subSign(btDefiData.begin() + 32, btDefiData.begin() + 96);
    vector<uint8> parentSign(btDefiData.begin() + 96, btDefiData.end());
    StdTrace("CoreProtocol", "Verify defi relation tx: sharedPubKey: %s, subSign: %s, parentSign: %s",
             sharedPubKey.ToString().c_str(), ToHexString(subSign).c_str(), ToHexString(parentSign).c_str());

    // sub_sign: sign blake2b(DeFiRelation + forkid + shared_pubkey) with sendto
    crypto::CPubKey subKey = tx.to.GetPubKey();
    string subSignStr = string("DeFiRelation") + tx.hashFork.ToString() + sharedPubKey.ToString();
    uint256 subSignHashStr = crypto::CryptoHash(subSignStr.data(), subSignStr.size());
    StdTrace("CoreProtocol", "Verify defi relation tx: subSignStr: %s, subSignHashStr: %s", subSignStr.c_str(), ToHexString(subSignHashStr.begin(), subSignHashStr.size()).c_str());
    if (!crypto::CryptoVerify(subKey, subSignHashStr.begin(), subSignHashStr.size(), subSign))
    {
        StdLog("CoreProtocol", "Verify defi relation tx: DeFi tx sub signature in vchData is not currect, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // parent_sign: sign blake2b(DeFiRelation + parent_pubkey) with sharedPubKey
    crypto::CPubKey parentKey = tx.from.GetPubKey();
    string parentSignStr = string("DeFiRelation") + parentKey.ToString();
    uint256 parentSignHashStr = crypto::CryptoHash(parentSignStr.data(), parentSignStr.size());
    StdTrace("CoreProtocol", "Verify defi relation tx: parentSignStr: %s, parentSignHashStr: %s", parentSignStr.c_str(), ToHexString(parentSignHashStr.begin(), parentSignHashStr.size()).c_str());
    if (!crypto::CryptoVerify(sharedPubKey, parentSignHashStr.begin(), parentSignHashStr.size(), parentSign))
    {
        StdLog("CoreProtocol", "Verify defi relation tx: DeFi tx parent signature in vchData is not currect, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CInviteContext ctxInvite;
    if (pBlockChain->RetrieveInviteParent(tx.hashFork, hashPrev, tx.to, ctxInvite) && !ctxInvite.destParent.IsNull())
    {
        StdLog("CoreProtocol", "Verify defi relation tx: To address has been invited, to: %s, txid: %s",
               tx.to.ToString().c_str(), tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    return OK;
}

///////////////////////////////
// CTestNetCoreProtocol

CTestNetCoreProtocol::CTestNetCoreProtocol()
{
}

/*
Address: 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb
PubKey : f3afc3a42a31836c9111acc4f65d3bf512e10124cb04a4137c7a6ce87d6f1329
Secret : 141a6728ded4f83f767ea770e3582be497c5088fcc3b9ca248751887534f5197
*/
void CTestNetCoreProtocol::GetGenesisBlock(CBlock& block)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    const CDestination destOwner = CDestination(metabasenet::crypto::CPubKey(uint256("f3afc3a42a31836c9111acc4f65d3bf512e10124cb04a4137c7a6ce87d6f1329")));

    block.SetNull();

    block.nVersion = 1;
    block.nType = CBlock::BLOCK_GENESIS;
    block.nTimeStamp = 1658224800;
    block.hashPrev = 0;

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.to = destOwner;
    tx.nAmount = BBCP_TOKEN_INIT; // initial number of token

    {
        hcode::CBufStream ss;
        ss << tx.nAmount;
        bytes btTempData;
        ss.GetData(btTempData);
        tx.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
    }

    CProfile profile;
    profile.strName = "MNT Core Test";
    profile.strSymbol = "MNTT";
    profile.destOwner = destOwner;
    profile.nAmount = tx.nAmount;
    profile.nMintReward = BBCP_REWARD_INIT;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = 0;
    profile.SetFlag(true, false, false);

    profile.Save(block.vchProof);

    std::map<CDestination, CDestState> mapBlockState;
    mapBlockState[destOwner] = CDestState(tx.nAmount);
    if (!storage::CStateDB::CreateStaticStateRoot(mapBlockState, block.hashStateRoot))
    {
        block.SetNull();
        return;
    }

    CBloomFilter<2048> bf;
    if (!block.txMint.to.IsNull())
    {
        bf.Add(block.txMint.to.data.begin(), block.txMint.to.data.size());
    }
    bytes btBloom;
    bf.GetData(btBloom);
    block.nBloom |= uint2048(btBloom);
}

///////////////////////////////
// CProofOfWorkParam

CProofOfWorkParam::CProofOfWorkParam(const bool fTestnetIn)
{
    fTestnet = fTestnetIn;
    nDelegateProofOfStakeEnrollMinimumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
    nDelegateProofOfStakeEnrollMaximumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT;

    if (fTestnet)
    {
        pCoreProtocol = (ICoreProtocol*)new CTestNetCoreProtocol();
    }
    else
    {
        pCoreProtocol = (ICoreProtocol*)new CCoreProtocol();
    }
    pCoreProtocol->InitializeGenesisBlock();

    hashGenesisBlock = pCoreProtocol->GetGenesisBlockHash();
}

CProofOfWorkParam::~CProofOfWorkParam()
{
    if (pCoreProtocol)
    {
        if (fTestnet)
        {
            delete (CTestNetCoreProtocol*)pCoreProtocol;
        }
        else
        {
            delete (CCoreProtocol*)pCoreProtocol;
        }
        pCoreProtocol = nullptr;
    }
}

uint32 CProofOfWorkParam::GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType)
{
    if (nPrevMintType == CTransaction::TX_WORK || nPrevMintType == CTransaction::TX_GENESIS)
    {
        if (nTargetMintType == CTransaction::TX_STAKE)
        {
            return nPrevTimeStamp + BLOCK_TARGET_SPACING;
        }
        return nPrevTimeStamp + PROOF_OF_WORK_BLOCK_SPACING;
    }
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

Errno CProofOfWorkParam::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const
{
    return pCoreProtocol->ValidateOrigin(block, parentProfile, forkProfile);
}

} // namespace metabasenet

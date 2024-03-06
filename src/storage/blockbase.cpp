// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockbase.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "bloomfilter/bloomfilter.h"
#include "delegatecomm.h"
#include "mevm/evmexec.h"
#include "mwvm/wasmrun.h"
#include "structure/merkletree.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/pledge.h"
#include "template/template.h"
#include "template/vote.h"
#include "util.h"

using namespace std;
using namespace boost::filesystem;
using namespace mtbase;
using namespace metabasenet::mvm;

#define BLOCKFILE_PREFIX "block"
#define LOGFILE_NAME "storage.log"
namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CBlockBaseDBWalker

class CBlockWalker : public CBlockDBWalker
{
public:
    CBlockWalker(CBlockBase* pBaseIn)
      : pBase(pBaseIn) {}
    bool Walk(CBlockOutline& outline) override
    {
        return pBase->LoadIndex(outline);
    }

public:
    CBlockBase* pBase;
};

//////////////////////////////
// CForkHeightIndex

void CForkHeightIndex::AddHeightIndex(const uint32 nHeight, const uint256& hashBlock, const uint64 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
{
    mapHeightIndex[nHeight][hashBlock] = CBlockHeightIndex(nBlockTimeStamp, destMint, hashRefBlock);
}

void CForkHeightIndex::RemoveHeightIndex(const uint32 nHeight, const uint256& hashBlock)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        it->second.erase(hashBlock);
    }
}

void CForkHeightIndex::UpdateBlockRef(const uint32 nHeight, const uint256& hashBlock, const uint256& hashRefBlock)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        auto mt = it->second.find(hashBlock);
        if (mt != it->second.end())
        {
            mt->second.hashRefBlock = hashRefBlock;
        }
    }
}

map<uint256, CBlockHeightIndex>* CForkHeightIndex::GetBlockMintList(const uint32 nHeight)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        return &(it->second);
    }
    return nullptr;
}

bool CForkHeightIndex::GetMaxHeight(uint32& nMaxHeight)
{
    auto it = mapHeightIndex.rbegin();
    if (it != mapHeightIndex.rend())
    {
        nMaxHeight = it->first;
        return true;
    }
    return false;
}

//////////////////////////////
// CBlockState

CBlockState::CBlockState(CBlockBase& dbBlockBaseIn, const uint256& hashForkIn, const CBlock& block, const uint256& hashPrevStateRootIn, const uint32 nPrevBlockTimeIn, const std::map<CDestination, CAddressContext>& mapAddressContext)
  : dbBlockBase(dbBlockBaseIn), nBlockType(block.nType), nLocalChainId(CBlock::GetBlockChainIdByHash(hashForkIn)), hashFork(hashForkIn), hashPrevBlock(block.hashPrev),
    hashPrevStateRoot(hashPrevStateRootIn), nPrevBlockTime(nPrevBlockTimeIn), nOriBlockGasLimit(block.nGasLimit.Get64()), nSurplusBlockGasLimit(block.nGasLimit.Get64()),
    nBlockTimestamp(block.GetBlockTime()), nBlockHeight(block.GetBlockHeight()), nBlockNumber(block.GetBlockNumber()), destMint(block.txMint.GetToAddress()),
    mintTx(block.txMint), txidMint(block.txMint.GetHash()), fPrimaryBlock(block.IsPrimary()), mapBlockAddressContext(mapAddressContext), mapBlockProve(block.mapProve)
{
    hashRefBlock = 0;
    if (fPrimaryBlock)
    {
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

bool CBlockState::AddTxState(const CTransaction& tx, const int nTxIndex)
{
    uint256 txid = tx.GetHash();

    if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
    {
        mapBlockRewardLocked[tx.GetToAddress()] += tx.GetAmount();
    }

    bool fToContract = false;
    CDestination destTo;
    if (tx.GetToAddress().IsNull())
    {
        uint8 nCodeType;
        CTemplateContext ctxTemplate;
        CTxContractData ctxContract;
        if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
        {
            StdLog("CBlockState", "Add tx state: Get create code fail, txid: %s, from: %s",
                   txid.GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
            return false;
        }

        if (nCodeType == CODE_TYPE_TEMPLATE)
        {
            CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
            if (!ptr)
            {
                StdLog("CBlockState", "Add tx state: Import template fail, txid: %s", txid.GetHex().c_str());
                return false;
            }
            CDestination dest(ptr->GetTemplateId());

            if (mapBlockState.find(dest) == mapBlockState.end())
            {
                CDestState state;
                if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, dest, state))
                {
                    state.SetNull();
                    state.SetType(CDestination::PREFIX_TEMPLATE, ptr->GetTemplateType());
                    state.SetCodeHash(static_cast<uint256>(ptr->GetTemplateId()));
                }
                mapBlockState.insert(make_pair(dest, state));
            }
        }
        else if (nCodeType == CODE_TYPE_CONTRACT)
        {
            //destTo.SetContractId(tx.GetFromAddress(), tx.GetNonce());
            destTo = CreateContractAddressByNonce(tx.GetFromAddress(), tx.GetNonce());
            fToContract = true;

            if (mapBlockState.find(destTo) == mapBlockState.end())
            {
                CDestState state;
                if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, destTo, state))
                {
                    state.SetNull();
                    state.SetType(CDestination::PREFIX_CONTRACT);
                }
                mapBlockState.insert(make_pair(destTo, state));
            }
        }
        else
        {
            StdLog("CBlockState", "Add tx state: Code type error, txid: %s", txid.GetHex().c_str());
            return false;
        }
    }
    else
    {
        destTo = tx.GetToAddress();

        CAddressContext ctxAddress;
        if (!GetAddressContext(destTo, ctxAddress))
        {
            StdLog("CBlockState", "Add tx state: Retrieve address context fail, txid: %s, to: %s",
                   txid.GetHex().c_str(), destTo.ToString().c_str());
            return false;
        }
        if (ctxAddress.IsContract())
        {
            fToContract = true;
        }

        auto nt = mapBlockState.find(destTo);
        if (nt == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, destTo, state))
            {
                if (ctxAddress.IsContract())
                {
                    StdLog("CBlockState", "Add tx state: Contract address no state, txid: %s, to: %s",
                           txid.GetHex().c_str(), destTo.ToString().c_str());
                    return false;
                }
                state.SetNull();
                state.SetType(ctxAddress.GetDestType(), ctxAddress.GetTemplateType());
            }
            nt = mapBlockState.insert(make_pair(destTo, state)).first;
        }
        if (nt != mapBlockState.end())
        {
            CDestState& state = nt->second;
            if (state.IsPubkey() && ctxAddress.IsTemplate())
            {
                // When correcting the address type, it is necessary to simultaneously modify the address type in the status data.
                state.SetType(ctxAddress.GetDestType(), ctxAddress.GetTemplateType());
            }
        }
    }
    if (tx.GetAmount() != 0 && !destTo.IsNull())
    {
        auto it = mapBlockState.find(destTo);
        if (it == mapBlockState.end())
        {
            StdLog("CBlockState", "Add tx state: Get address state fail, txid: %s, destTo: %s",
                   txid.GetHex().c_str(), destTo.ToString().c_str());
            return false;
        }
        it->second.IncBalance(tx.GetAmount());
    }

    uint256 nTvGasFee;
    uint256 nTvGas;
    uint256 nLeftGas;
    if (!tx.GetFromAddress().IsNull())
    {
        auto mt = mapBlockState.find(tx.GetFromAddress());
        if (mt == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, tx.GetFromAddress(), state))
            {
                StdLog("CBlockState", "Add tx state: Retrieve dest state fail, txid: %s, from: %s",
                       txid.GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
                return false;
            }
            mt = mapBlockState.insert(make_pair(tx.GetFromAddress(), state)).first;
        }
        CDestState& stateFrom = mt->second;
        if (stateFrom.GetBalance() < (tx.GetAmount() + tx.GetTxFee()))
        {
            StdLog("CBlockState", "Add tx state: From dest balance error, nBalance: %s, nAmount+Fee: %s, txid: %s, from: %s",
                   CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(tx.GetAmount() + tx.GetTxFee()).c_str(),
                   txid.GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
            return false;
        }
        stateFrom.DecBalance(tx.GetAmount() + tx.GetTxFee());

        if (tx.GetTxType() != CTransaction::TX_CERT)
        {
            stateFrom.IncTxNonce();

            CAddressContext ctxFromAddress;
            if (GetAddressContext(tx.GetFromAddress(), ctxFromAddress) && ctxFromAddress.IsPubkey())
            {
                CTimeVault tv;
                if (!dbBlockBase.RetrieveTimeVault(hashFork, hashPrevBlock, tx.GetFromAddress(), tv))
                {
                    //StdLog("CBlockState", "Add tx state: Retrieve time vault fail, from: %s, txid: %s", tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
                    tv.SetNull();
                }
                nTvGasFee = tv.EstimateTransTvGasFee(nBlockTimestamp, tx.GetAmount());

                uint256 nPayTvFee;
                auto ht = mapBlockPayTvFee.find(tx.GetFromAddress());
                if (ht != mapBlockPayTvFee.end())
                {
                    nPayTvFee = ht->second;
                }
                uint256 nSyTvFee;
                if (tv.nTvAmount > nPayTvFee)
                {
                    nSyTvFee = tv.nTvAmount - nPayTvFee;
                }
                // StdDebug("TEST", "Add tx state: Calc prev, nTvGasFee: %s, nSyTvFee: %s, tv.nTvAmount: %s%s, nPayTvFee: %s, from: %s, txid: %s",
                //          CoinToTokenBigFloat(nTvGasFee).c_str(), CoinToTokenBigFloat(nSyTvFee).c_str(),
                //          (tv.fSurplus ? "" : "-"), CoinToTokenBigFloat(tv.nTvAmount).c_str(), CoinToTokenBigFloat(nPayTvFee).c_str(),
                //          tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
                if (nTvGasFee > nSyTvFee)
                {
                    nTvGasFee = nSyTvFee;
                }
                CTimeVault::CalcRealityTvGasFee(tx.GetGasPrice(), nTvGasFee, nTvGas);

                // StdDebug("TEST", "Add tx state: Reality tv gas, nTvGasFee: %s, nTvGas: %lu, from: %s, txid: %s",
                //          CoinToTokenBigFloat(nTvGasFee).c_str(), nTvGas.Get64(),
                //          tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
            }

            uint256 nTxBaseGas = tx.GetTxBaseGas();
            if (nTxBaseGas + nTvGas > tx.GetGasLimit())
            {
                // Gas not enough, cancel transaction

                StdLog("CBlockState", "Add tx state: Gas not enough, cancel transaction, base gas: %lu, tv gas: %lu, gas limit: %lu, from: %s, txid: %s",
                       nTxBaseGas.Get64(), nTvGas.Get64(), tx.GetGasLimit().Get64(), tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());

                uint256 nCanLeftGas;
                if (tx.GetGasLimit() > nTxBaseGas)
                {
                    nCanLeftGas = tx.GetGasLimit() - nTxBaseGas;
                }
                uint256 nLeftFee = tx.GetGasPrice() * nCanLeftGas;
                uint256 nUsedFee = tx.GetGasPrice() * nTxBaseGas;

                if (tx.GetAmount() != 0 && !destTo.IsNull())
                {
                    auto it = mapBlockState.find(destTo);
                    if (it == mapBlockState.end())
                    {
                        StdLog("CBlockState", "Add tx state: Get address state fail, txid: %s, destTo: %s",
                               txid.GetHex().c_str(), destTo.ToString().c_str());
                        return false;
                    }
                    it->second.DecBalance(tx.GetAmount());
                }
                stateFrom.IncBalance(nLeftFee + tx.GetAmount());

                mapBlockTxFeeUsed[txid] = nUsedFee;
                nSurplusBlockGasLimit -= nTxBaseGas.Get64();
                nBlockFeeLeft += nLeftFee;

                // Fail receipt
                CTransactionReceipt receipt;

                receipt.nReceiptType = CTransactionReceipt::RECEIPT_TYPE_COMMON;

                receipt.nContractStatus = 1; // 0: success, 1: fail

                receipt.nTxIndex = nTxIndex;
                receipt.txid = txid;
                receipt.nBlockNumber = nBlockNumber;
                receipt.from = tx.GetFromAddress();
                receipt.to = tx.GetToAddress();
                receipt.nTxGasUsed = nTxBaseGas;
                receipt.nTvGasUsed = 0;
                receipt.nEffectiveGasPrice = tx.GetGasPrice();

                receipt.CalcLogsBloom();

                //nBlockBloom |= receipt.nLogsBloom;
                receipt.GetBloomDataSet(setBlockBloomData);
                if (mapBlockTxReceipts.count(txid) == 0)
                {
                    mapBlockTxReceipts.insert(std::make_pair(txid, vBlockTxReceipts.size()));
                    vBlockTxReceipts.push_back(receipt);
                    mtbase::CBufStream ss;
                    ss << receipt;
                    vReceiptHash.push_back(metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize()));
                }
                return true;
            }
        }
        nLeftGas = tx.GetGasLimit() - (tx.GetTxBaseGas() + nTvGas);
    }

    CTransactionReceipt receipt;
    if (fToContract)
    {
        bool fCallResult = true;
        if (!AddContractState(txid, tx, nTxIndex, nLeftGas.Get64(), nTvGas, fCallResult, receipt))
        {
            StdLog("CBlockState", "Add tx state: Add contract state fail, txid: %s", txid.GetHex().c_str());
            return false;
        }
        if (fCallResult)
        {
            if (tx.GetAmount() != 0 && !destTo.IsNull() && tx.GetToAddress().IsNull())
            {
                // Create contract and trans amount.
                CContractTransfer ctrTransfer(CContractTransfer::CT_CONTRACT, tx.GetFromAddress(), destTo, tx.GetAmount());
                mapBlockContractTransfer[txid].push_back(ctrTransfer);
                receipt.vTransfer.push_back(ctrTransfer);
            }
        }
        else
        {
            if (tx.GetAmount() != 0 && !tx.IsRewardTx())
            {
                // Call contract fail to rollback.
                // Reward transaction special transaction, when execution fails, also makes the transfer successful.
                if (!destTo.IsNull())
                {
                    auto it = mapBlockState.find(destTo);
                    if (it == mapBlockState.end())
                    {
                        StdLog("CBlockState", "Add tx state: Find to address fail, to: %s, txid: %s", destTo.ToString().c_str(), txid.GetHex().c_str());
                        return false;
                    }
                    it->second.DecBalance(tx.GetAmount());
                }
                if (!tx.GetFromAddress().IsNull())
                {
                    auto mt = mapBlockState.find(tx.GetFromAddress());
                    if (mt == mapBlockState.end())
                    {
                        StdLog("CBlockState", "Add tx state: Find from address fail, from: %s, txid: %s", tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
                        return false;
                    }
                    mt->second.IncBalance(tx.GetAmount());
                }
            }
        }
    }
    else
    {
        uint256 nUsedGas;
        if (tx.GetGasLimit() > nLeftGas)
        {
            nUsedGas = tx.GetGasLimit() - nLeftGas;
        }
        uint256 nLeftFee = tx.GetGasPrice() * nLeftGas;
        uint256 nUsedFee = tx.GetGasPrice() * nUsedGas;
        if (nLeftFee > 0 && !tx.GetFromAddress().IsNull())
        {
            auto mt = mapBlockState.find(tx.GetFromAddress());
            if (mt == mapBlockState.end())
            {
                StdLog("CBlockState", "Add tx state: Get from state fail, from: %s, txid: %s", tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
                return false;
            }
            mt->second.IncBalance(nLeftFee);
        }
        mapBlockTxFeeUsed[txid] = nUsedFee;
        nSurplusBlockGasLimit -= nUsedGas.Get64();
        nBlockFeeLeft += nLeftFee;

        // Common tx receipt
        receipt.nReceiptType = CTransactionReceipt::RECEIPT_TYPE_COMMON;

        receipt.nTxIndex = nTxIndex;
        receipt.txid = txid;
        receipt.nBlockNumber = nBlockNumber;
        receipt.from = tx.GetFromAddress();
        receipt.to = tx.GetToAddress();
        receipt.nTxGasUsed = nUsedGas;
        receipt.nTvGasUsed = nTvGas;
        receipt.nEffectiveGasPrice = tx.GetGasPrice();
    }

    if (nTvGasFee > 0)
    {
        nBlockFeeLeft += nTvGasFee;

        uint8 nDestType = CDestination::PREFIX_PUBKEY;
        uint8 nTemplateType = 0;
        CDestination destTimeVaultToAddress;
        CFunctionAddressContext ctxPrevFuncAddress;
        if (dbBlockBase.RetrieveFunctionAddress(hashPrevBlock, FUNCTION_ID_TIME_VAULT_TO_ADDRESS, ctxPrevFuncAddress))
        {
            destTimeVaultToAddress = ctxPrevFuncAddress.GetFunctionAddress();
            CAddressContext ctxTvAddress;
            if (GetAddressContext(destTimeVaultToAddress, ctxTvAddress))
            {
                // if (ctxTvAddress.IsContract())
                // {
                //     CTransaction txTv;
                //     txTv.SetTxType(CTransaction::TX_INTERNAL);
                //     txTv.SetChainId(CBlock::GetBlockChainIdByHash(hashFork));
                //     txTv.SetNonce((nBlockNumber << 32) | nTxIndex);
                //     txTv.SetToAddress(destTimeVaultToAddress);
                //     txTv.SetAmount(nTvGasFee);

                //     bool fCallResult = true;
                //     CTransactionReceipt receiptTv;
                //     if (!AddContractState(txTv.GetHash(), txTv, 0, DEF_TX_GAS_LIMIT.Get64(), 0, fCallResult, receiptTv))
                //     {
                //         // Execution failure does not affect the process.
                //         StdLog("CBlockState", "Add tx state: Add tv contract state fail, txid: %s", txid.GetHex().c_str());
                //     }
                //     else
                //     {
                //         StdDebug("CBlockState", "Add tx state: Add tv contract state success, call result: %s, txid: %s", (fCallResult ? "true" : "false"), txid.GetHex().c_str());
                //     }
                // }
                nDestType = ctxTvAddress.GetDestType();
                nTemplateType = ctxTvAddress.GetTemplateType();
            }
        }
        else
        {
            destTimeVaultToAddress = TIME_VAULT_TO_ADDRESS;
        }

        CDestState stateTvOwner;
        if (!GetDestState(destTimeVaultToAddress, stateTvOwner))
        {
            stateTvOwner.SetNull();
            stateTvOwner.SetType(nDestType, nTemplateType);
        }
        stateTvOwner.IncBalance(nTvGasFee);
        SetDestState(destTimeVaultToAddress, stateTvOwner);

        mapBlockPayTvFee[tx.GetFromAddress()] += nTvGasFee;

        CContractTransfer ctrTransfer(CContractTransfer::CT_TIMEVAULT, tx.GetFromAddress(), destTimeVaultToAddress, nTvGasFee);
        mapBlockContractTransfer[txid].push_back(ctrTransfer);
        receipt.vTransfer.push_back(ctrTransfer);
    }

    receipt.CalcLogsBloom();
    //nBlockBloom |= receipt.nLogsBloom;
    receipt.GetBloomDataSet(setBlockBloomData);
    if (mapBlockTxReceipts.count(txid) == 0)
    {
        mapBlockTxReceipts.insert(std::make_pair(txid, vBlockTxReceipts.size()));
        vBlockTxReceipts.push_back(receipt);
        mtbase::CBufStream ss;
        ss << receipt;
        vReceiptHash.push_back(metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize()));
    }
    return true;
}

bool CBlockState::DoBlockState(uint256& hashReceiptRoot, uint256& nBlockGasUsed, bytes& btBlockBloomDataOut, uint256& nTotalMintRewardOut, uint256& hashCrosschainMerkleRoot, bool& fMoStatus)
{
    nBlockGasUsed = uint256(nOriBlockGasLimit - nSurplusBlockGasLimit);

    if (nOriginalBlockMintReward < nBlockFeeLeft)
    {
        StdLog("TEST", "Do block state: Original block mint reward error, nOriginalBlockMintReward: %s, nBlockFeeLeft: %s",
               nOriginalBlockMintReward.GetValueHex().c_str(), nBlockFeeLeft.GetValueHex().c_str());
        return false;
    }
    nTotalMintRewardOut = nOriginalBlockMintReward - nBlockFeeLeft;

    if (nBlockType == CBlock::BLOCK_GENESIS || nBlockType == CBlock::BLOCK_ORIGIN)
    {
        auto nt = mapBlockState.find(destMint);
        if (nt == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, destMint, state))
            {
                CAddressContext ctxAddress;
                if (!GetAddressContext(destMint, ctxAddress))
                {
                    StdLog("CBlockState", "Do block state: Retrieve mint address context fail, mint address: %s",
                           destMint.ToString().c_str());
                    return false;
                }
                state.SetNull();
                state.SetType(ctxAddress.GetDestType(), ctxAddress.GetTemplateType());
            }
            nt = mapBlockState.insert(make_pair(destMint, state)).first;
        }
        nt->second.IncBalance(nOriginalBlockMintReward);
    }

    if (nBlockType == CBlock::BLOCK_GENESIS || nBlockType == CBlock::BLOCK_ORIGIN)
    {
        CreateFunctionContractData();
    }

    if (fPrimaryBlock)
    {
        if (!CalcPledgeRedeem())
        {
            StdError("CBlockState", "Do block state: Calc pledge redeem fail, block height: %d, prev block: %s", nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
    }

    if (nBlockType != CBlock::BLOCK_GENESIS
        && nBlockType != CBlock::BLOCK_ORIGIN
        && nBlockType != CBlock::BLOCK_VACANT)
    {
        if (!CalcCrosschainTransfer())
        {
            StdError("CBlockState", "Do block state: Calc crosschain transfer fail, block height: %d, prev block: %s", nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        bool fMatchDex = false;
        if (!CalcCrosschainDex(fMatchDex))
        {
            StdError("CBlockState", "Do block state: Calc crosschain dex fail, block height: %d, prev block: %s", nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        if (fMatchDex)
        {
            fMoStatus = true;
        }
        CalcCrosschainConfirmRecvBlock();
        SetPeerPrevCrossLastBlock();
    }

    for (const auto& kv : mapContractKvState)
    {
        const CDestination& destContract = kv.first;
        CDestState stateDestContract;
        if (!GetDestState(destContract, stateDestContract))
        {
            stateDestContract.SetNull();
            stateDestContract.SetType(CDestination::PREFIX_PUBKEY); // WAIT_CHECK
        }
        uint256 hashRoot;
        if (!dbBlockBase.AddBlockContractKvValue(hashFork, stateDestContract.GetStorageRoot(), hashRoot, kv.second))
        {
            StdLog("CBlockState", "Do block state: Add block contract state fail, contract address: %s", destContract.ToString().c_str());
            return false;
        }
        stateDestContract.SetStorageRoot(hashRoot);
        SetDestState(destContract, stateDestContract);
    }
    if (!mapContractKvState.empty())
    {
        fMoStatus = true;
    }

    for (auto& kv : mapBlockAddressContext)
    {
        setBlockBloomData.insert(kv.first.GetBytes());
    }
    for (auto& kv : mapBlockContractTransfer)
    {
        for (auto& vd : kv.second)
        {
            if (!vd.destFrom.IsNull())
            {
                setBlockBloomData.insert(vd.destFrom.GetBytes());
            }
            if (!vd.destTo.IsNull())
            {
                setBlockBloomData.insert(vd.destTo.GetBytes());
            }
        }
    }

    if (receiptMintTx.vTransfer.size() > 0)
    {
        receiptMintTx.nReceiptType = CTransactionReceipt::RECEIPT_TYPE_COMMON;

        receiptMintTx.nTxIndex = 0;
        receiptMintTx.txid = txidMint;
        receiptMintTx.nBlockNumber = nBlockNumber;
        receiptMintTx.from = mintTx.GetFromAddress();
        receiptMintTx.to = mintTx.GetToAddress();
        receiptMintTx.nTxGasUsed = 0;
        receiptMintTx.nTvGasUsed = 0;
        receiptMintTx.nEffectiveGasPrice = mintTx.GetGasPrice();

        receiptMintTx.CalcLogsBloom();
        //nBlockBloom |= receiptMintTx.nLogsBloom;
        receiptMintTx.GetBloomDataSet(setBlockBloomData);
        if (mapBlockTxReceipts.count(txidMint) == 0)
        {
            mapBlockTxReceipts.insert(std::make_pair(txidMint, vBlockTxReceipts.size()));
            vBlockTxReceipts.push_back(receiptMintTx);
            mtbase::CBufStream ss;
            ss << receiptMintTx;
            vReceiptHash.push_back(metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize()));
        }
    }

    hashReceiptRoot = CMerkleTree::CalcMerkleTreeRoot(vReceiptHash);
    hashCrosschainMerkleRoot = CalcCrosschainMerkleRoot();

    GetBlockBloomData(btBlockBloomDataOut);
    return true;
}

bool CBlockState::GetDestState(const CDestination& dest, CDestState& stateDest)
{
    auto mt = mapCacheContractData.find(dest);
    if (mt != mapCacheContractData.end() && !mt->second.cacheDestState.IsNull())
    {
        stateDest = mt->second.cacheDestState;
        return true;
    }
    auto it = mapBlockState.find(dest);
    if (it != mapBlockState.end())
    {
        stateDest = it->second;
        return true;
    }
    return dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, dest, stateDest);
}

void CBlockState::SetDestState(const CDestination& dest, const CDestState& stateDest)
{
    mapBlockState[dest] = stateDest;
}

void CBlockState::SetCacheDestState(const CDestination& dest, const CDestState& stateDest)
{
    mapCacheContractData[dest].cacheDestState = stateDest;
}

bool CBlockState::GetDestKvData(const CDestination& dest, const uint256& key, bytes& value)
{
    auto nt = mapCacheContractData.find(dest);
    if (nt != mapCacheContractData.end())
    {
        auto mt = nt->second.cacheContractKv.find(key);
        if (mt != nt->second.cacheContractKv.end())
        {
            value = mt->second;
            return true;
        }
    }
    auto it = mapContractKvState.find(dest);
    if (it != mapContractKvState.end())
    {
        auto mt = it->second.find(key);
        if (mt != it->second.end())
        {
            value = mt->second;
            return true;
        }
    }
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        return false;
    }
    return dbBlockBase.RetrieveContractKvValue(hashFork, stateDest.GetStorageRoot(), key, value);
}

bool CBlockState::GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress)
{
    auto it = mapCacheAddressContext.find(dest);
    if (it != mapCacheAddressContext.end())
    {
        ctxAddress = it->second;
        return true;
    }
    auto nt = mapBlockAddressContext.find(dest);
    if (nt != mapBlockAddressContext.end())
    {
        ctxAddress = nt->second;
        return true;
    }
    return dbBlockBase.RetrieveAddressContext(hashFork, hashPrevBlock, dest, ctxAddress);
}

bool CBlockState::IsContractAddress(const CDestination& addr)
{
    CAddressContext ctxAddress;
    if (!GetAddressContext(addr, ctxAddress))
    {
        return false;
    }
    return ctxAddress.IsContract();
}

bool CBlockState::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    CDestState stateContract;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Get contract run code: Get contract state failed, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    if (stateContract.IsDestroy())
    {
        StdLog("CBlockState", "Get contract run code: Contract has been destroyed, contract address: %s", destContractIn.ToString().c_str());
        fDestroy = true;
        return true;
    }
    fDestroy = false;

    bytes btDestCodeData;
    if (!GetDestKvData(destContractIn, destContractIn.ToHash(), btDestCodeData))
    {
        StdLog("CBlockState", "Get contract run code: Get contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get contract run code: Parse contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractRunCodeContext ctxRunCode;
    if (!dbBlockBase.RetrieveContractRunCodeContext(hashFork, hashPrevBlock, ctxDestCode.hashContractRunCode, ctxRunCode))
    {
        StdLog("CBlockState", "Get contract run code: Retrieve contract run code fail, hashContractRunCode: %s, contract address: %s",
               ctxDestCode.hashContractRunCode.GetHex().c_str(), destContractIn.ToString().c_str());
        return false;
    }

    hashContractCreateCode = ctxDestCode.hashContractCreateCode;
    destCodeOwner = ctxDestCode.destCodeOwner;
    hashContractRunCode = ctxDestCode.hashContractRunCode;
    btContractRunCode = ctxRunCode.btContractRunCode;
    return true;
}

bool CBlockState::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    bytes btDestCodeData;
    if (!GetDestKvData(destContractIn, destContractIn.ToHash(), btDestCodeData))
    {
        StdLog("CBlockState", "Get contract create code: Get contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get contract create code: Parse contract code fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }

    if (!dbBlockBase.GetBlockContractCreateCodeData(hashFork, hashPrevBlock, ctxDestCode.hashContractCreateCode, txcd))
    {
        StdLog("CBlockState", "Get contract create code: Get contract create code fail, hashContractCreateCode: %s, contract address: %s, prev block: %s",
               ctxDestCode.hashContractCreateCode.GetHex().c_str(), destContractIn.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockState::GetBlockHashByNumber(const uint64 nBlockNumberIn, uint256& hashBlockOut)
{
    return dbBlockBase.GetBlockHashByNumber(hashFork, nBlockNumberIn, hashBlockOut);
}

void CBlockState::GetBlockBloomData(bytes& btBloomDataOut)
{
    if (!setBlockBloomData.empty())
    {
        CNhBloomFilter bf(setBlockBloomData.size() * 4);
        for (auto& bt : setBlockBloomData)
        {
            bf.Add(bt);
        }
        bf.GetData(btBloomDataOut);
    }
}

bool CBlockState::GetDestBalance(const CDestination& dest, uint256& nBalance)
{
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        StdLog("CBlockState", "Get dest balance: Get dest state failed, dest: %s", dest.ToString().c_str());
        return false;
    }
    uint256 nLockedAmount;
    if (!GetDestLockedAmount(dest, nLockedAmount))
    {
        StdLog("CBlockState", "Get dest balance: Get locked amount failed, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (stateDest.GetBalance() > nLockedAmount)
    {
        nBalance = stateDest.GetBalance() - nLockedAmount;
    }
    else
    {
        nBalance = 0;
    }
    return true;
}

bool CBlockState::SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd, const uint256& txidCreate)
{
    uint256 hashContractCreateCode = txcd.GetContractCreateCodeHash();
    uint256 hashContractRunCode = metabasenet::crypto::CryptoKeccakHash(btContractRunCode.data(), btContractRunCode.size());

    CContractDestCodeContext ctxDestCode(hashContractCreateCode, hashContractRunCode, txcd.GetCodeOwner());
    bytes btDestCodeData;
    CBufStream ss;
    ss << ctxDestCode;
    ss.GetData(btDestCodeData);

    CDestState stateContractDest;
    if (GetDestState(destContractIn, stateContractDest) && stateContractDest.GetCodeHash() != 0)
    {
        StdLog("CBlockState", "Save contract run code: contract address already exists, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    stateContractDest.SetType(CDestination::PREFIX_CONTRACT);
    stateContractDest.SetCodeHash(hashContractRunCode);

    auto& cachContract = mapCacheContractData[destContractIn];
    cachContract.cacheContractKv[destContractIn.ToHash()] = btDestCodeData;
    cachContract.cacheDestState = stateContractDest;

    mapCacheContractCreateCodeContext[hashContractCreateCode] = CContractCreateCodeContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), txcd.GetCodeOwner(), txcd.GetCode(),
                                                                                           txidCreate, txcd.GetSourceCodeHash(), hashContractRunCode);
    mapCacheContractRunCodeContext[hashContractRunCode] = CContractRunCodeContext(hashContractCreateCode, btContractRunCode);
    mapCacheAddressContext[destContractIn] = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetCodeOwner(), txcd.GetName(), txcd.GetDescribe(), txidCreate, txcd.GetSourceCodeHash(), txcd.GetContractCreateCodeHash(), hashContractRunCode));
    return true;
}

bool CBlockState::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult)
{
    if (btData.size() < 4)
    {
        StdLog("CBlockState", "Exec function contract: data size error, data size: %lu", btData.size());
        return false;
    }
    bytes btTxParam;
    if (btData.size() > 4)
    {
        btTxParam.assign(btData.begin() + 4, btData.end());
    }
    bytes btFuncSign(btData.begin(), btData.begin() + 4);
    CTransactionLogs logs;
    nGasLeft = nGasLimit;
    return DoFuncContractCall(destFromIn, destToIn, btFuncSign, btTxParam, nGasLimit, nGasLeft, logs, btResult);
}

bool CBlockState::Selfdestruct(const CDestination& destContractIn, const CDestination& destBeneficiaryIn)
{
    CDestState stateContract;
    CDestState stateBeneficiary;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Selfdestruct: Get contract state fail, contract address: %s", destContractIn.ToString().c_str());
        return false;
    }
    if (stateContract.IsDestroy())
    {
        return true;
    }
    if (!GetDestState(destBeneficiaryIn, stateBeneficiary))
    {
        stateBeneficiary.SetNull();
        stateBeneficiary.SetType(CDestination::PREFIX_PUBKEY); // WAIT_CHECK
    }

    stateBeneficiary.IncBalance(stateContract.GetBalance());
    stateContract.SetBalance(0);
    stateContract.SetDestroy(true);

    SetCacheDestState(destContractIn, stateContract);
    SetCacheDestState(destBeneficiaryIn, stateBeneficiary);
    return true;
}

void CBlockState::SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed)
{
    if (nGasUsed > 0)
    {
        mapCacheCodeDestGasUsed[destCodeOwner] += nGasUsed;
    }
}

void CBlockState::SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv)
{
    auto& cacheContract = mapCacheContractData[destContractIn];

    // CDestState stateContractDest;
    // if (GetDestState(destContractIn, stateContractDest))
    // {
    //     cacheContract.cacheDestState = stateContractDest;
    // }

    for (auto& kv : mapCacheKv)
    {
        cacheContract.cacheContractKv[kv.first] = kv.second;
    }
    for (auto& logs : vLogsIn)
    {
        cacheContract.cacheContractLogs.push_back(logs);
    }

    if (mapCacheAddressContext.find(destContractIn) == mapCacheAddressContext.end())
    {
        CAddressContext ctxAddress;
        if (!GetAddressContext(destContractIn, ctxAddress))
        {
            StdError("CBlockState", "Save run result: Get address context fail, destContractIn: %s", destContractIn.ToString().c_str());
        }
        else
        {
            mapCacheAddressContext[destContractIn] = ctxAddress;
        }
    }
}

bool CBlockState::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft, const CAddressContext& ctxToAddress, const uint8 nTransferType)
{
    if (ctxToAddress.IsTemplate())
    {
        if (ctxToAddress.GetTemplateType() == TEMPLATE_VOTE && !fEnableStakeVote)
        {
            return false;
        }
        if (ctxToAddress.GetTemplateType() == TEMPLATE_PLEDGE && !fEnableStakePledge)
        {
            return false;
        }
    }
    if (nGasLeft < FUNCTION_TX_GAS_TRANS)
    {
        StdLog("CBlockState", "Contract transfer: Gas not enough, gas left: %lu, from: %s", nGasLeft, from.ToString().c_str());
        nGasLeft = 0;
        return false;
    }
    nGasLeft -= FUNCTION_TX_GAS_TRANS;

    CDestState stateFrom;
    if (!GetDestState(from, stateFrom))
    {
        StdLog("CBlockState", "Contract transfer: Get dest state fail, from: %s", from.ToString().c_str());
        return false;
    }
    if (stateFrom.GetBalance() < amount)
    {
        StdLog("CBlockState", "Contract transfer: Balance not enough, balance: %s, amount: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(amount).c_str(), from.ToString().c_str());
        return false;
    }

    CDestState stateTo;
    if (!GetDestState(to, stateTo))
    {
        stateTo.SetNull();

        CAddressContext ctxTempAddress;
        if (!GetAddressContext(to, ctxTempAddress))
        {
            mapCacheAddressContext[to] = ctxToAddress;
            stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType()); // WAIT_CHECK
        }
        else
        {
            mapCacheAddressContext[to] = ctxTempAddress;
            stateTo.SetType(ctxTempAddress.GetDestType(), ctxTempAddress.GetTemplateType());
        }
    }
    else
    {
        CAddressContext ctxTempAddress;
        if (!GetAddressContext(to, ctxTempAddress))
        {
            if (ctxToAddress.GetDestType() == stateTo.GetDestType()
                && ctxToAddress.GetTemplateType() == stateTo.GetTemplateType())
            {
                mapCacheAddressContext[to] = ctxToAddress;
            }
            else
            {
                StdLog("CBlockState", "Contract transfer: State address context error, State address type: %d-%d, in address type: %d-%d, from: %s, to: %s",
                       stateTo.GetDestType(), stateTo.GetTemplateType(),
                       ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType(),
                       from.ToString().c_str(), to.ToString().c_str());
                return false;
            }
        }
        else
        {
            mapCacheAddressContext[to] = ctxTempAddress;
        }
    }

    CAddressContext ctxTempFromAddress;
    if (!GetAddressContext(from, ctxTempFromAddress))
    {
        StdLog("CBlockState", "Contract transfer: Get from address context failed, from: %s", from.ToString().c_str());
        return false;
    }
    if (mapCacheAddressContext.count(from) == 0)
    {
        mapCacheAddressContext[from] = ctxTempFromAddress;
    }

    // verify reward lock
    uint256 nLockedAmount;
    if (!GetDestLockedAmount(from, nLockedAmount))
    {
        StdLog("CBlockState", "Contract transfer: Get reward locked amount failed, from: %s", from.ToString().c_str());
        return false;
    }
    if (nLockedAmount > 0 && stateFrom.GetBalance() < nLockedAmount + amount)
    {
        StdLog("CBlockState", "Contract transfer: Balance locked, balance: %s, lock amount: %s, amount: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(nLockedAmount).c_str(),
               CoinToTokenBigFloat(amount).c_str(), from.ToString().c_str());
        return false;
    }

    stateFrom.DecBalance(amount);
    stateTo.IncBalance(amount);

    SetCacheDestState(from, stateFrom);
    SetCacheDestState(to, stateTo);

    vCacheContractTransfer.push_back(CContractTransfer(nTransferType, from, to, amount));
    return true;
}

bool CBlockState::IsContractDestroy(const CDestination& destContractIn)
{
    CDestState stateContract;
    if (!GetDestState(destContractIn, stateContract))
    {
        StdLog("CBlockState", "Is contract destroy: Get contract state failed, contract address: %s", destContractIn.ToString().c_str());
        return true;
    }
    if (stateContract.IsDestroy())
    {
        StdLog("CBlockState", "Is contract destroy: Contract has been destroyed, contract address: %s", destContractIn.ToString().c_str());
        return true;
    }
    return false;
}

void CBlockState::UpdateCrosschainProveTail(const uint256& hashPrevBlock, const std::vector<std::pair<uint8, uint256>>& vPrevBlockMerkleProve,
                                            const uint256& hashRefBlock, const std::vector<std::pair<uint8, uint256>>& vRefBlockMerkleProve,
                                            const std::vector<std::pair<uint8, uint256>>& vCrossMerkleProveTail)
{
    proveBlockCrosschain.hashPrevBlock = hashPrevBlock;
    proveBlockCrosschain.vPrevBlockMerkleProve = vPrevBlockMerkleProve;

    proveBlockCrosschain.hashRefBlock = hashRefBlock;
    proveBlockCrosschain.vRefBlockMerkleProve = vRefBlockMerkleProve;

    for (auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        kv.second.second.insert(kv.second.second.end(), vCrossMerkleProveTail.begin(), vCrossMerkleProveTail.end());
    }
}

//////////////////////////////////////////
void CBlockState::CreateFunctionContractData()
{
    bytes btCreateCode = getFunctionContractCreateCode();
    bytes btRuntimeCode = getFunctionContractRuntimeCode();

    uint256 hashCreateCode = crypto::CryptoKeccakHash(btCreateCode.data(), btCreateCode.size());
    uint256 hashRuntimeCode = crypto::CryptoKeccakHash(btRuntimeCode.data(), btRuntimeCode.size());

    const CDestination& destFuncContract = FUNCTION_CONTRACT_ADDRESS;

    // create state
    CContractDestCodeContext ctxDestCode(hashCreateCode, hashRuntimeCode, destFuncContract);
    bytes btDestCodeData;
    CBufStream ss;
    ss << ctxDestCode;
    ss.GetData(btDestCodeData);
    CDestState stateContractDest;
    stateContractDest.SetType(CDestination::PREFIX_CONTRACT);
    stateContractDest.SetCodeHash(hashRuntimeCode);
    SetDestState(destFuncContract, stateContractDest);
    mapContractKvState[destFuncContract][destFuncContract.ToHash()] = btDestCodeData;

    // create address context
    CContractAddressContext ctxContract;
    ctxContract.destCodeOwner = destFuncContract;
    ctxContract.hashContractCreateCode = hashCreateCode;
    ctxContract.hashContractRunCode = hashRuntimeCode;
    mapBlockAddressContext[destFuncContract] = CAddressContext(ctxContract);

    mapBlockAddressContext[FUNCTION_BLACKHOLE_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[FUNCTION_CROSSCHAIN_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[PLEDGE_SURPLUS_REWARD_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[TIME_VAULT_TO_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[PROJECT_PARTY_REWARD_TO_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[FOUNDATION_REWARD_TO_ADDRESS] = CAddressContext(CPubkeyAddressContext());
    mapBlockAddressContext[FUNCTION_DEX_POOL_ADDRESS] = CAddressContext(CPubkeyAddressContext());

    // create create and runtime code context
    mapBlockContractCreateCodeContext[hashCreateCode] = CContractCreateCodeContext({}, {}, {}, destFuncContract, btCreateCode, {}, {}, hashRuntimeCode);
    mapBlockContractRunCodeContext[hashRuntimeCode] = CContractRunCodeContext(hashCreateCode, btRuntimeCode);
}

bool CBlockState::GetDestContractCode(const CTransaction& tx, CDestination& destContract, bytes& btContractCode, bytes& btRunParam, uint256& hashContractCreateCode,
                                      CDestination& destCodeOwner, CTxContractData& txcd, bool& fCall, bool& fDestroy)
{
    fCall = true;
    fDestroy = false;
    if (tx.GetToAddress().IsNull())
    {
        //destContract.SetContractId(tx.GetFromAddress(), tx.GetNonce());
        destContract = CreateContractAddressByNonce(tx.GetFromAddress(), tx.GetNonce());

        uint8 nCodeType;
        CTemplateContext ctxTemplate;
        CTxContractData ctxContract;
        if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
        {
            StdLog("CBlockState", "Get dest contract code: Get create code context fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }

        if (nCodeType != CODE_TYPE_CONTRACT)
        {
            StdLog("CBlockState", "Get dest contract code: Code type error, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
        if (ctxContract.IsUpcode())
        {
            fCall = false;
        }
        if (ctxContract.IsCreate() || ctxContract.IsUpcode())
        {
            ctxContract.UncompressCode();
            hashContractCreateCode = ctxContract.GetContractCreateCodeHash();

            if (ctxContract.IsCreate())
            {
                CContractCreateCodeContext ctxtContractCreateCode;
                bool fLinkGenesisFork;
                if (!dbBlockBase.RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashPrevBlock, hashContractCreateCode, ctxtContractCreateCode, fLinkGenesisFork))
                {
                    StdLog("CBlockState", "Get dest contract code: find create code context fail, curr code owner: %s, create code hash: %s",
                           ctxContract.destCodeOwner.ToString().c_str(), hashContractCreateCode.ToString().c_str());
                    if (ctxContract.destCodeOwner.IsNull())
                    {
                        ctxContract.destCodeOwner = tx.GetFromAddress();
                    }
                }
                else
                {
                    StdLog("CBlockState", "Get dest contract code: curr code owner: %s, old code owner: %s, create code hash: %s",
                           ctxContract.destCodeOwner.ToString().c_str(),
                           ctxtContractCreateCode.destCodeOwner.ToString().c_str(),
                           hashContractCreateCode.ToString().c_str());
                    ctxContract.destCodeOwner = ctxtContractCreateCode.destCodeOwner;
                }
            }
            txcd = ctxContract;
        }
        else if (ctxContract.IsSetup())
        {
            hashContractCreateCode = ctxContract.GetContractCreateCodeHash();
            if (!dbBlockBase.GetBlockContractCreateCodeData(hashFork, hashPrevBlock, hashContractCreateCode, txcd))
            {
                StdLog("CBlockState", "Get dest contract code: Get create code data fail, prev block: %s", hashPrevBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            StdLog("CBlockState", "Get dest contract code: Code flag error, flag: %d, prev block: %s",
                   ctxContract.nMuxType, hashPrevBlock.GetHex().c_str());
            return false;
        }
        btContractCode = txcd.GetCode();
        destCodeOwner = txcd.GetCodeOwner();

        if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btRunParam))
        {
            btRunParam.clear();
        }
    }
    else
    {
        destContract = tx.GetToAddress();
        if (tx.IsEthTx())
        {
            if (!tx.GetTxData(CTransaction::DF_ETH_TX_DATA, btRunParam))
            {
                btRunParam.clear();
            }
        }
        else
        {
            if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btRunParam))
            {
                btRunParam.clear();
            }
        }
        uint256 hashContractRunCode;
        if (!GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractCode, fDestroy))
        {
            StdLog("CBlockState", "Get dest contract code: Get contract run code fail, destContract: %s", destContract.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockState::CalcPledgeRedeem()
{
    std::map<CDestination, std::pair<uint32, uint32>> mapPledgeFinalHeight;
    if (!dbBlockBase.ListPledgeFinalHeight(hashPrevBlock, nBlockHeight, mapPledgeFinalHeight))
    {
        StdError("CBlockState", "Calc pledge redeem: List pledge final height failed, block height: %d, prev block: %s", nBlockHeight, hashPrevBlock.ToString().c_str());
        return false;
    }
    for (auto& kv : mapPledgeFinalHeight)
    {
        const CDestination& destPledge = kv.first;

        CVoteContext ctxVote;
        if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destPledge, ctxVote))
        {
            StdLog("CBlockState", "Calc pledge redeem: Retrieve dest vote context fail, pledge dest: %s, block height: %d, prev block: %s",
                   destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        if (kv.second.first != ctxVote.nFinalHeight)
        {
            StdLog("CBlockState", "Calc pledge redeem: Final height error, get final height: %d, vote final height: %d, pledge dest: %s, block height: %d, prev block: %s",
                   kv.second.first, ctxVote.nFinalHeight, destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            continue;
        }

        CAddressContext ctxFromAddress;
        if (!GetAddressContext(destPledge, ctxFromAddress))
        {
            StdLog("CBlockState", "Calc pledge redeem: Retrieve pledge address context fail, pledge address: %s, block height: %d, prev block: %s",
                   destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        CTemplateAddressContext ctxTemplate;
        if (!ctxFromAddress.GetTemplateAddressContext(ctxTemplate))
        {
            StdLog("CBlockState", "Calc pledge redeem: Get template address context failed, pledge address: %s, block height: %d, prev block: %s",
                   destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
        if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
        {
            StdLog("CBlockState", "Calc pledge redeem: Import pledge template fail or template type error, pledge address: %s, block height: %d, prev block: %s",
                   destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        const CDestination& destTo = boost::dynamic_pointer_cast<CTemplatePledge>(ptr)->destOwner;

        CDestState statePledge;
        if (!GetDestState(destPledge, statePledge))
        {
            StdLog("CBlockState", "Calc pledge redeem: Get pledge state fail, pledge address: %s, block height: %d, prev block: %s",
                   destPledge.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
            return false;
        }
        uint256 nRedeemAmount = statePledge.GetBalance();

        CAddressContext ctxToAddress;
        if (!GetAddressContext(destTo, ctxToAddress))
        {
            ctxToAddress = CAddressContext(CPubkeyAddressContext());
        }
        if (ctxToAddress.IsContract())
        {
            CTransaction txRedeem;
            txRedeem.SetTxType(CTransaction::TX_INTERNAL);
            txRedeem.SetChainId(CBlock::GetBlockChainIdByHash(hashFork));
            txRedeem.SetNonce(mintTx.GetNonce());
            txRedeem.SetFromAddress(destPledge);
            txRedeem.SetToAddress(destTo);
            txRedeem.SetAmount(nRedeemAmount);

            bool fCallResult = true;
            CTransactionReceipt receiptRedeem;
            if (!AddContractState(txRedeem.GetHash(), txRedeem, 0, PLEDGE_REDEEM_TX_GAS_LIMIT, 0, fCallResult, receiptRedeem))
            {
                // Execution failure does not affect the process.
                StdLog("CBlockState", "Calc pledge redeem: Add redeem contract state fail");
            }
            else
            {
                StdDebug("CBlockState", "Calc pledge redeem: Add redeem contract state success, call result: %s", (fCallResult ? "true" : "false"));
            }
        }

        CDestState stateTo;
        if (!GetDestState(destTo, stateTo))
        {
            stateTo.SetNull();
            stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType());
        }
        stateTo.IncBalance(nRedeemAmount);
        statePledge.SetBalance(0);

        SetDestState(destPledge, statePledge);
        SetDestState(destTo, stateTo);

        mapBlockAddressContext[destTo] = ctxToAddress;

        CContractTransfer ctrTransfer(CContractTransfer::CT_REDEEM, destPledge, destTo, nRedeemAmount);
        mapBlockContractTransfer[txidMint].push_back(ctrTransfer);

        receiptMintTx.vTransfer.push_back(ctrTransfer);

        // When redeeming pledge, give timevault
        mapBlockPayTvFee[destTo] += CTimeVault::CalcGiveTvFee(nRedeemAmount);

        StdLog("CBlockState", "Calc pledge redeem: Pledge redeem, redeem amount: %s, pledge address: %s, owner address: %s, block height: %d, prev block: %s",
               CoinToTokenBigFloat(nRedeemAmount).c_str(), destPledge.ToString().c_str(), destTo.ToString().c_str(), nBlockHeight, hashPrevBlock.ToString().c_str());
    }
    return true;
}

bool CBlockState::CalcCrosschainTransfer()
{
    const uint256 hashGenesisFork = dbBlockBase.GetGenesisBlockHash();
    const CChainId nGenChainId = CBlock::GetBlockChainIdByHash(hashGenesisFork);

    for (const auto& kv : mapBlockProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        auto funcTransferWork = [&](const CBlockCrosschainProve& ccProve) -> bool {
            for (const CBlockCoinTransferProve& transferProve : ccProve.GetCoinTransferProve())
            {
                if (transferProve.nDestChainId == nLocalChainId)
                {
                    const CDestination& destFrom = FUNCTION_CROSSCHAIN_ADDRESS;
                    const CDestination& destTo = transferProve.destTransfer;
                    const uint256& nTransferAmount = transferProve.nTransferAmount;

                    CAddressContext ctxToAddress;
                    if (!GetAddressContext(destTo, ctxToAddress))
                    {
                        ctxToAddress = CAddressContext(CPubkeyAddressContext()); // WAIT_CHECK
                    }

                    CDestState stateFrom;
                    CDestState stateTo;
                    if (!GetDestState(destFrom, stateFrom))
                    {
                        stateFrom.SetNull();
                        stateFrom.SetType(CDestination::PREFIX_PUBKEY, 0);
                    }
                    if (!GetDestState(destTo, stateTo))
                    {
                        stateTo.SetNull();
                        stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType());
                    }

                    stateTo.IncBalance(nTransferAmount);
                    if (nLocalChainId == nGenChainId)
                    {
                        stateFrom.DecBalance(nTransferAmount);
                    }
                    else
                    {
                        stateFrom.IncBalance(nTransferAmount);
                    }

                    SetDestState(destFrom, stateFrom);
                    SetDestState(destTo, stateTo);

                    mapBlockAddressContext[destTo] = ctxToAddress;

                    CContractTransfer ctrTransfer(CContractTransfer::CT_CROSSCHAIN_TRANSFER, destFrom, destTo, nTransferAmount);
                    mapBlockContractTransfer[txidMint].push_back(ctrTransfer);

                    receiptMintTx.vTransfer.push_back(ctrTransfer);
                }
                else
                {
                    StdLog("CBlockState", "Calc crosschain transfer: Transfer chainid error, dest chainid: %d, local chainid: %d", transferProve.nDestChainId, nLocalChainId);
                }
            }
            return true;
        };

        if (!funcTransferWork(blockProve.proveCrosschain))
        {
            StdLog("CBlockState", "Calc crosschain transfer: Transfer fail, block: %s, peer chainid: %d", blockProve.hashBlock.ToString().c_str(), nPeerChainId);
            return false;
        }

        uint256 hashAtBlock = blockProve.hashPrevBlock;
        for (const auto& prevProve : blockProve.vPrevBlockCcProve)
        {
            if (!funcTransferWork(prevProve.proveCrosschain))
            {
                StdLog("CBlockState", "Calc crosschain transfer: Transfer fail2, at block: %s, peer chainid: %d", hashAtBlock.ToString().c_str(), nPeerChainId);
                return false;
            }
            hashAtBlock = prevProve.hashPrevBlock;
        }
    }
    return true;
}

bool CBlockState::CalcCrosschainDex(bool& fMatchDex)
{
    std::map<uint256, CMatchOrderResult> mapMatchResult;
    if (!dbBlockBase.GetMatchDexData(hashPrevBlock, mapMatchResult))
    {
        StdLog("CBlockState", "Calc crosschain dex: Get match dex fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
        return false;
    }
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashPrevBlock);

    uint256 hashPrevRefBlock;
    if (!dbBlockBase.GetBlockForRefBlockNoLock(hashPrevBlock, hashPrevRefBlock))
    {
        StdLog("CBlockState", "Calc crosschain dex: Get prev ref block fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
        return false;
    }

    for (const auto& kv : mapMatchResult)
    {
        const uint256& hashCoinPair = kv.first;
        const CMatchOrderResult& matchResult = kv.second;

        CCoinContext ctxCoinSell;
        CCoinContext ctxCoinBuy;
        if (!dbBlockBase.GetForkCoinCtxByForkSymbol(matchResult.strCoinSymbolSell, ctxCoinSell, hashPrevRefBlock))
        {
            StdLog("CBlockState", "Calc crosschain dex: Get sell coin context fail, sell symbol: %s, prev ref block: %s", matchResult.strCoinSymbolSell.c_str(), hashPrevRefBlock.GetBhString().c_str());
            return false;
        }
        if (!dbBlockBase.GetForkCoinCtxByForkSymbol(matchResult.strCoinSymbolBuy, ctxCoinBuy, hashPrevRefBlock))
        {
            StdLog("CBlockState", "Calc crosschain dex: Get buy coin context fail, buy symbol: %s, prev ref block: %s", matchResult.strCoinSymbolBuy.c_str(), hashPrevRefBlock.GetBhString().c_str());
            return false;
        }

        for (const CMatchOrderRecord& matchRecord : matchResult.vMatchOrderRecord)
        {
            auto funcTrans = [&](const CDestination& destFrom, const CDestination& destTo, const uint256& nTransferAmount, const uint8 nCoinTypeFrom, const CDestination& destFromContractAddress) -> bool {
                if (nCoinTypeFrom == CCoinContext::CT_COIN_TYPE_FORK || nCoinTypeFrom == CCoinContext::CT_COIN_TYPE_FORK)
                {
                    CAddressContext ctxToAddress;
                    if (!GetAddressContext(destTo, ctxToAddress))
                    {
                        ctxToAddress = CAddressContext(CPubkeyAddressContext());
                    }

                    CDestState stateFrom;
                    CDestState stateTo;
                    if (!GetDestState(destFrom, stateFrom))
                    {
                        StdLog("CBlockState", "Calc crosschain dex: Get from state fail, from: %s, prev block: %s", destFrom.ToString().c_str(), hashPrevBlock.GetBhString().c_str());
                        return false;
                    }
                    if (stateFrom.GetBalance() < nTransferAmount)
                    {
                        StdLog("CBlockState", "Calc crosschain dex: From balance is not enough, balance: %s, transfer amount: %s, from: %s, prev block: %s",
                               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(nTransferAmount).c_str(),
                               destFrom.ToString().c_str(), hashPrevBlock.GetBhString().c_str());
                        return false;
                    }
                    if (!GetDestState(destTo, stateTo))
                    {
                        stateTo.SetNull();
                        stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType());
                    }
                    stateTo.IncBalance(nTransferAmount);
                    stateFrom.DecBalance(nTransferAmount);

                    SetDestState(destFrom, stateFrom);
                    SetDestState(destTo, stateTo);

                    mapBlockAddressContext[destTo] = ctxToAddress;

                    CContractTransfer ctrTransfer(CContractTransfer::CT_CROSSCHAIN_DEX, destFrom, destTo, nTransferAmount);

                    bool fAdd = true;
                    auto& ct = mapBlockContractTransfer[txidMint];
                    for (auto& d : ct)
                    {
                        if (d.nType == CContractTransfer::CT_CROSSCHAIN_DEX
                            && d.destFrom == destFrom
                            && d.destTo == destTo)
                        {
                            d.nAmount += nTransferAmount;
                            fAdd = false;
                            break;
                        }
                    }
                    if (fAdd)
                    {
                        ct.push_back(ctrTransfer);
                    }

                    fAdd = true;
                    for (auto& d : receiptMintTx.vTransfer)
                    {
                        if (d.nType == CContractTransfer::CT_CROSSCHAIN_DEX
                            && d.destFrom == destFrom
                            && d.destTo == destTo)
                        {
                            d.nAmount += nTransferAmount;
                            fAdd = false;
                            break;
                        }
                    }
                    if (fAdd)
                    {
                        receiptMintTx.vTransfer.push_back(ctrTransfer);
                    }
                }
                else if (nCoinTypeFrom == CCoinContext::CT_COIN_TYPE_CONTRACT)
                {
                    if (!ContractInTransfer(destFromContractAddress, destFrom, destTo, nTransferAmount))
                    {
                        StdLog("CBlockState", "Calc crosschain dex: Contract transfer fail, constract address: %s, from: %s, to: %s, amount: %s, prev block: %s",
                               destFromContractAddress.ToString().c_str(), destFrom.ToString().c_str(), destTo.ToString().c_str(),
                               CoinToTokenBigFloat(nTransferAmount).c_str(), hashPrevBlock.GetBhString().c_str());
                        return false;
                    }
                }
                else
                {
                    StdLog("CBlockState", "Calc crosschain dex: From coin type error, coin type: %d, from: %s, prev block: %s", nCoinTypeFrom, destFrom.ToString().c_str(), hashPrevBlock.GetBhString().c_str());
                    return false;
                }
                return true;
            };

            if (matchRecord.nSellOrderAtChainId == nChainId)
            {
                //if (!funcTrans(matchRecord.destSellOrder, matchRecord.destBuyOrder, matchRecord.nSellCompleteAmount, ctxCoinSell.nCoinType, ctxCoinSell.destContract))
                if (!funcTrans(FUNCTION_DEX_POOL_ADDRESS, matchRecord.destBuyOrder, matchRecord.nSellCompleteAmount, ctxCoinSell.nCoinType, ctxCoinSell.destContract))
                {
                    StdLog("CBlockState", "Calc crosschain dex: Transfer sell fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
                    continue;
                }
            }
            if (matchRecord.nBuyOrderAtChainId == nChainId)
            {
                // if (!funcTrans(matchRecord.destBuyOrder, matchRecord.destSellOrder, matchRecord.nBuyCompleteAmount, ctxCoinBuy.nCoinType, ctxCoinBuy.destContract))
                if (!funcTrans(FUNCTION_DEX_POOL_ADDRESS, matchRecord.destSellOrder, matchRecord.nBuyCompleteAmount, ctxCoinBuy.nCoinType, ctxCoinBuy.destContract))
                {
                    StdLog("CBlockState", "Calc crosschain dex: Transfer buy fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
                    continue;
                }
            }

            auto& dexOrderRecordSell = mapCompDexOrderRecord[CDexOrderHeader(matchRecord.nSellOrderAtChainId, matchRecord.destSellOrder, matchResult.strCoinSymbolSell, matchResult.strCoinSymbolBuy, matchRecord.nSellOrderNumber)];
            dexOrderRecordSell.push_back(CCompDexOrderRecord(matchRecord.destBuyOrder, matchRecord.nSellCompleteAmount, matchRecord.nCompletePrice));

            auto& dexOrderRecordBuy = mapCompDexOrderRecord[CDexOrderHeader(matchRecord.nBuyOrderAtChainId, matchRecord.destBuyOrder, matchResult.strCoinSymbolBuy, matchResult.strCoinSymbolSell, matchRecord.nBuyOrderNumber)];
            dexOrderRecordBuy.push_back(CCompDexOrderRecord(matchRecord.destSellOrder, matchRecord.nBuyCompleteAmount, matchRecord.nCompletePrice));

            mapCoinPairCompletePrice[hashCoinPair] = matchRecord.nCompletePrice;
        }
    }

    fMatchDex = !mapCompDexOrderRecord.empty();
    return true;
}

void CBlockState::CalcCrosschainConfirmRecvBlock()
{
    for (const auto& kv : mapBlockProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        if (!blockProve.IsCrossProveEmpty())
        {
            //StdDebug("TEST", "Calc Crosschain Confirm Recv Block: nPeerChainId: %d, hashPrevBlock: %s", nPeerChainId, hashPrevBlock.GetBhString().c_str());
            proveBlockCrosschain.mapCrossProve[nPeerChainId].first.AddCrossConfirmRecvBlock(blockProve.hashBlock);
        }
    }
}

void CBlockState::SetPeerPrevCrossLastBlock()
{
    for (auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        const CChainId nPeerChainId = kv.first;
        CBlockCrosschainProve& crossProve = kv.second.first;

        uint256 hashLastProveBlock;
        if (!dbBlockBase.GetPeerCrossLastBlock(hashPrevBlock, nPeerChainId, hashLastProveBlock))
        {
            hashLastProveBlock = hashFork;
        }
        crossProve.SetPrevProveBlock(hashLastProveBlock);
    }
}

uint256 CBlockState::CalcCrosschainMerkleRoot()
{
    if (proveBlockCrosschain.mapCrossProve.empty())
    {
        return 0;
    }

    std::vector<uint256> vMerkleTree;
    for (const auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        vMerkleTree.push_back(kv.second.first.GetHash());
    }

    std::size_t nLeafCount = vMerkleTree.size();
    uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);

    std::size_t nIndex = 0;
    for (auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        kv.second.second.clear();
        CMerkleTree::BuildMerkleProve(nIndex++, vMerkleTree, nLeafCount, kv.second.second);
    }
    return hashMerkleRoot;
}

bool CBlockState::AddContractState(const uint256& txid, const CTransaction& tx, const int nTxIndex, const uint64 nRunGasLimit, const uint256& nTvGasUsedIn, bool& fCallResult, CTransactionReceipt& receipt)
{
    ClearCacheContractData();

    fCallResult = true;
    if (isFunctionContractAddress(tx.GetToAddress()))
    {
        fCallResult = DoFunctionContractTx(txid, tx, nTxIndex, nRunGasLimit, nTvGasUsedIn, receipt);
        return true;
    }

    CDestination destContract;
    bytes btContractCode;
    bytes btRunParam;
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    CTxContractData txcd;
    bool fCall = false;
    bool fDestroy = false;
    if (!GetDestContractCode(tx, destContract, btContractCode, btRunParam, hashContractCreateCode, destCodeOwner, txcd, fCall, fDestroy))
    {
        StdLog("CBlockState", "Add contract state: Get dest contract code fail, txid: %s", txid.ToString().c_str());
        return false;
    }
    if (!tx.GetToAddress().IsNull() && fDestroy)
    {
        uint64 nGasLeft = 0;
        int nStatusCode = -2; //EVMC_REJECTED
        if (!DoRunResult(txid, tx, nTxIndex, destContract, hashContractCreateCode, nGasLeft, nTvGasUsedIn, nStatusCode, {}, receipt))
        {
            StdLog("CBlockState", "Add contract state: Do run result fail, txid: %s", txid.ToString().c_str());
            return false;
        }
        return true;
    }

    if (fCall)
    {
        // if (tx.IsEthTx() || tx.IsRewardTx() || tx.IsInternalTx())
        // {
        // StdLog("TEST", "GetGasLimit: %lu", tx.GetGasLimit().Get64());
        // StdLog("TEST", "RunGasLimit: %lu", nRunGasLimit);
        // StdLog("TEST", "GetTxBaseGas: %lu", tx.GetTxBaseGas().Get64());
        // StdLog("TEST", "GetTxDataSize: %lu", tx.GetTxDataSize());
        // StdLog("TEST", "GetTxDataGas: %lu", tx.GetTxDataGas().Get64());
        // StdLog("TEST", "destCodeOwner: %s", destCodeOwner.ToString().c_str());

        uint64 nSetRunGasLimit = nRunGasLimit;
        if (tx.IsRewardTx() || tx.IsInternalTx())
        {
            nSetRunGasLimit = REWARD_TX_GAS_LIMIT;
        }

        CContractHostDB dbHost(*this, destContract, destCodeOwner, txid, tx.GetNonce());
        CEvmExec vmExec(dbHost, hashFork, tx.GetChainId(), nAgreement);
        if (!(fCallResult = vmExec.evmExec(tx.GetFromAddress(), tx.GetToAddress(), destContract, destCodeOwner, nSetRunGasLimit,
                                           tx.GetGasPrice(), tx.GetAmount(), destMint, nBlockTimestamp,
                                           nBlockHeight, nSurplusBlockGasLimit, btContractCode, btRunParam, txcd)))
        {
            StdLog("CBlockState", "Add contract state: Evm exec fail, txid: %s", txid.ToString().c_str());
            ClearCacheContractData();
        }
        uint64 nSetGasLeft = vmExec.nGasLeft;
        uint256 nSetTvGasUsed = nTvGasUsedIn;
        if (tx.IsRewardTx() || tx.IsInternalTx())
        {
            nSetGasLeft = 0;
            nSetTvGasUsed = 0;
        }
        if (!DoRunResult(txid, tx, nTxIndex, destContract, hashContractCreateCode,
                         nSetGasLeft, nSetTvGasUsed, vmExec.nStatusCode, vmExec.vResult, receipt))
        {
            StdLog("CBlockState", "Add contract state: Do run result fail, txid: %s", txid.ToString().c_str());
            return false;
        }
        // }
        // else
        // {
        /////////////////////////////////
        // wasm vm
        // CContractHostDB dbHost(*this, destContract, destCodeOwner, txid, tx.GetNonce());
        // CContractRun vmExec(dbHost, hashFork);
        // if (!(fCallResult = vmExec.RunContract(tx.GetFromAddress(), tx.GetToAddress(), destContract, destCodeOwner, nRunGasLimit,
        //                                        tx.GetGasPrice(), tx.GetAmount(), destMint, nBlockTimestamp,
        //                                        nBlockHeight, nSurplusBlockGasLimit, btContractCode, btRunParam, txcd)))
        // {
        //     StdLog("CBlockState", "Add contract state: Run contract fail, txid: %s", txid.ToString().c_str());
        //     mapCacheContractData.clear();
        //     mapCacheAddressContext.clear();
        //     mapCacheContractCreateCodeContext.clear();
        //     mapCacheContractRunCodeContext.clear();
        //     vCacheContractTransfer.clear();
        //     mapCacheCodeDestGasUsed.clear();
        //     mapCacheModifyPledgeFinalHeight.clear();
        //     mapCacheFunctionAddress.clear();
        // }
        // if (!DoRunResult(txid, tx, nTxIndex, destContract, hashContractCreateCode,
        //                  vmExec.nGasLeft, nTvGasUsedIn, vmExec.nStatusCode, vmExec.vResult, receipt))
        // {
        //     StdLog("CBlockState", "Add contract state: Do run result fail, txid: %s", txid.ToString().c_str());
        //     return false;
        // }

        //---------------------------------------
        // uint64 nGasLeft = 0;
        // int nStatusCode = -2; //EVMC_REJECTED
        // if (!DoRunResult(txid, tx, nTxIndex, destContract, hashContractCreateCode, nGasLeft, nTvGasUsedIn, nStatusCode, {}, receipt))
        // {
        //     StdLog("CBlockState", "Add contract state: Do run result fail, txid: %s", txid.ToString().c_str());
        //     return false;
        // }
        // }
    }
    return true;
}

bool CBlockState::DoRunResult(const uint256& txid, const CTransaction& tx, const int nTxIndex, const CDestination& destContract,
                              const uint256& hashContractCreateCode, const uint64 nGasLeftIn, const uint256& nTvGasUsedIn,
                              const int nStatusCode, const bytes& vResult, CTransactionReceipt& receipt)
{
    for (const auto& vd : mapCacheContractData)
    {
        const CDestination& dest = vd.first;
        const CCacheContractData& conData = vd.second;

        auto& mapSetContractKv = mapContractKvState[dest];
        for (const auto& kv : conData.cacheContractKv)
        {
            mapSetContractKv[kv.first] = kv.second;
        }
        if (!conData.cacheDestState.IsNull())
        {
            SetDestState(dest, conData.cacheDestState);
        }

        for (auto& logs : conData.cacheContractLogs)
        {
            receipt.vLogs.push_back(logs);
        }
    }
    for (const auto& kv : mapCacheAddressContext)
    {
        mapBlockAddressContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractCreateCodeContext)
    {
        mapBlockContractCreateCodeContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractRunCodeContext)
    {
        mapBlockContractRunCodeContext[kv.first] = kv.second;
    }
    for (const auto& vd : vCacheContractTransfer)
    {
        mapBlockContractTransfer[txid].push_back(vd);
        receipt.vTransfer.push_back(vd);
    }
    uint256 nTotalCodeFeeUsed;
    if (!tx.GetFromAddress().IsNull() && tx.GetGasLimit() > 0)
    {
        for (const auto& kv : mapCacheCodeDestGasUsed)
        {
            if (kv.second > 0)
            {
                const CDestination& destCodeOwner = kv.first;
                uint256 nDestUsedGas(kv.second);
                uint256 nCodeFeeUsed = (nDestUsedGas * tx.GetGasPrice()) * CODE_REWARD_USED / CODE_REWARD_PER;

                CDestState stateCodeOwner;
                if (!GetDestState(destCodeOwner, stateCodeOwner))
                {
                    stateCodeOwner.SetNull();
                    stateCodeOwner.SetType(CDestination::PREFIX_PUBKEY); // WAIT_CHECK
                }
                stateCodeOwner.IncBalance(nCodeFeeUsed);
                SetDestState(destCodeOwner, stateCodeOwner);

                mapBlockCodeDestFeeUsed[txid][destCodeOwner] += nCodeFeeUsed;
                nTotalCodeFeeUsed += nCodeFeeUsed;

                if (mapBlockAddressContext.find(destCodeOwner) == mapBlockAddressContext.end())
                {
                    CAddressContext ctxCodeOwnerAddress;
                    if (!GetAddressContext(destCodeOwner, ctxCodeOwnerAddress))
                    {
                        mapBlockAddressContext[destCodeOwner] = CAddressContext(CPubkeyAddressContext());
                        StdLog("CBlockState", "Do run result: Get code owner address fail, destCodeOwner: %s", destCodeOwner.ToString().c_str());
                    }
                    else
                    {
                        mapBlockAddressContext[destCodeOwner] = ctxCodeOwnerAddress;
                    }
                }
            }
        }
    }
    for (auto& kv : mapCacheModifyPledgeFinalHeight)
    {
        mapBlockModifyPledgeFinalHeight[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheFunctionAddress)
    {
        mapBlockFunctionAddress[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheSymbolCoin)
    {
        mapBlockSymbolCoin[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheDexOrder)
    {
        mapBlockDexOrder[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheCrossProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockCrosschainProve& cacheCrossProve = kv.second;
        CBlockCrosschainProve& crossProve = proveBlockCrosschain.mapCrossProve[nPeerChainId].first;

        for (const CBlockCoinTransferProve& transferProve : cacheCrossProve.GetCoinTransferProve())
        {
            crossProve.AddCoinTransferProve(transferProve.destTransfer, transferProve.strCoinSymbol, transferProve.nOriChainId, transferProve.nDestChainId, transferProve.nTransferAmount);
        }
        for (const auto& kv : cacheCrossProve.GetDexOrderProve())
        {
            crossProve.AddDexOrderProve(kv.second);
        }
    }

    ClearCacheContractData();

    uint256 nTxGasUsed;
    if (!tx.GetFromAddress().IsNull() && tx.GetGasLimit() > 0 && tx.GetGasLimit() > nGasLeftIn)
    {
        nTxGasUsed = tx.GetGasLimit() - nGasLeftIn;
    }
    if (nTxGasUsed > 0)
    {
        mapBlockTxFeeUsed[txid] = nTxGasUsed * tx.GetGasPrice();
    }

    receipt.nReceiptType = CTransactionReceipt::RECEIPT_TYPE_CONTRACT;

    receipt.nTxIndex = nTxIndex;
    receipt.txid = txid;
    receipt.nBlockNumber = nBlockNumber;
    receipt.from = tx.GetFromAddress();
    receipt.to = tx.GetToAddress();
    receipt.nTxGasUsed = nTxGasUsed;
    receipt.nTvGasUsed = nTvGasUsedIn;
    receipt.destContract = destContract;
    receipt.hashContractCode = hashContractCreateCode;
    receipt.nContractStatus = nStatusCode;
    receipt.nContractGasLeft = nGasLeftIn;
    if (!tx.GetToAddress().IsNull())
    {
        receipt.btContractResult = vResult;
    }

    receipt.nEffectiveGasPrice = tx.GetGasPrice();

    // receipt.CalcLogsBloom();
    // nBlockBloom |= receipt.nLogsBloom;

    // mtbase::CBufStream ss;
    // ss << receipt;

    // vReceiptHash.push_back(metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize()));
    // vBlockTxReceipts.push_back(receipt);

    nSurplusBlockGasLimit -= nTxGasUsed.Get64();
    if (!tx.GetFromAddress().IsNull() && tx.GetGasLimit() > 0 && nGasLeftIn != 0)
    {
        CDestState stateDest;
        if (!GetDestState(tx.GetFromAddress(), stateDest))
        {
            StdLog("CBlockState", "Set run result: Get dest state fail, txid: %s", txid.ToString().c_str());
            return false;
        }
        stateDest.IncBalance(tx.GetGasPrice() * nGasLeftIn);
        SetDestState(tx.GetFromAddress(), stateDest);

        nBlockFeeLeft += (tx.GetGasPrice() * nGasLeftIn);
    }
    nBlockFeeLeft += nTotalCodeFeeUsed;
    return true;
}

void CBlockState::ClearCacheContractData()
{
    mapCacheContractData.clear();
    mapCacheAddressContext.clear();
    mapCacheContractCreateCodeContext.clear();
    mapCacheContractRunCodeContext.clear();
    vCacheContractTransfer.clear();
    mapCacheCodeDestGasUsed.clear();
    mapCacheModifyPledgeFinalHeight.clear();
    mapCacheFunctionAddress.clear();
    mapCacheSymbolCoin.clear();
    mapCacheDexOrder.clear();
    mapCacheCrossProve.clear();
}

bool CBlockState::CallContractInFunction(const CDestination& destFrom, const CDestination& destContract, const bytes& btRunParam)
{
    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractCode;
    CTxContractData txcd;
    bool fDestroy = false;

    if (!GetContractRunCode(destContract, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractCode, fDestroy))
    {
        StdLog("CBlockState", "Call contract function: Get contract run code fail, contract address: %s", destContract.ToString().c_str());
        return false;
    }
    if (fDestroy)
    {
        StdLog("CBlockState", "Call contract function: Contract has been destroyed, contract address: %s", destContract.ToString().c_str());
        return false;
    }

    ClearCacheContractData();

    uint256 txid;
    const CDestination& destTo = destContract;
    uint64 nSetRunGasLimit = REWARD_TX_GAS_LIMIT;
    uint64 nTxNonce = 0;
    uint256 nAgreement = 0;
    uint256 nTxGasPrice = 0;
    uint256 nTxAmount = 0;

    CContractHostDB dbHost(*this, destContract, destCodeOwner, txid, nTxNonce);
    CEvmExec vmExec(dbHost, hashFork, nLocalChainId, nAgreement);
    if (!vmExec.evmExec(destFrom, destTo, destContract, destCodeOwner, nSetRunGasLimit,
                        nTxGasPrice, nTxAmount, destMint, nBlockTimestamp,
                        nBlockHeight, nSurplusBlockGasLimit, btContractCode, btRunParam, txcd))
    {
        StdLog("CBlockState", "Call contract function: Evm exec fail, from address: %s, contract address: %s", destFrom.ToString().c_str(), destContract.ToString().c_str());
        ClearCacheContractData();
        return false;
    }

    //--------------------------------------------------------------
    for (const auto& vd : mapCacheContractData)
    {
        const CDestination& dest = vd.first;
        const CCacheContractData& conData = vd.second;

        auto& mapSetContractKv = mapContractKvState[dest];
        for (const auto& kv : conData.cacheContractKv)
        {
            mapSetContractKv[kv.first] = kv.second;
        }
        if (!conData.cacheDestState.IsNull())
        {
            SetDestState(dest, conData.cacheDestState);
        }

        // for (auto& logs : conData.cacheContractLogs)
        // {
        //     receipt.vLogs.push_back(logs);
        // }
    }
    for (const auto& kv : mapCacheAddressContext)
    {
        mapBlockAddressContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractCreateCodeContext)
    {
        mapBlockContractCreateCodeContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractRunCodeContext)
    {
        mapBlockContractRunCodeContext[kv.first] = kv.second;
    }
    for (const auto& vd : vCacheContractTransfer)
    {
        mapBlockContractTransfer[txid].push_back(vd);
        //receipt.vTransfer.push_back(vd);
    }
    for (auto& kv : mapCacheModifyPledgeFinalHeight)
    {
        mapBlockModifyPledgeFinalHeight[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheFunctionAddress)
    {
        mapBlockFunctionAddress[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheSymbolCoin)
    {
        mapBlockSymbolCoin[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheDexOrder)
    {
        mapBlockDexOrder[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheCrossProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockCrosschainProve& cacheCrossProve = kv.second;
        CBlockCrosschainProve& crossProve = proveBlockCrosschain.mapCrossProve[nPeerChainId].first;

        for (const CBlockCoinTransferProve& transferProve : cacheCrossProve.GetCoinTransferProve())
        {
            crossProve.AddCoinTransferProve(transferProve.destTransfer, transferProve.strCoinSymbol, transferProve.nOriChainId, transferProve.nDestChainId, transferProve.nTransferAmount);
        }
        for (const auto& kv : cacheCrossProve.GetDexOrderProve())
        {
            crossProve.AddDexOrderProve(kv.second);
        }
    }

    ClearCacheContractData();
    return true;
}

bool CBlockState::ContractInTransfer(const CDestination& destContract, const CDestination& destFrom, const CDestination& destTo, const uint256& nTransferAmount)
{
    //function transfer(address to, uint tokens) external returns (bool);

    std::vector<bytes> vParamList;
    vParamList.push_back(destTo.ToHash().GetBytes());
    vParamList.push_back(nTransferAmount.ToBigEndian());

    bytes btRunParam = MakeEthTxCallData("transfer(address,uint256)", vParamList);

    if (!CallContractInFunction(destFrom, destContract, btRunParam))
    {
        return false;
    }
    return true;
}

bool CBlockState::GetDestLockedAmount(const CDestination& dest, uint256& nLockedAmount)
{
    CAddressContext ctxAddress;
    if (!GetAddressContext(dest, ctxAddress))
    {
        StdLog("CBlockState", "Get loacked amount: Get address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    CDestState stateDest;
    if (!GetDestState(dest, stateDest))
    {
        StdLog("CBlockState", "Get loacked amount: Get address state fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (!dbBlockBase.GetAddressLockedAmount(hashFork, hashPrevBlock, dest, ctxAddress, stateDest.GetBalance(), nLockedAmount))
    {
        StdLog("CBlockState", "Get loacked amount: Get address locked amount fail, dest: %s", dest.ToString().c_str());
        return false;
    }

    auto nt = mapBlockRewardLocked.find(dest);
    if (nt != mapBlockRewardLocked.end())
    {
        nLockedAmount += nt->second;
    }
    return true;
}

bool CBlockState::VerifyFunctionAddressRepeat(const CDestination& destNewFunction)
{
    for (auto& kv : mapCacheFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            return false;
        }
    }
    for (auto& kv : mapBlockFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            return false;
        }
    }
    return true;
}

bool CBlockState::VerifyFunctionAddressDisable(const uint32 nFuncId)
{
    auto it = mapCacheFunctionAddress.find(nFuncId);
    if (it != mapCacheFunctionAddress.end())
    {
        if (it->second.IsDisableModify())
        {
            return false;
        }
    }
    else
    {
        auto mt = mapBlockFunctionAddress.find(nFuncId);
        if (mt != mapBlockFunctionAddress.end())
        {
            if (mt->second.IsDisableModify())
            {
                return false;
            }
        }
    }
    return true;
}

bool CBlockState::GetContractStringParam(const uint8* pParamBeginPos, const std::size_t nParamSize, const uint8* pCurrParamPos, const std::size_t nSurplusParamLen, std::string& strParamOut)
{
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Get contract string param: Param var pos error, surplus param len: %lu, param size: %lu", nSurplusParamLen, nParamSize);
        return false;
    }
    uint256 tempData;
    tempData.FromBigEndian(pCurrParamPos, 32);
    std::size_t nVarPos = tempData.Get64();

    if (nParamSize < nVarPos)
    {
        StdLog("CBlockState", "Get contract string param: Param var pos error, var pos: %lu, param size: %lu", nVarPos, nParamSize);
        return false;
    }
    const uint8* p = pParamBeginPos + nVarPos;
    std::size_t nSurplusSize = nParamSize - nVarPos;

    if (nSurplusSize < 32)
    {
        StdLog("CBlockState", "Get contract string param: Surplus size not enough1, surplus size: %lu", nSurplusSize);
        return false;
    }
    tempData.FromBigEndian(p, 32);
    uint64 nStringSize = tempData.Get64();
    p += 32;
    nSurplusSize -= 32;

    if (nStringSize == 0)
    {
        StdLog("CBlockState", "Get contract string param: String size is 0, surplus size: %lu", nSurplusSize);
        return false;
    }
    const std::size_t nSectByteCount = (nStringSize / 32 + ((nStringSize % 32) == 0 ? 0 : 1)) * 32;
    if (nSurplusSize < nSectByteCount)
    {
        StdLog("CBlockState", "Get contract string param: Surplus size not enough2, surplus size: %lu, sect size: %lu, string size: %lu", nSurplusSize, nSectByteCount, nStringSize);
        return false;
    }
    strParamOut.assign(p, p + nStringSize);
    return true;
}

uint64 CBlockState::GetMaxDexCoinOrderNumber(const CDestination& destFrom, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer)
{
    uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    uint64 nMaxNumber = 0;
    for (auto& kv : mapCacheDexOrder)
    {
        if (kv.first.GetOrderAddress() == destFrom && kv.first.GetCoinPairHash() == hashCoinPair && kv.first.GetOwnerCoinFlag() == nOwnerCoinFlag)
        {
            if (kv.first.GetOrderNumber() > nMaxNumber)
            {
                nMaxNumber = kv.first.GetOrderNumber();
            }
        }
    }
    if (nMaxNumber == 0)
    {
        for (auto& kv : mapBlockDexOrder)
        {
            if (kv.first.GetOrderAddress() == destFrom && kv.first.GetCoinPairHash() == hashCoinPair && kv.first.GetOwnerCoinFlag() == nOwnerCoinFlag)
            {
                if (kv.first.GetOrderNumber() > nMaxNumber)
                {
                    nMaxNumber = kv.first.GetOrderNumber();
                }
            }
        }
    }
    if (nMaxNumber == 0)
    {
        if (!dbBlockBase.GetDexOrderMaxNumber(hashPrevBlock, destFrom, strCoinSymbolOwner, strCoinSymbolPeer, nMaxNumber))
        {
            nMaxNumber = 0;
        }
    }
    return nMaxNumber;
}

//---------------------------------------------------------------------------------------------------------
bool CBlockState::DoFunctionContractTx(const uint256& txid, const CTransaction& tx, const int nTxIndex, const uint64 nRunGasLimit, const uint256& nTvGasUsedIn, CTransactionReceipt& receipt)
{
    StdDebug("CBlockState", "Do function contract tx, txid: %s", txid.ToString().c_str());

    bytes btTxData;
    if (tx.IsEthTx())
    {
        if (!tx.GetTxData(CTransaction::DF_ETH_TX_DATA, btTxData) || btTxData.size() < 4)
        {
            StdLog("CBlockState", "Do function contract tx: Eth tx param error, txid: %s", txid.GetHex().c_str());
            return false;
        }
    }
    else
    {
        if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btTxData) || btTxData.size() < 4)
        {
            StdLog("CBlockState", "Do function contract tx: tx contract param error, txid: %s", txid.GetHex().c_str());
            return false;
        }
    }
    bytes btTxParam;
    if (btTxData.size() > 4)
    {
        btTxParam.assign(btTxData.begin() + 4, btTxData.end());
    }
    bytes btFuncSign(btTxData.begin(), btTxData.begin() + 4);

    CDestination destFrom = tx.GetFromAddress();
    CDestination destTo = tx.GetToAddress();

    uint64 nGasLeft = nRunGasLimit;
    CTransactionLogs logs;
    bytes btResult;

    bool fRet = DoFuncContractCall(destFrom, destTo, btFuncSign, btTxParam, nRunGasLimit, nGasLeft, logs, btResult);

    for (const auto& vd : mapCacheContractData)
    {
        const CDestination& dest = vd.first;
        auto& mapSetContractKv = mapContractKvState[dest];
        for (const auto& kv : vd.second.cacheContractKv)
        {
            mapSetContractKv[kv.first] = kv.second;
        }
        if (!vd.second.cacheDestState.IsNull())
        {
            SetDestState(dest, vd.second.cacheDestState);
        }
    }
    for (const auto& kv : mapCacheAddressContext)
    {
        mapBlockAddressContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractCreateCodeContext)
    {
        mapBlockContractCreateCodeContext[kv.first] = kv.second;
    }
    for (const auto& kv : mapCacheContractRunCodeContext)
    {
        mapBlockContractRunCodeContext[kv.first] = kv.second;
    }
    for (const auto& vd : vCacheContractTransfer)
    {
        mapBlockContractTransfer[txid].push_back(vd);
        receipt.vTransfer.push_back(vd);
    }
    for (const auto& kv : mapCacheModifyPledgeFinalHeight)
    {
        mapBlockModifyPledgeFinalHeight[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheFunctionAddress)
    {
        mapBlockFunctionAddress[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheSymbolCoin)
    {
        mapBlockSymbolCoin[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheDexOrder)
    {
        mapBlockDexOrder[kv.first] = kv.second;
    }
    for (auto& kv : mapCacheCrossProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockCrosschainProve& cacheCrossProve = kv.second;
        CBlockCrosschainProve& crossProve = proveBlockCrosschain.mapCrossProve[nPeerChainId].first;

        for (const CBlockCoinTransferProve& transferProve : cacheCrossProve.GetCoinTransferProve())
        {
            crossProve.AddCoinTransferProve(transferProve.destTransfer, transferProve.strCoinSymbol, transferProve.nOriChainId, transferProve.nDestChainId, transferProve.nTransferAmount);
        }
        for (const auto& kv : cacheCrossProve.GetDexOrderProve())
        {
            crossProve.AddDexOrderProve(kv.second);
        }
    }

    ClearCacheContractData();

    uint256 nTxGasUsed;
    if (tx.GetGasLimit() > nGasLeft)
    {
        nTxGasUsed = tx.GetGasLimit() - nGasLeft;
    }
    if (nTxGasUsed > 0)
    {
        mapBlockTxFeeUsed[txid] = nTxGasUsed * tx.GetGasPrice();
    }

    {
        CDestination destContract(FUNCTION_CONTRACT_ADDRESS);
        uint256 hashContractCreateCode = crypto::CryptoKeccakHash(getFunctionContractCreateCode());

        receipt.nReceiptType = CTransactionReceipt::RECEIPT_TYPE_CONTRACT;

        receipt.nTxIndex = nTxIndex;
        receipt.txid = txid;
        receipt.nBlockNumber = nBlockNumber;
        receipt.from = destFrom;
        receipt.to = destTo;
        receipt.nTxGasUsed = nTxGasUsed;
        receipt.nTvGasUsed = nTvGasUsedIn;
        receipt.destContract = destContract;
        receipt.hashContractCode = hashContractCreateCode;
        receipt.nContractStatus = (fRet ? 0 : 1);
        receipt.nContractGasLeft = nGasLeft;
        if (btResult.empty())
        {
            receipt.btContractResult = uint256(fRet ? 1 : 0).ToBigEndian();
        }
        else
        {
            receipt.btContractResult = btResult;
        }
        receipt.vLogs.push_back(logs);
        receipt.nEffectiveGasPrice = tx.GetGasPrice();

        // receipt.CalcLogsBloom();
        // nBlockBloom |= receipt.nLogsBloom;

        // mtbase::CBufStream ss;
        // ss << receipt;

        // vReceiptHash.push_back(metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize()));
        // vBlockTxReceipts.push_back(receipt);
    }

    nSurplusBlockGasLimit -= nTxGasUsed.Get64();
    if (!tx.GetFromAddress().IsNull() && nGasLeft != 0)
    {
        CDestState stateDest;
        if (!GetDestState(tx.GetFromAddress(), stateDest))
        {
            StdLog("CBlockState", "Do function contract tx: Get dest state fail, txid: %s", txid.ToString().c_str());
            return false;
        }
        stateDest.IncBalance(tx.GetGasPrice() * nGasLeft);
        SetDestState(tx.GetFromAddress(), stateDest);

        nBlockFeeLeft += (tx.GetGasPrice() * nGasLeft);
    }
    return fRet;
}

bool CBlockState::DoFuncContractCall(const CDestination& destFrom, const CDestination& destTo, const bytes& btFuncSign, const bytes& btTxParam,
                                     const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    logs.data.clear();
    logs.data.resize(28);
    logs.data.insert(logs.data.end(), btFuncSign.begin(), btFuncSign.end());
    logs.address = destTo;

    if (nGasLeft < FUNCTION_TX_GAS_BASE)
    {
        StdLog("CBlockState", "Do func contrac call: Gas not enough, need gas %d, gas left: %ld", FUNCTION_TX_GAS_BASE, nGasLeft);
        nGasLeft = 0;
        return false;
    }
    nGasLeft -= FUNCTION_TX_GAS_BASE;

    bool fRet = false;

    // delegate vote
    if (btFuncSign == CryptoKeccakSign("delegateVote(address,uint16,uint256)"))
    {
        fRet = DoFuncTxDelegateVote(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("delegateRedeem(address,uint16,uint256)"))
    {
        fRet = DoFuncTxDelegateRedeem(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getDelegateVotes(address,address,uint16)"))
    {
        fRet = DoFuncTxGetDelegateVotes(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // user vote
    else if (btFuncSign == CryptoKeccakSign("userVote(address,uint8,uint256)"))
    {
        fRet = DoFuncTxUserVote(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("userRedeem(address,uint8,uint256)"))
    {
        fRet = DoFuncTxUserRedeem(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getUserVotes(address,address,uint8)"))
    {
        fRet = DoFuncTxGetUserVotes(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // pledge vote
    else if (btFuncSign == CryptoKeccakSign("pledgeVote(address,uint32,uint32,uint32,uint256)"))
    {
        fRet = DoFuncTxPledgeVote(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("pledgeReqRedeem(address,uint32,uint32,uint32)"))
    {
        fRet = DoFuncTxPledgeReqRedeem(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getPledgeVotes(address,address,uint32,uint32,uint32)"))
    {
        fRet = DoFuncTxGetPledgeVotes(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getPledgeUnlockHeight(address,address,uint32,uint32,uint32)"))
    {
        fRet = DoFuncTxGetPledgeUnlockHeight(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // query delegate
    else if (btFuncSign == CryptoKeccakSign("getPageSize()"))
    {
        fRet = DoFuncTxGetPageSize(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getDelegateCount()"))
    {
        fRet = DoFuncTxGetDelegateCount(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getDelegateAddress(uint32)"))
    {
        fRet = DoFuncTxGetDelegateAddress(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getDelegateTotalVotes(address)"))
    {
        fRet = DoFuncTxGetDelegateTotalVotes(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getVoteUnlockHeight(address)"))
    {
        fRet = DoFuncTxGetVoteUnlockHeight(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // function address
    else if (btFuncSign == CryptoKeccakSign("setFunctionAddress(uint32,address,bool)"))
    {
        fRet = DoFuncTxSetFunctionAddress(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getFunctionAddress(uint32)"))
    {
        fRet = DoFuncTxGetFunctionAddress(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // coin
    else if (btFuncSign == CryptoKeccakSign("addUserCoin(string,uint32)"))
    {
        fRet = DoFuncTxAddUserCoin(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("addContractCoin(string,uint32,address)"))
    {
        fRet = DoFuncTxAddContractCoin(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getCoin(string)"))
    {
        fRet = DoFuncTxGetCoin(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // dex order
    else if (btFuncSign == CryptoKeccakSign("addDexOrder(string,string,uint64,uint256,uint256)"))
    {
        fRet = DoFuncTxAddDexOrder(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getDexOrder(string,string,uint64)"))
    {
        fRet = DoFuncTxGetDexOrder(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getMaxDexOrderNumber(string,string)"))
    {
        fRet = DoFuncTxGetMaxDexOrderNumber(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    // crosschain transfer
    else if (btFuncSign == CryptoKeccakSign("transferCoin(uint32,uint256)"))
    {
        fRet = DoFuncTxTransferCoin(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else if (btFuncSign == CryptoKeccakSign("getCrosschainTransferAmount()"))
    {
        fRet = DoFuncTxGetCrosschainTransferAmount(destFrom, destTo, btTxParam, nGasLimit, nGasLeft, logs, btResult);
    }
    else
    {
        StdLog("CBlockState", "Do func contrac call: Function sign error, Func sign: %s", ToHexString(btFuncSign).c_str());
    }
    return fRet;
}

//-----------------------------------------
bool CBlockState::DoFuncTxDelegateVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                       const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx delegate vote: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx delegate vote: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destMint;
    uint16 nRewardRatio;
    uint256 nAmount;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destMint.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32, 32);
    nRewardRatio = (uint16)(tempData.Get32());

    nAmount.FromBigEndian(btTxParam.data() + 32 * 2, 32);

    StdDebug("CBlockState", "Do func tx delegate vote: mint address: %s, reward ratio: %d, vote amount: %s, from: %s",
             destMint.ToString().c_str(), nRewardRatio, CoinToTokenBigFloat(nAmount).c_str(), destFrom.ToString().c_str());

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx delegate vote: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }
    if (!ctxFromAddress.IsPubkey() && !ctxFromAddress.IsContract())
    {
        StdLog("CBlockState", "Do func tx delegate vote: From address not pubkey or contract address, from: %s", destFrom.ToString().c_str());
        return false;
    }

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateDelegate(destMint, destFrom, nRewardRatio));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx delegate vote: Create template fail, destMint: %s, from: %s, nRewardRatio: %d",
               destMint.ToString().c_str(), destFrom.ToString().c_str(), nRewardRatio);
        return false;
    }
    CDestination destDelegate(ptr->GetTemplateId());
    CAddressContext ctxToAddress(CTemplateAddressContext({}, {}, TEMPLATE_DELEGATE, ptr->Export()));

    if (!ContractTransfer(destFrom, destDelegate, nAmount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_VOTE))
    {
        StdLog("CBlockState", "Do func tx delegate vote: Contract transfer fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destDelegate.ToHash());
    logs.topics.push_back(uint256(nAmount.ToBigEndian()));

    btResult = uint256(1).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxDelegateRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                         const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destMint;
    uint16 nRewardRatio;
    uint256 nAmount;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destMint.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32, 32);
    nRewardRatio = (uint16)(tempData.Get32());

    nAmount.FromBigEndian(btTxParam.data() + 32 * 2, 32);

    StdDebug("CBlockState", "Do func tx delegate redeem: mint address: %s, reward ratio: %d, vote amount: %s, from: %s",
             destMint.ToString().c_str(), nRewardRatio, CoinToTokenBigFloat(nAmount).c_str(), destFrom.ToString().c_str());

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateDelegate(destMint, destFrom, nRewardRatio));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Create template fail, destMint: %s, from: %s, nRewardRatio: %d",
               destMint.ToString().c_str(), destFrom.ToString().c_str(), nRewardRatio);
        return false;
    }
    CDestination destDelegate(ptr->GetTemplateId());

    // verify redeem
    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destDelegate, ctxVote))
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Retrieve dest vote context fail, delegate address: %s", destDelegate.ToString().c_str());
        return false;
    }
    if (ctxVote.nFinalHeight == 0 || (CBlock::GetBlockHeightByHash(hashPrevBlock) + 1) < ctxVote.nFinalHeight)
    {
        StdDebug("CBlockState", "Do func tx delegate redeem: Vote locked, final height: %d, delegate address: %s",
                 ctxVote.nFinalHeight, destDelegate.ToString().c_str());
        return false;
    }

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    // transfer
    if (!ContractTransfer(destDelegate, destFrom, nAmount, nGasLimit, nGasLeft, ctxFromAddress, CContractTransfer::CT_REDEEM))
    {
        StdLog("CBlockState", "Do func tx delegate redeem: Contract transfer fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    logs.topics.push_back(destDelegate.ToHash());
    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(uint256(nAmount.ToBigEndian()));

    btResult = uint256(1).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetDelegateVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                           const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get delegate votes: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx get delegate votes: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destMint;
    CDestination destOwner;
    uint16 nRewardRatio;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destMint.SetHash(tempData);

    memcpy(tempData.begin(), btTxParam.data() + 32, 32);
    destOwner.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32 * 2, 32);
    nRewardRatio = (uint16)(tempData.Get32());

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateDelegate(destMint, destOwner, nRewardRatio));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx get delegate votes: Create template fail, destMint: %s, destOwner: %s, nRewardRatio: %d",
               destMint.ToString().c_str(), destOwner.ToString().c_str(), nRewardRatio);
        return false;
    }
    CDestination destDelegate(ptr->GetTemplateId());

    CDestState stateDest;
    if (!GetDestState(destDelegate, stateDest))
    {
        btResult = uint256().ToBigEndian();
    }
    else
    {
        btResult = stateDest.GetBalance().ToBigEndian();
    }
    return true;
}

bool CBlockState::DoFuncTxUserVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                   const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (!fEnableStakeVote)
    {
        StdLog("CBlockState", "Do func tx user vote: Disable vote, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx user vote: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx user vote: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    uint8 nRewardMode;
    uint256 nAmount;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destDelegate.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32, 32);
    nRewardMode = (uint8)(tempData.Get32());

    nAmount.FromBigEndian(btTxParam.data() + 32 * 2, 32);

    StdDebug("CBlockState", "Do func tx user vote: delegate address: %s, reward mode: %d, vote amount: %s, from: %s",
             destDelegate.ToString().c_str(), nRewardMode, CoinToTokenBigFloat(nAmount).c_str(), destFrom.ToString().c_str());

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx user vote: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }
    if (!ctxFromAddress.IsPubkey() && !ctxFromAddress.IsContract())
    {
        StdLog("CBlockState", "Do func tx user vote: From address not pubkey or contract address, from: %s", destFrom.ToString().c_str());
        return false;
    }

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateVote(destDelegate, destFrom, nRewardMode));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx user vote: Create template fail, destDelegate: %s, from: %s, nRewardMode: %d",
               destDelegate.ToString().c_str(), destFrom.ToString().c_str(), nRewardMode);
        return false;
    }
    CDestination destVote(ptr->GetTemplateId());
    CAddressContext ctxToAddress(CTemplateAddressContext({}, {}, TEMPLATE_VOTE, ptr->Export()));

    if (!ContractTransfer(destFrom, destVote, nAmount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_VOTE))
    {
        StdLog("CBlockState", "Do func tx user vote: Contract transfer fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destVote.ToHash());
    logs.topics.push_back(uint256(nAmount.ToBigEndian()));
    logs.topics.push_back(destDelegate.ToHash());

    btResult = uint256(1).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxUserRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                     const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx user redeem: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx user redeem: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    uint8 nRewardMode;
    uint256 nAmount;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destDelegate.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32, 32);
    nRewardMode = (uint8)(tempData.Get32());

    nAmount.FromBigEndian(btTxParam.data() + 32 * 2, 32);

    StdDebug("CBlockState", "Do func tx user redeem: delegate address: %s, reward mode: %d, redeem amount: %s, from: %s",
             destDelegate.ToString().c_str(), nRewardMode, CoinToTokenBigFloat(nAmount).c_str(), destFrom.ToString().c_str());

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateVote(destDelegate, destFrom, nRewardMode));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx user redeem: Create template fail, destDelegate: %s, from: %s, nRewardMode: %d",
               destDelegate.ToString().c_str(), destFrom.ToString().c_str(), nRewardMode);
        return false;
    }
    CDestination destVote(ptr->GetTemplateId());

    // verify redeem
    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destVote, ctxVote))
    {
        StdLog("CBlockState", "Do func tx user redeem: Retrieve dest vote context fail, vote dest: %s", destVote.ToString().c_str());
        return false;
    }
    if (ctxVote.nFinalHeight == 0 || (CBlock::GetBlockHeightByHash(hashPrevBlock) + 1) < ctxVote.nFinalHeight)
    {
        StdDebug("CBlockState", "Do func tx user redeem: Vote locked, final height: %d, vote dest: %s",
                 ctxVote.nFinalHeight, destVote.ToString().c_str());
        return false;
    }

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx user redeem: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    // transfer
    if (!ContractTransfer(destVote, destFrom, nAmount, nGasLimit, nGasLeft, ctxFromAddress, CContractTransfer::CT_REDEEM))
    {
        StdLog("CBlockState", "Do func tx user redeem: Contract transfer fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    logs.topics.push_back(destVote.ToHash());
    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(uint256(nAmount.ToBigEndian()));
    logs.topics.push_back(destDelegate.ToHash());

    btResult = uint256(1).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetUserVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                       const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get user votes: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx get user votes: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    CDestination destOwner;
    uint8 nRewardMode;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destDelegate.SetHash(tempData);

    memcpy(tempData.begin(), btTxParam.data() + 32, 32);
    destOwner.SetHash(tempData);

    tempData.FromBigEndian(btTxParam.data() + 32 * 2, 32);
    nRewardMode = (uint16)(tempData.Get32());

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateVote(destDelegate, destOwner, nRewardMode));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx get user votes: Create template fail, destDelegate: %s, destOwner: %s, nRewardMode: %d",
               destDelegate.ToString().c_str(), destOwner.ToString().c_str(), nRewardMode);
        return false;
    }
    CDestination destVote(ptr->GetTemplateId());

    CDestState stateDest;
    if (!GetDestState(destVote, stateDest))
    {
        btResult = uint256().ToBigEndian();
    }
    else
    {
        btResult = stateDest.GetBalance().ToBigEndian();
    }
    return true;
}

bool CBlockState::DoFuncTxPledgeVote(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (!fEnableStakePledge)
    {
        StdLog("CBlockState", "Do func tx pledge vote: Disable pledge, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx pledge vote: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 5)
    {
        StdLog("CBlockState", "Do func tx pledge vote: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    uint32 nPledgeType;
    uint32 nCycles;
    uint32 nNonce;
    uint256 nAmount;

    const uint8* p = btTxParam.data();

    memcpy(tempData.begin(), p, 32);
    destDelegate.SetHash(tempData);
    p += 32;

    tempData.FromBigEndian(p, 32);
    nPledgeType = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nCycles = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nNonce = tempData.Get32();
    p += 32;

    nAmount.FromBigEndian(p, 32);

    StdDebug("CBlockState", "Do func tx pledge vote: delegate address: %s, pledge type: %d, cycles: %d, nonce: %d, vote amount: %s, from: %s",
             destDelegate.ToString().c_str(), nPledgeType, nCycles, nNonce, CoinToTokenBigFloat(nAmount).c_str(), destFrom.ToString().c_str());

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx pledge vote: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }
    if (!ctxFromAddress.IsPubkey() && !ctxFromAddress.IsContract())
    {
        StdLog("CBlockState", "Do func tx pledge vote: From address not pubkey or contract address, from: %s", destFrom.ToString().c_str());
        return false;
    }

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplatePledge(destDelegate, destFrom, nPledgeType, nCycles, nNonce));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx pledge vote: Create template fail, destDelegate: %s, from: %s, pledge type: %d, cycles: %d, nonce: %d",
               destDelegate.ToString().c_str(), destFrom.ToString().c_str(), nPledgeType, nCycles, nNonce);
        return false;
    }
    auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
    uint32 nFinalHeight;
    if (!objPledge->GetPledgeFinalHeight(nBlockHeight, nFinalHeight))
    {
        StdLog("CBlockState", "Do func tx pledge vote: Pledge parameter error, delegate: %s, from: %s, pledge type: %d, cycles: %d, nonce: %d, block height: %d",
               destDelegate.ToString().c_str(), destFrom.ToString().c_str(), nPledgeType, nCycles, nNonce, nBlockHeight);
        return false;
    }

    CDestination destPledge(ptr->GetTemplateId());
    CAddressContext ctxToAddress(CTemplateAddressContext({}, {}, TEMPLATE_PLEDGE, ptr->Export()));

    if (!ContractTransfer(destFrom, destPledge, nAmount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_VOTE))
    {
        StdLog("CBlockState", "Do func tx pledge vote: Contract transfer fail, from: %s", destFrom.ToString().c_str());
        return false;
    }

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destPledge.ToHash());
    logs.topics.push_back(uint256(nAmount.ToBigEndian()));
    logs.topics.push_back(destDelegate.ToHash());

    btResult = uint256(1).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxPledgeReqRedeem(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (!fEnableStakePledge)
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Disable pledge, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 4)
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    uint32 nPledgeType;
    uint32 nCycles;
    uint32 nNonce;

    const uint8* p = btTxParam.data();

    memcpy(tempData.begin(), p, 32);
    destDelegate.SetHash(tempData);
    p += 32;

    tempData.FromBigEndian(p, 32);
    nPledgeType = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nCycles = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nNonce = tempData.Get32();
    p += 32;

    StdDebug("CBlockState", "Do func tx pledge req redeem: delegate address: %s, pledge type: %d, cycles: %d, nonce: %d, from: %s",
             destDelegate.ToString().c_str(), nPledgeType, nCycles, nNonce, destFrom.ToString().c_str());

    CAddressContext ctxFromAddress;
    if (!GetAddressContext(destFrom, ctxFromAddress))
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Get from address context fail, from: %s", destFrom.ToString().c_str());
        return false;
    }
    if (!ctxFromAddress.IsPubkey() && !ctxFromAddress.IsContract())
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: From address not pubkey or contract address, from: %s", destFrom.ToString().c_str());
        return false;
    }

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplatePledge(destDelegate, destFrom, nPledgeType, nCycles, nNonce));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Create template fail, destDelegate: %s, from: %s, pledge type: %d, cycles: %d, nonce: %d",
               destDelegate.ToString().c_str(), destFrom.ToString().c_str(), nPledgeType, nCycles, nNonce);
        return false;
    }
    CDestination destPledge(ptr->GetTemplateId());

    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destPledge, ctxVote))
    {
        StdLog("CBlockState", "Do func tx pledge req redeem: Retrieve dest vote context fail, pledge dest: %s", destPledge.ToString().c_str());
        return false;
    }
    if (ctxVote.nFinalHeight == 0 || nBlockHeight < ctxVote.nFinalHeight)
    {
        auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
        uint32 nPledgeDays = objPledge->GetPledgeDays(nBlockHeight);
        if (nPledgeDays == 0)
        {
            nPledgeDays = 1;
        }
        const uint32 nPledgeHeight = nPledgeDays * DAY_HEIGHT;
        uint32 nSetHeight = 0;
        if (ctxVote.nFinalHeight == 0)
        {
            nSetHeight = nBlockHeight + nPledgeHeight;
        }
        else
        {
            nSetHeight = ctxVote.nFinalHeight;
            while (nSetHeight > nPledgeHeight && nSetHeight - nPledgeHeight > nBlockHeight)
            {
                nSetHeight -= nPledgeHeight;
            }
        }
        mapCacheModifyPledgeFinalHeight[destPledge] = std::make_pair(nSetHeight, nBlockHeight);

        StdDebug("CBlockState", "Do func tx pledge req redeem: set height: %d, pledge height: %d, old final height: %d, block height: %d, delegate address: %s, pledge type: %d, cycles: %d, nonce: %d, from: %s",
                 nSetHeight, nPledgeHeight, ctxVote.nFinalHeight, nBlockHeight, destDelegate.ToString().c_str(), nPledgeType, nCycles, nNonce, destFrom.ToString().c_str());

        btResult = uint256(1).ToBigEndian();
    }
    else
    {
        btResult = uint256(0).ToBigEndian();
    }

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destPledge.ToHash());
    logs.topics.push_back(destDelegate.ToHash());
    return true;
}

bool CBlockState::DoFuncTxGetPledgeVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get pledge votes: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 5)
    {
        StdLog("CBlockState", "Do func tx get pledge votes: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    CDestination destOwner;
    uint32 nPledgeType;
    uint32 nCycles;
    uint32 nNonce;
    uint256 nAmount;

    const uint8* p = btTxParam.data();

    memcpy(tempData.begin(), p, 32);
    destDelegate.SetHash(tempData);
    p += 32;

    memcpy(tempData.begin(), p, 32);
    destOwner.SetHash(tempData);
    p += 32;

    tempData.FromBigEndian(p, 32);
    nPledgeType = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nCycles = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nNonce = tempData.Get32();

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplatePledge(destDelegate, destOwner, nPledgeType, nCycles, nNonce));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx get pledge votes: Create template fail, destDelegate: %s, destOwner: %s, nPledgeType: %d, nCycles: %d, nNonce: %d",
               destDelegate.ToString().c_str(), destOwner.ToString().c_str(), nPledgeType, nCycles, nNonce);
        return false;
    }
    CDestination destPledge(ptr->GetTemplateId());

    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destPledge, ctxVote))
    {
        StdLog("CBlockState", "Do func tx get pledge votes: Retrieve dest vote context fail, pledge dest: %s", destPledge.ToString().c_str());
        return false;
    }
    btResult = ctxVote.nVoteAmount.ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetPledgeUnlockHeight(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get pledge unlock height: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 5)
    {
        StdLog("CBlockState", "Do func tx get pledge unlock height: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;
    CDestination destOwner;
    uint32 nPledgeType;
    uint32 nCycles;
    uint32 nNonce;
    uint256 nAmount;

    const uint8* p = btTxParam.data();

    memcpy(tempData.begin(), p, 32);
    destDelegate.SetHash(tempData);
    p += 32;

    memcpy(tempData.begin(), p, 32);
    destOwner.SetHash(tempData);
    p += 32;

    tempData.FromBigEndian(p, 32);
    nPledgeType = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nCycles = tempData.Get32();
    p += 32;

    tempData.FromBigEndian(p, 32);
    nNonce = tempData.Get32();

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplatePledge(destDelegate, destOwner, nPledgeType, nCycles, nNonce));
    if (!ptr)
    {
        StdLog("CBlockState", "Do func tx get pledge unlock height: Create template fail, destDelegate: %s, destOwner: %s, nPledgeType: %d, nCycles: %d, nNonce: %d",
               destDelegate.ToString().c_str(), destOwner.ToString().c_str(), nPledgeType, nCycles, nNonce);
        return false;
    }
    CDestination destPledge(ptr->GetTemplateId());

    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destPledge, ctxVote))
    {
        StdLog("CBlockState", "Do func tx get pledge unlock height: Retrieve dest vote context fail, pledge dest: %s", destPledge.ToString().c_str());
        return false;
    }
    btResult = uint256(ctxVote.nFinalHeight).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetPageSize(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                      const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    btResult = uint256(DATA_PAGE_SIZE).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetDelegateCount(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                           const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get delegate count: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    std::multimap<uint256, CDestination> mapDelegateVote;
    if (!dbBlockBase.GetDelegateList(hashPrevBlock, 0, 0, mapDelegateVote))
    {
        StdLog("CBlockState", "Do func tx get delegate count: Get count fail");
        return false;
    }
    btResult = uint256(mapDelegateVote.size()).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetDelegateAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                             const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get delegate address: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32)
    {
        StdLog("CBlockState", "Do func tx get delegate address: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    uint32 nPageNo;

    tempData.FromBigEndian(btTxParam.data(), 32);
    nPageNo = tempData.Get32();

    std::multimap<uint256, CDestination> mapDelegateVote;
    if (!dbBlockBase.GetDelegateList(hashPrevBlock, nPageNo * DATA_PAGE_SIZE, DATA_PAGE_SIZE, mapDelegateVote))
    {
        StdLog("CBlockState", "Do func tx get delegate address: Get list fail");
        return false;
    }
    StdDebug("CBlockState", "Do func tx get delegate address: page: %d, count: %lu", nPageNo, mapDelegateVote.size());

    bytes btRetHead = uint256((uint32)0x20).ToBigEndian();
    bytes btCountHead = uint256(mapDelegateVote.size()).ToBigEndian();

    btResult.insert(btResult.end(), btRetHead.begin(), btRetHead.end());
    btResult.insert(btResult.end(), btCountHead.begin(), btCountHead.end());

    for (auto& kv : mapDelegateVote)
    {
        bytes btAddrData = kv.second.ToHash().GetBytes();
        btResult.insert(btResult.end(), btAddrData.begin(), btAddrData.end());
    }
    return true;
}

bool CBlockState::DoFuncTxGetDelegateTotalVotes(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam,
                                                const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get delegate total votes: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32)
    {
        StdLog("CBlockState", "Do func tx get delegate total votes: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destDelegate;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destDelegate.SetHash(tempData);

    uint256 nGetVoteAmount;
    if (!dbBlockBase.RetrieveDestDelegateVote(hashPrevBlock, destDelegate, nGetVoteAmount))
    {
        nGetVoteAmount = 0;
    }
    btResult = nGetVoteAmount.ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxGetVoteUnlockHeight(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get vote unlock height: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32)
    {
        StdLog("CBlockState", "Do func tx get vote unlock height: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destVote;

    memcpy(tempData.begin(), btTxParam.data(), 32);
    destVote.SetHash(tempData);

    CVoteContext ctxVote;
    if (!dbBlockBase.RetrieveDestVoteContext(hashPrevBlock, destVote, ctxVote))
    {
        StdLog("CBlockState", "Do func tx get vote unlock height: Retrieve dest vote context fail, dest: %s", destVote.ToString().c_str());
        return false;
    }
    btResult = uint256(ctxVote.nFinalHeight).ToBigEndian();
    return true;
}

bool CBlockState::DoFuncTxSetFunctionAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx set function address: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32 * 3)
    {
        StdLog("CBlockState", "Do func tx set function address: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    CDestination destNewFunction;
    uint32 nFuncId;
    bool fDisableModify;

    const uint8* p = btTxParam.data();

    tempData.FromBigEndian(p, 32);
    nFuncId = tempData.Get32();
    p += 32;

    memcpy(tempData.begin(), p, 32);
    destNewFunction.SetHash(tempData);
    p += 32;

    tempData.FromBigEndian(p, 32);
    fDisableModify = (tempData.Get32() == 0 ? false : true);

    std::string strErr;
    if (!dbBlockBase.VerifyNewFunctionAddress(hashPrevBlock, destFrom, nFuncId, destNewFunction, strErr))
    {
        StdLog("CBlockState", "Do func tx set function address: Verify new function address fail, err: %s, function id: %d, new function address: %s, disable modify: %s, from: %s",
               strErr.c_str(), nFuncId, destNewFunction.ToString().c_str(), (fDisableModify ? "true" : "false"), destFrom.ToString().c_str());
        return false;
    }
    if (!VerifyFunctionAddressRepeat(destNewFunction))
    {
        StdLog("CBlockState", "Do func tx set function address: Function address repeat, function id: %d, new function address: %s, disable modify: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), (fDisableModify ? "true" : "false"), destFrom.ToString().c_str());
        return false;
    }
    if (!VerifyFunctionAddressDisable(nFuncId))
    {
        StdLog("CBlockState", "Do func tx set function address: Disable modify, function id: %d, new function address: %s, disable modify: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), (fDisableModify ? "true" : "false"), destFrom.ToString().c_str());
        return false;
    }
    mapCacheFunctionAddress[nFuncId] = CFunctionAddressContext(destNewFunction, fDisableModify);

    StdDebug("CBlockState", "Do func tx set function address: Set success, function id: %d, new function address: %s, disable modify: %s, from: %s",
             nFuncId, destNewFunction.ToString().c_str(), (fDisableModify ? "true" : "false"), destFrom.ToString().c_str());

    btResult = uint256(1).ToBigEndian();

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destNewFunction.ToHash());
    return true;
}

bool CBlockState::DoFuncTxGetFunctionAddress(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get function address: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (btTxParam.size() != 32)
    {
        StdLog("CBlockState", "Do func tx get function address: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    uint32 nFuncId;

    const uint8* p = btTxParam.data();

    tempData.FromBigEndian(p, 32);
    nFuncId = tempData.Get32();

    CFunctionAddressContext ctxFuncAddress;
    if (!dbBlockBase.RetrieveFunctionAddress(hashPrevBlock, nFuncId, ctxFuncAddress))
    {
        StdLog("CBlockState", "Do func tx get function address: Get address fail");
        return false;
    }

    btResult = ctxFuncAddress.GetFunctionAddress().ToHash().GetBytes();
    return true;
}

bool CBlockState::DoFuncTxAddUserCoin(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx add user coin: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    uint256 tempData;
    std::string strCoinSymbol;
    uint32 nChainId;

    const uint8* p = btTxParam.data();
    std::size_t nSurplusParamLen = btTxParam.size();

    // coin symbol
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbol) || strCoinSymbol.empty() || strCoinSymbol.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx add user coin: Param coinsymbol error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbol);
    p += 32;
    nSurplusParamLen -= 32;

    // chainid
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add user coin: Param chainid error, param size: %lu", btTxParam.size());
        return false;
    }
    tempData.FromBigEndian(p, 32);
    nChainId = tempData.Get32();

    uint256 hashCoinFork;
    if (!dbBlockBase.GetForkHashByChainId(nChainId, hashCoinFork, hashPrevBlock))
    {
        StdLog("CBlockState", "Do func tx add user coin: Chainid not exist, chainid: %d", nChainId);
        return false;
    }

    CCoinContext ctxCoinTemp;
    if (dbBlockBase.GetForkCoinCtxByForkSymbol(strCoinSymbol, ctxCoinTemp, hashPrevBlock))
    {
        StdLog("CBlockState", "Do func tx add user coin: Coinsymbol existed, coinsymbol: %s", strCoinSymbol.c_str());
        return false;
    }

    mapCacheSymbolCoin[strCoinSymbol] = CCoinContext(hashCoinFork, CCoinContext::CT_COIN_TYPE_USER, {});

    btResult = uint256(1).ToBigEndian();

    logs.topics.push_back(hashCoinFork);

    StdDebug("CBlockState", "Do func tx add user coin: Add success, coinsymbol: %s, chainid: %d, from: %s",
             strCoinSymbol.c_str(), nChainId, destFrom.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxAddContractCoin(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx add contract coin: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    uint256 tempData;
    std::string strCoinSymbol;
    uint32 nChainId;
    CDestination destContract;

    const uint8* p = btTxParam.data();
    std::size_t nSurplusParamLen = btTxParam.size();

    // coin symbol
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbol) || strCoinSymbol.empty() || strCoinSymbol.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx add contract coin: Param coinsymbol error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbol);
    p += 32;
    nSurplusParamLen -= 32;

    // chainid
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add contract coin: Param chainid error, param size: %lu", btTxParam.size());
        return false;
    }
    tempData.FromBigEndian(p, 32);
    nChainId = tempData.Get32();
    p += 32;
    nSurplusParamLen -= 32;

    // contract address
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add contract coin: Param contractAddress error, param size: %lu", btTxParam.size());
        return false;
    }
    memcpy(tempData.begin(), p, 32);
    destContract.SetHash(tempData);

    uint256 hashCoinFork;
    if (!dbBlockBase.GetForkHashByChainId(nChainId, hashCoinFork, hashPrevBlock))
    {
        StdLog("CBlockState", "Do func tx add contract coin: Chainid not exist, chainid: %d", nChainId);
        return false;
    }

    CCoinContext ctxCoinTemp;
    if (dbBlockBase.GetForkCoinCtxByForkSymbol(strCoinSymbol, ctxCoinTemp, hashPrevBlock))
    {
        StdLog("CBlockState", "Do func tx add contract coin: Coinsymbol existed, coinsymbol: %s", strCoinSymbol.c_str());
        return false;
    }

    mapCacheSymbolCoin[strCoinSymbol] = CCoinContext(hashCoinFork, CCoinContext::CT_COIN_TYPE_CONTRACT, destContract);

    btResult = uint256(1).ToBigEndian();

    logs.topics.push_back(hashCoinFork);

    StdDebug("CBlockState", "Do func tx add contract coin: Add success, coinsymbol: %s, chainid: %d, contract address: %s, from: %s",
             strCoinSymbol.c_str(), nChainId, destContract.ToString().c_str(), destFrom.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxGetCoin(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (hashFork != dbBlockBase.GetGenesisBlockHash())
    {
        StdLog("CBlockState", "Do func tx get coin: Fork not is genesis, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    std::string strCoinSymbol;
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), btTxParam.data(), btTxParam.size(), strCoinSymbol) || strCoinSymbol.empty() || strCoinSymbol.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx get coin: Param coinsymbol error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbol);

    CChainId nChainId = 0;
    uint32 nCoinType = 0;
    CDestination destContract;
    CCoinContext ctxCoin;
    bool fRet = dbBlockBase.GetForkCoinCtxByForkSymbol(strCoinSymbol, ctxCoin, hashPrevBlock);
    if (!fRet)
    {
        StdLog("CBlockState", "Do func tx get coin: Coinsymbol not existed, coinsymbol: %s", strCoinSymbol.c_str());
    }
    else
    {
        nChainId = CBlock::GetBlockChainIdByHash(ctxCoin.hashAtFork);
        nCoinType = ctxCoin.nCoinType;
        destContract = ctxCoin.destContract;
    }

    bytes btSetData;
    btSetData = uint256((uint64)fRet).ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());
    btSetData = uint256(nChainId).ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());
    btSetData = uint256(nCoinType).ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());
    btSetData = destContract.ToHash().GetBytes();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());

    StdDebug("CBlockState", "Do func tx get coin: Get success, ret: %s, coinsymbol: %s, chainid: %d, cointype: %d, contract address: %s, from: %s",
             (fRet ? "true" : "false"), strCoinSymbol.c_str(), nChainId, nCoinType, destContract.ToString().c_str(), destFrom.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxAddDexOrder(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    uint256 tempData;
    std::string strCoinSymbolOwner;
    std::string strCoinSymbolPeer;
    uint64 nOrderNumber;
    uint256 nOrderAmount;
    uint256 nOrderPrice;

    const uint8* p = btTxParam.data();
    std::size_t nSurplusParamLen = btTxParam.size();

    // coin symbol owner
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolOwner) || strCoinSymbolOwner.empty() || strCoinSymbolOwner.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param coin symbol owner error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolOwner);
    p += 32;
    nSurplusParamLen -= 32;

    // coin symbol peer
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolPeer) || strCoinSymbolPeer.empty() || strCoinSymbolPeer.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param coin symbol peer error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolPeer);
    p += 32;
    nSurplusParamLen -= 32;

    if (strCoinSymbolOwner == strCoinSymbolPeer)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param symbol is same, symbol: %s, param size: %lu", strCoinSymbolOwner.c_str(), btTxParam.size());
        return false;
    }

    // order number
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param order number error, param size: %lu", btTxParam.size());
        return false;
    }
    tempData.FromBigEndian(p, 32);
    nOrderNumber = tempData.Get64();
    p += 32;
    nSurplusParamLen -= 32;

    // order amount
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param order amount error, param size: %lu", btTxParam.size());
        return false;
    }
    nOrderAmount.FromBigEndian(p, 32);
    if (nOrderAmount == 0)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param order amount is 0, param size: %lu", btTxParam.size());
        return false;
    }
    p += 32;
    nSurplusParamLen -= 32;

    // order price
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param order price error, param size: %lu", btTxParam.size());
        return false;
    }
    nOrderPrice.FromBigEndian(p, 32);
    if (nOrderPrice == 0)
    {
        StdLog("CBlockState", "Do func tx add dex order: Param order price is 0, param size: %lu", btTxParam.size());
        return false;
    }
    p += 32;
    nSurplusParamLen -= 32;

    uint256 hashPrevRefBlock;
    if (hashFork == dbBlockBase.GetGenesisBlockHash())
    {
        hashPrevRefBlock = hashPrevBlock;
    }
    else
    {
        if (!dbBlockBase.GetBlockForRefBlockNoLock(hashPrevBlock, hashPrevRefBlock))
        {
            StdLog("CBlockState", "Do func tx add dex order: Get prev ref block fail, prev block: %s", hashPrevBlock.ToString().c_str());
            return false;
        }
    }

    CCoinContext ctxCoinOwner;
    CCoinContext ctxCoinPeer;
    if (!dbBlockBase.GetForkCoinCtxByForkSymbol(strCoinSymbolOwner, ctxCoinOwner, hashPrevRefBlock))
    {
        StdLog("CBlockState", "Do func tx add dex order: Coin symbol owner not exist, symbol: %s", strCoinSymbolOwner.c_str());
        return false;
    }
    if (!dbBlockBase.GetForkCoinCtxByForkSymbol(strCoinSymbolPeer, ctxCoinPeer, hashPrevRefBlock))
    {
        StdLog("CBlockState", "Do func tx add dex order: Coin symbol peer not exist, symbol: %s", strCoinSymbolPeer.c_str());
        return false;
    }
    if (ctxCoinOwner.hashAtFork != hashFork)
    {
        StdLog("CBlockState", "Do func tx add dex order: Owner coin symbol not local fork, owner symbol: %s, owner fork: %s, local fork: %s",
               strCoinSymbolOwner.c_str(), ctxCoinOwner.hashAtFork.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    const CChainId nChainIdOwner = CBlock::GetBlockChainIdByHash(ctxCoinOwner.hashAtFork);
    const CChainId nChainIdPeer = CBlock::GetBlockChainIdByHash(ctxCoinPeer.hashAtFork);

    uint64 nMaxNumber = GetMaxDexCoinOrderNumber(destFrom, strCoinSymbolOwner, strCoinSymbolPeer);
    if (nOrderNumber == 0)
    {
        nOrderNumber = nMaxNumber + 1;
    }
    else
    {
        // nOrderNumber > 0: modify dex order
        if (nOrderNumber > nMaxNumber)
        {
            StdLog("CBlockState", "Do func tx add dex order: Order number out of range, set order number: %lu, max order number: %lu, from: %s, fork: %s",
                   nOrderNumber, nMaxNumber, destFrom.ToString().c_str(), hashFork.ToString().c_str());
            return false;
        }
    }

    CDexOrderHeader dexOrderHeader(nChainIdOwner, destFrom, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber);

    bool fGetCacheOrder = false;
    CDexOrderBody oldOrderBody;
    auto it = mapCacheDexOrder.find(dexOrderHeader);
    if (it != mapCacheDexOrder.end())
    {
        oldOrderBody = it->second;
        fGetCacheOrder = true;
    }
    else
    {
        auto mt = mapBlockDexOrder.find(dexOrderHeader);
        if (mt != mapBlockDexOrder.end())
        {
            oldOrderBody = mt->second;
            fGetCacheOrder = true;
        }
    }
    if (!fGetCacheOrder)
    {
        if (dbBlockBase.GetDexOrder(hashPrevBlock, destFrom, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, oldOrderBody))
        {
            fGetCacheOrder = true;
        }
    }
    if (fGetCacheOrder)
    {
        if (nOrderPrice != oldOrderBody.nOrderPrice)
        {
            StdLog("CBlockState", "Do func tx add dex order: Modify dex order, order price error, set order price: %s, old order price: %s, from: %s, fork: %s",
                   CoinToTokenBigFloat(nOrderPrice).c_str(), CoinToTokenBigFloat(oldOrderBody.nOrderPrice).c_str(),
                   destFrom.ToString().c_str(), hashFork.ToString().c_str());
            return false;
        }
    }

    uint256 nCompleteAmount;
    uint64 nCompleteOrderCount = 0;
    if (!dbBlockBase.GetCompleteOrder(hashPrevBlock, destFrom, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, nCompleteAmount, nCompleteOrderCount))
    {
        nCompleteAmount = 0;
        nCompleteOrderCount = 0;
    }
    if (nOrderAmount < nCompleteAmount)
    {
        StdLog("CBlockState", "Do func tx add dex order: Order amount out of range, order amount: %s, complete amount: %s, from: %s, fork: %s",
               CoinToTokenBigFloat(nOrderAmount).c_str(), CoinToTokenBigFloat(nCompleteAmount).c_str(),
               destFrom.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }

    if (nOrderAmount < oldOrderBody.nOrderAmount)
    {
        // Indicates a decrease in order volume
        if (nOrderAmount > nCompleteAmount)
        {
            // Return the remaining coins to the user
            uint256 nTransferAmount = nOrderAmount - nCompleteAmount;
            if (ctxCoinOwner.IsContractCoin())
            {
                if (!ContractInTransfer(ctxCoinOwner.destContract, FUNCTION_DEX_POOL_ADDRESS, destFrom, nTransferAmount))
                {
                    StdLog("CBlockState", "Do func tx add dex order: Return the remaining coins to the user, contract transfer fail, constract address: %s, from: %s, amount: %s",
                           ctxCoinOwner.destContract.ToString().c_str(), destFrom.ToString().c_str(), CoinToTokenBigFloat(nTransferAmount).c_str());
                    return false;
                }
            }
            else
            {
                CAddressContext ctxFromAddress;
                if (!GetAddressContext(destFrom, ctxFromAddress))
                {
                    StdLog("CBlockState", "Do func tx add dex order: Get from address context fail, from: %s", destFrom.ToString().c_str());
                    return false;
                }
                uint64 nGasLeftTemp = FUNCTION_TX_GAS_TRANS;
                if (!ContractTransfer(FUNCTION_DEX_POOL_ADDRESS, destFrom, nTransferAmount, nGasLimit, nGasLeftTemp, ctxFromAddress, CContractTransfer::CT_CROSSCHAIN_DEX))
                {
                    StdLog("CBlockState", "Do func tx add dex order: Return the remaining coins to the user, dex pool transfer fail, from: %s, amount: %s",
                           destFrom.ToString().c_str(), CoinToTokenBigFloat(nTransferAmount).c_str());
                    return false;
                }
            }
        }
    }
    else if (nOrderAmount > oldOrderBody.nOrderAmount)
    {
        // Indicates the addition of new orders or an increase in order volume
        uint256 nTransferAmount = nOrderAmount - oldOrderBody.nOrderAmount;
        if (ctxCoinOwner.IsContractCoin())
        {
            if (!ContractInTransfer(ctxCoinOwner.destContract, destFrom, FUNCTION_DEX_POOL_ADDRESS, nTransferAmount))
            {
                StdLog("CBlockState", "Do func tx add dex order: Increase order, contract transfer fail, constract address: %s, from: %s, amount: %s",
                       ctxCoinOwner.destContract.ToString().c_str(), destFrom.ToString().c_str(), CoinToTokenBigFloat(nTransferAmount).c_str());
                return false;
            }
        }
        else
        {
            uint64 nGasLeftTemp = FUNCTION_TX_GAS_TRANS;
            if (!ContractTransfer(destFrom, FUNCTION_DEX_POOL_ADDRESS, nTransferAmount, nGasLimit, nGasLeftTemp, CAddressContext(CPubkeyAddressContext()), CContractTransfer::CT_CROSSCHAIN_DEX))
            {
                StdLog("CBlockState", "Do func tx add dex order: Increase order, dex pool transfer fail, from: %s, amount: %s",
                       destFrom.ToString().c_str(), CoinToTokenBigFloat(nTransferAmount).c_str());
                return false;
            }
        }
    }

    if (ctxCoinPeer.hashAtFork != hashFork)
    {
        mapCacheCrossProve[nChainIdPeer].AddDexOrderProve(destFrom, nChainIdOwner, nChainIdPeer, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, nOrderAmount, nOrderPrice);
    }
    mapCacheDexOrder[dexOrderHeader] = CDexOrderBody(strCoinSymbolOwner, strCoinSymbolPeer, nChainIdPeer, nOrderAmount, nOrderPrice);

    btResult = uint256(1).ToBigEndian();
    logs.topics.push_back(uint256(uint256(nOrderNumber).ToBigEndian()));

    StdDebug("CBlockState", "Do func tx add dex order: Add success, coin pair owner: %s, coin pair peer: %s, order number: %lu, amount: %s, price: %s, from: %s, fork: %s",
             strCoinSymbolOwner.c_str(), strCoinSymbolPeer.c_str(), nOrderNumber, CoinToTokenBigFloat(nOrderAmount).c_str(),
             CoinToTokenBigFloat(nOrderPrice).c_str(), destFrom.ToString().c_str(), hashFork.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxGetDexOrder(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    uint256 tempData;
    std::string strCoinSymbolOwner;
    std::string strCoinSymbolPeer;
    uint64 nOrderNumber;

    const uint8* p = btTxParam.data();
    std::size_t nSurplusParamLen = btTxParam.size();

    // coin symbol owner
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolOwner) || strCoinSymbolOwner.empty() || strCoinSymbolOwner.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx get user coin: Param coin symbol owner error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolOwner);
    p += 32;
    nSurplusParamLen -= 32;

    // coin symbol peer
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolPeer) || strCoinSymbolPeer.empty() || strCoinSymbolPeer.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx get user coin: Param coin symbol peer error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolPeer);
    p += 32;
    nSurplusParamLen -= 32;

    if (strCoinSymbolOwner == strCoinSymbolPeer)
    {
        StdLog("CBlockState", "Do func tx get user coin: Param symbol is same, symbol: %s, param size: %lu", strCoinSymbolOwner.c_str(), btTxParam.size());
        return false;
    }

    // order number
    if (nSurplusParamLen < 32)
    {
        StdLog("CBlockState", "Do func tx get dex order: Param order number error, param size: %lu", btTxParam.size());
        return false;
    }
    tempData.FromBigEndian(p, 32);
    nOrderNumber = tempData.Get64();
    p += 32;
    nSurplusParamLen -= 32;

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const CChainId nChainIdOwner = CBlock::GetBlockChainIdByHash(hashPrevBlock);

    CDexOrderBody dexOrder;
    bool fGetOk = false;
    for (auto& kv : mapCacheDexOrder)
    {
        if (kv.first.GetOrderAddress() == destFrom && kv.first.GetCoinPairHash() == hashCoinPair && kv.first.GetOwnerCoinFlag() == nOwnerCoinFlag && kv.first.GetOrderNumber() == nOrderNumber)
        {
            dexOrder = kv.second;
            fGetOk = true;
            break;
        }
    }
    if (!fGetOk)
    {
        for (auto& kv : mapBlockDexOrder)
        {
            if (kv.first.GetOrderAddress() == destFrom && kv.first.GetCoinPairHash() == hashCoinPair && kv.first.GetOwnerCoinFlag() == nOwnerCoinFlag && kv.first.GetOrderNumber() == nOrderNumber)
            {
                dexOrder = kv.second;
                fGetOk = true;
                break;
            }
        }
    }
    if (!fGetOk)
    {
        fGetOk = dbBlockBase.GetDexOrder(hashPrevBlock, destFrom, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, dexOrder);
        if (!fGetOk)
        {
            dexOrder.SetNull();
        }
    }

    bytes btSetData;
    btSetData = uint256((uint64)fGetOk).ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());
    btSetData = dexOrder.nOrderAmount.ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());
    btSetData = dexOrder.nOrderPrice.ToBigEndian();
    btResult.insert(btResult.end(), btSetData.begin(), btSetData.end());

    StdDebug("CBlockState", "Do func tx get dex order: result: %s, coin pair owner: %s, coin pair peer: %s, order number: %lu, amount: %s, price: %s, from: %s",
             (fGetOk ? "true" : "false"), strCoinSymbolOwner.c_str(), strCoinSymbolPeer.c_str(), nOrderNumber, CoinToTokenBigFloat(dexOrder.nOrderAmount).c_str(),
             CoinToTokenBigFloat(dexOrder.nOrderPrice).c_str(), destFrom.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxGetMaxDexOrderNumber(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    uint256 tempData;
    std::string strCoinSymbolOwner;
    std::string strCoinSymbolPeer;

    const uint8* p = btTxParam.data();
    std::size_t nSurplusParamLen = btTxParam.size();

    // coin symbol owner
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolOwner) || strCoinSymbolOwner.empty() || strCoinSymbolOwner.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx get max dex order number: Param coin symbol owner error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolOwner);
    p += 32;
    nSurplusParamLen -= 32;

    // coin symbol peer
    if (!GetContractStringParam(btTxParam.data(), btTxParam.size(), p, nSurplusParamLen, strCoinSymbolPeer) || strCoinSymbolPeer.empty() || strCoinSymbolPeer.size() > MAX_COIN_SYMBOL_SIZE)
    {
        StdLog("CBlockState", "Do func tx get max dex order number: Param coin symbol peer error, param size: %lu", btTxParam.size());
        return false;
    }
    StringToUpper(strCoinSymbolPeer);
    p += 32;
    nSurplusParamLen -= 32;

    if (strCoinSymbolOwner == strCoinSymbolPeer)
    {
        StdLog("CBlockState", "Do func tx get max dex order number: Param symbol is same, symbol: %s, param size: %lu", strCoinSymbolOwner.c_str(), btTxParam.size());
        return false;
    }

    uint64 nMaxNumber = GetMaxDexCoinOrderNumber(destFrom, strCoinSymbolOwner, strCoinSymbolPeer);
    btResult = uint256(nMaxNumber).ToBigEndian();

    StdDebug("CBlockState", "Do func tx get max dex order number: coin pair owner: %s, coin pair peer: %s, max order number: %lu, from: %s",
             strCoinSymbolOwner.c_str(), strCoinSymbolPeer.c_str(), nMaxNumber, destFrom.ToString().c_str());

    return true;
}

bool CBlockState::DoFuncTxTransferCoin(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    if (btTxParam.size() != 32 * 2)
    {
        StdLog("CBlockState", "Do func tx transfer coin: Tx param error, param size: %lu", btTxParam.size());
        return false;
    }

    uint256 tempData;
    uint32 nPeerChainId;
    uint256 nTransferAmount;

    const uint8* p = btTxParam.data();

    tempData.FromBigEndian(p, 32);
    nPeerChainId = tempData.Get32();
    p += 32;

    nTransferAmount.FromBigEndian(p, 32);

    const uint256 hashGenesisFork = dbBlockBase.GetGenesisBlockHash();
    const CChainId nGenChainId = CBlock::GetBlockChainIdByHash(hashGenesisFork);
    const CDestination& destTransferTo = FUNCTION_CROSSCHAIN_ADDRESS;

    if (nPeerChainId == nLocalChainId)
    {
        StdLog("CBlockState", "Do func tx transfer coin: Peer chainid is same as source chainid, peer chainid: %d, source chainid: %d, from: %s", nPeerChainId, nLocalChainId, destFrom.ToString().c_str());
        return false;
    }

    uint256 hashPrimaryRefBlock;
    if (fPrimaryBlock)
    {
        hashPrimaryRefBlock = hashPrevBlock;
    }
    else
    {
        hashPrimaryRefBlock = hashRefBlock;
    }

    CForkContext ctxLocalFork;
    if (!dbBlockBase.RetrieveForkContext(hashFork, ctxLocalFork, hashPrimaryRefBlock))
    {
        StdLog("CBlockState", "Do func tx transfer coin: Retrieve genesis fork context fail, fork: %s, ref block: %s, from: %s",
               hashFork.ToString().c_str(), hashPrimaryRefBlock.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }
    if (hashFork != hashGenesisFork && !ctxLocalFork.IsClonemapFork())
    {
        StdLog("CBlockState", "Do func tx transfer coin: Clonemap fork only allows crosschain transfer, fork: %s, from: %s", hashFork.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }

    CForkContext ctxGenesisFork;
    if (!dbBlockBase.RetrieveForkContext(hashGenesisFork, ctxGenesisFork, hashPrimaryRefBlock))
    {
        StdLog("CBlockState", "Do func tx transfer coin: Retrieve genesis fork context fail, ref block: %s, from: %s", hashPrimaryRefBlock.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }

    CDestState stateFrom;
    if (!GetDestState(destFrom, stateFrom))
    {
        StdLog("CBlockState", "Do func tx transfer coin: Get from state fail, fork: %s, from: %s", hashFork.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }
    if (stateFrom.GetBalance() < nTransferAmount)
    {
        StdLog("CBlockState", "Do func tx transfer coin: From address balance not enough, from balance: %s, transfer amount: %s, fork: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(nTransferAmount).c_str(), hashFork.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }

    CAddressContext ctxToAddress;
    if (!GetAddressContext(destTransferTo, ctxToAddress))
    {
        ctxToAddress = CAddressContext(CPubkeyAddressContext());
    }

    // if (!ContractTransfer(destFrom, destTransferTo, nTransferAmount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_CROSSCHAIN_TRANSFER))
    // {
    //     StdLog("CBlockState", "Do func tx transfer coin: Contract transfer fail, from: %s", destFrom.ToString().c_str());
    //     return false;
    // }

    // verify reward lock
    uint256 nLockedAmount;
    if (!GetDestLockedAmount(destFrom, nLockedAmount))
    {
        StdLog("CBlockState", "Do func tx transfer coin: Get reward locked amount failed, fork: %s, from: %s", hashFork.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }
    if (nLockedAmount > 0 && stateFrom.GetBalance() < nLockedAmount + nTransferAmount)
    {
        StdLog("CBlockState", "Do func tx transfer coin: Balance locked, balance: %s, lock amount: %s, amount: %s, fork: %s, from: %s",
               CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(nLockedAmount).c_str(),
               CoinToTokenBigFloat(nTransferAmount).c_str(), hashFork.ToString().c_str(), destFrom.ToString().c_str());
        return false;
    }

    CDestState stateTo;
    if (!GetDestState(destTransferTo, stateTo))
    {
        stateTo.SetNull();
        stateTo.SetType(ctxToAddress.GetDestType(), ctxToAddress.GetTemplateType());
    }

    stateFrom.DecBalance(nTransferAmount);
    if (nLocalChainId == nGenChainId)
    {
        stateTo.IncBalance(nTransferAmount);
    }
    else
    {
        stateTo.DecBalance(nTransferAmount);
    }

    SetCacheDestState(destFrom, stateFrom);
    SetCacheDestState(destTransferTo, stateTo);

    mapBlockAddressContext[destTransferTo] = ctxToAddress;

    // CContractTransfer ctrTransfer(CContractTransfer::CT_CROSSCHAIN_TRANSFER, destFrom, destTransferTo, nTransferAmount);
    // mapBlockContractTransfer[txidMint].push_back(ctrTransfer);

    // receiptMintTx.vTransfer.push_back(ctrTransfer);

    vCacheContractTransfer.push_back(CContractTransfer(CContractTransfer::CT_CROSSCHAIN_TRANSFER, destFrom, destTransferTo, nTransferAmount));

    mapCacheCrossProve[nPeerChainId].AddCoinTransferProve(destFrom, ctxGenesisFork.strSymbol, nLocalChainId, nPeerChainId, nTransferAmount);

    logs.topics.push_back(destFrom.ToHash());
    logs.topics.push_back(destTransferTo.ToHash());
    logs.topics.push_back(uint256(nTransferAmount.ToBigEndian()));

    btResult = uint256(1).ToBigEndian();

    return true;
}

bool CBlockState::DoFuncTxGetCrosschainTransferAmount(const CDestination& destFrom, const CDestination& destTo, const bytes& btTxParam, const uint64 nGasLimit, uint64& nGasLeft, CTransactionLogs& logs, bytes& btResult)
{
    uint256 nTransferAmount;
    CDestState stateCross;
    if (!GetDestState(FUNCTION_CROSSCHAIN_ADDRESS, stateCross))
    {
        nTransferAmount = 0;
    }
    else
    {
        nTransferAmount = stateCross.GetBalance();
    }
    btResult = nTransferAmount.ToBigEndian();
    return true;
}

//////////////////////////////
// CContractHostDB

bool CContractHostDB::Get(const uint256& key, bytes& value) const
{
    return blockState.GetDestKvData(destContract, key, value);
}

uint256 CContractHostDB::GetTxid() const
{
    return txid;
}

uint256 CContractHostDB::GetTxNonce() const
{
    return nTxNonce;
}

CDestination CContractHostDB::GetContractAddress() const
{
    return destContract;
}

CDestination CContractHostDB::GetCodeOwnerAddress() const
{
    return destCodeOwner;
}

bool CContractHostDB::GetBalance(const CDestination& addr, uint256& balance) const
{
    if (!blockState.GetDestBalance(addr, balance))
    {
        StdLog("CContractHostDB", "Get Balance: Get dest state fail, dest: %s", addr.ToString().c_str());
        return false;
    }
    return true;
}

bool CContractHostDB::ContractTransfer(const CDestination& from, const CDestination& to, const uint256& amount, const uint64 nGasLimit, uint64& nGasLeft)
{
    CAddressContext ctxToAddress;
    if (!blockState.GetAddressContext(to, ctxToAddress))
    {
        StdLog("CContractHostDB", "Contract transfer: Get to address context fail, to: %s", to.ToString().c_str());
        ctxToAddress = CAddressContext(CPubkeyAddressContext());
    }
    if (!blockState.ContractTransfer(from, to, amount, nGasLimit, nGasLeft, ctxToAddress, CContractTransfer::CT_CONTRACT))
    {
        StdLog("CContractHostDB", "Contract transfer: Contract transfer fail, from: %s, to: %s", from.ToString().c_str(), to.ToString().c_str());
        return false;
    }
    return true;
}

uint256 CContractHostDB::GetBlockHash(const uint64 nBlockNumber)
{
    uint256 hashBlock;
    if (!blockState.GetBlockHashByNumber(nBlockNumber, hashBlock))
    {
        StdLog("CContractHostDB", "GetBlockHash: Get block hash fail, number: %lu", nBlockNumber);
        return uint256();
    }
    return hashBlock;
}

bool CContractHostDB::IsContractAddress(const CDestination& addr)
{
    return blockState.IsContractAddress(addr);
}

bool CContractHostDB::GetContractRunCode(const CDestination& destContractIn, uint256& hashContractCreateCode, CDestination& destCodeOwner, uint256& hashContractRunCode, bytes& btContractRunCode, bool& fDestroy)
{
    return blockState.GetContractRunCode(destContractIn, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy);
}

bool CContractHostDB::GetContractCreateCode(const CDestination& destContractIn, CTxContractData& txcd)
{
    return blockState.GetContractCreateCode(destContractIn, txcd);
}

bool CContractHostDB::SaveContractRunCode(const CDestination& destContractIn, const bytes& btContractRunCode, const CTxContractData& txcd)
{
    return blockState.SaveContractRunCode(destContractIn, btContractRunCode, txcd, txid);
}

bool CContractHostDB::ExecFunctionContract(const CDestination& destFromIn, const CDestination& destToIn, const bytes& btData, const uint64 nGasLimit, uint64& nGasLeft, bytes& btResult)
{
    return blockState.ExecFunctionContract(destFromIn, destToIn, btData, nGasLimit, nGasLeft, btResult);
}

bool CContractHostDB::Selfdestruct(const CDestination& destBeneficiaryIn)
{
    return blockState.Selfdestruct(destContract, destBeneficiaryIn);
}

CVmHostFaceDBPtr CContractHostDB::CloneHostDB(const CDestination& destContractIn)
{
    return CVmHostFaceDBPtr(new CContractHostDB(blockState, destContractIn, destCodeOwner, txid, nTxNonce));
}

void CContractHostDB::SaveGasUsed(const CDestination& destCodeOwnerIn, const uint64 nGasUsed)
{
    blockState.SaveGasUsed(destCodeOwnerIn, nGasUsed);
}

void CContractHostDB::SaveRunResult(const CDestination& destContractIn, const std::vector<CTransactionLogs>& vLogsIn, const std::map<uint256, bytes>& mapCacheKv)
{
    blockState.SaveRunResult(destContractIn, vLogsIn, mapCacheKv);
}

//////////////////////////////
// CCacheBlockReceipt

void CCacheBlockReceipt::AddBlockReceiptCache(const uint256& hashBlock, const std::vector<CTransactionReceipt>& vBlockReceipt)
{
    CWriteLock wlock(rwBrcAccess);

    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    auto it = mapBlockReceiptCache.find(nChainId);
    if (it == mapBlockReceiptCache.end())
    {
        it = mapBlockReceiptCache.insert(std::make_pair(nChainId, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>())).first;
    }
    auto& mapReceipts = it->second;
    while (mapReceipts.size() >= MAX_CACHE_BLOCK_RECEIPT_COUNT)
    {
        mapReceipts.erase(mapReceipts.begin());
    }
    mapReceipts[hashBlock] = vBlockReceipt;
}

bool CCacheBlockReceipt::GetBlockReceiptCache(const uint256& hashBlock, std::vector<CTransactionReceipt>& vBlockReceipt)
{
    CReadLock rlock(rwBrcAccess);

    auto it = mapBlockReceiptCache.find(CBlock::GetBlockChainIdByHash(hashBlock));
    if (it == mapBlockReceiptCache.end())
    {
        return false;
    }
    auto mt = it->second.find(hashBlock);
    if (mt == it->second.end())
    {
        return false;
    }
    vBlockReceipt = mt->second;
    return true;
}

//////////////////////////////
// CBlockBase

CBlockBase::CBlockBase()
  : fCfgFullDb(false), fCfgRewardCheck(false)
{
}

CBlockBase::~CBlockBase()
{
    dbBlock.Deinitialize();
    tsBlock.Deinitialize();
}

bool CBlockBase::Initialize(const path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fRewardCheckIn, const bool fRenewDB)
{
    hashGenesisBlock = hashGenesisBlockIn;
    fCfgFullDb = fFullDbIn;
    fCfgRewardCheck = fRewardCheckIn;

    StdLog("BlockBase", "Initializing... (Path : %s)", pathDataLocation.string().c_str());

    if (!dbBlock.Initialize(pathDataLocation, hashGenesisBlockIn, fFullDbIn))
    {
        StdError("BlockBase", "Failed to initialize block db");
        return false;
    }

    if (!tsBlock.Initialize(pathDataLocation / "block", BLOCKFILE_PREFIX))
    {
        dbBlock.Deinitialize();
        StdError("BlockBase", "Failed to initialize block tsfile");
        return false;
    }

    if (fRenewDB)
    {
        Clear();
    }
    else if (!LoadDB())
    {
        dbBlock.Deinitialize();
        tsBlock.Deinitialize();
        {
            CWriteLock wlock(rwAccess);

            ClearCache();
        }
        StdError("BlockBase", "Failed to load block db");
        return false;
    }
    StdLog("BlockBase", "Initialized");
    return true;
}

void CBlockBase::Deinitialize()
{
    dbBlock.Deinitialize();
    tsBlock.Deinitialize();
    {
        CWriteLock wlock(rwAccess);

        ClearCache();
    }
    StdLog("BlockBase", "Deinitialized");
}

const uint256& CBlockBase::GetGenesisBlockHash() const
{
    return hashGenesisBlock;
}

bool CBlockBase::Exists(const uint256& hash) const
{
    CReadLock rlock(rwAccess);

    return (!!mapIndex.count(hash));
}

bool CBlockBase::ExistsTx(const uint256& hashFork, const uint256& txid)
{
    uint256 hashAtFork;
    CTxIndex txIndex;
    return GetTxIndex(hashFork, txid, hashAtFork, txIndex);
}

bool CBlockBase::IsEmpty() const
{
    CReadLock rlock(rwAccess);

    return mapIndex.empty();
}

void CBlockBase::Clear()
{
    CWriteLock wlock(rwAccess);

    dbBlock.RemoveAll();
    ClearCache();
}

bool CBlockBase::Initiate(const uint256& hashGenesis, const CBlock& blockGenesis, const uint256& nChainTrust)
{
    if (!IsEmpty())
    {
        StdError("BlockBase", "Add genesis block: Is not empty");
        return false;
    }

    CBlockEx blockex(blockGenesis, nChainTrust);
    CBlockChainUpdate update;
    if (!StorageNewBlock(hashGenesis, hashGenesis, blockex, update))
    {
        StdError("BlockBase", "Add genesis block: Save block fail");
        return false;
    }
    return true;
}

bool CBlockBase::StorageNewBlock(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, CBlockChainUpdate& update)
{
    CWriteLock wlock(rwAccess);

    if (mapIndex.count(hashBlock) > 0)
    {
        StdTrace("BlockBase", "Storage new block: Exist block: %s", hashBlock.ToString().c_str());
        return true;
    }

    CBlockIndex* pIndexNew = nullptr;
    CBlockRoot blockRoot;
    std::vector<CTransactionReceipt> vBlockTxReceipts;
    if (!SaveBlock(hashFork, hashBlock, block, &pIndexNew, blockRoot, vBlockTxReceipts, false))
    {
        StdError("BlockBase", "Storage new block: Save block failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    if (CheckForkLongChain(hashFork, hashBlock, block, pIndexNew))
    {
        update = CBlockChainUpdate(pIndexNew, vBlockTxReceipts);
        if (block.IsOrigin())
        {
            update.vBlockAddNew.push_back(block);
        }
        else
        {
            if (!GetBlockBranchListNolock(hashBlock, 0, &block, update.vBlockRemove, update.vBlockAddNew))
            {
                StdLog("BlockBase", "Storage new block: Get block branch list fail, block: %s", hashBlock.ToString().c_str());
                return false;
            }
        }

        CBlockVoteSig proofVote;
        if (block.GetBlockVoteSig(proofVote) && !proofVote.IsNull())
        {
            if (!dbBlock.AddBlockVoteResult(proofVote.hashBlockVote, true, proofVote.btBlockVoteBitmap, proofVote.btBlockVoteAggSig, true, hashBlock))
            {
                StdError("BlockBase", "Save block: Add block vote result failed, vote block: %s, block: %s", proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            if (!dbBlock.UpdateBlockAggSign(proofVote.hashBlockVote, proofVote.btBlockVoteBitmap, proofVote.btBlockVoteAggSig, update.mapBlockProve))
            {
                StdError("BlockBase", "Save block: Update crosschain block agg sign failed, vote block: %s, block: %s", proofVote.hashBlockVote.ToString().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }

        if (!UpdateBlockLongChain(hashFork, update.vBlockRemove, update.vBlockAddNew))
        {
            StdLog("BlockBase", "Storage new block: Update block long chain fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }

        UpdateBlockNext(pIndexNew);
        if (!dbBlock.UpdateForkLast(hashFork, hashBlock))
        {
            StdError("BlockBase", "Storage new block: Update fork last fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }

        StdLog("CBlockChain", "Storage new block: Long chain, type: %s, time: %s, txs: %lu, block: [%lu-%u-%u] %s, chain trust: %s, fork: [%lu] %s",
               GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), GetTimeString(block.GetBlockTime()).c_str(), block.vtx.size(), pIndexNew->GetBlockNumber(),
               pIndexNew->GetBlockHeight(), pIndexNew->GetBlockSlot(), hashBlock.GetHex().c_str(), pIndexNew->nChainTrust.GetValueHex().c_str(), pIndexNew->nChainId, pIndexNew->GetOriginHash().GetHex().c_str());
    }
    return true;
}

bool CBlockBase::SaveBlock(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, CBlockIndex** ppIndexNew, CBlockRoot& blockRoot, std::vector<CTransactionReceipt>& vBlockTxReceipts, const bool fRepair)
{
    uint32 nFile, nOffset, nCrc;
    CBlockVerify verifyBlock;
    if (dbBlock.RetrieveBlockVerify(hashBlock, verifyBlock))
    {
        nFile = verifyBlock.nFile;
        nOffset = verifyBlock.nOffset;
        nCrc = verifyBlock.nBlockCrc;
    }
    else
    {
        if (!tsBlock.Write(block, nFile, nOffset, nCrc))
        {
            StdError("BlockBase", "Save block: Write block failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }

    if (block.IsOrigin())
    {
        if (fRepair)
        {
            if (!dbBlock.LoadFork(hashFork))
            {
                StdError("BlockBase", "Save block: Load fork failed, fork: %s", hashFork.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (!dbBlock.AddNewFork(hashFork))
            {
                StdError("BlockBase", "Save block: Add new fork failed, fork: %s", hashFork.ToString().c_str());
                return false;
            }
        }
    }

    CBlockIndex* pIndexNew = AddNewIndex(hashBlock, block, nFile, nOffset, nCrc, block.nBlockTrust, block.hashStateRoot);
    if (pIndexNew == nullptr)
    {
        StdError("BlockBase", "Save block: Add New Index failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    bool fRet = true;
    do
    {
        std::map<CDestination, CAddressContext> mapTempAddressContext;
        if (!GetBlockAddress(hashFork, hashBlock, block, mapTempAddressContext))
        {
            StdError("BlockBase", "Save block: Get block address failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        SHP_BLOCK_STATE ptrBlockStateOut = nullptr;
        if (!UpdateBlockState(hashFork, hashBlock, block, mapTempAddressContext, blockRoot.hashStateRoot, ptrBlockStateOut))
        {
            StdError("BlockBase", "Save block: Update block state failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockTxIndex(hashFork, hashBlock, block, nFile, nOffset, ptrBlockStateOut->vBlockTxReceipts, blockRoot.hashTxIndexRoot))
        {
            StdError("BlockBase", "Save block: Update block tx index failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockAddress(hashFork, hashBlock, block, ptrBlockStateOut->mapBlockAddressContext, ptrBlockStateOut->mapBlockState,
                                ptrBlockStateOut->mapBlockPayTvFee, ptrBlockStateOut->mapBlockFunctionAddress, blockRoot.hashAddressRoot))
        {
            StdError("BlockBase", "Save block: Update block address failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockCode(hashFork, hashBlock, block, nFile, nOffset, ptrBlockStateOut->mapBlockContractCreateCodeContext,
                             ptrBlockStateOut->mapBlockContractRunCodeContext, ptrBlockStateOut->mapBlockAddressContext, blockRoot.hashCodeRoot))
        {
            StdError("BlockBase", "Save block: Update block code failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (fCfgFullDb)
        {
            uint256 hashAddressTxInfoRoot;
            if (!UpdateBlockAddressTxInfo(hashFork, hashBlock, block, ptrBlockStateOut->mapBlockContractTransfer,
                                          ptrBlockStateOut->mapBlockTxFeeUsed, ptrBlockStateOut->mapBlockCodeDestFeeUsed, hashAddressTxInfoRoot))
            {
                StdError("BlockBase", "Save block: Update block address tx info failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }
        }

        if (pIndexNew->IsPrimary())
        {
            if (!AddBlockForkContext(hashBlock, block, ptrBlockStateOut->mapBlockAddressContext, ptrBlockStateOut->mapBlockSymbolCoin, blockRoot.hashForkContextRoot))
            {
                StdError("BlockBase", "Save block: Add bock fork context failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateDelegate(hashFork, hashBlock, block, nFile, nOffset, DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT,
                                ptrBlockStateOut->mapBlockAddressContext, ptrBlockStateOut->mapBlockState, blockRoot.hashDelegateRoot))
            {
                StdError("BlockBase", "Save block: Update delegate failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateVote(hashFork, hashBlock, block, ptrBlockStateOut->mapBlockAddressContext, ptrBlockStateOut->mapBlockState, ptrBlockStateOut->mapBlockModifyPledgeFinalHeight, blockRoot.hashVoteRoot))
            {
                StdError("BlockBase", "Save block: Update vote failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }
        }

        if (!UpdateBlockVoteReward(hashFork, pIndexNew->nChainId, hashBlock, block, blockRoot.hashVoteRewardRoot))
        {
            StdError("BlockBase", "Save block: Update block vote reward failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockDexOrder(hashFork, pIndexNew->GetRefBlock(), block.hashPrev, hashBlock, block, ptrBlockStateOut->mapBlockDexOrder, ptrBlockStateOut->proveBlockCrosschain,
                                 ptrBlockStateOut->mapCoinPairCompletePrice, ptrBlockStateOut->mapCompDexOrderRecord, blockRoot.hashDexOrderRoot))
        {
            StdError("BlockBase", "Save block: Update block dex order failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!dbBlock.AddNewBlockNumber(hashFork, pIndexNew->nChainId, block.hashPrev, block.nNumber, hashBlock, blockRoot.hashBlockNumberRoot))
        {
            StdError("BlockBase", "Save block: Add new block number failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        CBlockOutline outline(pIndexNew);
        if (!dbBlock.AddNewBlockIndex(outline))
        {
            StdError("BlockBase", "Save block: Add new block index failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!dbBlock.AddBlockVerify(outline, blockRoot.GetRootCrc()))
        {
            StdError("BlockBase", "Save block: Add block verify failed, block: %s", hashBlock.ToString().c_str());
            dbBlock.RemoveBlockIndex(hashBlock);
            fRet = false;
            break;
        }

        // add destroy token
        for (auto& kv : ptrBlockStateOut->mapBlockContractTransfer)
        {
            for (auto& ts : kv.second)
            {
                if (ts.destTo == FUNCTION_BLACKHOLE_ADDRESS)
                {
                    pIndexNew->nMoneyDestroy += ts.nAmount;
                }
            }
        }

        vBlockTxReceipts = ptrBlockStateOut->vBlockTxReceipts;
    } while (0);

    if (!fRet)
    {
        RemoveBlockIndex(hashFork, hashBlock);
        delete pIndexNew;
        return false;
    }
    *ppIndexNew = pIndexNew;

    StdLog("BlockBase", "Save block: Save block success, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());

    if (fCfgRewardCheck)
    {
        if (block.IsPrimary() && (block.GetBlockHeight() % VOTE_REWARD_DISTRIBUTE_HEIGHT) == 1)
        {
            std::map<CDestination, CDestState> mapBlockState;
            if (!dbBlock.ListDestState(hashFork, block.hashStateRoot, mapBlockState))
            {
                StdLog("BlockBase", "Save block: List dest state fail, block: %s", hashBlock.ToString().c_str());
            }
            else
            {
                uint256 nTotalAmount;
                for (const auto& kv : mapBlockState)
                {
                    nTotalAmount += kv.second.GetBalance();
                }

                uint256 nBlockMint;
                block.GetBlockMint(nBlockMint);
                uint256 nNoDistributeBlockFee = 0;
                if (block.txMint.GetTxType() == CTransaction::TX_STAKE)
                {
                    nNoDistributeBlockFee = block.GetBlockTotalReward() - nBlockMint;
                }
                if (pIndexNew->GetMoneySupply() - nBlockMint == nTotalAmount + nNoDistributeBlockFee)
                {
                    StdLog("BlockBase", "Save block: Reward check: State amount success, height: %d, total address balance: %s, no distribute fee: %s, total amount: %s, prev supply: %s, block mint: %s, block: %s",
                           block.GetBlockHeight(), CoinToTokenBigFloat(nTotalAmount).c_str(), CoinToTokenBigFloat(nNoDistributeBlockFee).c_str(), CoinToTokenBigFloat(nTotalAmount + nNoDistributeBlockFee).c_str(),
                           CoinToTokenBigFloat(pIndexNew->GetMoneySupply() - nBlockMint).c_str(), CoinToTokenBigFloat(nBlockMint).c_str(), hashBlock.ToString().c_str());
                }
                else
                {
                    StdLog("BlockBase", "Save block: Reward check: State amount error, height: %d, total address balance: %s, no distribute fee: %s, total amount: %s, prev supply: %s, block mint: %s, block: %s",
                           block.GetBlockHeight(), CoinToTokenBigFloat(nTotalAmount).c_str(), CoinToTokenBigFloat(nNoDistributeBlockFee).c_str(), CoinToTokenBigFloat(nTotalAmount + nNoDistributeBlockFee).c_str(),
                           CoinToTokenBigFloat(pIndexNew->GetMoneySupply() - nBlockMint).c_str(), CoinToTokenBigFloat(nBlockMint).c_str(), hashBlock.ToString().c_str());
                }

                // uint256 nBlockMint;
                // block.GetBlockMint(nBlockMint);
                // uint256 nBlockReward = 0;
                // if (block.txMint.GetTxType() == CTransaction::TX_STAKE)
                // {
                //     nBlockReward = block.GetBlockTotalReward();
                // }
                // else
                // {
                //     nBlockReward = block.txMint.GetAmount();
                // }
                // uint256 nBlockFee = nBlockReward - nBlockMint;
                // uint256 nStateAmount = nTotalAmount - block.txMint.GetAmount() + nBlockFee;
                // uint256 nPrevMoneySupply = pIndexNew->GetMoneySupply() - nBlockMint;
                // if (nPrevMoneySupply != nStateAmount)
                // {
                //     StdLog("BlockBase", "Save block: State amount error, height: %d, money supply: %s, state amount: %s, total amount: %s, block reward: %s, block amount: %s",
                //            block.GetBlockHeight(), CoinToTokenBigFloat(nPrevMoneySupply).c_str(),
                //            CoinToTokenBigFloat(nStateAmount).c_str(),
                //            CoinToTokenBigFloat(nTotalAmount).c_str(), CoinToTokenBigFloat(nBlockReward).c_str(),
                //            CoinToTokenBigFloat(block.txMint.GetAmount()).c_str());
                // }
                // else
                // {
                //     StdLog("BlockBase", "Save block: State amount success, height: %d, money supply: %s, block reward: %s, block amount: %s",
                //            block.GetBlockHeight(), CoinToTokenBigFloat(nPrevMoneySupply).c_str(),
                //            CoinToTokenBigFloat(nBlockReward).c_str(), CoinToTokenBigFloat(block.txMint.GetAmount()).c_str());
                // }
            }
        }
    }
    return true;
}

bool CBlockBase::CheckForkLongChain(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const CBlockIndex* pIndexNew)
{
    if (!pIndexNew->IsPrimary() && !pIndexNew->IsOrigin())
    {
        CProofOfPiggyback proof;
        if (!block.GetPiggybackProof(proof) || proof.hashRefBlock == 0)
        {
            StdLog("BlockBase", "Add long chain last: Load proof fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }

        CBlockIndex* pIndexRef = GetIndex(proof.hashRefBlock);
        if (pIndexRef == nullptr || !pIndexRef->IsPrimary())
        {
            StdLog("BlockBase", "Add long chain last: Retrieve ref index fail, ref block: %s, block: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }

        if (!VerifyRefBlock(GetGenesisBlockHash(), proof.hashRefBlock))
        {
            StdLog("BlockBase", "Add long chain last: Ref block short chain, refblock: %s, new block: %s, fork: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
    }
    if (!VerifyBlockConfirmChain(pIndexNew))
    {
        StdLog("BlockBase", "Add long chain last: Verify block confirm chain fail, new block: %s, fork: %s",
               hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    CBlockIndex* pIndexFork = GetForkLastIndex(hashFork);
    if (pIndexFork
        && (pIndexFork->nChainTrust > pIndexNew->nChainTrust
            || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
    {
        StdLog("BlockBase", "Add long chain last: Short chain, new block height: %d, block type: %s, block: %s, fork chain trust: %s, new block trust: %s, fork last block: %s, fork: %s",
               pIndexNew->GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), hashBlock.GetHex().c_str(),
               pIndexFork->nChainTrust.GetValueHex().c_str(), pIndexNew->nChainTrust.GetValueHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str(), pIndexFork->GetOriginHash().GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const CBlockIndex* pIndex, CBlock& block)
{
    block.SetNull();

    CBlockEx blockex;
    if (!tsBlock.Read(blockex, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "RetrieveFromIndex::Read %s block failed, File: %d, Offset: %d",
                 pIndex->GetBlockHash().ToString().c_str(), pIndex->nFile, pIndex->nOffset);
        return false;
    }
    block = static_cast<CBlock>(blockex);
    return true;
}

bool CBlockBase::Retrieve(const uint256& hash, CBlockEx& block)
{
    block.SetNull();

    CBlockIndex* pIndex;
    {
        CReadLock rlock(rwAccess);

        if (!(pIndex = GetIndex(hash)))
        {
            StdTrace("BlockBase", "RetrieveBlockEx::GetIndex %s block failed", hash.ToString().c_str());
            return false;
        }
    }
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "RetrieveBlockEx::Read %s block failed", hash.ToString().c_str());

        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const CBlockIndex* pIndex, CBlockEx& block)
{
    block.SetNull();

    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, true, true))
    {
        StdTrace("BlockBase", "RetrieveFromIndex::GetIndex %s block failed", pIndex->GetBlockHash().ToString().c_str());

        return false;
    }
    return true;
}

bool CBlockBase::RetrieveIndex(const uint256& hashBlock, CBlockIndex** ppIndex)
{
    CReadLock rlock(rwAccess);

    *ppIndex = GetIndex(hashBlock);
    return (*ppIndex != nullptr);
}

bool CBlockBase::RetrieveFork(const uint256& hashFork, CBlockIndex** ppIndex)
{
    CReadLock rlock(rwAccess);

    *ppIndex = GetForkLastIndex(hashFork);
    return (*ppIndex != nullptr);
}

bool CBlockBase::RetrieveFork(const string& strName, CBlockIndex** ppIndex)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return false;
    }
    for (const auto& kv : mapForkCtxt)
    {
        if (kv.second.strName == strName)
        {
            return RetrieveFork(kv.first, ppIndex);
        }
    }
    return false;
}

bool CBlockBase::RetrieveProfile(const uint256& hashFork, CProfile& profile, const uint256& hashMainChainRefBlock)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock))
    {
        StdTrace("BlockBase", "Retrieve Profile: Retrieve fork context fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    profile = ctxt.GetProfile();
    return true;
}

bool CBlockBase::RetrieveAncestry(const uint256& hashFork, vector<pair<uint256, uint256>> vAncestry)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
    {
        StdTrace("BlockBase", "Ancestry Retrieve hashFork %s failed", hashFork.ToString().c_str());
        return false;
    }

    while (ctxt.hashParent != 0)
    {
        vAncestry.push_back(make_pair(ctxt.hashParent, ctxt.hashJoint));
        if (!dbBlock.RetrieveForkContext(ctxt.hashParent, ctxt))
        {
            return false;
        }
    }

    std::reverse(vAncestry.begin(), vAncestry.end());
    return true;
}

bool CBlockBase::RetrieveOrigin(const uint256& hashFork, CBlock& block)
{
    block.SetNull();

    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
    {
        StdTrace("BlockBase", "Retrieve Origin: Retrieve Fork Context %s block failed", hashFork.ToString().c_str());
        return false;
    }

    uint256 hashAtFork;
    CTxIndex txIndex;
    CTransaction tx;
    if (!RetrieveTxAndIndex(GetGenesisBlockHash(), ctxt.txidEmbedded, tx, hashAtFork, txIndex))
    {
        StdTrace("BlockBase", "Retrieve Origin: Retrieve Tx %s tx failed", ctxt.txidEmbedded.ToString().c_str());
        return false;
    }

    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        StdTrace("BlockBase", "Retrieve Origin: fork data error, txid: %s", ctxt.txidEmbedded.ToString().c_str());
        return false;
    }

    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTxAndIndex(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, CTxIndex& txIndex)
{
    if (!GetTxIndex(hashFork, txid, hashAtFork, txIndex))
    {
        StdLog("BlockBase", "Retrieve Tx: Get tx index fail, txid: %s, fork: %s", txid.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    tx.SetNull();
    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset, false, true))
    {
        StdLog("BlockBase", "Retrieve Tx: Read fail, txid: %s, fork: %s", txid.ToString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveAvailDelegate(const uint256& hash, int height, const vector<uint256>& vBlockRange,
                                       const uint256& nMinEnrollAmount,
                                       map<CDestination, size_t>& mapWeight,
                                       map<CDestination, vector<unsigned char>>& mapEnrollData,
                                       vector<pair<CDestination, uint256>>& vecAmount)
{
    map<CDestination, uint256> mapVote;
    if (!dbBlock.RetrieveDelegate(hash, mapVote))
    {
        StdTrace("BlockBase", "Retrieve Avail Delegate: Retrieve Delegate %s block failed",
                 hash.ToString().c_str());
        return false;
    }

    map<CDestination, CDiskPos> mapEnrollTxPos;
    if (!dbBlock.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos))
    {
        StdTrace("BlockBase", "Retrieve Avail Retrieve Enroll block %s height %d failed",
                 hash.ToString().c_str(), height);
        return false;
    }

    map<pair<uint256, CDiskPos>, pair<CDestination, vector<uint8>>> mapSortEnroll;
    for (auto it = mapVote.begin(); it != mapVote.end(); ++it)
    {
        if ((*it).second >= nMinEnrollAmount)
        {
            const CDestination& dest = (*it).first;
            map<CDestination, CDiskPos>::iterator mi = mapEnrollTxPos.find(dest);
            if (mi != mapEnrollTxPos.end())
            {
                CTransaction tx;
                if (!tsBlock.Read(tx, (*mi).second, false, true))
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate::Read %s tx failed", tx.GetHash().ToString().c_str());
                    return false;
                }

                bytes btCertData;
                if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btCertData))
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate: tx data error1, txid: %s",
                           tx.GetHash().ToString().c_str());
                    return false;
                }

                mapSortEnroll.insert(make_pair(make_pair(it->second, mi->second), make_pair(dest, btCertData)));
            }
        }
    }
    // first 25 destination sorted by amount and sequence
    for (auto it = mapSortEnroll.rbegin(); it != mapSortEnroll.rend() && mapWeight.size() < MAX_DELEGATE_THRESH; it++)
    {
        mapWeight.insert(make_pair(it->second.first, 1));
        mapEnrollData.insert(make_pair(it->second.first, it->second.second));
        vecAmount.push_back(make_pair(it->second.first, it->first.first));
    }
    for (const auto& d : vecAmount)
    {
        StdTrace("BlockBase", "Retrieve Avail Delegate: dest: %s, amount: %s",
                 d.first.ToString().c_str(), CoinToTokenBigFloat(d.second).c_str());
    }
    return true;
}

bool CBlockBase::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& destDelegate, uint256& nVoteAmount)
{
    return dbBlock.RetrieveDestDelegateVote(hashBlock, destDelegate, nVoteAmount);
}

void CBlockBase::ListForkIndex(std::map<uint256, CBlockIndex*>& mapForkIndex)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return;
    }
    for (const auto& kv : mapForkCtxt)
    {
        CBlockIndex* pIndex = GetForkLastIndex(kv.first);
        if (pIndex)
        {
            mapForkIndex.insert(make_pair(kv.first, pIndex));
        }
    }
}

void CBlockBase::ListForkIndex(multimap<int, CBlockIndex*>& mapForkIndex)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return;
    }
    for (const auto& kv : mapForkCtxt)
    {
        CBlockIndex* pIndex = GetForkLastIndex(kv.first);
        if (pIndex)
        {
            mapForkIndex.insert(make_pair(pIndex->pOrigin->GetBlockHeight() - 1, pIndex));
        }
    }
}

bool CBlockBase::GetBlockBranchList(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder)
{
    CReadLock rlock(rwAccess);

    return GetBlockBranchListNolock(hashBlock, 0, nullptr, vRemoveBlockInvertedOrder, vAddBlockPositiveOrder);
}

bool CBlockBase::GetPrevBlockHashList(const uint256& hashBlock, const uint32 nGetCount, std::vector<uint256>& vPrevBlockhash)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndex = GetIndex(hashBlock);
    if (!pIndex)
    {
        return false;
    }
    pIndex = pIndex->pPrev;
    while (pIndex && !pIndex->IsOrigin() && vPrevBlockhash.size() < (std::size_t)nGetCount)
    {
        vPrevBlockhash.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }
    return true;
}

bool CBlockBase::LoadIndex(CBlockOutline& outline)
{
    uint256 hash = outline.GetBlockHash();
    CBlockIndex* pIndexNew = nullptr;

    map<uint256, CBlockIndex*>::iterator mi = mapIndex.find(hash);
    if (mi != mapIndex.end())
    {
        pIndexNew = (*mi).second;
        *pIndexNew = static_cast<CBlockIndex&>(outline);
    }
    else
    {
        pIndexNew = new CBlockIndex(static_cast<CBlockIndex&>(outline));
        if (pIndexNew == nullptr)
        {
            StdLog("B", "LoadIndex: new CBlockIndex fail");
            return false;
        }
        mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
    }

    pIndexNew->phashBlock = &((*mi).first);
    pIndexNew->pPrev = nullptr;
    pIndexNew->pOrigin = pIndexNew;

    if (outline.hashPrev != 0)
    {
        pIndexNew->pPrev = GetOrCreateIndex(outline.hashPrev);
        if (pIndexNew->pPrev == nullptr)
        {
            StdLog("B", "LoadIndex: GetOrCreateIndex prev block index fail");
            return false;
        }
    }

    if (!pIndexNew->IsOrigin())
    {
        pIndexNew->pOrigin = GetOrCreateIndex(outline.hashOrigin);
        if (pIndexNew->pOrigin == nullptr)
        {
            StdLog("B", "LoadIndex: GetOrCreateIndex origin block index fail");
            return false;
        }
    }

    UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, pIndexNew->GetBlockTime(), CDestination(), pIndexNew->GetRefBlock());
    return true;
}

bool CBlockBase::LoadTx(const uint32 nTxFile, const uint32 nTxOffset, CTransaction& tx)
{
    tx.SetNull();
    if (!tsBlock.Read(tx, nTxFile, nTxOffset, false, true))
    {
        StdTrace("BlockBase", "LoadTx::Read %s block failed", tx.GetHash().ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<std::string, CCoinContext>& mapNewSymbolCoin, uint256& hashNewRoot)
{
    map<uint256, CForkContext> mapNewForkCtxt;
    if (hashBlock == GetGenesisBlockHash())
    {
        CProfile profile;
        if (!blockex.GetForkProfile(profile))
        {
            StdError("BlockBase", "Add block fork context: Load genesis block proof failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }

        CForkContext ctxt(hashBlock, uint256(), blockex.txMint.GetHash(), profile);
        ctxt.nJointHeight = 0;
        mapNewForkCtxt.insert(make_pair(hashBlock, ctxt));
    }
    else
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        for (size_t i = 0; i < blockex.vtx.size(); i++)
        {
            const CTransaction& tx = blockex.vtx[i];
            if (!tx.GetToAddress().IsNull())
            {
                auto it = mapAddressContext.find(tx.GetToAddress());
                if (it != mapAddressContext.end())
                {
                    if (it->second.IsTemplate() && it->second.GetTemplateType() == TEMPLATE_FORK)
                    {
                        if (!VerifyBlockForkTx(blockex.hashPrev, tx, vForkCtxt))
                        {
                            StdLog("BlockBase", "Add block fork context: Verify block fork tx fail, txid: %s, block: %s", tx.GetHash().ToString().c_str(), hashBlock.ToString().c_str());
                        }
                    }
                }
            }
            if (!tx.GetFromAddress().IsNull())
            {
                auto mt = mapAddressContext.find(tx.GetFromAddress());
                if (mt != mapAddressContext.end())
                {
                    if (mt->second.IsTemplate() && mt->second.GetTemplateType() == TEMPLATE_FORK)
                    {
                        auto it = vForkCtxt.begin();
                        while (it != vForkCtxt.end())
                        {
                            if (it->first == tx.GetFromAddress())
                            {
                                StdLog("BlockBase", "Add block fork context: cancel fork, block: %s, fork: %s, dest: %s",
                                       hashBlock.ToString().c_str(), it->second.hashFork.ToString().c_str(),
                                       tx.GetFromAddress().ToString().c_str());
                                vForkCtxt.erase(it++);
                            }
                            else
                            {
                                ++it;
                            }
                        }
                    }
                }
            }
        }
        for (const auto& vd : vForkCtxt)
        {
            mapNewForkCtxt.insert(make_pair(vd.second.hashFork, vd.second));
        }
    }

    if (!dbBlock.AddForkContext(blockex.hashPrev, hashBlock, mapNewForkCtxt, mapNewSymbolCoin, hashNewRoot))
    {
        StdLog("BlockBase", "Add block fork context: Add fork context to db fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt)
{
    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        StdLog("BlockBase", "Verify block fork tx: tx data error, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }

    CBlock block;
    CProfile profile;
    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        StdLog("BlockBase", "Verify block fork tx: invalid fork data, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    if (!block.IsOrigin() || block.IsPrimary())
    {
        StdLog("BlockBase", "Verify block fork tx: invalid block, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    if (!block.GetForkProfile(profile))
    {
        StdLog("BlockBase", "Verify block fork tx: invalid profile, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    uint256 hashNewFork = block.GetHash();

    do
    {
        CForkContext ctxtParent;
        if (!dbBlock.RetrieveForkContext(profile.hashParent, ctxtParent, hashPrev))
        {
            bool fFindParent = false;
            for (const auto& vd : vForkCtxt)
            {
                if (vd.second.hashFork == profile.hashParent)
                {
                    ctxtParent = vd.second;
                    fFindParent = true;
                    break;
                }
            }
            if (!fFindParent)
            {
                StdLog("BlockBase", "Verify block fork tx: Retrieve parent context, tx: %s", tx.GetHash().ToString().c_str());
                break;
            }
        }

        if (!VerifyOriginBlock(block, ctxtParent.GetProfile()))
        {
            StdLog("BlockBase", "Verify block fork tx: Verify origin fail, tx: %s", tx.GetHash().ToString().c_str());
            break;
        }

        if (!VerifyForkFlag(hashNewFork, profile.nChainId, profile.strSymbol, profile.strName, hashPrev))
        {
            StdLog("BlockBase", "Verify block fork tx: Verify fork flag fail, fork: %s, txid: %s, prev block: %s", hashNewFork.ToString().c_str(), tx.GetHash().ToString().c_str(), hashPrev.ToString().c_str());
            break;
        }

        bool fCheckRet = true;
        for (const auto& vd : vForkCtxt)
        {
            if (vd.second.hashFork == hashNewFork
                || vd.second.strSymbol == profile.strSymbol
                || vd.second.strName == profile.strName
                || vd.second.nChainId == profile.nChainId)
            {
                StdLog("BlockBase", "Verify block fork tx: fork existed or name repeated, tx: %s, new fork: %s, symbol: %s, name: %s, chainid: %d",
                       tx.GetHash().ToString().c_str(), hashNewFork.GetHex().c_str(), vd.second.strSymbol.c_str(), vd.second.strName.c_str(), profile.nChainId);
                fCheckRet = false;
                break;
            }
        }
        if (!fCheckRet)
        {
            break;
        }

        vForkCtxt.push_back(make_pair(tx.GetToAddress(), CForkContext(block.GetHash(), block.hashPrev, tx.GetHash(), profile)));
        StdLog("BlockBase", "Verify block fork tx success: valid fork: %s, tx: %s", hashNewFork.GetHex().c_str(), tx.GetHash().ToString().c_str());
    } while (0);

    return true;
}

bool CBlockBase::VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainId, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashPrevBlock)
{
    if (hashNewFork != 0)
    {
        CForkContext ctxt;
        if (dbBlock.RetrieveForkContext(hashNewFork, ctxt, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Fork id existed, fork: %s, symbol: %s, create txid: %s", hashNewFork.GetHex().c_str(), ctxt.strSymbol.c_str(), ctxt.txidEmbedded.ToString().c_str());
            return false;
        }
    }
    if (nChainId != 0)
    {
        uint256 hashTempFork;
        if (dbBlock.GetForkHashByChainId(nChainId, hashTempFork, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Chain id existed, chainid: %d", nChainId);
            return false;
        }
    }
    if (!strForkSymbol.empty())
    {
        CCoinContext ctxCoin;
        if (dbBlock.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Symbol existed, symbol: %s", strForkSymbol.c_str());
            return false;
        }
    }
    if (!strForkName.empty())
    {
        uint256 hashTempFork;
        if (dbBlock.GetForkHashByForkName(strForkName, hashTempFork, hashPrevBlock))
        {
            StdLog("BlockBase", "Verify fork flag: Fork name existed, name: %s", strForkName.c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile)
{
    CProfile forkProfile;
    if (!block.GetForkProfile(forkProfile))
    {
        StdLog("BlockBase", "Verify origin block: load profile error");
        return false;
    }
    if (forkProfile.IsNull())
    {
        StdLog("BlockBase", "Verify origin block: invalid profile");
        return false;
    }
    if (forkProfile.nChainId == 0 || forkProfile.nChainId != block.txMint.GetChainId())
    {
        StdLog("BlockBase", "Verify origin block: invalid chainid");
        return false;
    }
    if (!MoneyRange(forkProfile.nAmount))
    {
        StdLog("BlockBase", "Verify origin block: invalid fork amount");
        return false;
    }
    if (!RewardRange(forkProfile.nMintReward))
    {
        StdLog("BlockBase", "Verify origin block: invalid fork reward");
        return false;
    }
    if (block.txMint.GetToAddress() != forkProfile.destOwner)
    {
        StdLog("BlockBase", "Verify origin block: invalid fork to");
        return false;
    }
    // if (parentProfile.IsPrivate())
    // {
    //     if (!forkProfile.IsPrivate() || parentProfile.destOwner != forkProfile.destOwner)
    //     {
    //         StdLog("BlockBase", "Verify origin block: permission denied");
    //         return false;
    //     }
    // }
    return true;
}

bool CBlockBase::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return dbBlock.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockBase::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock)
{
    return dbBlock.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock);
}

bool CBlockBase::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbBlock.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockBase::GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkCoinCtxByForkSymbol(strForkSymbol, ctxCoin, hashMainChainRefBlock);
}

bool CBlockBase::GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkHashByForkName(strForkName, hashFork, hashMainChainRefBlock);
}

bool CBlockBase::GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetForkHashByChainId(nChainId, hashFork, hashMainChainRefBlock);
}

bool CBlockBase::ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashMainChainRefBlock)
{
    return dbBlock.ListCoinContext(mapSymbolCoin, hashMainChainRefBlock);
}

bool CBlockBase::GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetDexCoinPairBySymbolPair(strSymbol1, strSymbol2, nCoinPair, hashMainChainRefBlock);
}

bool CBlockBase::GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock)
{
    return dbBlock.GetSymbolPairByDexCoinPair(nCoinPair, strSymbol1, strSymbol2, hashMainChainRefBlock);
}

bool CBlockBase::ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock)
{
    return dbBlock.ListDexCoinPair(nCoinPair, strCoinSymbol, mapDexCoinPair, hashMainChainRefBlock);
}

int CBlockBase::GetForkCreatedHeight(const uint256& hashFork, const uint256& hashRefBlock)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt, hashRefBlock))
    {
        return -1;
    }
    return ctxt.nJointHeight;
}

bool CBlockBase::GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight)
{
    auto it = mapForkHeightIndex.find(hashFork);
    if (it == mapForkHeightIndex.end())
    {
        return false;
    }
    return it->second.GetMaxHeight(nMaxHeight);
}

bool CBlockBase::GetBlockHashByNumber(const uint256& hashFork, const uint64 nBlockNumber, uint256& hashBlock)
{
    uint256 hashLastBlock;
    if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork))
    {
        return false;
    }
    return dbBlock.RetrieveBlockHashByNumber(hashFork, ctxFork.nChainId, hashLastBlock, nBlockNumber, hashBlock);
}

bool CBlockBase::GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndex = GetForkLastIndex(hashFork);
    if (pIndex == nullptr)
    {
        StdLog("BlockBase", "Get Fork Block Locator: Get frok last index failed, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    if (hashDepth != 0)
    {
        CBlockIndex* pStartIndex = GetIndex(hashDepth);
        if (pStartIndex != nullptr && pStartIndex->pNext != nullptr)
        {
            pIndex = pStartIndex;
        }
    }

    while (pIndex)
    {
        if (pIndex->GetOriginHash() != hashFork)
        {
            hashDepth = 0;
            break;
        }
        locator.vBlockHash.push_back(pIndex->GetBlockHash());
        if (pIndex->IsOrigin())
        {
            hashDepth = 0;
            break;
        }
        if (locator.vBlockHash.size() >= nIncStep / 2)
        {
            pIndex = pIndex->pPrev;
            if (pIndex == nullptr)
            {
                hashDepth = 0;
            }
            else
            {
                hashDepth = pIndex->GetBlockHash();
            }
            break;
        }
        for (int i = 0; i < nIncStep && !pIndex->IsOrigin(); i++)
        {
            pIndex = pIndex->pPrev;
            if (pIndex == nullptr)
            {
                hashDepth = 0;
                break;
            }
        }
    }

    return true;
}

bool CBlockBase::GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, vector<uint256>& vBlockHash, size_t nMaxCount)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndexLast = GetForkLastIndex(hashFork);
    if (pIndexLast == nullptr)
    {
        StdLog("BlockBase", "Get Fork Block Inv: Retrieve fork failed, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    CBlockIndex* pIndex = nullptr;
    for (const uint256& hash : locator.vBlockHash)
    {
        pIndex = GetIndex(hash);
        if (pIndex != nullptr && (pIndex == pIndexLast || pIndex->pNext != nullptr))
        {
            if (pIndex->GetOriginHash() != hashFork)
            {
                StdTrace("BlockBase", "GetForkBlockInv GetOriginHash error, fork: %s", hashFork.ToString().c_str());
                return false;
            }
            break;
        }
        pIndex = nullptr;
    }

    if (pIndex != nullptr)
    {
        pIndex = pIndex->pNext;
        while (pIndex != nullptr && vBlockHash.size() < nMaxCount)
        {
            vBlockHash.push_back(pIndex->GetBlockHash());
            pIndex = pIndex->pNext;
        }
    }
    return true;
}

bool CBlockBase::GetDelegateVotes(const uint256& hashGenesis, const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(hashGenesis, hashLastBlock))
        {
            return false;
        }
    }
    if (!dbBlock.RetrieveDestDelegateVote(hashLastBlock, destDelegate, nVotes))
    {
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbBlock.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockBase::ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight)
{
    return dbBlock.ListPledgeFinalHeight(hashBlock, nFinalHeight, mapPledgeFinalHeight);
}

bool CBlockBase::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbBlock.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockBase::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbBlock.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockBase::GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation)
{
    CAddressContext ctxAddress;
    if (!dbBlock.RetrieveAddressContext(GetGenesisBlockHash(), hashBlock, destDelegate, ctxAddress))
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Retrieve address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (!ctxAddress.IsTemplate())
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Address not is template, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Get template address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
    if (ptr == nullptr)
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Create template fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (ptr->GetTemplateType() != TEMPLATE_DELEGATE)
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Not delegate template, template type: %d, delegate: %s, block: %s",
               ptr->GetTemplateType(), destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    nRewardRation = boost::dynamic_pointer_cast<CTemplateDelegate>(ptr)->nRewardRatio;
    return true;
}

bool CBlockBase::GetDelegateList(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(GetGenesisBlockHash(), hashLastBlock))
        {
            return false;
        }
    }
    std::map<CDestination, uint256> mapAddressVote;
    if (!dbBlock.RetrieveDelegate(hashLastBlock, mapAddressVote))
    {
        return false;
    }
    if (nCount == 0)
    {
        for (const auto& d : mapAddressVote)
        {
            mapVotes.insert(std::make_pair(d.second, d.first));
        }
    }
    else
    {
        std::multimap<uint256, CDestination> mapTempAmountVote;
        for (const auto& d : mapAddressVote)
        {
            mapTempAmountVote.insert(std::make_pair(d.second, d.first));
        }
        auto it = mapTempAmountVote.begin();
        uint32 nJumpIndex = nStartIndex;
        while (it != mapTempAmountVote.end() && nJumpIndex > 0)
        {
            ++it;
            --nJumpIndex;
        }
        uint32 nJumpCount = nCount;
        while (it != mapTempAmountVote.end() && nJumpCount > 0)
        {
            mapVotes.insert(std::make_pair(it->first, it->second));
            ++it;
            --nJumpCount;
        }
    }
    return true;
}

bool CBlockBase::VerifyRepeatBlock(const uint256& hashFork, const uint256& hashBlock, const uint32 height, const CDestination& destMint, const uint16 nBlockType,
                                   const uint64 nBlockTimeStamp, const uint64 nRefBlockTimeStamp, const uint32 nExtendedBlockSpacing)
{
    CWriteLock wlock(rwAccess);

    map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        map<uint256, CBlockHeightIndex>* pBlockMint = it->second.GetBlockMintList(height);
        if (pBlockMint != nullptr)
        {
            for (auto& mt : *pBlockMint)
            {
                const uint256& hashHiBlock = mt.first;
                if (hashBlock == hashHiBlock)
                {
                    continue;
                }
                if (mt.second.destMint.IsNull())
                {
                    CBlockIndex* pBlockIndex = GetIndex(hashHiBlock);
                    if (pBlockIndex)
                    {
                        if (pBlockIndex->IsVacant())
                        {
                            CBlock block;
                            if (Retrieve(pBlockIndex, block) && !block.txMint.GetToAddress().IsNull())
                            {
                                mt.second.destMint = block.txMint.GetToAddress();
                            }
                        }
                        else
                        {
                            CTransaction tx;
                            uint256 hashAtFork;
                            CTxIndex txIndex;
                            if (RetrieveTxAndIndex(hashFork, pBlockIndex->txidMint, tx, hashAtFork, txIndex))
                            {
                                mt.second.destMint = tx.GetToAddress();
                            }
                        }
                    }
                }
                if (mt.second.destMint == destMint)
                {
                    if (nBlockType == CBlock::BLOCK_SUBSIDIARY || nBlockType == CBlock::BLOCK_EXTENDED)
                    {
                        if ((nBlockTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing
                            == (mt.second.nTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing)
                        {
                            StdTrace("BlockBase", "Verify Repeat Block: subsidiary or extended repeat block, block time: %lu, cache block time: %lu, ref block time: %lu, destMint: %s",
                                     nBlockTimeStamp, mt.second.nTimeStamp, mt.second.nTimeStamp, destMint.ToString().c_str());
                            return false;
                        }
                    }
                    else
                    {
                        StdTrace("BlockBase", "Verify Repeat Block: repeat block: %s, destMint: %s", hashHiBlock.GetHex().c_str(), destMint.ToString().c_str());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool CBlockBase::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, uint256>& mapVote)
{
    return dbBlock.RetrieveDelegate(hashBlock, mapVote);
}

bool CBlockBase::GetRangeDelegateEnroll(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbBlock.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockBase::VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock)
{
    CBlockIndex* pIndexGenesisLast = GetForkLastIndex(hashGenesis);
    if (!pIndexGenesisLast)
    {
        return false;
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

CBlockIndex* CBlockBase::GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndexGenesisLast = GetForkLastIndex(hashGenesis);
    if (!pIndexGenesisLast)
    {
        return nullptr;
    }

    CBlockIndex* pForkLast = GetForkLastIndex(hashFork);
    if (!pForkLast || pForkLast->IsOrigin())
    {
        return nullptr;
    }

    set<uint256> setInvalidHash;
    CBlockIndex* pIndex = pForkLast;
    while (pIndex && !pIndex->IsOrigin())
    {
        if (VerifyValidBlock(pIndexGenesisLast, pIndex))
        {
            break;
        }
        setInvalidHash.insert(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }
    if (pIndex == nullptr)
    {
        pIndex = GetIndex(hashFork);
    }
    if (pIndex == pForkLast)
    {
        return nullptr;
    }
    CBlockIndex* pIndexValidLast = GetLongChainLastBlock(hashFork, pIndex->GetBlockHeight(), pIndexGenesisLast, setInvalidHash);
    if (pIndexValidLast == nullptr)
    {
        return pIndex;
    }
    return pIndexValidLast;
}

bool CBlockBase::VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock)
{
    CReadLock rlock(rwAccess);
    return VerifySameChainNoLock(hashPrevBlock, hashAfterBlock);
}

bool CBlockBase::GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin)
{
    hashRefBlock = 0;
    fOrigin = false;
    CBlockIndex* pIndexUpdateRef = nullptr;

    {
        CReadLock rlock(rwAccess);

        auto it = mapForkHeightIndex.find(hashFork);
        if (it == mapForkHeightIndex.end())
        {
            return false;
        }
        std::map<uint256, CBlockHeightIndex>* pHeightIndex = it->second.GetBlockMintList(CBlock::GetBlockHeightByHash(hashBlock));
        if (pHeightIndex)
        {
            auto mt = pHeightIndex->find(hashBlock);
            if (mt != pHeightIndex->end() && mt->second.hashRefBlock != 0)
            {
                hashRefBlock = mt->second.hashRefBlock;
                return true;
            }
        }

        CBlockIndex* pIndex = GetIndex(hashBlock);
        while (pIndex)
        {
            if (pIndex->IsOrigin())
            {
                fOrigin = true;
                return true;
            }
            CBlockEx block;
            if (!Retrieve(pIndex, block))
            {
                return false;
            }
            CProofOfPiggyback proof;
            if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
            {
                hashRefBlock = proof.hashRefBlock;
                pIndexUpdateRef = pIndex;
                break;
            }
            pIndex = pIndex->pPrev;
        }
    }

    if (pIndexUpdateRef)
    {
        CWriteLock wlock(rwAccess);
        UpdateBlockRef(pIndexUpdateRef->GetOriginHash(), pIndexUpdateRef->GetBlockHash(), hashRefBlock);
        return true;
    }
    return false;
}

bool CBlockBase::GetBlockForRefBlockNoLock(const uint256& hashBlock, uint256& hashRefBlock)
{
    CBlockIndex* pIndex = GetIndex(hashBlock);
    if (!pIndex)
    {
        return false;
    }
    hashRefBlock = pIndex->GetRefBlock();
    return true;
}

bool CBlockBase::GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndex = GetIndex(hashLastBlock);
    if (pIndex == nullptr || !pIndex->IsPrimary())
    {
        return false;
    }
    while (pIndex && pIndex->GetBlockHeight() >= nHeight)
    {
        if (pIndex->GetBlockHeight() == nHeight)
        {
            hashBlock = pIndex->GetBlockHash();
            nTime = pIndex->GetBlockTime();
            return true;
        }
        pIndex = pIndex->pPrev;
    }
    return false;
}

bool CBlockBase::VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime)
{
    CReadLock rlock(rwAccess);

    const std::map<uint256, CBlockHeightIndex>* pMapHeight = mapForkHeightIndex[GetGenesisBlockHash()].GetBlockMintList(nHeight);
    if (pMapHeight == nullptr)
    {
        return false;
    }
    for (const auto& vd : *pMapHeight)
    {
        if (vd.second.nTimeStamp != nTime)
        {
            return false;
        }
    }
    return true;
}

bool CBlockBase::UpdateForkNext(const uint256& hashFork, CBlockIndex* pIndexLast, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew)
{
    CWriteLock wlock(rwAccess);

    if (!UpdateBlockLongChain(hashFork, vBlockRemove, vBlockAddNew))
    {
        StdLog("BlockBase", "Update Fork Next: Update block long chain fail, fork: %s, last block: %s", hashFork.ToString().c_str(), pIndexLast->GetBlockHash().ToString().c_str());
        return false;
    }

    UpdateBlockNext(pIndexLast);
    return dbBlock.UpdateForkLast(hashFork, pIndexLast->GetBlockHash());
}

bool CBlockBase::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbBlock.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

SHP_BLOCK_STATE CBlockBase::CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, const uint256& hashPrevStateRoot, const uint32 nPrevBlockTime, uint256& hashStateRoot, uint256& hashReceiptRoot,
                                                 uint256& hashCrosschainMerkleRoot, uint256& nBlockGasUsed, bytes& btBloomDataOut, uint256& nTotalMintRewardOut, bool& fMoStatus, const std::map<CDestination, CAddressContext>& mapAddressContext)
{
    SHP_BLOCK_STATE ptrBlockState(new CBlockState(*this, hashFork, block, hashPrevStateRoot, nPrevBlockTime, mapAddressContext));

    for (int i = 0; i < (int)(block.vtx.size()); i++)
    {
        const auto& tx = block.vtx[i];
        if (!ptrBlockState->AddTxState(tx, i + 1))
        {
            StdLog("BlockBase", "Create block state root: Add tx state fail, txid: %s, prev: %s",
                   tx.GetHash().ToString().c_str(), block.hashPrev.GetHex().c_str());
            return nullptr;
        }
    }

    if (!ptrBlockState->DoBlockState(hashReceiptRoot, nBlockGasUsed, btBloomDataOut, nTotalMintRewardOut, hashCrosschainMerkleRoot, fMoStatus))
    {
        StdLog("BlockBase", "Create block state root: Do block state fail, prev: %s", block.hashPrev.GetHex().c_str());
        return nullptr;
    }

    CBlockRootStatus statusBlockRoot(block.nType, block.GetBlockTime(), block.txMint.GetToAddress());
    if (!dbBlock.CreateCacheStateTrie(hashFork, hashPrevStateRoot, statusBlockRoot, ptrBlockState->mapBlockState, hashStateRoot))
    {
        StdLog("BlockBase", "Create block state root: Create cache state trie fail, prev: %s", block.hashPrev.GetHex().c_str());
        return nullptr;
    }
    return ptrBlockState;
}

bool CBlockBase::UpdateBlockState(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block,
                                  const std::map<CDestination, CAddressContext>& mapAddressContext,
                                  uint256& hashNewStateRoot, SHP_BLOCK_STATE& ptrBlockStateOut)
{
    uint256 hashStateRoot;
    uint256 hashPrevStateRoot;
    uint32 nPrevBlockTime = 0;
    uint256 nTotalMintReward;
    uint256 nBlockGasUsed;
    uint256 hashReceiptRoot;
    uint256 hashCrosschainMerkleRoot;
    bytes btBloomDataTemp;

    if (block.hashPrev != 0)
    {
        CBlockIndex* pIndexPrev = GetIndex(block.hashPrev);
        if (pIndexPrev == nullptr)
        {
            StdLog("BlockBase", "Update block state: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        if (!block.IsOrigin())
        {
            hashPrevStateRoot = pIndexPrev->GetStateRoot();
        }
        nPrevBlockTime = pIndexPrev->GetBlockTime();
    }

    bool fMoStatus = false;
    SHP_BLOCK_STATE ptrBlockState = CreateBlockStateRoot(hashFork, block, hashPrevStateRoot, nPrevBlockTime, hashStateRoot, hashReceiptRoot,
                                                         hashCrosschainMerkleRoot, nBlockGasUsed, btBloomDataTemp, nTotalMintReward, fMoStatus, mapAddressContext);
    if (!ptrBlockState)
    {
        StdLog("BlockBase", "Update block state: Create block state root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (block.hashStateRoot != 0 && block.hashStateRoot != hashStateRoot)
    {
        StdLog("BlockBase", "Update block state: Create state root error, block state root: %s, calc state root: %s, prev state root: %s, block: [type: %d] %s",
               block.hashStateRoot.GetHex().c_str(), hashStateRoot.GetHex().c_str(),
               hashPrevStateRoot.GetHex().c_str(), block.nType, hashBlock.GetHex().c_str());
        return false;
    }
    if (block.hashReceiptsRoot != hashReceiptRoot)
    {
        StdLog("BlockBase", "Update block state: Create receipt root error, block receipt root: %s, calc receipt root: %s, block: %s",
               block.hashReceiptsRoot.GetHex().c_str(), hashReceiptRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (block.hashCrosschainMerkleRoot != hashCrosschainMerkleRoot)
    {
        StdLog("BlockBase", "Update block state: Create receipt root error, block crosschain merkle root: %s, calc crosschain merkle root: %s, block: %s",
               block.hashCrosschainMerkleRoot.GetHex().c_str(), hashCrosschainMerkleRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock = MAKE_SHARED_MERKLE_PROVE_DATA();
    SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock = MAKE_SHARED_MERKLE_PROVE_DATA();
    SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve = MAKE_SHARED_MERKLE_PROVE_DATA();
    if (!ptrMerkleProvePrevBlock || !ptrMerkleProveRefBlock || !ptrCrossMerkleProve
        || !block.GetMerkleProve(ptrMerkleProvePrevBlock, ptrMerkleProveRefBlock, ptrCrossMerkleProve))
    {
        StdLog("BlockBase", "Update block state: Get block prove fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    ptrBlockState->UpdateCrosschainProveTail(block.hashPrev, *ptrMerkleProvePrevBlock, block.GetRefBlock(), *ptrMerkleProveRefBlock, *ptrCrossMerkleProve);

    CBlockRootStatus statusBlockRoot(block.nType, block.GetBlockTime(), block.txMint.GetToAddress());
    if (!dbBlock.AddBlockState(hashFork, hashPrevStateRoot, statusBlockRoot, ptrBlockState->mapBlockState, hashStateRoot))
    {
        StdLog("BlockBase", "Update block state: Add block state fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (block.hashStateRoot != 0 && block.hashStateRoot != hashStateRoot)
    {
        StdLog("BlockBase", "Update block state: Add state root error, block state root: %s, calc state  root: %s, block: %s",
               block.hashStateRoot.GetHex().c_str(), hashStateRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    hashNewStateRoot = hashStateRoot;
    ptrBlockStateOut = ptrBlockState;
    return true;
}

bool CBlockBase::UpdateBlockTxIndex(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset, const std::vector<CTransactionReceipt>& vBlockTxReceipts, uint256& hashNewRoot)
{
    CBufStream ss;
    std::map<uint256, CTxIndex> mapBlockTxIndex;

    uint32 nTxOffset = nOffset + block.GetTxSerializedOffset();

    mapBlockTxIndex.insert(make_pair(block.txMint.GetHash(), CTxIndex(block.GetBlockNumber(), 0, nFile, nTxOffset)));
    nTxOffset += ss.GetSerializeSize(block.txMint);

    CVarInt var(block.vtx.size());
    nTxOffset += ss.GetSerializeSize(var);

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        auto& tx = block.vtx[i];
        mapBlockTxIndex.insert(make_pair(tx.GetHash(), CTxIndex(block.GetBlockNumber(), i + 1, nFile, nTxOffset)));
        nTxOffset += ss.GetSerializeSize(tx);
    }

    if (!dbBlock.AddBlockTxIndexReceipt(hashFork, hashBlock, mapBlockTxIndex, vBlockTxReceipts))
    {
        StdLog("BlockBase", "Update block tx index: Add block tx index fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    blockReceiptCache.AddBlockReceiptCache(hashBlock, vBlockTxReceipts);
    return true;
}

bool CBlockBase::UpdateBlockAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block,
                                          const std::map<uint256, std::vector<CContractTransfer>>& mapContractTransferIn,
                                          const std::map<uint256, uint256>& mapBlockTxFeeUsedIn, const std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedIn, uint256& hashNewRoot)
{
    std::map<CDestination, std::vector<CDestTxInfo>> mapAddressTxInfo;

    auto funcAddTxInfo = [&](const CTransaction& tx) {
        const uint256 txid = tx.GetHash();

        if (!tx.IsMintTx())
        {
            uint256 nTxFeeUsed;
            auto it = mapBlockTxFeeUsedIn.find(txid);
            if (it != mapBlockTxFeeUsedIn.end())
            {
                nTxFeeUsed = it->second;
            }

            if (!tx.GetFromAddress().IsNull() && tx.GetFromAddress() == tx.GetToAddress())
            {
                CDestTxInfo destTxInfoFrom;
                destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_INOUT;
                destTxInfoFrom.nBlockNumber = block.nNumber;
                destTxInfoFrom.txid = txid;
                destTxInfoFrom.nTxType = tx.GetTxType();
                destTxInfoFrom.nTimeStamp = block.GetBlockTime();
                destTxInfoFrom.destPeer = tx.GetToAddress();
                destTxInfoFrom.nAmount = tx.GetAmount();
                destTxInfoFrom.nTxFee = nTxFeeUsed;

                mapAddressTxInfo[tx.GetFromAddress()].push_back(destTxInfoFrom);
            }
            else
            {
                if (!tx.GetFromAddress().IsNull())
                {
                    CDestTxInfo destTxInfoFrom;
                    destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_OUT;
                    destTxInfoFrom.nBlockNumber = block.nNumber;
                    destTxInfoFrom.txid = txid;
                    destTxInfoFrom.nTxType = tx.GetTxType();
                    destTxInfoFrom.nTimeStamp = block.GetBlockTime();
                    destTxInfoFrom.destPeer = tx.GetToAddress();
                    destTxInfoFrom.nAmount = tx.GetAmount();
                    destTxInfoFrom.nTxFee = nTxFeeUsed;

                    mapAddressTxInfo[tx.GetFromAddress()].push_back(destTxInfoFrom);
                }
                if (!tx.GetToAddress().IsNull())
                {
                    CDestTxInfo destTxInfoTo;
                    destTxInfoTo.nDirection = CDestTxInfo::DXI_DIRECTION_IN;
                    destTxInfoTo.nBlockNumber = block.nNumber;
                    destTxInfoTo.txid = txid;
                    destTxInfoTo.nTxType = tx.GetTxType();
                    destTxInfoTo.nTimeStamp = block.GetBlockTime();
                    destTxInfoTo.destPeer = tx.GetFromAddress();
                    destTxInfoTo.nAmount = tx.GetAmount();
                    destTxInfoTo.nTxFee = 0; // When nDirection is DXI_DIRECTION_IN, nTxFee is equal to 0

                    mapAddressTxInfo[tx.GetToAddress()].push_back(destTxInfoTo);
                }
            }
        }

        auto mt = mapContractTransferIn.find(txid);
        if (mt != mapContractTransferIn.end())
        {
            for (const CContractTransfer& vd : mt->second)
            {
                if (!vd.destFrom.IsNull() && vd.destFrom == vd.destTo)
                {
                    CDestTxInfo destTxInfoFrom;
                    destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_CINOUT;
                    destTxInfoFrom.nBlockNumber = block.nNumber;
                    destTxInfoFrom.txid = txid;
                    destTxInfoFrom.nTxType = tx.GetTxType();
                    destTxInfoFrom.nTimeStamp = block.GetBlockTime();
                    destTxInfoFrom.destPeer = vd.destTo;
                    destTxInfoFrom.nAmount = vd.nAmount;
                    destTxInfoFrom.nTxFee = 0;

                    mapAddressTxInfo[vd.destFrom].push_back(destTxInfoFrom);
                }
                else
                {
                    if (!vd.destFrom.IsNull())
                    {
                        CDestTxInfo destTxInfoFrom;
                        destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_COUT;
                        destTxInfoFrom.nBlockNumber = block.nNumber;
                        destTxInfoFrom.txid = txid;
                        destTxInfoFrom.nTxType = tx.GetTxType();
                        destTxInfoFrom.nTimeStamp = block.GetBlockTime();
                        destTxInfoFrom.destPeer = vd.destTo;
                        destTxInfoFrom.nAmount = vd.nAmount;
                        destTxInfoFrom.nTxFee = 0;

                        mapAddressTxInfo[vd.destFrom].push_back(destTxInfoFrom);
                    }
                    if (!vd.destTo.IsNull())
                    {
                        CDestTxInfo destTxInfoTo;
                        destTxInfoTo.nDirection = CDestTxInfo::DXI_DIRECTION_CIN;
                        destTxInfoTo.nBlockNumber = block.nNumber;
                        destTxInfoTo.txid = txid;
                        destTxInfoTo.nTxType = tx.GetTxType();
                        destTxInfoTo.nTimeStamp = block.GetBlockTime();
                        destTxInfoTo.destPeer = vd.destFrom;
                        destTxInfoTo.nAmount = vd.nAmount;
                        destTxInfoTo.nTxFee = 0;

                        mapAddressTxInfo[vd.destTo].push_back(destTxInfoTo);
                    }
                }
            }
        }

        auto kt = mapBlockCodeDestFeeUsedIn.find(txid);
        if (kt != mapBlockCodeDestFeeUsedIn.end())
        {
            for (const auto& kv : kt->second)
            {
                CDestTxInfo destTxInfo;
                destTxInfo.nDirection = CDestTxInfo::DXI_DIRECTION_CODEREWARD;
                destTxInfo.nBlockNumber = block.nNumber;
                destTxInfo.txid = txid;
                destTxInfo.nTxType = tx.GetTxType();
                destTxInfo.nTimeStamp = block.GetBlockTime();
                destTxInfo.destPeer = tx.GetFromAddress();
                destTxInfo.nAmount = kv.second;
                destTxInfo.nTxFee = 0;

                mapAddressTxInfo[kv.first].push_back(destTxInfo);

                StdDebug("BlockBase", "Update block address tx info: code reward: dest: %s, amount: %s, block: %s",
                         kv.first.ToString().c_str(), CoinToTokenBigFloat(kv.second).c_str(), hashBlock.GetHex().c_str());
            }
        }
    };

    funcAddTxInfo(block.txMint);
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        if (!block.vtx[i].IsCertTx())
        {
            funcAddTxInfo(block.vtx[i]);
        }
    }

    if (!dbBlock.AddAddressTxInfo(hashFork, block.hashPrev, hashBlock, block.nNumber, mapAddressTxInfo, hashNewRoot))
    {
        StdLog("BlockBase", "Update block address tx info: Add block address tx info fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<CDestination, CAddressContext>& mapAddressContextIn, const std::map<CDestination, CDestState>& mapBlockStateIn,
                                    const std::map<CDestination, uint256>& mapBlockPayTvFeeIn, const std::map<uint32, CFunctionAddressContext>& mapBlockFunctionAddressIn, uint256& hashNewRoot)
{
    CDestination destGenesisMintAddress;
    CBlockIndex* pIndex = GetIndex(hashFork);
    if (pIndex)
    {
        destGenesisMintAddress = pIndex->destMint;
    }

    uint64 nNewAddressCount = 0;
    std::map<CDestination, CAddressContext> mapAddAddress;
    for (auto& kv : mapAddressContextIn)
    {
        // If there is address data in the DB and the address type is the same, there is no need to store it again.
        // If there is address data in the DB, but the address type is different, it needs to be re stored to change the address data.
        bool fExist = false;
        if (block.hashPrev != 0)
        {
            CAddressContext ctxAddress;
            if (RetrieveAddressContext(hashFork, block.hashPrev, kv.first, ctxAddress))
            {
                if (ctxAddress.nType == kv.second.nType)
                {
                    continue;
                }
                fExist = true;
            }
        }
        if (kv.second.IsContract())
        {
            CContractAddressContext ctxContract;
            if (!kv.second.GetContractAddressContext(ctxContract))
            {
                StdLog("BlockBase", "Update Block Address: Get contract address context fail, address: %s, block: %s", kv.first.ToString().c_str(), hashBlock.GetHex().c_str());
                continue;
            }
            if (ctxContract.hashContractRunCode == 0)
            {
                StdLog("BlockBase", "Update Block Address: Contract run code is null, address: %s, block: %s", kv.first.ToString().c_str(), hashBlock.GetHex().c_str());
                continue;
            }
        }
        mapAddAddress.insert(kv);
        if (!fExist)
        {
            nNewAddressCount++;
        }
    }

    // Settlement time vault
    const uint64 nBlockTime = block.GetBlockTime();
    std::map<CDestination, CTimeVault> mapTimeVault;
    for (auto& kv : mapBlockStateIn)
    {
        const CDestination& dest = kv.first;
        const CDestState& state = kv.second;

        if (!destGenesisMintAddress.IsNull() && destGenesisMintAddress == dest)
        {
            continue;
        }
        auto it = mapAddressContextIn.find(dest);
        if (it == mapAddressContextIn.end() || !it->second.IsPubkey())
        {
            // Only settle time vault for public key addresses
            continue;
        }

        CTimeVault tv;
        if (!dbBlock.RetrieveTimeVault(hashFork, block.hashPrev, dest, tv))
        {
            tv.SetNull();
        }
        tv.SettlementTimeVault(nBlockTime);
        tv.ModifyBalance(state.GetBalance());

        // StdDebug("TEST", "Update Block Address: Settlement time vault, tv: %s%s, balance: %s, dest: %s, block: %s",
        //          (tv.fSurplus ? "" : "-"), CoinToTokenBigFloat(tv.nTvAmount).c_str(), CoinToTokenBigFloat(tv.nBalanceAmount).c_str(),
        //          dest.ToString().c_str(), hashBlock.GetHex().c_str());

        mapTimeVault.insert(std::make_pair(dest, tv));
    }
    for (auto& kv : mapBlockPayTvFeeIn)
    {
        if (kv.second > 0)
        {
            const CDestination& dest = kv.first;

            if (!destGenesisMintAddress.IsNull() && destGenesisMintAddress == dest)
            {
                continue;
            }
            auto nt = mapAddressContextIn.find(dest);
            if (nt == mapAddressContextIn.end() || !nt->second.IsPubkey())
            {
                // Only settle time vault for public key addresses
                // StdDebug("TEST", "Update Block Address: Pay time vault, find address context fail, find ret: %s, dest: %s, block: %s",
                //          (nt == mapAddressContextIn.end() ? "false" : "true"), dest.ToString().c_str(), hashBlock.GetHex().c_str());
                continue;
            }

            auto it = mapTimeVault.find(dest);
            if (it == mapTimeVault.end())
            {
                CTimeVault tv;
                if (!dbBlock.RetrieveTimeVault(hashFork, block.hashPrev, dest, tv))
                {
                    tv.SetNull();
                }
                tv.SettlementTimeVault(nBlockTime);
                it = mapTimeVault.insert(std::make_pair(dest, tv)).first;
            }
            it->second.PayTvGasFee(kv.second);

            // StdDebug("TEST", "Update Block Address: Pay time vault, tv: %s%s, balance: %s, dest: %s, block: %s",
            //          (it->second.fSurplus ? "" : "-"), CoinToTokenBigFloat(it->second.nTvAmount).c_str(), CoinToTokenBigFloat(it->second.nBalanceAmount).c_str(),
            //          dest.ToString().c_str(), hashBlock.GetHex().c_str());
        }
    }

    // Vote reward give time vault
    for (auto& tx : block.vtx)
    {
        if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
        {
            const CDestination& dest = tx.GetToAddress();

            if (!destGenesisMintAddress.IsNull() && destGenesisMintAddress == dest)
            {
                continue;
            }
            auto it = mapAddressContextIn.find(dest);
            if (it == mapAddressContextIn.end() || !it->second.IsPubkey())
            {
                // Only settle time vault for public key addresses
                continue;
            }

            auto mt = mapTimeVault.find(dest);
            if (mt == mapTimeVault.end())
            {
                CTimeVault tv;
                if (!dbBlock.RetrieveTimeVault(hashFork, block.hashPrev, dest, tv))
                {
                    tv.SetNull();
                }
                tv.SettlementTimeVault(nBlockTime);
                mt = mapTimeVault.insert(std::make_pair(dest, tv)).first;
            }
            mt->second.PayTvGasFee(CTimeVault::CalcGiveTvFee(tx.GetAmount()));

            // StdDebug("TEST", "Update Block Address: Reward give time vault, tv: %s%s, balance: %s, give tv: %s, dest: %s, block: %s",
            //          (mt->second.fSurplus ? "" : "-"), CoinToTokenBigFloat(mt->second.nTvAmount).c_str(), CoinToTokenBigFloat(mt->second.nBalanceAmount).c_str(),
            //          CoinToTokenBigFloat(CTimeVault::CalcGiveTvFee(tx.GetAmount())).c_str(), dest.ToString().c_str(), hashBlock.GetHex().c_str());
        }
    }

    std::map<CDestination, uint384> mapBlsPubkeyContext;
    if (hashFork == hashGenesisBlock)
    {
        for (auto& tx : block.vtx)
        {
            if (tx.IsCertTx())
            {
                uint384 blsPubkey;
                if (!dbBlock.RetrieveBlsPubkeyContext(hashFork, block.hashPrev, tx.GetToAddress(), blsPubkey)
                    && mapBlsPubkeyContext.count(tx.GetToAddress()) == 0)
                {
                    bytes btKeyData;
                    if (tx.GetTxData(CTransaction::DF_BLSPUBKEY, btKeyData))
                    {
                        try
                        {
                            CBufStream ss(btKeyData);
                            ss >> blsPubkey;
                        }
                        catch (std::exception& e)
                        {
                            StdLog("BlockBase", "Update Block Address: bls pubkey error, block: %s, txid: %s", hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
                            continue;
                        }
                        mapBlsPubkeyContext[tx.GetToAddress()] = blsPubkey;
                    }
                }
            }
        }
    }

    if (!dbBlock.AddAddressContext(hashFork, block.hashPrev, hashBlock, mapAddAddress, nNewAddressCount, mapTimeVault, mapBlockFunctionAddressIn, mapBlsPubkeyContext, hashNewRoot))
    {
        StdLog("BlockBase", "Update Block Address: Add address context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockCode(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nBlockOffset,
                                 const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCodeContextIn,
                                 const std::map<uint256, CContractRunCodeContext>& mapContractRunCodeContextIn,
                                 const std::map<CDestination, CAddressContext>& mapAddressContext, uint256& hashCodeRoot)
{
    std::map<uint256, CContractSourceCodeContext> mapSourceCode;
    std::map<uint256, CContractCreateCodeContext> mapContractCreateCode = mapContractCreateCodeContextIn;
    std::map<uint256, CContractRunCodeContext> mapContractRunCode;
    std::map<uint256, CTemplateContext> mapTemplateData;

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nOffset = nBlockOffset + block.GetTxSerializedOffset()
                     + ss.GetSerializeSize(block.txMint)
                     + ss.GetSerializeSize(var);
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        auto& tx = block.vtx[i];

        if (tx.GetToAddress().IsNull())
        {
            uint8 nCodeType;
            CTemplateContext ctxTemplate;
            CTxContractData ctxContract;
            if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
            {
                StdLog("CBlockState", "Update Block Code: Get create code fail, block: %s", hashBlock.GetHex().c_str());
                return false;
            }

            if (nCodeType == CODE_TYPE_TEMPLATE)
            {
                CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
                if (!ptr)
                {
                    StdLog("BlockBase", "Update Block Code: Import tempate data fail, block: %s", hashBlock.GetHex().c_str());
                    return false;
                }
                mapTemplateData[static_cast<uint256>(ptr->GetTemplateId())] = ctxTemplate;
            }
            else
            {
                if (ctxContract.IsUpcode())
                {
                    bool fLinkGenesisFork = false;
                    uint256 hashContractCreateCode = ctxContract.GetContractCreateCodeHash();
                    if (hashContractCreateCode != 0)
                    {
                        CContractCreateCodeContext ctxtCode;
                        bool fLinkGenesisFork;
                        if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, block.hashPrev, hashContractCreateCode, ctxtCode, fLinkGenesisFork))
                        {
                            if (mapContractCreateCode.find(hashContractCreateCode) == mapContractCreateCode.end())
                            {
                                mapContractCreateCode[hashContractCreateCode] = CContractCreateCodeContext(ctxContract.GetType(), ctxContract.GetName(), ctxContract.GetDescribe(),
                                                                                                           ctxContract.GetCodeOwner(), ctxContract.GetCode(), tx.GetHash(), uint256(), uint256());
                            }
                        }
                    }
                }
                uint256 hashSourceCode = ctxContract.GetSourceCodeHash();
                if (hashSourceCode != 0)
                {
                    auto it = mapSourceCode.find(hashSourceCode);
                    if (it == mapSourceCode.end())
                    {
                        CContractSourceCodeContext ctxtCode;
                        if (!dbBlock.RetrieveSourceCodeContext(hashFork, block.hashPrev, hashSourceCode, ctxtCode))
                        {
                            ctxtCode.Clear();
                        }
                        it = mapSourceCode.insert(make_pair(hashSourceCode, ctxtCode)).first;
                    }
                    it->second.AddPos(ctxContract.GetName(), ctxContract.GetCodeOwner(), nFile, nOffset);
                }
            }
        }

        nOffset += ss.GetSerializeSize(tx);
    }

    for (const auto& kv : mapContractRunCodeContextIn)
    {
        CContractRunCodeContext ctxtCode;
        if (!dbBlock.RetrieveContractRunCodeContext(hashFork, block.hashPrev, kv.first, ctxtCode)
            || ctxtCode.hashContractCreateCode != kv.second.hashContractCreateCode
            || ctxtCode.btContractRunCode != kv.second.btContractRunCode)
        {
            mapContractRunCode[kv.first] = kv.second;
        }
    }

    if (!dbBlock.AddCodeContext(hashFork, block.hashPrev, hashBlock, mapSourceCode,
                                mapContractCreateCode, mapContractRunCode, mapTemplateData, hashCodeRoot))
    {
        StdLog("BlockBase", "Update Block Code: Add code context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewRoot)
{
    std::map<CDestination, uint256> mapVoteReward;
    for (const auto& tx : block.vtx)
    {
        if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
        {
            mapVoteReward[tx.GetToAddress()] += tx.GetAmount();
        }
    }

    if (!dbBlock.AddVoteReward(hashFork, nChainId, block.hashPrev, hashBlock, block.GetBlockHeight(), mapVoteReward, hashNewRoot))
    {
        StdLog("BlockBase", "Update block vote reward: Add vote reward fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockDexOrder(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const CBlockEx& block, const std::map<CDexOrderHeader, CDexOrderBody>& mapDexOrder,
                                     const CBlockStorageProve& proveBlockCrosschain, const std::map<uint256, uint256>& mapCoinPairCompletePrice, std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>>& mapCompDexOrderRecord, uint256& hashNewRoot)
{
    std::set<CChainId> setPeerCrossChainId;
    std::map<CChainId, std::vector<CBlockCoinTransferProve>> mapCrossTransferProve;
    for (const auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        const CChainId nPeerChainId = kv.first;
        const CBlockCrosschainProve& crossProve = kv.second.first;

        if (crossProve.GetPrevProveBlock() == 0)
        {
            StdLog("BlockBase", "Update block dex order: Prev prove block is 0, peer chainid: %d, block: %s", nPeerChainId, hashBlock.GetBhString().c_str());
            return false;
        }

        setPeerCrossChainId.insert(nPeerChainId);
        if (!crossProve.GetCoinTransferProve().empty())
        {
            mapCrossTransferProve.insert(std::make_pair(nPeerChainId, crossProve.GetCoinTransferProve()));
        }
    }
    if (!dbBlock.AddDexOrder(hashFork, hashRefBlock, hashPrevBlock, hashBlock, mapDexOrder, mapCrossTransferProve, mapCoinPairCompletePrice, setPeerCrossChainId, mapCompDexOrderRecord, block.mapProve, hashNewRoot))
    {
        StdLog("BlockBase", "Update block dex order: Add dex order fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    if (!dbBlock.AddBlockCrosschainProve(hashBlock, proveBlockCrosschain))
    {
        StdLog("BlockBase", "Update block dex order: Add crosschain prove fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    return dbBlock.RetrieveContractKvValue(hashFork, hashContractRoot, key, value);
}

bool CBlockBase::AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState)
{
    return dbBlock.AddBlockContractKvValue(hashFork, hashPrevRoot, hashContractRoot, mapContractState);
}

bool CBlockBase::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    return dbBlock.RetrieveAddressContext(hashFork, hashBlock, dest, ctxAddress);
}

bool CBlockBase::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    return dbBlock.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockBase::RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    return dbBlock.RetrieveTimeVault(hashFork, hashBlock, dest, tv);
}

bool CBlockBase::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return dbBlock.GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CBlockBase::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    uint256 hashLastBlock;
    if (hashBlock == 0)
    {
        if (!dbBlock.RetrieveForkLast(hashGenesisBlock, hashLastBlock))
        {
            return false;
        }
    }
    else
    {
        hashLastBlock = hashBlock;
    }

    if (!dbBlock.ListFunctionAddress(hashGenesisBlock, hashLastBlock, mapFunctionAddress))
    {
        return false;
    }

    if (mapFunctionAddress.find(FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS] = CFunctionAddressContext(PLEDGE_SURPLUS_REWARD_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_TIME_VAULT_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_TIME_VAULT_TO_ADDRESS] = CFunctionAddressContext(TIME_VAULT_TO_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS] = CFunctionAddressContext(PROJECT_PARTY_REWARD_TO_ADDRESS, false);
    }
    if (mapFunctionAddress.find(FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS) == mapFunctionAddress.end())
    {
        mapFunctionAddress[FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS] = CFunctionAddressContext(FOUNDATION_REWARD_TO_ADDRESS, false);
    }
    return true;
}

bool CBlockBase::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    if (!dbBlock.RetrieveFunctionAddress(hashGenesisBlock, hashBlock, nFuncId, ctxFuncAddress))
    {
        CDestination destDefFunction;
        if (!GetDefaultFunctionAddress(nFuncId, destDefFunction))
        {
            return false;
        }
        ctxFuncAddress.SetFunctionAddress(destDefFunction);
        ctxFuncAddress.SetDisableModify(false);
    }
    return true;
}

bool CBlockBase::GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction)
{
    switch (nFuncId)
    {
    case FUNCTION_ID_PLEDGE_SURPLUS_REWARD_ADDRESS:
        destDefFunction = PLEDGE_SURPLUS_REWARD_ADDRESS;
        break;
    case FUNCTION_ID_TIME_VAULT_TO_ADDRESS:
        destDefFunction = TIME_VAULT_TO_ADDRESS;
        break;
    case FUNCTION_ID_PROJECT_PARTY_REWARD_TO_ADDRESS:
        destDefFunction = PROJECT_PARTY_REWARD_TO_ADDRESS;
        break;
    case FUNCTION_ID_FOUNDATION_REWARD_TO_ADDRESS:
        destDefFunction = FOUNDATION_REWARD_TO_ADDRESS;
        break;
    default:
        return false;
    }
    return true;
}

bool CBlockBase::VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr)
{
    CAddressContext ctxNewAddress;
    if (!RetrieveAddressContext(hashGenesisBlock, hashBlock, destNewFunction, ctxNewAddress))
    {
        StdLog("CBlockState", "Verify new function address: Retrieve new function address context fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "Address context error";
        return false;
    }

    CDestination destDefFunc;
    if (!GetDefaultFunctionAddress(nFuncId, destDefFunc))
    {
        StdLog("CBlockState", "Verify new function address: Get default function address fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "Function id error";
        return false;
    }
    if (destFrom != destDefFunc)
    {
        StdLog("CBlockState", "Verify new function address: From address is not default function address, function id: %d, new function address: %s, default function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destDefFunc.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "From address is not default function address";
        return false;
    }

    std::map<uint32, CFunctionAddressContext> mapFunctionAddress;
    if (!ListFunctionAddress(hashBlock, mapFunctionAddress))
    {
        StdLog("CBlockState", "Verify new function address: List function address fail, function id: %d, new function address: %s, from: %s",
               nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
        strErr = "List function address fail";
        return false;
    }
    for (auto& kv : mapFunctionAddress)
    {
        if (kv.second.GetFunctionAddress() == destNewFunction)
        {
            StdLog("CBlockState", "Verify new function address: Function address already exists, function id: %d, new function address: %s, repeat function id: %d, from: %s",
                   nFuncId, destNewFunction.ToString().c_str(), kv.first, destFrom.ToString().c_str());
            strErr = "Function address already exists";
            return false;
        }
    }

    auto it = mapFunctionAddress.find(nFuncId);
    if (it != mapFunctionAddress.end())
    {
        if (it->second.IsDisableModify())
        {
            StdLog("CBlockState", "Verify new function address: Disable modify, function id: %d, new function address: %s, from: %s",
                   nFuncId, destNewFunction.ToString().c_str(), destFrom.ToString().c_str());
            strErr = "Disable modify";
            return false;
        }
    }
    return true;
}

bool CBlockBase::RetrieveBlsPubkeyContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint384& blsPubkey)
{
    return dbBlock.RetrieveBlsPubkeyContext(hashFork, hashBlock, dest, blsPubkey);
}

bool CBlockBase::CallContractCode(const bool fEthCall, const uint256& hashFork, const CChainId& chainId, const uint256& nAgreement, const uint32 nHeight, const CDestination& destMint, const uint256& nBlockGasLimit,
                                  const CDestination& from, const CDestination& to, const uint256& nGasPrice, const uint256& nGasLimit, const uint256& nAmount,
                                  const bytes& data, const uint64 nTimeStamp, const uint256& hashPrevBlock, const uint256& hashPrevStateRoot, const uint64 nPrevBlockTime, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    if (isFunctionContractAddress(to))
    {
        if (data.size() < 4)
        {
            StdLog("CBlockState", "Call contract code: data size error, data size: %lu", data.size());
            return false;
        }
        bytes btTxParam;
        if (data.size() > 4)
        {
            btTxParam.assign(data.begin() + 4, data.end());
        }
        bytes btFuncSign(data.begin(), data.begin() + 4);

        CBlockState blockState(*this, hashFork, hashPrevBlock, hashPrevStateRoot, nPrevBlockTime);
        CTransactionLogs logs;

        nGasLeft = nGasLimit.Get64();
        return blockState.DoFuncContractCall(from, to, btFuncSign, btTxParam, nGasLimit.Get64(), nGasLeft, logs, btResult);
    }

    if (fEthCall && to.IsNull())
    {
        CBlockState blockState(*this, hashFork, hashPrevBlock, hashPrevStateRoot, nPrevBlockTime);
        CContractHostDB dbHost(blockState, to, {}, 0, 0);
        CEvmExec vmExec(dbHost, hashFork, chainId, nAgreement);
        CTxContractData txcd;
        if (!vmExec.evmExec(from, to, to, {}, nGasLimit.Get64(),
                            nGasPrice, nAmount, destMint, nTimeStamp,
                            nHeight, nBlockGasLimit.Get64(), data, {}, txcd))
        {
            StdLog("BlockBase", "Call evm code: Exec fail1, to: %s", to.ToString().c_str());
            nGasLeft = vmExec.nGasLeft;
            nStatus = vmExec.nStatusCode;
            btResult = vmExec.vResult;
            return false;
        }
        nGasLeft = vmExec.nGasLeft;
        nStatus = vmExec.nStatusCode;
        btResult = vmExec.vResult;
        return true;
    }

    //if (!to.IsContract() || to.IsNullContract())
    if (to.IsNull())
    {
        StdLog("BlockBase", "Call contract code: to is null, to: %s", to.ToString().c_str());
        return false;
    }

    CAddressContext ctxAddress;
    if (!RetrieveAddressContext(hashFork, hashPrevBlock, to, ctxAddress))
    {
        StdLog("BlockBase", "Call contract code: Retrieve to address context fail, to: %s", to.ToString().c_str());
        return false;
    }
    if (!ctxAddress.IsContract())
    {
        StdLog("BlockBase", "Call contract code: to not is contract address, to: %s", to.ToString().c_str());
        return false;
    }

    CBlockState blockState(*this, hashFork, hashPrevBlock, hashPrevStateRoot, nPrevBlockTime);

    uint256 hashContractCreateCode;
    CDestination destCodeOwner;
    uint256 hashContractRunCode;
    bytes btContractRunCode;
    bool fDestroy = false;
    if (!blockState.GetContractRunCode(to, hashContractCreateCode, destCodeOwner, hashContractRunCode, btContractRunCode, fDestroy))
    {
        StdLog("BlockBase", "Call contract code: Get contract run code fail, to: %s", to.ToString().c_str());
        return false;
    }
    if (fDestroy)
    {
        nGasLeft = 0;
        nStatus = -2; //EVMC_REJECTED
        btResult = {};
        return true;
    }
    if (nAmount > 0)
    {
        CDestState state;
        if (!blockState.GetDestState(to, state))
        {
            state.SetNull();
            state.SetType(CDestination::PREFIX_CONTRACT);
        }
        state.IncBalance(nAmount);
        blockState.SetDestState(to, state);
    }

    // if (fEthCall)
    // {
    StdDebug("BlockBase", "Call evm code: from: %s", from.ToString().c_str());
    StdDebug("BlockBase", "Call evm code: to: %s", to.ToString().c_str());
    StdDebug("BlockBase", "Call evm code: gas limit: %lu", nGasLimit.Get64());

    CContractHostDB dbHost(blockState, to, destCodeOwner, 0, 1);
    CEvmExec vmExec(dbHost, hashFork, chainId, nAgreement);
    CTxContractData txcd;
    if (!vmExec.evmExec(from, to, to, destCodeOwner, nGasLimit.Get64(),
                        nGasPrice, nAmount, destMint, nTimeStamp,
                        nHeight, nBlockGasLimit.Get64(), btContractRunCode, data, txcd))
    {
        StdLog("BlockBase", "Call evm code: Exec fail2, to: %s", to.ToString().c_str());
        nGasLeft = vmExec.nGasLeft;
        nStatus = vmExec.nStatusCode;
        btResult = vmExec.vResult;
        return false;
    }
    nGasLeft = vmExec.nGasLeft;
    nStatus = vmExec.nStatusCode;
    btResult = vmExec.vResult;
    // }
    // else
    // {
    // CContractHostDB dbHost(blockState, to, destCodeOwner, 0, 1);
    // CContractRun contractRun(dbHost, hashFork);
    // CTxContractData txcd;
    // if (!contractRun.RunContract(from, to, to, destCodeOwner, nGasLimit.Get64(),
    //                              nGasPrice, nAmount, destMint, nTimeStamp,
    //                              nHeight, nBlockGasLimit.Get64(), btContractRunCode, data, txcd))
    // {
    //     StdLog("BlockBase", "Call contract code: Run contract fail, to: %s", to.ToString().c_str());
    // }
    // nGasLeft = contractRun.nGasLeft;
    // nStatus = contractRun.nStatusCode;
    // btResult = contractRun.vResult;

    //     nGasLeft = 0;
    //     nStatus = -2; //EVMC_REJECTED
    //     btResult = {};
    // }
    return true;
}

////////////////////////////////////////////////////////////////////////
CBlockIndex* CBlockBase::GetIndex(const uint256& hash) const
{
    map<uint256, CBlockIndex*>::const_iterator mi = mapIndex.find(hash);
    return (mi != mapIndex.end() ? (*mi).second : nullptr);
}

CBlockIndex* CBlockBase::GetForkLastIndex(const uint256& hashFork)
{
    uint256 hashLastBlock;
    if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        return nullptr;
    }
    return GetIndex(hashLastBlock);
}

CBlockIndex* CBlockBase::GetOrCreateIndex(const uint256& hash)
{
    map<uint256, CBlockIndex*>::const_iterator mi = mapIndex.find(hash);
    if (mi == mapIndex.end())
    {
        CBlockIndex* pIndexNew = new CBlockIndex();
        mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
        if (mi == mapIndex.end())
        {
            return nullptr;
        }
        pIndexNew->phashBlock = &((*mi).first);
    }
    return ((*mi).second);
}

CBlockIndex* CBlockBase::GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, vector<CBlockIndex*>& vPath)
{
    vPath.clear();
    while (pIndex != pIndexRef)
    {
        if (pIndexRef->GetBlockTime() > pIndex->GetBlockTime())
        {
            pIndexRef = pIndexRef->pPrev;
        }
        else if (pIndex->GetBlockTime() > pIndexRef->GetBlockTime())
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
        }
        else
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
            pIndexRef = pIndexRef->pPrev;
        }
    }
    return pIndex;
}

CBlockIndex* CBlockBase::GetOriginIndex(const uint256& txidMint)
{
    CReadLock rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbBlock.ListForkContext(mapForkCtxt))
    {
        return nullptr;
    }
    for (const auto& kv : mapForkCtxt)
    {
        CBlockIndex* pIndex = GetForkLastIndex(kv.first);
        if (pIndex && pIndex->txidMint == txidMint)
        {
            return pIndex;
        }
    }
    return nullptr;
}

void CBlockBase::UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, const uint64 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
{
    mapForkHeightIndex[hashFork].AddHeightIndex(CBlock::GetBlockHeightByHash(hashBlock), hashBlock, nBlockTimeStamp, destMint, hashRefBlock);
}

void CBlockBase::RemoveBlockIndex(const uint256& hashFork, const uint256& hashBlock)
{
    std::map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        it->second.RemoveHeightIndex(CBlock::GetBlockHeightByHash(hashBlock), hashBlock);
    }
    mapIndex.erase(hashBlock);
}

void CBlockBase::UpdateBlockRef(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock)
{
    std::map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        it->second.UpdateBlockRef(CBlock::GetBlockHeightByHash(hashBlock), hashBlock, hashRefBlock);
    }
}

bool CBlockBase::UpdateBlockLongChain(const uint256& hashFork, const std::vector<CBlockEx>& vBlockRemove, const std::vector<CBlockEx>& vBlockAddNew)
{
    std::vector<uint256> vRemoveTx;
    std::map<uint256, uint256> mapNewTx;
    for (const auto& blockex : vBlockAddNew)
    {
        const uint256 hashBlock = blockex.GetHash();
        mapNewTx.insert(std::make_pair(blockex.txMint.GetHash(), hashBlock));
        for (const auto& tx : blockex.vtx)
        {
            mapNewTx.insert(std::make_pair(tx.GetHash(), hashBlock));
        }
    }
    for (const auto& blockex : vBlockRemove)
    {
        uint256 txid = blockex.txMint.GetHash();
        if (mapNewTx.find(txid) == mapNewTx.end())
        {
            vRemoveTx.push_back(txid);
        }
        for (const auto& tx : blockex.vtx)
        {
            uint256 txid = tx.GetHash();
            if (mapNewTx.find(txid) == mapNewTx.end())
            {
                vRemoveTx.push_back(txid);
            }
        }
    }
    return dbBlock.UpdateBlockLongChain(hashFork, vRemoveTx, mapNewTx);
}

void CBlockBase::UpdateBlockNext(CBlockIndex* pIndexLast)
{
    if (pIndexLast != nullptr)
    {
        CBlockIndex* pIndexNext = pIndexLast;
        if (pIndexLast->pNext)
        {
            CBlockIndex* p = pIndexLast->pNext;
            while (p != nullptr)
            {
                p->pPrev->pNext = nullptr;
                p = p->pNext;
            }
            pIndexLast->pNext = nullptr;
        }
        while (!pIndexNext->IsOrigin() && pIndexNext->pPrev->pNext != pIndexNext)
        {
            CBlockIndex* pIndex = pIndexNext->pPrev;
            if (pIndex->pNext != nullptr)
            {
                CBlockIndex* p = pIndex->pNext;
                while (p != nullptr)
                {
                    p->pPrev->pNext = nullptr;
                    p = p->pNext;
                }
            }
            pIndex->pNext = pIndexNext;
            pIndexNext = pIndex;
        }
    }
}

CBlockIndex* CBlockBase::AddNewIndex(const uint256& hash, const CBlock& block, const uint32 nFile, const uint32 nOffset, const uint32 nCrc, const uint256& nChainTrust, const uint256& hashNewStateRoot)
{
    CBlockIndex* pIndexNew = nullptr;
    auto it = mapIndex.find(hash);
    if (it != mapIndex.end())
    {
        pIndexNew = it->second;
    }
    else
    {
        pIndexNew = new CBlockIndex(hash, block, nFile, nOffset, nCrc);
        if (pIndexNew != nullptr)
        {
            map<uint256, CBlockIndex*>::iterator mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
            pIndexNew->phashBlock = &((*mi).first);

            if (!block.GetBlockMint(pIndexNew->nMoneySupply))
            {
                StdError("BlockBase", "Add new index: Get block mint fail, block: %s", hash.GetHex().c_str());
                delete pIndexNew;
                pIndexNew = nullptr;
                return nullptr;
            }
            pIndexNew->nMoneyDestroy = block.GetBlockMoneyDestroy();
            pIndexNew->nChainTrust = nChainTrust;
            pIndexNew->nRandBeacon = block.GetBlockBeacon();
            pIndexNew->hashStateRoot = hashNewStateRoot;

            if (block.hashPrev != 0)
            {
                CBlockIndex* pIndexPrev = nullptr;
                map<uint256, CBlockIndex*>::iterator miPrev = mapIndex.find(block.hashPrev);
                if (miPrev != mapIndex.end())
                {
                    pIndexPrev = (*miPrev).second;
                    pIndexNew->pPrev = pIndexPrev;
                    if (pIndexNew->IsOrigin())
                    {
                        CProfile profile;
                        if (!block.GetForkProfile(profile))
                        {
                            StdError("BlockBase", "Add new index: Load proof fail, block: %s", hash.GetHex().c_str());
                            delete pIndexNew;
                            pIndexNew = nullptr;
                            return nullptr;
                        }
                        pIndexNew->nChainId = profile.nChainId;
                    }
                    else
                    {
                        pIndexNew->pOrigin = pIndexPrev->pOrigin;
                        pIndexNew->nChainId = (pIndexNew->pOrigin ? pIndexNew->pOrigin->nChainId : 0);
                        pIndexNew->nRandBeacon ^= pIndexNew->pOrigin->nRandBeacon;
                        pIndexNew->nMoneySupply += pIndexPrev->nMoneySupply;
                        pIndexNew->nMoneyDestroy += pIndexPrev->nMoneyDestroy;
                        pIndexNew->nTxCount += pIndexPrev->nTxCount;
                        pIndexNew->nRewardTxCount += pIndexPrev->nRewardTxCount;
                        pIndexNew->nUserTxCount += pIndexPrev->nUserTxCount;
                    }
                    pIndexNew->nChainTrust += pIndexPrev->nChainTrust;
                }
            }
            else
            {
                if (pIndexNew->IsOrigin())
                {
                    CProfile profile;
                    if (!block.GetForkProfile(profile))
                    {
                        StdError("BlockBase", "Add new index: Load proof fail, block: %s", hash.GetHex().c_str());
                        delete pIndexNew;
                        pIndexNew = nullptr;
                        return nullptr;
                    }
                    pIndexNew->nChainId = profile.nChainId;
                }
                else
                {
                    StdError("BlockBase", "Add new index: Prev is null, not origin block, block: %s", hash.GetHex().c_str());
                    delete pIndexNew;
                    pIndexNew = nullptr;
                    return nullptr;
                }
            }

            UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, block.GetBlockTime(), block.txMint.GetToAddress(), pIndexNew->GetRefBlock());
        }
    }
    return pIndexNew;
}

bool CBlockBase::LoadForkProfile(const CBlockIndex* pIndexOrigin, CProfile& profile)
{
    CForkContext ctxt;
    if (!RetrieveForkContext(pIndexOrigin->GetBlockHash(), ctxt))
    {
        return false;
    }
    profile = ctxt.GetProfile();
    return true;
}

bool CBlockBase::UpdateDelegate(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                                const uint256& nMinEnrollAmount, const std::map<CDestination, CAddressContext>& mapAddressContext,
                                const std::map<CDestination, CDestState>& mapAccStateIn, uint256& hashDelegateRoot)
{
    //StdTrace("BlockBase", "Update delegate: height: %d, block: %s", block.GetBlockHeight(), hashBlock.GetHex().c_str());

    std::map<CDestination, uint256> mapDelegateVote;
    std::map<int, std::map<CDestination, CDiskPos>> mapEnrollTx;

    uint256 hashPrevStateRoot;
    if (!block.IsOrigin() && block.hashPrev != 0)
    {
        CBlockIndex* pIndexPrev = GetIndex(block.hashPrev);
        if (pIndexPrev == nullptr)
        {
            StdLog("BlockBase", "Update delegate: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
    }

    auto addVote = [&](const CDestination& destDelegate, const uint256& nVoteAmount, const bool fAdd) -> bool {
        if (nVoteAmount == 0)
        {
            return true;
        }
        auto it = mapDelegateVote.find(destDelegate);
        if (it == mapDelegateVote.end())
        {
            uint256 nGetVoteAmount;
            if (!dbBlock.RetrieveDestDelegateVote(block.hashPrev, destDelegate, nGetVoteAmount))
            {
                nGetVoteAmount = 0;
            }
            it = mapDelegateVote.insert(make_pair(destDelegate, nGetVoteAmount)).first;
        }
        if (fAdd)
        {
            it->second += nVoteAmount;
        }
        else
        {
            it->second -= nVoteAmount;
        }
        return true;
    };

    for (auto& kv : mapAccStateIn)
    {
        const CDestination& dest = kv.first;
        const CDestState& state = kv.second;

        CAddressContext ctxAddress;
        auto it = mapAddressContext.find(dest);
        if (it == mapAddressContext.end())
        {
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                StdError("BlockBase", "Update delegate: Find address context fail, dest: %s", dest.ToString().c_str());
                return false;
            }
        }
        else
        {
            ctxAddress = it->second;
        }
        if (ctxAddress.IsTemplate()
            && (ctxAddress.GetTemplateType() == TEMPLATE_DELEGATE
                || ctxAddress.GetTemplateType() == TEMPLATE_VOTE
                || ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE))
        {
            CTemplateAddressContext ctxtTemplate;
            if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
            {
                StdLog("BlockBase", "Update delegate: Get template address context fail, dest: %s", dest.ToString().c_str());
                return false;
            }
            CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
            if (!ptr)
            {
                StdLog("BlockBase", "Update delegate: Import template fail, dest: %s", dest.ToString().c_str());
                return false;
            }
            CDestination destDelegate;
            uint8 nTidType = ptr->GetTemplateType();
            if (nTidType == TEMPLATE_DELEGATE)
            {
                destDelegate = dest;
            }
            else if (nTidType == TEMPLATE_VOTE)
            {
                if (!fEnableStakeVote)
                {
                    continue;
                }
                destDelegate = boost::dynamic_pointer_cast<CTemplateVote>(ptr)->destDelegate;
            }
            else if (nTidType == TEMPLATE_PLEDGE)
            {
                if (!fEnableStakePledge)
                {
                    continue;
                }
                destDelegate = boost::dynamic_pointer_cast<CTemplatePledge>(ptr)->destDelegate;
            }
            if (!destDelegate.IsNull())
            {
                uint256 nPrevBalance;
                CDestState statePrev;
                if (RetrieveDestState(hashFork, hashPrevStateRoot, dest, statePrev))
                {
                    if (statePrev.IsPubkey())
                    {
                        StdLog("BlockBase", "Update delegate: Prev state is pubkey, dest: %s, block: %s",
                               dest.ToString().c_str(), hashBlock.GetHex().c_str());
                        nPrevBalance = 0;
                    }
                    else
                    {
                        nPrevBalance = statePrev.GetBalance();
                    }
                }
                uint256 nChangeAmount;
                bool fAdd = false;
                if (state.GetBalance() > nPrevBalance)
                {
                    nChangeAmount = state.GetBalance() - nPrevBalance;
                    fAdd = true;
                }
                else
                {
                    nChangeAmount = nPrevBalance - state.GetBalance();
                }
                if (!addVote(destDelegate, nChangeAmount, fAdd))
                {
                    StdLog("BlockBase", "Update delegate: Add vote fail, dest: %s, block: %s",
                           dest.ToString().c_str(), hashBlock.GetHex().c_str());
                    return false;
                }
            }
        }
    }

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nSetOffset = nOffset + block.GetTxSerializedOffset()
                        + ss.GetSerializeSize(block.txMint)
                        + ss.GetSerializeSize(var);
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];

        if (tx.GetTxType() == CTransaction::TX_CERT)
        {
            const CDestination& destToDelegateTemplate = tx.GetToAddress();
            uint256 nDelegateVote;
            if (!dbBlock.RetrieveDestDelegateVote(block.hashPrev, destToDelegateTemplate, nDelegateVote))
            {
                StdLog("BlockBase", "Update delegate: TX CERT find from vote fail, delegate address: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), tx.GetHash().GetHex().c_str());
                continue;
            }
            if (nDelegateVote < nMinEnrollAmount)
            {
                StdLog("BlockBase", "Update delegate: TX CERT not enough votes, delegate address: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                continue;
            }

            int nCertAnchorHeight = tx.GetNonce();
            bytes btCertData;
            if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btCertData))
            {
                StdLog("BlockBase", "Update delegate: TX CERT tx data error1, delegate address: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destToDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            mapEnrollTx[nCertAnchorHeight].insert(make_pair(destToDelegateTemplate, CDiskPos(nFile, nSetOffset)));
            StdTrace("BlockBase", "Update delegate: Enroll cert tx, anchor height: %d, nAmount: %s, vote: %s, delegate address: %s, txid: %s",
                     nCertAnchorHeight, CoinToTokenBigFloat(tx.GetAmount()).c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(), destToDelegateTemplate.ToString().c_str(), tx.GetHash().GetHex().c_str());
        }
        nSetOffset += ss.GetSerializeSize(tx);
    }

    for (auto it = mapDelegateVote.begin(); it != mapDelegateVote.end(); ++it)
    {
        StdTrace("BlockBase", "Update delegate: delegate address: %s, votes: %s",
                 it->first.ToString().c_str(), CoinToTokenBigFloat(it->second).c_str());
    }

    if (!dbBlock.UpdateDelegateContext(block.hashPrev, hashBlock, mapDelegateVote, mapEnrollTx, hashDelegateRoot))
    {
        StdError("BlockBase", "Update delegate: Update delegate context failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block,
                            const std::map<CDestination, CAddressContext>& mapAddressContext, const std::map<CDestination, CDestState>& mapAccStateIn,
                            const std::map<CDestination, std::pair<uint32, uint32>>& mapBlockModifyPledgeFinalHeightIn, uint256& hashVoteRoot)
{
    uint256 hashPrevStateRoot;
    if (!block.IsOrigin() && block.hashPrev != 0)
    {
        CBlockIndex* pIndexPrev = GetIndex(block.hashPrev);
        if (pIndexPrev == nullptr)
        {
            StdLog("BlockBase", "Update vote: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pIndexPrev->GetStateRoot();
    }

    auto funcGetMintRewardAmount = [&](const CDestination& dest) -> uint256 {
        uint256 nMintReward;
        if (block.txMint.GetToAddress() == dest)
        {
            nMintReward += block.txMint.GetAmount();
        }
        for (const CTransaction& tx : block.vtx)
        {
            if (tx.IsRewardTx() && tx.GetToAddress() == dest)
            {
                nMintReward += tx.GetAmount();
            }
        }
        return nMintReward;
    };
    auto funcCheckNewVote = [&](const CDestination& dest) -> bool {
        const uint256 nMintReward = funcGetMintRewardAmount(dest);

        auto it = mapAccStateIn.find(dest);
        if (it == mapAccStateIn.end())
        {
            return false;
        }
        const uint256 nNewBalance = it->second.GetBalance();

        uint256 nPrevBalance;
        CDestState statePrev;
        if (RetrieveDestState(hashFork, hashPrevStateRoot, dest, statePrev))
        {
            nPrevBalance = statePrev.GetBalance();
        }

        if (nNewBalance > nPrevBalance + nMintReward)
        {
            return true;
        }
        return false;
    };
    auto funcGetAddressContext = [&](const CDestination& dest, CAddressContext& ctxAddress) -> bool {
        auto it = mapAddressContext.find(dest);
        if (it != mapAddressContext.end())
        {
            ctxAddress = it->second;
        }
        else
        {
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                return false;
            }
        }
        return true;
    };

    std::map<CDestination, CVoteContext> mapBlockVote;
    std::map<CDestination, std::pair<uint32, uint32>> mapAddPledgeFinalHeight; // key: pledge address, value first: final height, value second: pledge height
    for (auto& kv : mapAccStateIn)
    {
        const CDestination& dest = kv.first;
        const CDestState& state = kv.second;

        CAddressContext ctxAddress;
        if (!funcGetAddressContext(dest, ctxAddress))
        {
            StdError("BlockBase", "Update vote: Get address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }

        if (ctxAddress.IsTemplate()
            && (ctxAddress.GetTemplateType() == TEMPLATE_DELEGATE
                || ctxAddress.GetTemplateType() == TEMPLATE_VOTE
                || ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE))
        {
            uint256 nPrevVoteAmount;
            CVoteContext ctxVote;
            if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, dest, ctxVote))
            {
                if (ctxAddress.GetTemplateType() == TEMPLATE_VOTE)
                {
                    if (!fEnableStakeVote)
                    {
                        continue;
                    }
                    CTemplateAddressContext ctxTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                    {
                        StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                    if (!ptr || ptr->GetTemplateType() != TEMPLATE_VOTE)
                    {
                        StdError("BlockBase", "Update vote: Import vote template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
                    ctxVote.destDelegate = objVote->destDelegate;
                    ctxVote.destOwner = objVote->destOwner;
                    ctxVote.nRewardMode = objVote->nRewardMode;
                    ctxVote.nRewardRate = PLEDGE_REWARD_PER;
                    ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                }
                else if (ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE)
                {
                    if (!fEnableStakePledge)
                    {
                        continue;
                    }
                    CTemplateAddressContext ctxTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                    {
                        StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                    if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
                    {
                        StdError("BlockBase", "Update vote: Import pledge template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
                    ctxVote.destDelegate = objPledge->destDelegate;
                    ctxVote.destOwner = objPledge->destOwner;
                    ctxVote.nRewardMode = CVoteContext::REWARD_MODE_OWNER;
                    ctxVote.nRewardRate = objPledge->GetRewardRate(block.GetBlockHeight());
                    if (ctxVote.nRewardRate == 0)
                    {
                        StdLog("BlockBase", "Update vote: Get pledge reward rate fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        continue;
                    }
                    if (!objPledge->GetPledgeFinalHeight(block.GetBlockHeight(), ctxVote.nFinalHeight))
                    {
                        StdLog("BlockBase", "Update vote: Get pledge final height fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                        continue;
                    }
                    mapAddPledgeFinalHeight[dest] = std::make_pair(ctxVote.nFinalHeight, block.GetBlockHeight());
                }
                else
                {
                    ctxVote.destDelegate = dest;
                    ctxVote.destOwner = ctxVote.destDelegate;
                    ctxVote.nRewardMode = CVoteContext::REWARD_MODE_VOTE;
                    ctxVote.nRewardRate = PLEDGE_REWARD_PER;
                    ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                }
            }
            else
            {
                nPrevVoteAmount = ctxVote.nVoteAmount;
                if (ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE)
                {
                    if (ctxVote.nFinalHeight > 0 && block.GetBlockHeight() >= ctxVote.nFinalHeight
                        && state.GetBalance() > nPrevVoteAmount && state.GetBalance() >= DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
                    {
                        CTemplateAddressContext ctxTemplate;
                        if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
                        {
                            StdError("BlockBase", "Update vote: Get template address context failed, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            return false;
                        }
                        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
                        if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
                        {
                            StdError("BlockBase", "Update vote: Import pledge template fail or template type error, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            return false;
                        }
                        auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
                        ctxVote.nRewardRate = objPledge->GetRewardRate(block.GetBlockHeight());
                        if (ctxVote.nRewardRate == 0)
                        {
                            StdLog("BlockBase", "Update vote: Get pledge reward rate fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            continue;
                        }
                        if (!objPledge->GetPledgeFinalHeight(block.GetBlockHeight(), ctxVote.nFinalHeight))
                        {
                            StdLog("BlockBase", "Update vote: Get pledge final height fail, dest: %s, block: %s", dest.ToString().c_str(), hashBlock.ToString().c_str());
                            continue;
                        }
                        mapAddPledgeFinalHeight[dest] = std::make_pair(ctxVote.nFinalHeight, block.GetBlockHeight());
                    }
                }
                else
                {
                    if (funcCheckNewVote(dest))
                    {
                        ctxVote.nFinalHeight = block.GetBlockHeight() + VOTE_REDEEM_HEIGHT;
                    }
                }
            }
            if (state.GetBalance() < DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
            {
                if (ctxVote.nVoteAmount != 0)
                {
                    ctxVote.nVoteAmount = 0;
                    mapBlockVote[dest] = ctxVote;
                }
            }
            else
            {
                ctxVote.nVoteAmount = state.GetBalance();
                mapBlockVote[dest] = ctxVote;
            }
        }
    }

    std::map<CDestination, uint32> mapRemovePledgeFinalHeight; // key: pledge address, value: old final height
    for (auto& kv : mapBlockModifyPledgeFinalHeightIn)
    {
        const auto& destPledge = kv.first;
        const uint32 nNewFinalHeight = kv.second.first;
        CVoteContext ctxVote;
        auto it = mapBlockVote.find(destPledge);
        if (it != mapBlockVote.end())
        {
            ctxVote = it->second;
        }
        else
        {
            if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, destPledge, ctxVote))
            {
                continue;
            }
        }
        if (ctxVote.nFinalHeight != nNewFinalHeight)
        {
            mapRemovePledgeFinalHeight[destPledge] = ctxVote.nFinalHeight;
            ctxVote.nFinalHeight = nNewFinalHeight;
            mapBlockVote[destPledge] = ctxVote;
            mapAddPledgeFinalHeight[destPledge] = kv.second;
        }
    }

    // key: pledge address, value first: final height, value second: pledge height
    for (auto& kv : mapAddPledgeFinalHeight)
    {
        StdDebug("BlockBase", "Update vote: Update address final height, address: %s, final height: %d, block: [%d] %s",
                 kv.first.ToString().c_str(), kv.second.first, CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());
    }

    if (!dbBlock.AddBlockVote(block.hashPrev, hashBlock, mapBlockVote, mapAddPledgeFinalHeight, mapRemovePledgeFinalHeight, hashVoteRoot))
    {
        StdError("BlockBase", "Update vote: Add block vote failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::IsValidBlock(CBlockIndex* pForkLast, const uint256& hashBlock)
{
    if (hashBlock != 0)
    {
        int nBlockHeight = CBlock::GetBlockHeightByHash(hashBlock);
        CBlockIndex* pIndex = pForkLast;
        while (pIndex && pIndex->GetBlockHeight() >= nBlockHeight)
        {
            if (pIndex->GetBlockHeight() == nBlockHeight && pIndex->GetBlockHash() == hashBlock)
            {
                return true;
            }
            pIndex = pIndex->pPrev;
        }
    }
    return false;
}

bool CBlockBase::VerifyValidBlock(CBlockIndex* pIndexGenesisLast, const CBlockIndex* pIndex)
{
    if (pIndex->IsOrigin())
    {
        return true;
    }

    uint256 hashRefBlock;
    auto it = mapForkHeightIndex.find(pIndex->GetOriginHash());
    if (it == mapForkHeightIndex.end())
    {
        return false;
    }
    std::map<uint256, CBlockHeightIndex>* pHeightIndex = it->second.GetBlockMintList(pIndex->GetBlockHeight());
    if (pHeightIndex)
    {
        auto mt = pHeightIndex->find(pIndex->GetBlockHash());
        if (mt != pHeightIndex->end() && mt->second.hashRefBlock != 0)
        {
            hashRefBlock = mt->second.hashRefBlock;
        }
    }
    if (hashRefBlock == 0)
    {
        CBlockEx block;
        if (!Retrieve(pIndex, block))
        {
            return false;
        }
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
        {
            hashRefBlock = proof.hashRefBlock;
        }
        if (hashRefBlock == 0)
        {
            return false;
        }
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

bool CBlockBase::VerifyBlockConfirmChain(const CBlockIndex* pNewIndex)
{
    if (!pNewIndex)
    {
        return false;
    }
    const uint256 hashFork = pNewIndex->GetOriginHash();
    const CBlockIndex* pIndex = pNewIndex;
    while (pIndex && !pIndex->IsOrigin())
    {
        if (pIndex->pNext && VerifyBlockConfirmNoLock(hashFork, pIndex->pNext->GetBlockHash()))
        {
            return false;
        }
        if (VerifyBlockConfirmNoLock(hashFork, pIndex->GetBlockHash()))
        {
            break;
        }
        pIndex = pIndex->pPrev;
    }
    return true;
}

bool CBlockBase::VerifyBlockConfirmNoLock(const uint256& hashFork, const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        return false;
    }
    if (hashBlock == hashFork)
    {
        return true;
    }
    uint256 hashLastConfirmBlock;
    if (!dbBlock.GetLastConfirmBlock(hashFork, hashLastConfirmBlock))
    {
        return false;
    }
    if (!VerifySameChainNoLock(hashBlock, hashLastConfirmBlock))
    {
        return false;
    }
    return true;
}

bool CBlockBase::VerifySameChainNoLock(const uint256& hashPrevBlock, const uint256& hashAfterBlock)
{
    CBlockIndex* pPrevIndex = GetIndex(hashPrevBlock);
    if (pPrevIndex == nullptr)
    {
        return false;
    }
    CBlockIndex* pAfterIndex = GetIndex(hashAfterBlock);
    if (pAfterIndex == nullptr)
    {
        return false;
    }
    while (pAfterIndex->GetBlockHeight() >= pPrevIndex->GetBlockHeight())
    {
        if (pAfterIndex == pPrevIndex)
        {
            return true;
        }
        pAfterIndex = pAfterIndex->pPrev;
    }
    return false;
}

CBlockIndex* CBlockBase::GetLongChainLastBlock(const uint256& hashFork, int nStartHeight, CBlockIndex* pIndexGenesisLast, const std::set<uint256>& setInvalidHash)
{
    auto it = mapForkHeightIndex.find(hashFork);
    if (it == mapForkHeightIndex.end())
    {
        return nullptr;
    }
    CForkHeightIndex& indexHeight = it->second;
    CBlockIndex* pMaxTrustIndex = nullptr;
    while (1)
    {
        std::map<uint256, CBlockHeightIndex>* pHeightIndex = indexHeight.GetBlockMintList(nStartHeight);
        if (pHeightIndex == nullptr)
        {
            break;
        }
        auto mt = pHeightIndex->begin();
        for (; mt != pHeightIndex->end(); ++mt)
        {
            const uint256& hashBlock = mt->first;
            if (setInvalidHash.count(hashBlock) == 0)
            {
                CBlockIndex* pIndex;
                if (!(pIndex = GetIndex(hashBlock)))
                {
                    StdError("BlockBase", "GetLongChainLastBlock GetIndex failed, block: %s", hashBlock.ToString().c_str());
                }
                else if (!pIndex->IsOrigin())
                {
                    if (VerifyValidBlock(pIndexGenesisLast, pIndex))
                    {
                        if (pMaxTrustIndex == nullptr)
                        {
                            pMaxTrustIndex = pIndex;
                        }
                        else if (!(pMaxTrustIndex->nChainTrust > pIndex->nChainTrust
                                   || (pMaxTrustIndex->nChainTrust == pIndex->nChainTrust && !pIndex->IsEquivalent(pMaxTrustIndex))))
                        {
                            pMaxTrustIndex = pIndex;
                        }
                    }
                }
            }
        }
        nStartHeight++;
    }
    return pMaxTrustIndex;
}

// bool CBlockBase::GetBlockDelegateAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const std::map<CDestination, CAddressContext>& mapAddressContext, std::map<CDestination, CDestination>& mapDelegateAddress)
// {
//     auto getAddress = [&](const CDestination& dest) -> bool
//     {
//         if (dest.IsNull())
//         {
//             return true;
//         }
//         if (mapDelegateAddress.count(dest) == 0)
//         {
//             auto it = mapAddressContext.find(dest);
//             if (it == mapAddressContext.end())
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Find address context fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             const CAddressContext& ctxAddress = it->second;
//             if (!ctxAddress.IsTemplate())
//             {
//                 return true;
//             }
//             CTemplateAddressContext ctxtTemplate;
//             if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Get template address context fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
//             if (!ptr)
//             {
//                 StdLog("BlockBase", "Get Block Delegate Address: Import template fail, dest: %s", dest.ToString().c_str());
//                 return false;
//             }
//             uint8 nTidType = ptr->GetTemplateType();
//             if (nTidType == TEMPLATE_DELEGATE)
//             {
//                 mapDelegateAddress.insert(make_pair(dest, dest));
//             }
//             else if (nTidType == TEMPLATE_VOTE)
//             {
//                 auto vote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
//                 mapDelegateAddress.insert(make_pair(dest, vote->destDelegate));
//             }
//         }
//         return true;
//     };

//     if (!getAddress(block.txMint.GetToAddress()))
//     {
//         StdLog("BlockBase", "Get Block Delegate Address: Get mint to failed, mint to: %s", block.txMint.GetToAddress().ToString().c_str());
//         return false;
//     }
//     for (const CTransaction& tx : block.vtx)
//     {
//         if (!getAddress(tx.GetFromAddress()))
//         {
//             StdLog("BlockBase", "Get Block Delegate Address: Get from failed, from: %s, txid: %s",
//                    tx.GetFromAddress().ToString().c_str(), tx.GetHash().ToString().c_str());
//             return false;
//         }
//         if (!getAddress(tx.GetToAddress()))
//         {
//             StdLog("BlockBase", "Get Block Delegate Address: Get to failed, to: %s, txid: %s",
//                    tx.GetToAddress().ToString().c_str(), tx.GetHash().ToString().c_str());
//             return false;
//         }
//     }
//     return true;
// }

bool CBlockBase::GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate)
{
    CTransaction tx;
    if (!tsBlock.Read(tx, nTxFile, nTxOffset, false, true))
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    if (!tx.GetToAddress().IsNull())
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, to not is null contract address, to: %s, nTxFile: %d, nTxOffset: %d",
               tx.GetToAddress().ToString().c_str(), nTxFile, nTxOffset);
        return false;
    }
    uint8 nCodeType;
    CTemplateContext ctxTemplate;
    if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, txcdCode))
    {
        StdLog("BlockBase", "Get tx contract data: Get create code fail, txid: %s, from: %s",
               tx.GetHash().GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return false;
    }
    if (nCodeType != CODE_TYPE_CONTRACT)
    {
        StdLog("BlockBase", "Get tx contract data: Code type error, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    txidCreate = tx.GetHash();
    return true;
}

bool CBlockBase::GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode)
{
    CContractSourceCodeContext ctxtCode;
    if (!dbBlock.RetrieveSourceCodeContext(hashFork, hashBlock, hashSourceCode, ctxtCode))
    {
        StdLog("BlockBase", "Get block source code data: Retrieve source code context fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    uint32 nFile, nOffset;
    if (!ctxtCode.GetLastPos(nFile, nOffset))
    {
        StdLog("BlockBase", "Get block source code data: Get last pos fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    uint256 txidCreate;
    if (!GetTxContractData(nFile, nOffset, txcdCode, txidCreate))
    {
        StdLog("BlockBase", "Get block source code data: Get tx contract data fail, code: %s, block: %s",
               hashSourceCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::GetBlockContractCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CTxContractData& txcdCode)
{
    CContractCreateCodeContext ctxtCode;
    bool fLinkGenesisFork;
    if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode, fLinkGenesisFork))
    {
        StdLog("BlockBase", "Get block contract create code data: Retrieve contract create code context fail, code: %s, block: %s",
               hashContractCreateCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    txcdCode.nMuxType = 1;
    txcdCode.strType = ctxtCode.strType;
    txcdCode.strName = ctxtCode.strName;
    txcdCode.destCodeOwner = ctxtCode.destCodeOwner;
    txcdCode.nCompressDescribe = 0;
    txcdCode.btDescribe.assign(ctxtCode.strDescribe.begin(), ctxtCode.strDescribe.end());
    txcdCode.nCompressCode = 0;
    txcdCode.btCode = ctxtCode.btCreateCode; // contract create code
    txcdCode.nCompressSource = 0;
    txcdCode.btSource.clear(); // source code
    return true;
}

bool CBlockBase::RetrieveForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    return dbBlock.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode);
}

bool CBlockBase::RetrieveLinkGenesisContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode, bool& fLinkGenesisFork)
{
    fLinkGenesisFork = false;
    if (dbBlock.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode))
    {
        return true;
    }
    if (hashFork != GetGenesisBlockHash())
    {
        CBlockIndex* pSubIndex = GetIndex(hashBlock);
        if (pSubIndex && pSubIndex->hashRefBlock != 0)
        {
            if (dbBlock.RetrieveContractCreateCodeContext(GetGenesisBlockHash(), pSubIndex->hashRefBlock, hashContractCreateCode, ctxtCode))
            {
                fLinkGenesisFork = true;
                return true;
            }
        }
    }
    return false;
}

bool CBlockBase::RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    return dbBlock.RetrieveContractRunCodeContext(hashFork, hashBlock, hashContractRunCode, ctxtCode);
}

bool CBlockBase::GetForkContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCodeContext& ctxtContractCode)
{
    CContractCreateCodeContext ctxtCode;
    if (!RetrieveForkContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode))
    {
        return false;
    }
    ctxtContractCode.hashCode = ctxtCode.GetContractCreateCodeHash();
    ctxtContractCode.hashSource = ctxtCode.hashSourceCode;
    ctxtContractCode.strType = ctxtCode.strType;
    ctxtContractCode.strName = ctxtCode.strName;
    ctxtContractCode.destOwner = ctxtCode.destCodeOwner;
    ctxtContractCode.strDescribe = ctxtCode.strDescribe;
    ctxtContractCode.hashCreateTxid = ctxtCode.txidCreate;
    return true;
}

bool CBlockBase::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode)
{
    std::map<uint256, CContractCreateCodeContext> mapContractCreateCode;
    if (!dbBlock.ListContractCreateCodeContext(hashFork, hashBlock, mapContractCreateCode))
    {
        return false;
    }
    for (const auto& kv : mapContractCreateCode)
    {
        CContractCodeContext ctxt;

        ctxt.hashCode = kv.second.GetContractCreateCodeHash();
        ctxt.hashSource = kv.second.hashSourceCode;
        ctxt.strType = kv.second.strType;
        ctxt.strName = kv.second.strName;
        ctxt.destOwner = kv.second.destCodeOwner;
        ctxt.strDescribe = kv.second.strDescribe;
        ctxt.hashCreateTxid = kv.second.txidCreate;

        mapCreateCode.insert(make_pair(kv.first, ctxt));
    }
    return true;
}

bool CBlockBase::VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract)
{
    uint256 hashStateRoot;
    {
        CReadLock rlock(rwAccess);
        CBlockIndex* pIndex = GetIndex(hashBlock);
        if (pIndex == nullptr)
        {
            StdLog("CBlockBase", "Verify contract address: Get index fail, hashBlock: %s", hashBlock.ToString().c_str());
            return false;
        }
        hashStateRoot = pIndex->hashStateRoot;
    }

    CDestState stateDest;
    if (!RetrieveDestState(hashFork, hashStateRoot, destContract, stateDest))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve dest state fail, state root: %s, destContract: %s, hashBlock: %s",
               hashStateRoot.GetHex().c_str(), destContract.ToString().c_str(), hashBlock.ToString().c_str());
        return false;
    }

    bytes btDestCodeData;
    if (!RetrieveContractKvValue(hashFork, stateDest.GetStorageRoot(), destContract.ToHash(), btDestCodeData))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve contract state fail, storage root: %s, block: %s, destContract: %s",
               stateDest.GetStorageRoot().GetHex().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockBase", "Verify contract address: Parse contract code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CContractRunCodeContext ctxtRunCode;
    if (!RetrieveContractRunCodeContext(hashFork, hashBlock, ctxDestCode.hashContractRunCode, ctxtRunCode))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve contract run code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    // CContractCreateCodeContext ctxtCreateCode;
    // bool fLinkGenesisFork;
    // if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, ctxDestCode.hashContractCreateCode, ctxtCreateCode, fLinkGenesisFork))
    // {
    //     StdLog("CBlockBase", "Verify contract address: Retrieve contract create code fail, hashContractCreateCode: %s, block: %s, destContract: %s",
    //            ctxDestCode.hashContractCreateCode.ToString().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
    //     return false;
    // }
    return true;
}

bool CBlockBase::VerifyCreateCodeTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    uint8 nCodeType;
    CTemplateContext ctxTemplate;
    CTxContractData txcdCode;
    if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, txcdCode))
    {
        StdLog("CBlockBase", "Verify create code tx: Get create code fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }

    if (nCodeType == CODE_TYPE_TEMPLATE)
    {
        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
        if (!ptr)
        {
            StdLog("CBlockBase", "Verify create code tx: Import template fail, tx: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    else
    {
        uint256 hashContractCreateCode;
        if (txcdCode.IsCreate() || txcdCode.IsUpcode())
        {
            // txcdCode.UncompressCode();
            // hashContractCreateCode = txcdCode.GetContractCreateCodeHash();

            // if (txcdCode.IsCreate() && fEnableContractCodeVerify)
            // {
            //     CContractCreateCodeContext ctxtContractCreateCode;
            //     bool fLinkGenesisFork;
            //     if (!RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtContractCreateCode, fLinkGenesisFork))
            //     {
            //         StdLog("CBlockState", "Verify create code tx: Code not exist, code: %s, tx: %s, block: %s",
            //                hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            //         return false;
            //     }
            //     // if (!ctxtContractCreateCode.IsActivated())
            //     // {
            //     //     StdLog("CBlockState", "Verify create code tx: Code not activate, code: %s, tx: %s, block: %s",
            //     //            hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            //     //     return false;
            //     // }
            // }
        }
        else if (txcdCode.IsSetup())
        {
            CTxContractData txcd;
            hashContractCreateCode = txcdCode.GetContractCreateCodeHash();
            if (!GetBlockContractCreateCodeData(hashFork, hashBlock, hashContractCreateCode, txcd))
            {
                StdLog("CBlockBase", "Verify create code tx: Get create code data fail, code hash: %s, tx: %s, block: %s",
                       hashContractCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            StdLog("CBlockBase", "Verify create code tx: Code flag error, flag: %d, tx: %s, block: %s",
                   txcdCode.nMuxType, tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    return dbBlock.GetAddressTxCount(hashFork, hashBlock, dest, nTxCount);
}

bool CBlockBase::RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    return dbBlock.RetrieveAddressTxInfo(hashFork, hashBlock, dest, nTxIndex, ctxtAddressTxInfo);
}

bool CBlockBase::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    return dbBlock.ListAddressTxInfo(hashFork, hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
}

bool CBlockBase::GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount)
{
    CBlockOutline outline;
    if (!dbBlock.RetrieveBlockIndex(hashPrevBlock, outline))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: Retrieve fork context failed, prev block: %s", hashPrevBlock.ToString().c_str());
        return false;
    }
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork, outline.hashRefBlock))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: Retrieve fork context failed, prev block: %s", hashPrevBlock.ToString().c_str());
        return false;
    }

    const uint32 nPrevHeight = CBlock::GetBlockHeightByHash(hashPrevBlock);
    const uint32 nVoteRewardLockDays = VOTE_REWARD_LOCK_DAYS;
    std::vector<std::pair<uint32, uint256>> vVoteReward;
    if (!dbBlock.ListVoteReward(ctxFork.nChainId, hashPrevBlock, dest, nVoteRewardLockDays, vVoteReward))
    {
        StdLog("CBlockBase", "Get Vote Reward Locked Amount: List reward lock fail, dest: %s, fork: %s", dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    nLockedAmount = 0;
    if (!vVoteReward.empty())
    {
        for (const auto& vd : vVoteReward)
        {
            const uint32& nRewardHeight = vd.first;
            const uint256& nRewardAmount = vd.second;
            uint32 nElapsedDays = (nPrevHeight + 1 - nRewardHeight) / VOTE_REWARD_LOCK_DAY_HEIGHT;
            if (nElapsedDays == 0)
            {
                nLockedAmount += nRewardAmount;
            }
            else if (nElapsedDays < nVoteRewardLockDays)
            {
                uint32 nLockDays = nVoteRewardLockDays - nElapsedDays;
                nLockedAmount += (nRewardAmount * uint256(nLockDays) / uint256(nVoteRewardLockDays));
            }
        }
    }
    return true;
}

bool CBlockBase::GetBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CAddressContext>& mapBlockAddress)
{
    auto loadAddress = [&](const CDestination& dest) -> bool {
        auto it = mapBlockAddress.find(dest);
        if (it == mapBlockAddress.end())
        {
            CAddressContext ctxAddress;
            if (!RetrieveAddressContext(hashFork, block.hashPrev, dest, ctxAddress))
            {
                // StdLog("CBlockBase", "Verify block address: Retrieve address context fail, dest: %s, fork: %s, prev: %s",
                //        dest.ToString().c_str(), hashFork.ToString().c_str(), block.hashPrev.ToString().c_str());
                return false;
            }
            mapBlockAddress.insert(make_pair(dest, ctxAddress));
        }
        return true;
    };

    auto verifyAddress = [&](const CTransaction& tx) -> bool {
        if (!tx.GetFromAddress().IsNull())
        {
            if (!loadAddress(tx.GetFromAddress()))
            {
                StdLog("CBlockBase", "Verify block address: Load from address fail, from: %s, txid: %s, block: %s",
                       tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }

        if (tx.GetToAddress().IsNull())
        {
            if (tx.GetFromAddress().IsNull())
            {
                StdLog("CBlockBase", "Verify block address: Create contract from address is null, from: %s, txid: %s, block: %s",
                       tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            uint8 nCodeType;
            CTemplateContext ctxTemplate;
            CTxContractData ctxContract;
            if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
            {
                StdLog("CBlockBase", "Verify block address: Get create code fail, to: %s, txid: %s, block: %s",
                       tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
            if (nCodeType == CODE_TYPE_TEMPLATE)
            {
                CTemplatePtr ptr = CTemplate::Import(ctxTemplate.GetTemplateData());
                if (!ptr)
                {
                    StdLog("CBlockBase", "Verify block address: Imprt template fail, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                CAddressContext ctxAddress(CTemplateAddressContext(ctxTemplate.GetName(), std::string(), ptr->GetTemplateType(), ctxTemplate.GetTemplateData()));
                mapBlockAddress[CDestination(ptr->GetTemplateId())] = ctxAddress;
            }
            else if (nCodeType == CODE_TYPE_CONTRACT)
            {
                CDestination destTo;
                //destTo.SetContractId(tx.GetFromAddress(), tx.GetNonce());
                destTo = CreateContractAddressByNonce(tx.GetFromAddress(), tx.GetNonce());
                if (loadAddress(destTo))
                {
                    StdLog("CBlockBase", "Verify block address: Contract address exited, contract address: %s, from: %s, txid: %s, block: %s",
                           destTo.ToString().c_str(), tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxContract.IsCreate() || ctxContract.IsSetup())
                {
                    CAddressContext ctxAddress(CContractAddressContext(ctxContract.GetType(), ctxContract.GetCodeOwner(), ctxContract.GetName(), ctxContract.GetDescribe(), tx.GetHash(),
                                                                       ctxContract.GetSourceCodeHash(), ctxContract.GetContractCreateCodeHash(), uint256()));
                    mapBlockAddress[destTo] = ctxAddress;
                }
            }
            else
            {
                StdLog("CBlockBase", "Verify block address: Code type error, to: %s, txid: %s, block: %s",
                       tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (!loadAddress(tx.GetToAddress()))
            {
                CAddressContext ctxAddress;
                if (!tx.GetToAddressData(ctxAddress))
                {
                    StdLog("CBlockBase", "Verify block address: Get tx to address fail, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxAddress.IsContract())
                {
                    StdLog("CBlockBase", "Verify block address: Contract address error, to: %s, txid: %s, block: %s",
                           tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                if (ctxAddress.IsTemplate())
                {
                    CTemplateAddressContext ctxtTemplate;
                    if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
                    {
                        StdLog("CBlockBase", "Verify block address: Get template context fail, to: %s, txid: %s, block: %s",
                               tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::Import(ctxtTemplate.btData);
                    if (!ptr || tx.GetToAddress() != CDestination(ptr->GetTemplateId()))
                    {
                        StdLog("CBlockBase", "Verify block address: To template error, to: %s, txid: %s, block: %s, ptr: %s, template data: %s",
                               tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str(),
                               hashBlock.ToString().c_str(), (ptr == nullptr ? "has" : "null"), ToHexString(ctxtTemplate.btData).c_str());
                        return false;
                    }
                }
                mapBlockAddress.insert(make_pair(tx.GetToAddress(), ctxAddress));
            }
            else
            {
                // Correct Address
                CAddressContext ctxTxAddress;
                if (tx.GetToAddressData(ctxTxAddress) && ctxTxAddress.IsTemplate())
                {
                    auto it = mapBlockAddress.find(tx.GetToAddress());
                    if (it != mapBlockAddress.end() && it->second.IsPubkey())
                    {
                        it->second = ctxTxAddress;
                    }
                }
            }
        }
        return true;
    };

    if (!verifyAddress(block.txMint))
    {
        StdLog("CBlockBase", "Verify block address: Verify mint address fail, txid: %s, block: %s",
               block.txMint.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
        return false;
    }

    for (const auto& tx : block.vtx)
    {
        if (!verifyAddress(tx))
        {
            StdLog("CBlockBase", "Verify block address: Verify address fail, txid: %s, block: %s",
                   tx.GetHash().GetHex().c_str(), hashBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& txReceiptex)
{
    CTransactionReceipt receipt;
    if (!dbBlock.RetrieveTxReceipt(hashFork, txid, receipt))
    {
        CTransaction tx;
        uint256 hashAtFork;
        CTxIndex txIndex;
        if (!RetrieveTxAndIndex(hashFork, txid, tx, hashAtFork, txIndex))
        {
            StdLog("CBlockBase", "Get transaction receipt: Retrieve tx fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
        receipt.nTxIndex = txIndex.nTxSeq;
        receipt.txid = txid;
        receipt.nBlockNumber = txIndex.nBlockNumber;
        receipt.from = tx.GetFromAddress();
        receipt.to = tx.GetToAddress();
        receipt.nTxGasUsed = tx.GetGasLimit();
        receipt.nTvGasUsed = 0;
        receipt.nEffectiveGasPrice = tx.GetGasPrice();
    }
    else
    {
        if (receipt.IsCommonReceipt())
        {
            CTransaction tx;
            uint256 hashAtFork;
            CTxIndex txIndex;
            if (!RetrieveTxAndIndex(hashFork, txid, tx, hashAtFork, txIndex))
            {
                StdLog("CBlockBase", "Get transaction receipt: Retrieve tx fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
                return false;
            }
            receipt.nTxIndex = txIndex.nTxSeq;
            receipt.txid = txid;
            receipt.nBlockNumber = txIndex.nBlockNumber;
            receipt.from = tx.GetFromAddress();
            receipt.to = tx.GetToAddress();
            receipt.nEffectiveGasPrice = tx.GetGasPrice();
        }
    }
    receipt.CalcLogsBloom();
    txReceiptex = CTransactionReceiptEx(receipt);

    uint256 hashLastBlock;
    if (!dbBlock.RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve fork last fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    CForkContext ctxFork;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxFork))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve fork context fail, txid: %s, fork: %s", txid.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    uint256 hashBlock;
    if (!dbBlock.RetrieveBlockHashByNumber(hashFork, ctxFork.nChainId, hashLastBlock, txReceiptex.nBlockNumber, hashBlock))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve block hash, block number: %lu, fork: %s", txReceiptex.nBlockNumber, hashFork.GetHex().c_str());
        return false;
    }
    txReceiptex.hashBlock = hashBlock;

    CBlockIndex* pBlockIndex = nullptr;
    if (!RetrieveIndex(hashBlock, &pBlockIndex))
    {
        StdLog("CBlockBase", "Get transaction receipt: Retrieve block index fail, block: %s, fork: %s", hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    txReceiptex.nBlockGasUsed = pBlockIndex->GetBlockGasUsed();
    return true;
}

bool CBlockBase::GetBlockReceiptsByBlock(const uint256& hashFork, const uint256& hashFromBlock, const uint256& hashToBlock, std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapBlockReceipts)
{
    CBlockIndex* pIndexFrom = nullptr;
    CBlockIndex* pIndexTo = nullptr;
    if (hashFromBlock == 0)
    {
        pIndexFrom = GetIndex(hashFork);
    }
    else
    {
        pIndexFrom = GetIndex(hashFromBlock);
    }
    if (hashToBlock == 0)
    {
        if (!RetrieveFork(hashFork, &pIndexTo))
        {
            StdLog("CBlockBase", "Get block receipts by logs filter: from or to block error, fork: %s", hashFork.ToString().c_str());
            return false;
        }
    }
    else
    {
        pIndexTo = GetIndex(hashToBlock);
    }
    if (!pIndexFrom || !pIndexTo)
    {
        StdLog("CBlockBase", "Get block receipts by logs filter: from or to block error, from: %s, to: %s",
               hashFromBlock.ToString().c_str(), hashToBlock.ToString().c_str());
        return false;
    }

    std::vector<uint256> vBlockHash;
    while (pIndexTo && pIndexTo->GetBlockNumber() >= pIndexFrom->GetBlockNumber())
    {
        vBlockHash.push_back(pIndexTo->GetBlockHash());
        pIndexTo = pIndexTo->pPrev;
    }

    for (auto& hashBlock : vBlockHash)
    {
        std::vector<CTransactionReceipt> vTxReceipt;
        if (!blockReceiptCache.GetBlockReceiptCache(hashBlock, vTxReceipt))
        {
            CBlockEx block;
            if (!Retrieve(hashBlock, block))
            {
                StdLog("CBlockBase", "Get block receipts by logs filter: Retrieve block failed, block: %s", hashBlock.ToString().c_str());
                return false;
            }
            {
                CTransactionReceipt receipt;
                if (dbBlock.RetrieveTxReceipt(hashFork, block.txMint.GetHash(), receipt))
                {
                    vTxReceipt.push_back(receipt);
                }
            }
            for (auto& tx : block.vtx)
            {
                CTransactionReceipt receipt;
                if (dbBlock.RetrieveTxReceipt(hashFork, tx.GetHash(), receipt))
                {
                    vTxReceipt.push_back(receipt);
                }
            }
            blockReceiptCache.AddBlockReceiptCache(hashBlock, vTxReceipt);
        }
        mapBlockReceipts.insert(std::make_pair(hashBlock, vTxReceipt));
    }
    return true;
}

bool CBlockBase::GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount)
{
    if (btAddressData.empty())
    {
        StdLog("CBlockBase", "Get fork lock amount: Address data is empty, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = CTemplate::Import(btAddressData);
    if (!ptr || ptr->GetTemplateType() != TEMPLATE_FORK)
    {
        StdLog("CBlockBase", "Get fork lock amount: Create template fail, dest: %s, prev: %s",
               dest.ToString().c_str(), hashPrevBlock.GetHex().c_str());
        return false;
    }
    uint256 hashForkLocked = boost::dynamic_pointer_cast<CTemplateFork>(ptr)->hashFork;
    int nCreateHeight = GetForkCreatedHeight(hashForkLocked, hashPrevBlock);
    if (nCreateHeight < 0)
    {
        StdLog("CBlockBase", "Get fork lock amount: Get fork created height fail, dest: %s, fork: %s",
               dest.ToString().c_str(), hashForkLocked.GetHex().c_str());
        return false;
    }
    int nForkValidHeight = CBlock::GetBlockHeightByHash(hashPrevBlock) - nCreateHeight;
    if (nForkValidHeight < 0)
    {
        nForkValidHeight = 0;
    }
    nLockedAmount = CTemplateFork::LockedCoin(nForkValidHeight);
    return true;
}

bool CBlockBase::VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock)
{
    CVoteContext ctxtVote;
    if (!RetrieveDestVoteContext(hashPrevBlock, dest, ctxtVote))
    {
        StdLog("CBlockBase", "Verify Vote Redeem: Retrieve dest vote context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    if (ctxtVote.nFinalHeight == 0 || (CBlock::GetBlockHeightByHash(hashPrevBlock) + 1) < ctxtVote.nFinalHeight)
    {
        StdDebug("CBlockBase", "Verify Vote Redeem: Vote locked, final vote height: %d, prev: [%d] %s, dest: %s",
                 ctxtVote.nFinalHeight, CBlock::GetBlockHeightByHash(hashPrevBlock), hashPrevBlock.ToString().c_str(), dest.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::GetAddressLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, const CAddressContext& ctxAddress, const uint256& nBalance, uint256& nLockedAmount)
{
    if (hashFork == hashGenesisBlock && ctxAddress.IsTemplate())
    {
        switch (ctxAddress.GetTemplateType())
        {
        case TEMPLATE_FORK:
        {
            CTemplateAddressContext ctxTemplate;
            if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
            {
                StdLog("CBlockState", "Get address loacked amount: Get template address context fail, dest: %s", dest.ToString().c_str());
                nLockedAmount = nBalance;
                return true;
            }
            uint256 nLocked;
            if (!GetCreateForkLockedAmount(dest, hashPrevBlock, ctxTemplate.btData, nLocked))
            {
                nLockedAmount = nBalance;
                return true;
            }
            nLockedAmount += nLocked;
            break;
        }
        case TEMPLATE_DELEGATE:
        case TEMPLATE_VOTE:
            if (!VerifyAddressVoteRedeem(dest, hashPrevBlock))
            {
                nLockedAmount = nBalance;
                return true;
            }
            break;
        case TEMPLATE_PLEDGE:
            nLockedAmount = nBalance;
            return true;
        }
    }

    uint256 nLocked;
    if (!GetVoteRewardLockedAmount(hashFork, hashPrevBlock, dest, nLocked))
    {
        nLockedAmount = nBalance;
        return true;
    }
    nLockedAmount += nLocked;
    return true;
}

bool CBlockBase::GetDexOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, CDexOrderBody& dexOrder)
{
    return dbBlock.GetDexOrder(hashBlock, destOrder, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, dexOrder);
}

bool CBlockBase::GetDexCompletePrice(const uint256& hashBlock, const uint256& hashCoinPair, uint256& nCompletePrice)
{
    return dbBlock.GetDexCompletePrice(hashBlock, hashCoinPair, nCompletePrice);
}

bool CBlockBase::GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    return dbBlock.GetCompleteOrder(hashBlock, destOrder, nChainIdOwner, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber, nCompleteAmount, nCompleteOrderCount);
}

bool CBlockBase::GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    return dbBlock.GetCompleteOrder(hashBlock, hashDexOrder, nCompleteAmount, nCompleteOrderCount);
}

bool CBlockBase::ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                     const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
{
    return dbBlock.ListAddressDexOrder(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nBeginOrderNumber, nGetCount, mapDexOrder);
}

bool CBlockBase::GetDexOrderMaxNumber(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, uint64& nMaxOrderNumber)
{
    return dbBlock.GetDexOrderMaxNumber(hashBlock, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nMaxOrderNumber);
}

bool CBlockBase::GetPeerCrossLastBlock(const uint256& hashBlock, const CChainId nPeerChainId, uint256& hashLastProveBlock)
{
    return dbBlock.GetPeerCrossLastBlock(hashBlock, nPeerChainId, hashLastProveBlock);
}

bool CBlockBase::GetMatchDexData(const uint256& hashBlock, std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    return dbBlock.GetMatchDexData(hashBlock, mapMatchResult);
}

bool CBlockBase::ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    return dbBlock.ListMatchDexOrder(hashBlock, strCoinSymbolSell, strCoinSymbolBuy, nGetCount, realDexOrder);
}

bool CBlockBase::GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve)
{
    return dbBlock.GetCrosschainProveForPrevBlock(nRecvChainId, hashRecvPrevBlock, mapBlockCrosschainProve);
}

bool CBlockBase::AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve)
{
    return dbBlock.AddRecvCrosschainProve(nRecvChainId, blockProve);
}

bool CBlockBase::GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve)
{
    return dbBlock.GetRecvCrosschainProve(nRecvChainId, nSendChainId, hashSendProvePrevBlock, blockProve);
}

bool CBlockBase::AddBlacklistAddress(const CDestination& dest)
{
    return dbBlock.AddBlacklistAddress(dest);
}

void CBlockBase::RemoveBlacklistAddress(const CDestination& dest)
{
    dbBlock.RemoveBlacklistAddress(dest);
}

bool CBlockBase::IsExistBlacklistAddress(const CDestination& dest)
{
    return dbBlock.IsExistBlacklistAddress(dest);
}

void CBlockBase::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    dbBlock.ListBlacklistAddress(setAddressOut);
}

bool CBlockBase::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return dbBlock.UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

uint256 CBlockBase::GetForkMintMinGasPrice(const uint256& hashFork)
{
    uint256 nMinGasPrice;
    if (!dbBlock.GetForkMintMinGasPrice(hashFork, nMinGasPrice))
    {
        nMinGasPrice = MIN_GAS_PRICE;
    }
    return nMinGasPrice;
}

bool CBlockBase::AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock)
{
    return dbBlock.AddBlockVoteResult(hashBlock, fLongChain, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockBase::RemoveBlockVoteResult(const uint256& hashBlock)
{
    return dbBlock.RemoveBlockVoteResult(hashBlock);
}

bool CBlockBase::RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    return dbBlock.RetrieveBlockVoteResult(hashBlock, btBitmap, btAggSig, fAtChain, hashAtBlock);
}

bool CBlockBase::GetMakerVoteBlock(const uint256& hashPrevBlock, bytes& btBitmap, bytes& btAggSig, uint256& hashVoteBlock)
{
    CBlockOutline outline;
    if (!dbBlock.RetrieveBlockIndex(hashPrevBlock, outline))
    {
        return false;
    }
    uint256 hashLastVoteBlock;
    bytes btVoteBitmap;
    bytes btVoteAggSig;
    bool fVoteAtChain = false;
    uint256 hashVoteAtBlock;
    if (!dbBlock.GetLastBlockVoteResult(outline.hashOrigin, hashLastVoteBlock, btVoteBitmap, btVoteAggSig, fVoteAtChain, hashVoteAtBlock))
    {
        return false;
    }
    if (fVoteAtChain)
    {
        return false;
    }
    if (!VerifySameChain(hashLastVoteBlock, hashPrevBlock))
    {
        return false;
    }
    hashVoteBlock = hashLastVoteBlock;
    btBitmap = btVoteBitmap;
    btAggSig = btVoteAggSig;
    return true;
}

bool CBlockBase::IsBlockConfirm(const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        return false;
    }
    CBlockOutline outline;
    if (!dbBlock.RetrieveBlockIndex(hashBlock, outline))
    {
        return false;
    }
    if (hashBlock == outline.hashOrigin)
    {
        return true;
    }
    uint256 hashLastConfirmBlock;
    if (!dbBlock.GetLastConfirmBlock(outline.hashOrigin, hashLastConfirmBlock))
    {
        return false;
    }
    if (!VerifySameChain(hashBlock, hashLastConfirmBlock))
    {
        return false;
    }
    return true;
}

bool CBlockBase::AddBlockLocalVoteSignFlag(const uint256& hashBlock)
{
    return dbBlock.AddBlockLocalVoteSignFlag(hashBlock);
}

//----------------------------------------------------------------------------
bool CBlockBase::GetTxIndex(const uint256& hashFork, const uint256& txid, uint256& hashAtFork, CTxIndex& txIndex)
{
    if (hashFork == 0)
    {
        std::map<uint256, CForkContext> mapForkCtxt;
        if (!ListForkContext(mapForkCtxt))
        {
            return false;
        }
        for (const auto& kv : mapForkCtxt)
        {
            const uint256& hashGetFork = kv.first;
            if (dbBlock.RetrieveTxIndex(hashGetFork, txid, txIndex))
            {
                hashAtFork = hashGetFork;
                return true;
            }
        }
    }
    else
    {
        if (dbBlock.RetrieveTxIndex(hashFork, txid, txIndex))
        {
            hashAtFork = hashFork;
            return true;
        }
    }
    return false;
}

void CBlockBase::ClearCache()
{
    map<uint256, CBlockIndex*>::iterator mi;
    for (mi = mapIndex.begin(); mi != mapIndex.end(); ++mi)
    {
        delete (*mi).second;
    }
    mapIndex.clear();
    mapForkHeightIndex.clear();
}

bool CBlockBase::LoadDB()
{
    CWriteLock wlock(rwAccess);

    ClearCache();

    /*CBlockWalker walker(this);
    if (!dbBlock.WalkThroughBlockIndex(walker))
    {
        StdLog("BlockBase", "LoadDB: Walk Through Block Index fail");
        ClearCache();
        return false;
    }*/
    StdLog("BlockBase", "Start verify db.");
    if (!VerifyDB())
    {
        StdError("BlockBase", "Load DB: Verify DB fail.");
        return false;
    }
    StdLog("BlockBase", "Verify db success!");
    return true;
}

bool CBlockBase::VerifyDB()
{
    const uint32 nNeedVerifyCount = 128;
    bool fAllVerify = false;
    std::map<uint256, uint256> mapForkLast;
    std::size_t nVerifyCount = dbBlock.GetBlockVerifyCount();
    if (nVerifyCount > nNeedVerifyCount)
    {
        for (std::size_t i = nVerifyCount - nNeedVerifyCount; i < nVerifyCount; i++)
        {
            CBlockVerify verifyBlock;
            if (!dbBlock.GetBlockVerify(i, verifyBlock))
            {
                StdError("BlockBase", "Verify DB: Get block verify fail, pos: %ld.", i);
                return false;
            }
            CBlockOutline outline;
            CBlockRoot blockRoot;
            if (!VerifyBlockDB(verifyBlock, outline, blockRoot, true))
            {
                fAllVerify = true;
                break;
            }
        }
    }
    for (std::size_t i = 0; i < nVerifyCount; i++)
    {
        CBlockVerify verifyBlock;
        if (!dbBlock.GetBlockVerify(i, verifyBlock))
        {
            StdError("BlockBase", "Verify DB: Get block verify fail, pos: %ld.", i);
            return false;
        }

        bool fVerify = false;
        if (fAllVerify)
        {
            fVerify = true;
        }
        else
        {
            if (i <= nNeedVerifyCount / 2
                || nVerifyCount <= nNeedVerifyCount
                || i > (nVerifyCount - nNeedVerifyCount))
            {
                fVerify = true;
            }
        }

        CBlockOutline outline;
        CBlockRoot blockRoot;
        CBlockIndex* pIndexNew = nullptr;
        CBlockEx blockex;
        bool fRepairBlock = false;
        if (VerifyBlockDB(verifyBlock, outline, blockRoot, fVerify))
        {
            if (!LoadBlockIndex(outline, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Load block index fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (!fAllVerify)
            {
                fAllVerify = true;
            }
            StdDebug("BlockBase", "Verify DB: Verify block db fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            if (!RepairBlockDB(verifyBlock, blockRoot, blockex, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Repair block db fail, pos: %ld, block: [%d] %s.", i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            fRepairBlock = true;
        }

        auto funcUpdateLongChain = [&](const uint256& hashForkIn, const uint256& hashForkLastBlockIn, const uint256& hashBlockIn, const CBlockEx& blockIn, const CBlockIndex* pIndexIn) -> bool {
            CBlockChainUpdate update = CBlockChainUpdate(pIndexIn, {});
            if (blockIn.IsOrigin())
            {
                update.vBlockAddNew.push_back(blockIn);
            }
            else
            {
                if (!GetBlockBranchListNolock(hashBlockIn, hashForkLastBlockIn, &blockIn, update.vBlockRemove, update.vBlockAddNew))
                {
                    StdLog("BlockBase", "Verify DB: Get block branch list fail, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlockIn), hashBlockIn.ToString().c_str());
                    return false;
                }
            }
            if (!UpdateBlockLongChain(hashForkIn, update.vBlockRemove, update.vBlockAddNew))
            {
                StdLog("BlockBase", "Verify DB: Update block long chain fail, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlockIn), hashBlockIn.ToString().c_str());
                return false;
            }
            return true;
        };

        if (pIndexNew->IsOrigin())
        {
            if (fRepairBlock)
            {
                if (!funcUpdateLongChain(pIndexNew->GetBlockHash(), pIndexNew->GetBlockHash(), pIndexNew->GetBlockHash(), blockex, pIndexNew))
                {
                    StdError("BlockBase", "Verify DB: Update origin long chain fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(pIndexNew->GetBlockHash()), pIndexNew->GetBlockHash().GetHex().c_str());
                    return false;
                }
            }
            mapForkLast[pIndexNew->GetBlockHash()] = pIndexNew->GetBlockHash();
        }
        else
        {
            auto it = mapForkLast.find(pIndexNew->GetOriginHash());
            if (it == mapForkLast.end())
            {
                StdError("BlockBase", "Verify DB: Find fork last fail, fork: %s, block: [%d] %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            CBlockIndex* pIndexFork = GetIndex(it->second);
            if (pIndexFork == nullptr)
            {
                StdError("BlockBase", "Verify DB: Get last index fail, fork: %s, block: [%d] %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            if (!(pIndexFork->nChainTrust > pIndexNew->nChainTrust
                  || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
            {
                if (fRepairBlock)
                {
                    if (!funcUpdateLongChain(pIndexNew->GetOriginHash(), it->second, pIndexNew->GetBlockHash(), blockex, pIndexNew))
                    {
                        StdError("BlockBase", "Verify DB: Update fork long chain fail, fork: %s, block: [%d] %s.",
                                 pIndexNew->GetOriginHash().GetHex().c_str(), CBlock::GetBlockHeightByHash(pIndexNew->GetBlockHash()), pIndexNew->GetBlockHash().GetHex().c_str());
                        return false;
                    }
                }
                UpdateBlockNext(pIndexNew);
                it->second = pIndexNew->GetBlockHash();
            }
        }

        if (i % 100000 == 0)
        {
            StdLog("BlockBase", "Verify DB: Verify block count: %ld, block: [%d] %s.",
                   i + 1, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        }
    }

    for (const auto& kv : mapForkLast)
    {
        uint256 hashLastBlock;
        if (!dbBlock.RetrieveForkLast(kv.first, hashLastBlock) || hashLastBlock != kv.second)
        {
            StdError("BlockBase", "Verify DB: Fork last error, last block: [%d] %s, fork: %s",
                     CBlock::GetBlockHeightByHash(kv.second), kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
            if (!dbBlock.UpdateForkLast(kv.first, kv.second))
            {
                StdError("BlockBase", "Verify DB: Repair fork last fail, last block: [%d] %s, fork: %s",
                         CBlock::GetBlockHeightByHash(kv.second), kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
                return false;
            }
        }
    }
    return true;
}

bool CBlockBase::VerifyBlockDB(const CBlockVerify& verifyBlock, CBlockOutline& outline, CBlockRoot& blockRoot, const bool fVerify)
{
    if (!dbBlock.RetrieveBlockIndex(verifyBlock.hashBlock, outline))
    {
        StdError("BlockBase", "Verify block DB: Retrieve block index fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    if (outline.GetCrc() != verifyBlock.nIndexCrc)
    {
        StdError("BlockBase", "Verify block DB: Block index crc error, db index crc: 0x%8.8x, verify index crc: 0x%8.8x, block: [%d] %s.",
                 outline.GetCrc(), verifyBlock.nIndexCrc, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerify || outline.IsOrigin())
    {
        if (!dbBlock.VerifyBlockRoot(outline.IsPrimary(), outline.hashOrigin, outline.hashPrev,
                                     verifyBlock.hashBlock, outline.hashStateRoot, blockRoot, false))
        {
            StdError("BlockBase", "Verify block DB: Verify block root fail, block: [%d] %s.", CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
        if (blockRoot.GetRootCrc() != verifyBlock.nRootCrc)
        {
            StdError("BlockBase", "Verify block DB: Root crc error, db root crc: 0x%8.8x, verify root crc: 0x%8.8x, block: [%d] %s.",
                     blockRoot.GetRootCrc(), verifyBlock.nRootCrc, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockEx& block, CBlockIndex** ppIndexNew)
{
    //CBlockEx block;
    if (!tsBlock.Read(block, verifyBlock.nFile, verifyBlock.nOffset, true, true))
    {
        StdError("BlockBase", "Repair block DB: Read block fail, file: %d, offset: %d, block: [%d] %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    uint256 hashBlock = block.GetHash();
    if (hashBlock != verifyBlock.hashBlock)
    {
        StdError("BlockBase", "Repair block DB: Block error, file: %d, offset: %d, calc block: [%d] %s, block: [%d] %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(),
                 CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }

    uint256 hashFork;
    if (block.IsOrigin())
    {
        hashFork = hashBlock;
    }
    else
    {
        CBlockIndex* pPrevIndex = GetIndex(block.hashPrev);
        if (!pPrevIndex)
        {
            StdError("BlockBase", "Repair block DB: Get prev index fail, block: [%d] %s, prev block: [%d] %s.",
                     CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str(),
                     CBlock::GetBlockHeightByHash(block.hashPrev), block.hashPrev.GetHex().c_str());
            return false;
        }
        hashFork = pPrevIndex->GetOriginHash();
    }

    std::vector<CTransactionReceipt> vBlockTxReceipts;
    if (!SaveBlock(hashFork, hashBlock, block, ppIndexNew, blockRoot, vBlockTxReceipts, true))
    {
        StdError("BlockBase", "Repair block DB: Save block failed, block: [%d] %s", CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::LoadBlockIndex(CBlockOutline& outline, CBlockIndex** ppIndexNew)
{
    uint256 hashBlock = outline.GetBlockHash();

    if (mapIndex.find(hashBlock) != mapIndex.end())
    {
        StdError("BlockBase", "Load block index: Block index exist, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    CBlockIndex* pIndexNew = new CBlockIndex(static_cast<CBlockIndex&>(outline));
    if (pIndexNew == nullptr)
    {
        StdError("BlockBase", "Load block index: New block index fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    auto mi = mapIndex.insert(make_pair(hashBlock, pIndexNew)).first;
    if (mi == mapIndex.end())
    {
        StdError("BlockBase", "Load block index: Block index insert fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    pIndexNew->phashBlock = &(mi->first);
    pIndexNew->pPrev = nullptr;
    pIndexNew->pOrigin = nullptr;

    if (outline.hashPrev != 0)
    {
        pIndexNew->pPrev = GetIndex(outline.hashPrev);
        if (pIndexNew->pPrev == nullptr)
        {
            StdError("BlockBase", "Load block index: Get prev index fail, block: %s, prev block: %s",
                     hashBlock.ToString().c_str(), outline.hashPrev.GetHex().c_str());
            return false;
        }
    }

    if (pIndexNew->IsOrigin())
    {
        pIndexNew->pOrigin = pIndexNew;
    }
    else
    {
        pIndexNew->pOrigin = GetIndex(outline.hashOrigin);
        if (pIndexNew->pOrigin == nullptr)
        {
            StdError("BlockBase", "Load block index: Get origin index fail, block: %s, origin block: %s",
                     hashBlock.ToString().c_str(), outline.hashOrigin.GetHex().c_str());
            return false;
        }
    }

    UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hashBlock, pIndexNew->nTimeStamp, CDestination(), pIndexNew->GetRefBlock());

    *ppIndexNew = pIndexNew;
    return true;
}

bool CBlockBase::GetBlockBranchListNolock(const uint256& hashBlock, const uint256& hashForkLastBlock, const CBlockEx* pBlockex, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder)
{
    CBlockIndex* pIndex = GetIndex(hashBlock);
    if (pIndex == nullptr)
    {
        return false;
    }

    CBlockIndex* pForkLast = nullptr;
    if (hashForkLastBlock == 0)
    {
        pForkLast = GetForkLastIndex(pIndex->GetOriginHash());
    }
    else
    {
        pForkLast = GetIndex(hashForkLastBlock);
    }
    if (pForkLast == nullptr)
    {
        return false;
    }

    vector<CBlockIndex*> vPath;
    CBlockIndex* pBranch = GetBranch(pForkLast, pIndex, vPath);

    for (CBlockIndex* p = pForkLast; p != pBranch; p = p->pPrev)
    {
        CBlockEx block;
        if (!tsBlock.Read(block, p->nFile, p->nOffset, true, true))
        {
            return false;
        }
        vRemoveBlockInvertedOrder.push_back(block);
    }

    for (int i = vPath.size() - 1; i >= 0; i--)
    {
        if (pBlockex && vPath[i]->GetBlockHash() == hashBlock)
        {
            vAddBlockPositiveOrder.push_back(*pBlockex);
        }
        else
        {
            CBlockEx block;
            if (!tsBlock.Read(block, vPath[i]->nFile, vPath[i]->nOffset, true, true))
            {
                return false;
            }
            vAddBlockPositiveOrder.push_back(block);
        }
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

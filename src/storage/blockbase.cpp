// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockbase.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "bloomfilter/bloomfilter.h"
#include "delegatecomm.h"
#include "template/delegate.h"
#include "template/redeem.h"
#include "template/template.h"
#include "template/vote.h"
#include "util.h"
//#include "wasmhost.h"
#include "wasmrun.h"

using namespace std;
using namespace boost::filesystem;
using namespace hcode;

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

void CForkHeightIndex::AddHeightIndex(uint32 nHeight, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
{
    mapHeightIndex[nHeight][hashBlock] = CBlockHeightIndex(nBlockTimeStamp, destMint, hashRefBlock);
}

void CForkHeightIndex::RemoveHeightIndex(uint32 nHeight, const uint256& hashBlock)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        it->second.erase(hashBlock);
    }
}

void CForkHeightIndex::UpdateBlockRef(int nHeight, const uint256& hashBlock, const uint256& hashRefBlock)
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

map<uint256, CBlockHeightIndex>* CForkHeightIndex::GetBlockMintList(uint32 nHeight)
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

bool CBlockState::AddTxState(const CTransaction& tx, const int nTxIndex)
{
    uint256 txid = tx.GetHash();

    CDestination destTo;
    if (tx.nAmount != 0)
    {
        if (tx.to.IsNullContract())
        {
            destTo.SetContractId(tx.from.data, tx.nTxNonce);
        }
        else
        {
            destTo = tx.to;
        }
        auto it = mapBlockState.find(destTo);
        if (it == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, destTo, state))
            {
                state.SetNull();
            }
            it = mapBlockState.insert(make_pair(destTo, state)).first;
        }
        it->second.nBalance += tx.nAmount;
    }

    if (!tx.from.IsNull())
    {
        auto mt = mapBlockState.find(tx.from);
        if (mt == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, tx.from, state))
            {
                StdLog("CBlockState", "Add tx state: Retrieve dest state fail, txid: %s, from: %s",
                       txid.GetHex().c_str(), tx.from.ToString().c_str());
                return false;
            }
            mt = mapBlockState.insert(make_pair(tx.from, state)).first;
        }
        if (mt->second.nBalance < (tx.nAmount + tx.GetTxFee()))
        {
            StdLog("CBlockState", "Add tx state: From dest balance error, nBalance: %s, nAmount+Fee: %s, txid: %s, from: %s",
                   mt->second.nBalance.GetHex().c_str(), (tx.nAmount + tx.GetTxFee()).GetHex().c_str(),
                   txid.GetHex().c_str(), tx.from.ToString().c_str());
            return false;
        }
        mt->second.nBalance -= (tx.nAmount + tx.GetTxFee());
        mt->second.nTxNonce++;
    }

    if (tx.to.IsContract())
    {
        bool fCallResult = true;
        if (!AddWasmState(txid, tx, nTxIndex, fCallResult))
        {
            StdLog("CBlockState", "Add tx state: Add wasm state fail, txid: %s", txid.GetHex().c_str());
            return false;
        }
        if (!fCallResult && tx.nAmount != 0)
        {
            auto it = mapBlockState.find(destTo);
            if (it == mapBlockState.end())
            {
                StdLog("CBlockState", "Add tx state: Find to address fail, to: %s, txid: %s", destTo.ToString().c_str(), txid.GetHex().c_str());
                return false;
            }
            it->second.nBalance -= tx.nAmount;

            if (!tx.from.IsNull())
            {
                auto mt = mapBlockState.find(tx.from);
                if (mt == mapBlockState.end())
                {
                    StdLog("CBlockState", "Add tx state: Find from address fail, from: %s, txid: %s", tx.from.ToString().c_str(), txid.GetHex().c_str());
                    return false;
                }
                mt->second.nBalance += tx.nAmount;
            }
        }
    }
    else
    {
        mapBlockTxFeeUsed[txid] = tx.GetTxFee();
    }
    return true;
}

bool CBlockState::DoBlockState(uint256& hashReceiptRoot, uint256& nBlockGasUsed, uint2048& nBlockBloomOut, const bool fCreateBlock,
                               const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut)
{
    nBlockGasUsed = uint256(nOriBlockGasLimit - nSurplusBlockGasLimit);
    hashReceiptRoot = CReceiptMerkleTree::BuildMerkleTree(vReceiptHash);
    if (fCreateBlock)
    {
        nTotalMintRewardOut = nMintReward - nBlockFeeLeft;
        if (fMintRatio)
        {
            uint32 nRewardRation = MINT_REWARD_PER;
            if (destMint.IsTemplate() && destMint.GetTemplateId().GetType() == TEMPLATE_DELEGATE)
            {
                uint256 hashPrevRefBlock;
                if (fPrimaryBlock)
                {
                    hashPrevRefBlock = hashPrevBlock;
                }
                else
                {
                    CProofOfPiggyback proof;
                    if (!proof.Load(btBlockProof) || proof.hashRefBlock == 0)
                    {
                        StdLog("CBlockState", "Do block state: Load proof fail, destMint: %s, hashBlock: %s",
                               destMint.ToString().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                    CBlockIndex* pPrevRefIndex = nullptr;
                    if (!dbBlockBase.RetrieveIndex(proof.hashRefBlock, &pPrevRefIndex))
                    {
                        StdLog("CBlockState", "Do block state: Get ref index fail, hashRefBlock: %s, hashBlock: %s",
                               proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                    hashPrevRefBlock = pPrevRefIndex->GetPrevHash();
                }
                if (!dbBlockBase.GetDelegateMintRewardRatio(hashPrevRefBlock, destMint, nRewardRation))
                {
                    StdLog("CBlockState", "Do block state: Get delegate mint reward ratio fail, destMint: %s, hashPrevRefBlock: %s",
                           destMint.ToString().c_str(), hashPrevRefBlock.GetHex().c_str());
                    return false;
                }
            }
            nBlockMintRewardOut = (nTotalMintRewardOut * uint256(nRewardRation) / uint256(MINT_REWARD_PER));
        }
        else
        {
            nBlockMintRewardOut = nTotalMintRewardOut;
        }
    }
    else
    {
        nBlockMintRewardOut = nMintReward;
    }

    if (nBlockMintRewardOut != 0)
    {
        auto nt = mapBlockState.find(destMint);
        if (nt == mapBlockState.end())
        {
            CDestState state;
            if (!dbBlockBase.RetrieveDestState(hashFork, hashPrevStateRoot, destMint, state))
            {
                state.SetNull();
            }
            nt = mapBlockState.insert(make_pair(destMint, state)).first;
        }
        nt->second.nBalance += nBlockMintRewardOut;
    }

    for (const auto& kv : mapWasmKvState)
    {
        const CDestination& destWasm = kv.first;
        CDestState stateDestWasm;
        if (!GetDestState(destWasm, stateDestWasm))
        {
            stateDestWasm.SetNull();
        }
        if (!dbBlockBase.AddBlockWasmState(hashFork, stateDestWasm.hashStorageRoot, stateDestWasm.hashStorageRoot, kv.second))
        {
            StdLog("CBlockState", "Do block state: Add block wasm state fail, destWasm: %s", destWasm.ToString().c_str());
            return false;
        }
        SetDestState(destWasm, stateDestWasm);
    }

    if (!mapBlockContractTransfer.empty())
    {
        CBloomFilter<2048> bfTransfer;
        for (const auto& kv : mapBlockContractTransfer)
        {
            for (const auto& vd : kv.second)
            {
                bfTransfer.Add(vd.destFrom.data.begin(), vd.destFrom.data.size());
                bfTransfer.Add(vd.destTo.data.begin(), vd.destTo.data.size());
            }
        }
        bytes btTransferBloom;
        bfTransfer.GetData(btTransferBloom);
        nBlockBloom |= uint2048(btTransferBloom);
    }

    nBlockBloomOut = nBlockBloom;
    return true;
}

bool CBlockState::GetDestState(const CDestination& dest, CDestState& stateDest)
{
    auto mt = mapCacheWasmData.find(dest);
    if (mt != mapCacheWasmData.end() && !mt->second.cacheDestState.IsNull())
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
    mapCacheWasmData[dest].cacheDestState = stateDest;
}

bool CBlockState::GetDestKvData(const CDestination& dest, const uint256& key, bytes& value)
{
    auto nt = mapCacheWasmData.find(dest);
    if (nt != mapCacheWasmData.end())
    {
        auto mt = nt->second.cacheWasmKv.find(key);
        if (mt != nt->second.cacheWasmKv.end())
        {
            value = mt->second;
            return true;
        }
    }
    auto it = mapWasmKvState.find(dest);
    if (it != mapWasmKvState.end())
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
    return dbBlockBase.RetrieveWasmState(hashFork, stateDest.hashStorageRoot, key, value);
}

bool CBlockState::RetrieveAddressContext(const uint256& dest, CAddressContext& ctxtAddress)
{
    auto it = mapCacheAddressContext.find(dest);
    if (it != mapCacheAddressContext.end())
    {
        ctxtAddress = it->second;
        return true;
    }
    auto nt = mapBlockAddressContext.find(dest);
    if (nt != mapBlockAddressContext.end())
    {
        ctxtAddress = nt->second;
        return true;
    }
    return dbBlockBase.RetrieveAddressContext(hashFork, hashPrevBlock, dest, ctxtAddress);
}

bool CBlockState::GetWasmRunCode(const CDestination& destWasm, uint256& hashWasmCreateCode, CDestination& destCodeOwner, bytes& btWasmRunCode)
{
    bytes btDestCodeData;
    if (!GetDestKvData(destWasm, destWasm.data, btDestCodeData))
    {
        StdLog("CBlockState", "Get wasm run code: Get wasm code fail, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxtDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxtDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get wasm run code: Parse wasm code fail, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }

    CWasmRunCodeContext ctxtRunCode;
    if (!dbBlockBase.RetrieveWasmRunCodeContext(hashFork, hashPrevBlock, ctxtDestCode.hashWasmRunCode, ctxtRunCode))
    {
        StdLog("CBlockState", "Get wasm run code: Retrieve wasm run code fail, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }

    CWasmCreateCodeContext ctxtCreateCode;
    if (!dbBlockBase.RetrieveWasmCreateCodeContext(hashFork, hashPrevBlock, ctxtDestCode.hashWasmCreateCode, ctxtCreateCode))
    {
        StdLog("CBlockState", "Get wasm run code: Retrieve wasm create code fail, hashWasmCreateCode: %s, destWasm: %s", ctxtDestCode.hashWasmCreateCode.ToString().c_str(), destWasm.ToString().c_str());
        return false;
    }

    hashWasmCreateCode = ctxtDestCode.hashWasmCreateCode;
    destCodeOwner = ctxtCreateCode.destCodeOwner;
    btWasmRunCode = ctxtRunCode.btWasmRunCode;
    return true;
}

bool CBlockState::GetWasmCreateCode(const CDestination& destWasm, CTxContractData& txcd)
{
    bytes btDestCodeData;
    if (!GetDestKvData(destWasm, destWasm.data, btDestCodeData))
    {
        StdLog("CBlockState", "Get wasm create code: Get wasm code fail, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxtDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxtDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockState", "Get wasm create code: Parse wasm code fail, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }

    if (!dbBlockBase.GetBlockWasmCreateCodeData(hashFork, hashPrevBlock, ctxtDestCode.hashWasmCreateCode, txcd))
    {
        StdLog("CBlockState", "Get wasm create code: Get wasm create code fail, prev block: %s", hashPrevBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockState::SaveWasmRunCode(const CDestination& destWasm, const bytes& btWasmRunCode, const CTxContractData& txcd)
{
    uint256 hashWasmCreateCode = txcd.GetWasmCreateCodeHash();
    uint256 hashWasmRunCode = metabasenet::crypto::CryptoHash(btWasmRunCode.data(), btWasmRunCode.size());

    CContractDestCodeContext ctxtDestCode(hashWasmCreateCode, hashWasmRunCode);
    bytes btDestCodeData;
    CBufStream ss;
    ss << ctxtDestCode;
    ss.GetData(btDestCodeData);

    CDestState stateWasmDest;
    if (GetDestState(destWasm, stateWasmDest) && stateWasmDest.hashCode != 0)
    {
        StdLog("CBlockState", "Save wasm run code: address code exist, destWasm: %s", destWasm.ToString().c_str());
        return false;
    }
    stateWasmDest.hashCode = hashWasmRunCode;

    auto& cachWasm = mapCacheWasmData[destWasm];
    cachWasm.cacheWasmKv[destWasm.data] = btDestCodeData;
    cachWasm.cacheDestState = stateWasmDest;

    //CContractAddressContext(const std::string& strTypeIn, const CDestination& destCodeOwnerIn, const std::string& strNameIn, const std::string& strDescribeIn, const uint256& hashCreateTxidIn,
    //                        const uint256& hashSourceCodeIn, const uint256& hashWasmCreateCodeIn, const uint256& hashWasmRunCodeIn)

    mapCacheWasmRunCodeContext[hashWasmRunCode] = CWasmRunCodeContext(hashWasmCreateCode, btWasmRunCode);
    mapCacheAddressContext[destWasm.data] = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), uint256(), txcd.GetSourceCodeHash(), txcd.GetWasmCreateCodeHash(), hashWasmRunCode), nBlockHeight);
    return true;
}

void CBlockState::SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed)
{
    if (nGasUsed > 0)
    {
        mapCacheCodeDestGasUsed[destCodeOwner] += nGasUsed;
    }
}

void CBlockState::SaveRunResult(const CDestination& destWasm, const CTransactionLogs& logs, const std::map<uint256, bytes>& mapCacheKv)
{
    auto& cacheWasm = mapCacheWasmData[destWasm];
    for (auto& kv : mapCacheKv)
    {
        cacheWasm.cacheWasmKv[kv.first] = kv.second;
    }
    cacheWasm.cacheWasmLogs.push_back(logs);
}

bool CBlockState::ContractTransfer(const uint256& from, const uint256& to, const uint256& amount)
{
    CAddressContext ctxtAddressFrom;
    if (!RetrieveAddressContext(from, ctxtAddressFrom))
    {
        StdLog("CBlockState", "Contract transfer: Retrieve address context fail, from: %s", from.ToString().c_str());
        return false;
    }
    CDestination destFrom(ctxtAddressFrom.nType, from);

    CAddressContext ctxtAddressTo;
    if (!RetrieveAddressContext(to, ctxtAddressTo))
    {
        ctxtAddressTo.nType = CDestination::PREFIX_PUBKEY;
    }
    CDestination destTo(ctxtAddressTo.nType, to);

    CDestState stateFrom;
    if (!GetDestState(destFrom, stateFrom))
    {
        StdLog("CBlockState", "Contract transfer: Get dest state fail, from: %s", destFrom.ToString().c_str());
        return false;
    }
    if (stateFrom.nBalance < amount)
    {
        StdLog("CBlockState", "Contract transfer: nBalance < amount, nBalance: %s, amount: %s, from: %s",
               stateFrom.nBalance.GetHex().c_str(), amount.GetHex().c_str(), destFrom.ToString().c_str());
        return false;
    }

    CDestState stateTo;
    if (!GetDestState(destTo, stateTo))
    {
        stateTo.SetNull();
    }

    stateFrom.nBalance -= amount;
    stateTo.nBalance += amount;

    SetCacheDestState(destFrom, stateFrom);
    SetCacheDestState(destTo, stateTo);

    vCacheContractTransfer.push_back(CContractTransfer(destFrom, destTo, amount));
    return true;
}

///////////////////
bool CBlockState::GetDestWasmCode(const CTransaction& tx, CDestination& destWasm, bytes& btWasmCode, bytes& btRunParam, uint256& hashWasmCreateCode, CDestination& destCodeOwner, CTxContractData& txcd, bool& fCall)
{
    fCall = true;
    if (tx.to.IsNullContract())
    {
        destWasm.SetContractId(tx.from.data, tx.nTxNonce);
        bytes btTempData;
        if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
        {
            StdLog("CBlockState", "Get dest wasm code: Get tx data fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
        CTxContractData txcdCode;
        try
        {
            hcode::CBufStream ss(btTempData);
            ss >> txcdCode;
        }
        catch (std::exception& e)
        {
            StdLog("CBlockState", "Get dest wasm code: Parse tx data fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
        if (txcdCode.IsUpcode())
        {
            fCall = false;
        }
        if (txcdCode.IsCreate() || txcdCode.IsUpcode())
        {
            txcdCode.UncompressCode();
            hashWasmCreateCode = txcdCode.GetWasmCreateCodeHash();

            if (txcdCode.IsCreate())
            {
                CWasmCreateCodeContext ctxtWasmCreateCode;
                if (dbBlockBase.RetrieveWasmCreateCodeContext(hashFork, hashPrevBlock, hashWasmCreateCode, ctxtWasmCreateCode))
                {
                    txcdCode.destCodeOwner = ctxtWasmCreateCode.destCodeOwner;
                }
            }
            txcd = txcdCode;
        }
        else if (txcdCode.IsSetup())
        {
            hashWasmCreateCode = txcdCode.GetWasmCreateCodeHash();
            if (!dbBlockBase.GetBlockWasmCreateCodeData(hashFork, hashPrevBlock, hashWasmCreateCode, txcd))
            {
                StdLog("CBlockState", "Get dest wasm code: Get create code data fail, prev block: %s", hashPrevBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            StdLog("CBlockState", "Get dest wasm code: Code flag error, flag: %d, prev block: %s",
                   txcdCode.nMuxType, hashPrevBlock.GetHex().c_str());
            return false;
        }
        btWasmCode = txcd.GetCode();
        destCodeOwner = txcd.GetCodeOwner();

        if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btRunParam))
        {
            btRunParam.clear();
        }
    }
    else
    {
        destWasm = tx.to;
        if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btRunParam))
        {
            btRunParam.clear();
        }
        if (!GetWasmRunCode(destWasm, hashWasmCreateCode, destCodeOwner, btWasmCode))
        {
            StdLog("CBlockState", "Get dest wasm code: Get wasm run code fail, destWasm: %s", destWasm.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockState::AddWasmState(const uint256& txid, const CTransaction& tx, const int nTxIndex, bool& fCallResult)
{
    mapCacheWasmData.clear();
    mapCacheAddressContext.clear();
    mapCacheWasmRunCodeContext.clear();
    vCacheContractTransfer.clear();
    mapCacheCodeDestGasUsed.clear();

    CDestination destWasm;
    bytes btWasmCode;
    bytes btRunParam;
    uint256 hashWasmCreateCode;
    CDestination destCodeOwner;
    CTxContractData txcd;
    bool fCall = false;
    if (!GetDestWasmCode(tx, destWasm, btWasmCode, btRunParam, hashWasmCreateCode, destCodeOwner, txcd, fCall))
    {
        StdLog("CBlockState", "Add wasm state: Get dest wasm code fail, txid: %s", txid.ToString().c_str());
        return false;
    }

    if (fCall)
    {
        CWasmHostDB dbHost(*this, destWasm);
        CWasmRun wasmRun(dbHost, hashFork);

        if (!(fCallResult = wasmRun.RunWasm(tx.from.data, tx.to.data, destWasm.data, destCodeOwner, tx.GetRunGasLimit().Get64(),
                                            tx.nGasPrice, tx.nAmount, hashMintDest, nBlockTimestamp,
                                            nBlockHeight, nSurplusBlockGasLimit, btWasmCode, btRunParam, txcd)))
        {
            StdLog("CBlockState", "Add wasm state: Run wasm fail, txid: %s", txid.ToString().c_str());
            mapCacheWasmData.clear();
            mapCacheAddressContext.clear();
            mapCacheWasmRunCodeContext.clear();
            vCacheContractTransfer.clear();
            mapCacheCodeDestGasUsed.clear();
        }

        if (!DoRunResult(txid, tx, nTxIndex, destWasm, hashWasmCreateCode, wasmRun.nGasLeft,
                         wasmRun.nStatusCode, wasmRun.vResult, wasmRun.logs))
        {
            StdLog("CBlockState", "Add wasm state: Do run result fail, txid: %s", txid.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockState::DoRunResult(const uint256& txid, const CTransaction& tx, const int nTxIndex, const CDestination& destWasm, const uint256& hashWasmCreateCode,
                              const uint64 nGasLeftIn, const int nStatusCode, const bytes& vResult, const CTransactionLogs& logs)
{
    for (const auto& vd : mapCacheWasmData)
    {
        const CDestination& dest = vd.first;
        auto& mapSetWasmKv = mapWasmKvState[dest];
        for (const auto& kv : vd.second.cacheWasmKv)
        {
            mapSetWasmKv[kv.first] = kv.second;
        }
        if (!vd.second.cacheDestState.IsNull())
        {
            SetDestState(dest, vd.second.cacheDestState);
        }
    }
    for (const auto& kv : mapCacheAddressContext)
    {
        if (kv.second.IsContract())
        {
            CContractAddressContext ctxtContractAddress;
            if (kv.second.GetContractAddressContext(ctxtContractAddress))
            {
                ctxtContractAddress.hashCreateTxid = txid;
                mapBlockAddressContext[kv.first] = CAddressContext(ctxtContractAddress, kv.second.nBlockNumber);
            }
        }
        else
        {
            mapBlockAddressContext[kv.first] = kv.second;
        }
    }
    for (const auto& kv : mapCacheWasmRunCodeContext)
    {
        mapBlockWasmRunCodeContext[kv.first] = kv.second;
    }
    for (const auto& vd : vCacheContractTransfer)
    {
        mapBlockContractTransfer[txid].push_back(vd);
    }
    uint256 nTotalCodeFeeUsed;
    uint256 nTxGasUsed;
    for (const auto& kv : mapCacheCodeDestGasUsed)
    {
        if (kv.second > 0)
        {
            const CDestination& destCodeOwner = kv.first;
            uint256 nDestUsedGas(kv.second);
            uint256 nCodeFeeUsed = (nDestUsedGas * tx.nGasPrice) * CODE_REWARD_USED / CODE_REWARD_PER;

            CDestState stateCodeOwner;
            if (!GetDestState(destCodeOwner, stateCodeOwner))
            {
                stateCodeOwner.SetNull();
            }
            stateCodeOwner.nBalance += nCodeFeeUsed;
            SetDestState(destCodeOwner, stateCodeOwner);

            mapBlockCodeDestFeeUsed[txid][destCodeOwner] += nCodeFeeUsed;
            nTotalCodeFeeUsed += nCodeFeeUsed;
            nTxGasUsed += nDestUsedGas;
        }
    }

    mapCacheWasmData.clear();
    mapCacheAddressContext.clear();
    mapCacheWasmRunCodeContext.clear();
    vCacheContractTransfer.clear();
    mapCacheCodeDestGasUsed.clear();

    auto& mapWasmKv = mapWasmKvState[destWasm];
    uint256 nGasLeft;
    if (tx.nGasLimit > nTxGasUsed)
    {
        nGasLeft = tx.nGasLimit - nTxGasUsed;
    }
    mapBlockTxFeeUsed[txid] = nTxGasUsed * tx.nGasPrice;

    CTransactionReceipt receipt;

    receipt.nTxIndex = nTxIndex;
    receipt.nBlockNumber = nBlockNumber;
    receipt.from = tx.from;
    receipt.to = tx.to;
    receipt.nTxGasUsed = nTxGasUsed;
    receipt.destContract = destWasm;
    receipt.hashContractCode = hashWasmCreateCode;
    receipt.nContractStatus = nStatusCode;
    receipt.nContractGasLeft = nGasLeft;
    if (!tx.to.IsNullContract())
    {
        receipt.btContractResult = vResult;
    }
    receipt.logs = logs;
    if (logs.address.size() > 0 || logs.topics.size() > 0)
    {
        CBloomFilter<2048> bf;
        if (logs.address.size() > 0)
        {
            bf.Add(logs.address.data(), logs.address.size());
        }
        for (const auto& vd : logs.topics)
        {
            bf.Add(vd.data(), vd.size());
        }
        bytes btLogsBloom;
        bf.GetData(btLogsBloom);
        receipt.nLogsBloom = uint2048(btLogsBloom);

        nBlockBloom |= receipt.nLogsBloom;
    }

    hcode::CBufStream ss;
    ss << receipt;

    bytes btReceipt(ss.GetData(), ss.GetData() + ss.GetSize());
    mapWasmKv[txid] = btReceipt;

    uint256 hashReceipt = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());
    vReceiptHash.push_back(hashReceipt);

    nSurplusBlockGasLimit -= nTxGasUsed.Get64();
    if (nGasLeft != 0)
    {
        CDestState stateDest;
        if (!GetDestState(tx.from, stateDest))
        {
            StdLog("CBlockState", "Set run result: Get dest state fail, txid: %s", txid.ToString().c_str());
            return false;
        }
        stateDest.nBalance += (tx.nGasPrice * nGasLeft);
        SetDestState(tx.from, stateDest);

        nBlockFeeLeft += (tx.nGasPrice * nGasLeft);
    }
    nBlockFeeLeft += nTotalCodeFeeUsed;
    return true;
}

//////////////////////////////
// CWasmHostDB

bool CWasmHostDB::Get(const uint256& key, bytes& value) const
{
    return blockState.GetDestKvData(destWasm, key, value);
}

bool CWasmHostDB::GetBalance(const uint256& addr, uint256& balance) const
{
    CAddressContext ctxtAddress;
    if (!blockState.RetrieveAddressContext(addr, ctxtAddress))
    {
        StdLog("CWasmHostDB", "Get Balance: Retrieve address context fail, addr: %s", addr.ToString().c_str());
        return false;
    }
    CDestination dest(ctxtAddress.nType, addr);

    CDestState stateDest;
    if (!blockState.GetDestState(dest, stateDest))
    {
        StdLog("CWasmHostDB", "Get Balance: Get dest state fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    balance = stateDest.nBalance;
    return true;
}

bool CWasmHostDB::Transfer(const uint256& from, const uint256& to, const uint256& amount)
{
    if (!blockState.ContractTransfer(from, to, amount))
    {
        StdLog("CWasmHostDB", "Transfer: Contract transfer fail, from: %s", from.ToString().c_str());
        return false;
    }
    return true;
}

bool CWasmHostDB::GetWasmRunCode(const uint256& hashWasm, uint256& hashWasmCreateCode, CDestination& destCodeOwner, bytes& btWasmRunCode)
{
    CDestination destWasmNs;
    destWasmNs.SetContractId(hashWasm);
    return blockState.GetWasmRunCode(destWasmNs, hashWasmCreateCode, destCodeOwner, btWasmRunCode);
}

bool CWasmHostDB::GetWasmCreateCode(const uint256& hashWasm, CTxContractData& txcd)
{
    CDestination destWasmNs;
    destWasmNs.SetContractId(hashWasm);
    return blockState.GetWasmCreateCode(destWasmNs, txcd);
}

bool CWasmHostDB::SaveWasmRunCode(const uint256& hashWasm, const bytes& btWasmRunCode, const CTxContractData& txcd)
{
    CDestination destWasmNs;
    destWasmNs.SetContractId(hashWasm);
    return blockState.SaveWasmRunCode(destWasmNs, btWasmRunCode, txcd);
}

CHostBaseDBPtr CWasmHostDB::CloneHostDB(const uint256& hashWasm)
{
    CDestination destWasmNs;
    destWasmNs.SetContractId(hashWasm);
    return CHostBaseDBPtr(new CWasmHostDB(blockState, destWasmNs));
}

void CWasmHostDB::SaveGasUsed(const CDestination& destCodeOwner, const uint64 nGasUsed)
{
    blockState.SaveGasUsed(destCodeOwner, nGasUsed);
}

void CWasmHostDB::SaveRunResult(const uint256& hashWasm, const CTransactionLogs& logs, const std::map<uint256, bytes>& mapCacheKv)
{
    CDestination destWasmNs;
    destWasmNs.SetContractId(hashWasm);
    blockState.SaveRunResult(destWasmNs, logs, mapCacheKv);
}

//////////////////////////////
// CBlockBase

CBlockBase::CBlockBase()
  : fCfgFullDb(false)
{
}

CBlockBase::~CBlockBase()
{
    dbBlock.Deinitialize();
    tsBlock.Deinitialize();
}

bool CBlockBase::Initialize(const path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fFullDbIn, const bool fRenewDB)
{
    hashGenesisBlock = hashGenesisBlockIn;
    fCfgFullDb = fFullDbIn;

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

bool CBlockBase::Exists(const uint256& hash) const
{
    CReadLock rlock(rwAccess);

    return (!!mapIndex.count(hash));
}

bool CBlockBase::ExistsTx(const uint256& txid)
{
    uint256 hashFork;
    CTxIndex txIndex;
    return GetTxIndex(txid, txIndex, hashFork);
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

    CWriteLock wlock(rwAccess);

    CBlockEx blockex(blockGenesis, nChainTrust);

    CBlockIndex* pIndexNew = nullptr;
    CBlockRoot blockRoot;
    if (!SaveBlock(hashGenesis, hashGenesis, blockex, &pIndexNew, blockRoot, false))
    {
        StdError("BlockBase", "Add genesis block: Save block failed, block: %s", hashGenesis.ToString().c_str());
        return false;
    }

    UpdateBlockNext(pIndexNew);
    if (!dbBlock.UpdateForkLast(hashGenesis, pIndexNew->GetBlockHash()))
    {
        StdError("BlockBase", "Add genesis block: Update fork last fail, fork: %s", hashGenesis.ToString().c_str());
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
        return false;
    }

    CBlockIndex* pIndexNew = nullptr;
    CBlockRoot blockRoot;
    if (!SaveBlock(hashFork, hashBlock, block, &pIndexNew, blockRoot, false))
    {
        StdError("BlockBase", "Storage new block: Save block failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    if (CheckForkLongChain(hashFork, hashBlock, block, pIndexNew))
    {
        update = CBlockChainUpdate(pIndexNew);
        if (block.IsOrigin())
        {
            update.vBlockAddNew.push_back(block);
        }
        else
        {
            if (!GetBlockBranchListNolock(hashBlock, update.vBlockRemove, update.vBlockAddNew))
            {
                StdLog("BlockBase", "Storage new block: Get block branch list fail, block: %s", hashBlock.ToString().c_str());
                return false;
            }
        }

        UpdateBlockNext(pIndexNew);
        if (!dbBlock.UpdateForkLast(hashFork, hashBlock))
        {
            StdError("BlockBase", "Storage new block: Update fork last fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }

        StdLog("CBlockChain", "Storage new block: Long chain, new block height: %d, block type: %s, block: %s, fork chain trust: %s, fork: %s",
               pIndexNew->GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.nType).c_str(), hashBlock.GetHex().c_str(),
               pIndexNew->nChainTrust.GetHex().c_str(), pIndexNew->GetOriginHash().GetHex().c_str());
    }
    return true;
}

bool CBlockBase::SaveBlock(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, CBlockIndex** ppIndexNew, CBlockRoot& blockRoot, const bool fRepair)
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
        if (!UpdateBlockTxIndex(hashFork, block, nFile, nOffset, blockRoot.hashTxIndexRoot))
        {
            StdError("BlockBase", "Save block: Update block tx index failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        std::map<uint256, CAddressContext> mapTempAddressContext;
        std::map<uint256, CWasmRunCodeContext> mapTempWasmRunCodeContext;
        std::map<uint256, std::vector<CContractTransfer>> mapTempContractTransfer;
        std::map<uint256, uint256> mapTempBlockTxFeeUsed;
        std::map<uint256, std::map<CDestination, uint256>> mapBlockCodeDestFeeUsed;
        if (!UpdateBlockState(hashFork, hashBlock, block, blockRoot.hashStateRoot, mapTempAddressContext, mapTempWasmRunCodeContext, mapTempContractTransfer, mapTempBlockTxFeeUsed, mapBlockCodeDestFeeUsed))
        {
            StdError("BlockBase", "Save block: Update block state failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockCode(hashFork, hashBlock, block, nFile, nOffset, mapTempWasmRunCodeContext, blockRoot.hashCodeRoot))
        {
            StdError("BlockBase", "Save block: Update block code failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (!UpdateBlockAddress(hashFork, hashBlock, block, mapTempAddressContext, blockRoot.hashAddressRoot))
        {
            StdError("BlockBase", "Save block: Update block address failed, block: %s", hashBlock.ToString().c_str());
            fRet = false;
            break;
        }

        if (fCfgFullDb)
        {
            uint256 hashAddressTxInfoRoot;
            if (!UpdateBlockAddressTxInfo(hashFork, hashBlock, block, mapTempContractTransfer, mapTempBlockTxFeeUsed, mapBlockCodeDestFeeUsed, hashAddressTxInfoRoot))
            {
                StdError("BlockBase", "Save block: Update block address tx info failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }
        }

        if (pIndexNew->IsPrimary())
        {
            if (!AddBlockForkContext(hashBlock, block, blockRoot.hashForkContextRoot))
            {
                StdError("BlockBase", "Save block: Add bock fork context failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateDelegate(hashBlock, block, nFile, nOffset, DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT, blockRoot.hashDelegateRoot))
            {
                StdError("BlockBase", "Save block: Update delegate failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateVote(hashFork, hashBlock, block, mapTempAddressContext, blockRoot.hashVoteRoot))
            {
                StdError("BlockBase", "Save block: Update vote failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateVoteRedeem(hashFork, hashBlock, block, mapTempAddressContext, blockRoot.hashVoteRedeemRoot))
            {
                StdError("BlockBase", "Save block: Update vote redeem failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }

            if (!UpdateBlockInvite(hashFork, hashBlock, block, mapTempAddressContext, blockRoot.hashInviteRoot))
            {
                StdError("BlockBase", "Save block: Update block invite failed, block: %s", hashBlock.ToString().c_str());
                fRet = false;
                break;
            }
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
    } while (0);

    if (!fRet)
    {
        RemoveBlockIndex(hashFork, hashBlock);
        delete pIndexNew;
        return false;
    }
    *ppIndexNew = pIndexNew;

    StdLog("BlockBase", "Save block: Save block success, block: %s", hashBlock.ToString().c_str());

#if 1
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
            uint256 nBlockReward = 0;
            if (block.txMint.nType == CTransaction::TX_STAKE)
            {
                nBlockReward = block.GetBlockTotalReward();
            }
            else
            {
                nBlockReward = block.txMint.nAmount;
            }
            uint256 nBlockFee = nBlockReward - nBlockMint;
            uint256 nStateAmount = nTotalAmount - block.txMint.nAmount + nBlockFee;
            uint256 nPrevMoneySupply = pIndexNew->GetMoneySupply() - nBlockMint;
            if (nPrevMoneySupply != nStateAmount)
            {
                StdLog("BlockBase", "Save block: State amount error, height: %d, money supply: %s, state amount: %s, total amount: %s, block reward: %s, block amount: %s",
                       block.GetBlockHeight(), CoinToTokenBigFloat(nPrevMoneySupply).c_str(),
                       CoinToTokenBigFloat(nStateAmount).c_str(),
                       CoinToTokenBigFloat(nTotalAmount).c_str(), CoinToTokenBigFloat(nBlockReward).c_str(),
                       CoinToTokenBigFloat(block.txMint.nAmount).c_str());
            }
            else
            {
                StdLog("BlockBase", "Save block: State amount success, height: %d, money supply: %s, block reward: %s, block amount: %s",
                       block.GetBlockHeight(), CoinToTokenBigFloat(nPrevMoneySupply).c_str(),
                       CoinToTokenBigFloat(nBlockReward).c_str(), CoinToTokenBigFloat(block.txMint.nAmount).c_str());
            }
        }
    }
#endif
    return true;
}

bool CBlockBase::CheckForkLongChain(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const CBlockIndex* pIndexNew)
{
    if (!pIndexNew->IsPrimary())
    {
        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
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

        if (!VerifyRefBlock(hashGenesisBlock, proof.hashRefBlock))
        {
            StdLog("BlockBase", "Add long chain last: Ref block short chain, refblock: %s, new block: %s, fork: %s",
                   proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
    }

    CBlockIndex* pIndexFork = GetForkLastIndex(hashFork);
    if (pIndexFork
        && (pIndexFork->nChainTrust > pIndexNew->nChainTrust
            || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
    {
        StdLog("BlockBase", "Add long chain last: Short chain, new block height: %d, block type: %s, block: %s, fork chain trust: %s, fork last block: %s, fork: %s",
               pIndexNew->GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.nType).c_str(), hashBlock.GetHex().c_str(),
               pIndexFork->nChainTrust.GetHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str(), pIndexFork->GetOriginHash().GetHex().c_str());
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

bool CBlockBase::RetrieveProfile(const uint256& hashFork, CProfile& profile)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
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

    CTxIndex txIndex;
    CTransaction tx;
    if (!RetrieveTx(hashGenesisBlock, uint256(), ctxt.txidEmbedded, txIndex, tx))
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

bool CBlockBase::RetrieveTx(const uint256& txid, CTransaction& tx)
{
    CTxIndex txIndex;
    return RetrieveTx(uint256(), uint256(), txid, txIndex, tx);
}

bool CBlockBase::RetrieveTx(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight)
{
    tx.SetNull();
    CTxIndex txIndex;
    if (!GetTxIndex(txid, txIndex, hashFork))
    {
        StdTrace("BlockBase", "Retrieve Tx: Get tx index fail, txid: %s", txid.ToString().c_str());
        return false;
    }
    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset, false, true))
    {
        StdTrace("BlockBase", "Retrieve Tx: Read fail, txid: %s", txid.ToString().c_str());
        return false;
    }
    nHeight = txIndex.nBlockHeight;
    return true;
}

bool CBlockBase::RetrieveTx(const uint256& hashFork, const uint256& txid, CTxIndex& txIndex, CTransaction& tx)
{
    return RetrieveTx(hashFork, uint256(), txid, txIndex, tx);
}

bool CBlockBase::RetrieveTx(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTransaction& tx)
{
    CTxIndex txIndex;
    return RetrieveTx(hashFork, hashBlock, txid, txIndex, tx);
}

bool CBlockBase::RetrieveTx(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex, CTransaction& tx)
{
    if (!GetTxIndex(hashFork, hashBlock, txid, txIndex))
    {
        StdLog("BlockBase", "Retrieve Tx: Get tx index fail, forkid: %s, block: %s, txid: %s",
               hashFork.ToString().c_str(), hashBlock.GetHex().c_str(), txid.ToString().c_str());
        return false;
    }
    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset, false, true))
    {
        StdLog("BlockBase", "Retrieve Tx: Read fail, nFile: %d, nOffset: %d, txid: %s",
               txIndex.nFile, txIndex.nOffset, txid.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTxLocation(const uint256& txid, uint256& hashFork, int& nHeight)
{
    CTxIndex txIndex;
    if (!GetTxIndex(txid, txIndex, hashFork))
    {
        StdTrace("BlockBase", "Retrieve Tx Location: Get tx index fail, txid: %s", txid.ToString().c_str());
        return false;
    }
    nHeight = txIndex.nBlockHeight;
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
    if (!dbBlock.RetrieveEnroll(height, vBlockRange, mapEnrollTxPos))
    {
        StdTrace("BlockBase", "Retrieve Avail Delegate::RetrieveEnroll block %s height %d failed",
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

                bytes btTempData;
                if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btTempData))
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate: tx data error1, txid: %s",
                           tx.GetHash().ToString().c_str());
                    return false;
                }
                int nBlockHeight;
                bytes btCertData;
                try
                {
                    hcode::CBufStream ss(btTempData);
                    ss >> nBlockHeight >> btCertData;
                }
                catch (std::exception& e)
                {
                    StdLog("BlockBase", "Retrieve Avail Delegate: tx data error2, txid: %s",
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

    return GetBlockBranchListNolock(hashBlock, vRemoveBlockInvertedOrder, vAddBlockPositiveOrder);
}

bool CBlockBase::GetBlockBranchListNolock(const uint256& hashBlock, std::vector<CBlockEx>& vRemoveBlockInvertedOrder, std::vector<CBlockEx>& vAddBlockPositiveOrder)
{
    CBlockIndex* pIndex = GetIndex(hashBlock);
    if (pIndex == nullptr)
    {
        return false;
    }

    CBlockIndex* pForkLast = GetForkLastIndex(pIndex->GetOriginHash());
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
        CBlockEx block;
        if (!tsBlock.Read(block, vPath[i]->nFile, vPath[i]->nOffset, true, true))
        {
            return false;
        }
        vAddBlockPositiveOrder.push_back(block);
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

    UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, pIndexNew->nTimeStamp, CDestination(), pIndexNew->GetRefBlock());
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

bool CBlockBase::AddBlockForkContext(const uint256& hashBlock, const CBlockEx& blockex, uint256& hashNewRoot)
{
    map<uint256, CForkContext> mapNewForkCtxt;
    if (hashBlock == hashGenesisBlock)
    {
        CProfile profile;
        if (!profile.Load(blockex.vchProof))
        {
            StdError("BlockBase", "Add block fork context: Load genesis block proof failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }

        CForkContext ctxt(hashBlock, uint64(0), blockex.txMint.GetHash(), profile);
        ctxt.nJointHeight = 0;
        mapNewForkCtxt.insert(make_pair(hashBlock, ctxt));
    }
    else
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        for (size_t i = 0; i < blockex.vtx.size(); i++)
        {
            const CTransaction& tx = blockex.vtx[i];
            if (tx.to.IsTemplate() && tx.to.GetTemplateId().GetType() == TEMPLATE_FORK)
            {
                if (!VerifyBlockForkTx(blockex.hashPrev, tx, vForkCtxt))
                {
                    StdLog("BlockBase", "Add block fork context: Verify block fork tx fail, block: %s", hashBlock.ToString().c_str());
                }
            }
            if (tx.from.IsTemplate() && tx.from.GetTemplateId().GetType() == TEMPLATE_FORK)
            {
                auto it = vForkCtxt.begin();
                while (it != vForkCtxt.end())
                {
                    if (it->first == tx.from)
                    {
                        StdLog("BlockBase", "Add block fork context: cancel fork, block: %s, fork: %s, dest: %s",
                               hashBlock.ToString().c_str(), it->second.hashFork.ToString().c_str(),
                               tx.from.ToString().c_str());
                        vForkCtxt.erase(it++);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
        for (const auto& vd : vForkCtxt)
        {
            mapNewForkCtxt.insert(make_pair(vd.second.hashFork, vd.second));
        }
    }

    if (!dbBlock.AddForkContext(blockex.hashPrev, hashBlock, mapNewForkCtxt, hashNewRoot))
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
    if (!profile.Load(block.vchProof))
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

        map<uint256, CForkContext> mapPrevForkCtxt;
        if (!dbBlock.ListForkContext(mapPrevForkCtxt, hashPrev))
        {
            mapPrevForkCtxt.clear();
        }
        if (mapPrevForkCtxt.count(hashNewFork) > 0)
        {
            StdLog("BlockBase", "Verify block fork tx: fork existed, new fork: %s, tx: %s",
                   hashNewFork.GetHex().c_str(), tx.GetHash().ToString().c_str());
            break;
        }
        const uint16 nNewForkSn = CTransaction::GetForkSn(hashNewFork);
        bool fSame = false;
        for (const auto& kv : mapPrevForkCtxt)
        {
            if (kv.second.strName == profile.strName)
            {
                StdLog("BlockBase", "Verify block fork tx: fork name repeated, new fork: %s, name: %s, tx: %s",
                       hashNewFork.GetHex().c_str(), kv.second.strName.c_str(), tx.GetHash().ToString().c_str());
                fSame = true;
                break;
            }
            if (CTransaction::GetForkSn(kv.first) == nNewForkSn)
            {
                StdLog("BlockBase", "Verify block fork tx: fork sn repeated, new fork: %s, name: %s, tx: %s",
                       hashNewFork.GetHex().c_str(), kv.second.strName.c_str(), tx.GetHash().ToString().c_str());
                fSame = true;
                break;
            }
        }
        if (fSame)
        {
            break;
        }

        bool fCheckRet = true;
        for (const auto& vd : vForkCtxt)
        {
            if (vd.second.hashFork == hashNewFork || vd.second.strName == profile.strName)
            {
                StdLog("BlockBase", "Verify block fork tx: fork existed or name repeated, tx: %s, new fork: %s, name: %s",
                       tx.GetHash().ToString().c_str(), hashNewFork.GetHex().c_str(), vd.second.strName.c_str());
                fCheckRet = false;
                break;
            }
        }
        if (!fCheckRet)
        {
            break;
        }

        vForkCtxt.push_back(make_pair(tx.to, CForkContext(block.GetHash(), block.hashPrev, tx.GetHash(), profile)));
        StdLog("BlockBase", "Verify block fork tx success: valid fork: %s, tx: %s", hashNewFork.GetHex().c_str(), tx.GetHash().ToString().c_str());
    } while (0);

    return true;
}

bool CBlockBase::VerifyOriginBlock(const CBlock& block, const CProfile& parentProfile)
{
    CProfile forkProfile;
    if (!forkProfile.Load(block.vchProof))
    {
        StdLog("BlockBase", "Verify origin block: load profile error");
        return false;
    }
    if (forkProfile.IsNull())
    {
        StdLog("BlockBase", "Verify origin block: invalid profile");
        return false;
    }
    //if (!MoneyRange(forkProfile.nAmount))
    //{
    //    StdLog("BlockBase", "Verify origin block: invalid fork amount");
    //    return false;
    //}
    //if (!RewardRange(forkProfile.nMintReward))
    //{
    //    StdLog("BlockBase", "Verify origin block: invalid fork reward");
    //    return false;
    //}
    if (block.txMint.to != forkProfile.destOwner)
    {
        StdLog("BlockBase", "Verify origin block: invalid fork to");
        return false;
    }
    if (parentProfile.IsPrivate())
    {
        if (!forkProfile.IsPrivate() || parentProfile.destOwner != forkProfile.destOwner)
        {
            StdLog("BlockBase", "Verify origin block: permission denied");
            return false;
        }
    }
    return true;
}

bool CBlockBase::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return dbBlock.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockBase::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashBlock)
{
    return dbBlock.RetrieveForkContext(hashFork, ctxt, hashBlock);
}

bool CBlockBase::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbBlock.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockBase::GetForkIdByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashBlock)
{
    return dbBlock.GetForkIdByForkName(strForkName, hashFork, hashBlock);
}

bool CBlockBase::GetForkIdByForkSn(const uint16 nForkSn, uint256& hashFork, const uint256& hashBlock)
{
    return dbBlock.GetForkIdByForkSn(nForkSn, hashFork, hashBlock);
}

int CBlockBase::GetForkCreatedHeight(const uint256& hashFork)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hashFork, ctxt))
    {
        return false;
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

bool CBlockBase::GetTxToAddressTemplateData(const uint256& hashFork, const uint256& hashPrevBlock, const CTransaction& tx, const std::map<uint256, CAddressContext>& mapBlockAddressContext, bytes& btTemplateData)
{
    auto mt = mapBlockAddressContext.find(tx.to.data);
    if (mt != mapBlockAddressContext.end())
    {
        if (!mt->second.IsTemplate())
        {
            StdError("BlockBase", "Get to address template data: Address not is template, to: %s, hashPrev: %s",
                     tx.to.ToString().c_str(), hashPrevBlock.ToString().c_str());
            return false;
        }
        CTemplateAddressContext ctxtTemplate;
        if (!mt->second.GetTemplateAddressContext(ctxtTemplate))
        {
            StdError("BlockBase", "Get to address template data: Get template address context fail, to: %s, hashPrev: %s",
                     tx.to.ToString().c_str(), hashPrevBlock.ToString().c_str());
            return false;
        }
        btTemplateData = ctxtTemplate.btData;
    }
    else
    {
        CAddressContext ctxtAddress;
        if (!dbBlock.RetrieveAddressContext(hashFork, hashPrevBlock, tx.to.data, ctxtAddress))
        {
            if (!tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTemplateData))
            {
                StdError("BlockBase", "Get to address template data: Retrieve address context failed and tx data error, to: %s, hashPrev: %s",
                         tx.to.ToString().c_str(), hashPrevBlock.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (!ctxtAddress.IsTemplate())
            {
                StdError("BlockBase", "Get to address template data: Address not is template, to: %s, hashPrev: %s",
                         tx.to.ToString().c_str(), hashPrevBlock.ToString().c_str());
                return false;
            }
            CTemplateAddressContext ctxtTemplate;
            if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
            {
                StdError("BlockBase", "Get to address template data: Get template address context failed, to: %s, hashPrev: %s",
                         tx.to.ToString().c_str(), hashPrevBlock.ToString().c_str());
                return false;
            }
            btTemplateData = ctxtTemplate.btData;
        }
    }
    return true;
}

bool CBlockBase::GetTxFromAddressTemplateData(const uint256& hashFork, const uint256& hashPrevBlock, const CTransaction& tx, const std::map<uint256, CAddressContext>& mapBlockAddressContext, bytes& btTemplateData)
{
    CAddressContext ctxtAddress;
    auto mt = mapBlockAddressContext.find(tx.from.data);
    if (mt != mapBlockAddressContext.end())
    {
        ctxtAddress = mt->second;
    }
    else
    {
        if (!dbBlock.RetrieveAddressContext(hashFork, hashPrevBlock, tx.from.data, ctxtAddress))
        {
            StdError("BlockBase", "Get from address template data: Retrieve address context failed, from: %s, hashPrev: %s",
                     tx.from.ToString().c_str(), hashPrevBlock.ToString().c_str());
            return false;
        }
    }
    if (!ctxtAddress.IsTemplate())
    {
        StdError("BlockBase", "Get from address template data: Address not is template, from: %s, hashPrev: %s",
                 tx.from.ToString().c_str(), hashPrevBlock.ToString().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdError("BlockBase", "Get from address template data: Get template address context failed, from: %s, hashPrev: %s",
                 tx.from.ToString().c_str(), hashPrevBlock.ToString().c_str());
        return false;
    }
    btTemplateData = ctxtTemplate.btData;
    return true;
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

bool CBlockBase::GetVotes(const uint256& hashGenesis, const CDestination& destDelegate, uint256& nVotes)
{
    CBlockIndex* pForkLastIndex = nullptr;
    if (!RetrieveFork(hashGenesis, &pForkLastIndex))
    {
        return false;
    }
    std::map<CDestination, uint256> mapVote;
    if (!dbBlock.RetrieveDelegate(pForkLastIndex->GetBlockHash(), mapVote))
    {
        return false;
    }
    nVotes = mapVote[destDelegate];
    return true;
}

bool CBlockBase::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbBlock.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockBase::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbBlock.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockBase::RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destRedeem, CVoteRedeemContext& ctxtVoteRedeem)
{
    return dbBlock.RetrieveDestVoteRedeemContext(hashBlock, destRedeem, ctxtVoteRedeem);
}

bool CBlockBase::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbBlock.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockBase::GetDelegateMintRewardRatio(const uint256& hashBlock, const CDestination& destDelegate, uint32& nRewardRation)
{
    CAddressContext ctxtAddress;
    if (!dbBlock.RetrieveAddressContext(hashGenesisBlock, hashBlock, destDelegate.data, ctxtAddress))
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Retrieve address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (!ctxtAddress.IsTemplate())
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Address not is template, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Get template address context fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_DELEGATE, ctxtTemplate.btData);
    if (ptr == nullptr)
    {
        StdLog("CBlockState", "Get Delegate Mint Reward Ratio: Create template fail, delegate: %s, block: %s",
               destDelegate.ToString().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    nRewardRation = boost::dynamic_pointer_cast<CTemplateDelegate>(ptr)->nRewardRatio;
    return true;
}

bool CBlockBase::GetDelegateList(const uint256& hashGenesis, uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    CBlockIndex* pForkLastIndex = nullptr;
    if (!RetrieveFork(hashGenesis, &pForkLastIndex))
    {
        return false;
    }
    std::map<CDestination, uint256> mapVote;
    if (!dbBlock.RetrieveDelegate(pForkLastIndex->GetBlockHash(), mapVote))
    {
        return false;
    }
    for (const auto& d : mapVote)
    {
        mapVotes.insert(std::make_pair(d.second, d.first));
    }
    if (nCount > 0)
    {
        std::size_t nGetVotesCount = mapVotes.size();
        auto it = mapVotes.begin();
        while (it != mapVotes.end() && nGetVotesCount > nCount)
        {
            mapVotes.erase(it++);
            --nGetVotesCount;
        }
    }
    return true;
}

bool CBlockBase::VerifyRepeatBlock(const uint256& hashFork, uint32 height, const CDestination& destMint, uint16 nBlockType,
                                   uint32 nBlockTimeStamp, uint32 nRefBlockTimeStamp, uint32 nExtendedBlockSpacing)
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
                if (mt.second.destMint.IsNull())
                {
                    CBlockIndex* pBlockIndex = GetIndex(mt.first);
                    if (pBlockIndex)
                    {
                        if (pBlockIndex->IsVacant())
                        {
                            CBlock block;
                            if (Retrieve(pBlockIndex, block) && !block.txMint.to.IsNull())
                            {
                                mt.second.destMint = block.txMint.to;
                            }
                        }
                        else
                        {
                            CTransaction tx;
                            if (RetrieveTx(hashFork, pBlockIndex->GetBlockHash(), pBlockIndex->txidMint, tx))
                            {
                                mt.second.destMint = tx.to;
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
                            StdTrace("BlockBase", "VerifyRepeatBlock: subsidiary or extended repeat block, block time: %d, cache block time: %d, ref block time: %d, destMint: %s",
                                     nBlockTimeStamp, mt.second.nTimeStamp, mt.second.nTimeStamp, destMint.ToString().c_str());
                            return false;
                        }
                    }
                    else
                    {
                        StdTrace("BlockBase", "VerifyRepeatBlock: repeat block: %s, destMint: %s", mt.first.GetHex().c_str(), destMint.ToString().c_str());
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

bool CBlockBase::GetDelegateEnrollTx(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbBlock.RetrieveEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockBase::GetBlockDelegatedEnrollTx(const uint256& hashBlock, map<int, set<CDestination>>& mapEnrollDest)
{
    map<int, map<CDestination, CDiskPos>> mapEnrollTxPos;
    if (!dbBlock.RetrieveEnroll(hashBlock, mapEnrollTxPos))
    {
        return false;
    }
    for (const auto& d : mapEnrollTxPos)
    {
        set<CDestination>& destEnroll = mapEnrollDest[d.first];
        for (const auto& m : d.second)
        {
            destEnroll.insert(m.first);
        }
    }
    return true;
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
            if (!block.vchProof.empty())
            {
                CProofOfPiggyback proof;
                if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
                {
                    hashRefBlock = proof.hashRefBlock;
                    pIndexUpdateRef = pIndex;
                    break;
                }
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

    const std::map<uint256, CBlockHeightIndex>* pMapHeight = mapForkHeightIndex[hashGenesisBlock].GetBlockMintList(nHeight);
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

bool CBlockBase::UpdateForkNext(const uint256& hashFork, CBlockIndex* pIndexLast)
{
    CWriteLock wlock(rwAccess);

    UpdateBlockNext(pIndexLast);
    return dbBlock.UpdateForkLast(hashFork, pIndexLast->GetBlockHash());
}

bool CBlockBase::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbBlock.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

bool CBlockBase::CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, const uint256& hashPrevStateRoot, uint256& hashStateRoot, uint256& hashReceiptRoot, uint256& nBlockGasUsed,
                                      uint2048& nBlockBloom, const bool fCreateBlock, const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut)
{
    CBloomFilter<2048> bf;
    CBlockState blockState(*this, hashFork, block, hashPrevStateRoot);

    for (int i = 0; i < (int)(block.vtx.size()); i++)
    {
        const auto& tx = block.vtx[i];
        if (!blockState.AddTxState(tx, i + 1))
        {
            StdLog("BlockBase", "Create block state root: Add tx state fail, txid: %s, prev: %s",
                   tx.GetHash().ToString().c_str(), block.hashPrev.GetHex().c_str());
            return false;
        }
        if (!tx.from.IsNull())
        {
            bf.Add(tx.from.data.begin(), tx.from.data.size());
        }
        if (!tx.to.IsNull())
        {
            bf.Add(tx.to.data.begin(), tx.to.data.size());
        }
    }

    if (!block.txMint.to.IsNull())
    {
        bf.Add(block.txMint.to.data.begin(), block.txMint.to.data.size());
    }

    if (!blockState.DoBlockState(hashReceiptRoot, nBlockGasUsed, nBlockBloom, fCreateBlock,
                                 fMintRatio, nTotalMintRewardOut, nBlockMintRewardOut))
    {
        StdLog("BlockBase", "Create block state root: Do block state fail, prev: %s", block.hashPrev.GetHex().c_str());
        return false;
    }

    bytes btBloom;
    bf.GetData(btBloom);
    nBlockBloom |= uint2048(btBloom);

    if (!dbBlock.CreateCacheStateTrie(hashFork, hashPrevStateRoot, hashStateRoot, blockState.mapBlockState))
    {
        StdLog("BlockBase", "Create block state root: Create cache state trie fail, prev: %s", block.hashPrev.GetHex().c_str());
        return false;
    }

    auto mt = mapCacheBlockState.find(hashStateRoot);
    if (mt != mapCacheBlockState.end())
    {
        mapCacheBlockState.erase(mt);
    }
    while (mapCacheBlockState.size() > MAX_CACHE_BLOCK_STATE)
    {
        mapCacheBlockState.erase(mapCacheBlockState.begin());
    }
    mapCacheBlockState.insert(make_pair(hashStateRoot, CCacheBlockState(hashPrevStateRoot, hashStateRoot, blockState.mapBlockState,
                                                                        blockState.mapBlockAddressContext, blockState.mapBlockWasmRunCodeContext,
                                                                        blockState.mapBlockContractTransfer, blockState.mapBlockTxFeeUsed, blockState.mapBlockCodeDestFeeUsed)));
    return true;
}

bool CBlockBase::UpdateBlockState(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, uint256& hashNewStateRoot,
                                  std::map<uint256, CAddressContext>& mapAddressContextOut, std::map<uint256, CWasmRunCodeContext>& mapWasmRunCodeContextOut,
                                  std::map<uint256, std::vector<CContractTransfer>>& mapBlockContractTransferOut, std::map<uint256, uint256>& mapBlockTxFeeUsedOut,
                                  std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedOut)
{
    uint256 hashStateRoot = block.hashStateRoot;
    auto it = mapCacheBlockState.find(hashStateRoot);
    if (hashStateRoot == 0 || it == mapCacheBlockState.end())
    {
        uint256 hashPrevStateRoot;
        if (!block.IsOrigin() && block.hashPrev != 0)
        {
            CBlockIndex* pIndexPrev = GetIndex(block.hashPrev);
            if (pIndexPrev == nullptr)
            {
                StdLog("BlockBase", "Update block state: Get prev index fail, prev: %s", block.hashPrev.GetHex().c_str());
                return false;
            }
            hashPrevStateRoot = pIndexPrev->GetStateRoot();
        }

        uint256 nTotalMintReward;
        uint256 nBlockMintReward;
        uint256 nBlockGasUsed;
        uint256 hashReceiptRoot;
        uint2048 nBlockBloom;
        if (!CreateBlockStateRoot(hashFork, block, hashPrevStateRoot, hashStateRoot, hashReceiptRoot, nBlockGasUsed, nBlockBloom,
                                  false, false, nTotalMintReward, nBlockMintReward))
        {
            StdLog("BlockBase", "Update block state: Create block state root fail, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        if (block.hashStateRoot != 0 && block.hashStateRoot != hashStateRoot)
        {
            StdLog("BlockBase", "Update block state: Create state root error, block state root: %s, calc state root: %s, prev state root: %s, block: [type: %d] %s",
                   block.hashStateRoot.GetHex().c_str(), hashStateRoot.GetHex().c_str(),
                   hashPrevStateRoot.GetHex().c_str(), block.nType, block.GetHash().GetHex().c_str());
            return false;
        }
        if (block.hashReceiptsRoot != hashReceiptRoot)
        {
            StdLog("BlockBase", "Update block state: Create receipt root error, block receipt root: %s, calc receipt  root: %s, block: %s",
                   block.hashReceiptsRoot.GetHex().c_str(), hashReceiptRoot.GetHex().c_str(), block.GetHash().GetHex().c_str());
            return false;
        }
        it = mapCacheBlockState.find(hashStateRoot);
        if (it == mapCacheBlockState.end())
        {
            StdLog("BlockBase", "Update block state: Find cache state fail, state root: %s, block: %s",
                   hashStateRoot.GetHex().c_str(), block.GetHash().GetHex().c_str());
            return false;
        }
    }
    if (!dbBlock.AddBlockState(hashFork, it->second.hashPrevRoot, hashStateRoot, it->second.mapAccState))
    {
        StdLog("BlockBase", "Update block state: Add block state fail, block: %s", block.GetHash().GetHex().c_str());
        return false;
    }
    if (block.hashStateRoot != 0 && block.hashStateRoot != hashStateRoot)
    {
        StdLog("BlockBase", "Update block state: Add state root error, block state root: %s, calc state  root: %s, block: %s",
               block.hashStateRoot.GetHex().c_str(), hashStateRoot.GetHex().c_str(), block.GetHash().GetHex().c_str());
        return false;
    }
    hashNewStateRoot = hashStateRoot;
    mapAddressContextOut = it->second.mapBlockStateAddressContext;
    mapWasmRunCodeContextOut = it->second.mapBlockStateWasmRunCodeContext;
    mapBlockContractTransferOut = it->second.mapBlockStateContractTransfer;
    mapBlockTxFeeUsedOut = it->second.mapBlockStateTxFeeUsed;
    mapBlockCodeDestFeeUsedOut = it->second.mapBlockStateCodeDestFeeUsed;
    return true;
}

bool CBlockBase::UpdateBlockTxIndex(const uint256& hashFork, const CBlockEx& block, const uint32 nFile, const uint32 nOffset, uint256& hashNewRoot)
{
    CBufStream ss;
    std::map<uint256, CTxIndex> mapBlockTxIndex;

    uint32 nTxOffset = nOffset + block.GetTxSerializedOffset();

    mapBlockTxIndex.insert(make_pair(block.txMint.GetHash(), CTxIndex(block.GetBlockHeight(), block.GetBlockNumber(), 0, nFile, nTxOffset)));
    nTxOffset += ss.GetSerializeSize(block.txMint);

    CVarInt var(block.vtx.size());
    nTxOffset += ss.GetSerializeSize(var);

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        auto& tx = block.vtx[i];
        mapBlockTxIndex.insert(make_pair(tx.GetHash(), CTxIndex(block.GetBlockHeight(), block.GetBlockNumber(), i + 1, nFile, nTxOffset)));
        nTxOffset += ss.GetSerializeSize(tx);
    }

    if (!dbBlock.AddBlockTxIndex(hashFork, block.hashPrev, block.GetHash(), mapBlockTxIndex, hashNewRoot))
    {
        StdLog("BlockBase", "Update block tx index: Add block tx index fail, block: %s", block.GetHash().GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block,
                                          const std::map<uint256, std::vector<CContractTransfer>>& mapContractTransferIn,
                                          const std::map<uint256, uint256>& mapBlockTxFeeUsedIn, const std::map<uint256, std::map<CDestination, uint256>>& mapBlockCodeDestFeeUsedIn, uint256& hashNewRoot)
{
    std::map<CDestination, std::vector<CDestTxInfo>> mapAddressTxInfo;

    if (!block.txMint.to.IsNull() && !block.txMint.to.IsNullContract())
    {
        CDestTxInfo destTxInfoMint;

        destTxInfoMint.nDirection = CDestTxInfo::DXI_DIRECTION_IN;
        destTxInfoMint.nBlockNumber = block.nNumber;
        destTxInfoMint.txid = block.txMint.GetHash();
        destTxInfoMint.nTxType = block.txMint.nType;
        destTxInfoMint.nTimeStamp = block.txMint.nTimeStamp;
        destTxInfoMint.destPeer = block.txMint.from;
        destTxInfoMint.nAmount = block.txMint.nAmount;
        destTxInfoMint.nTxFee = 0; // When nDirection is DXI_DIRECTION_IN, nTxFee is equal to 0

        mapAddressTxInfo[block.txMint.to].push_back(destTxInfoMint);
    }

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const auto& tx = block.vtx[i];
        const uint256 txid = tx.GetHash();

        {
            uint256 nTxFeeUsed;
            auto it = mapBlockTxFeeUsedIn.find(txid);
            if (it != mapBlockTxFeeUsedIn.end())
            {
                nTxFeeUsed = it->second;
            }

            if (!tx.from.IsNull() && !tx.from.IsNullContract() && tx.from == tx.to)
            {
                CDestTxInfo destTxInfoFrom;
                destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_INOUT;
                destTxInfoFrom.nBlockNumber = block.nNumber;
                destTxInfoFrom.txid = txid;
                destTxInfoFrom.nTxType = tx.nType;
                destTxInfoFrom.nTimeStamp = tx.nTimeStamp;
                destTxInfoFrom.destPeer = tx.to;
                destTxInfoFrom.nAmount = tx.nAmount;
                destTxInfoFrom.nTxFee = nTxFeeUsed;

                mapAddressTxInfo[tx.from].push_back(destTxInfoFrom);
            }
            else
            {
                if (!tx.from.IsNull() && !tx.from.IsNullContract())
                {
                    CDestTxInfo destTxInfoFrom;
                    destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_OUT;
                    destTxInfoFrom.nBlockNumber = block.nNumber;
                    destTxInfoFrom.txid = txid;
                    destTxInfoFrom.nTxType = tx.nType;
                    destTxInfoFrom.nTimeStamp = tx.nTimeStamp;
                    destTxInfoFrom.destPeer = tx.to;
                    destTxInfoFrom.nAmount = tx.nAmount;
                    destTxInfoFrom.nTxFee = nTxFeeUsed;

                    mapAddressTxInfo[tx.from].push_back(destTxInfoFrom);
                }
                if (!tx.to.IsNull() && !tx.to.IsNullContract())
                {
                    CDestTxInfo destTxInfoTo;
                    destTxInfoTo.nDirection = CDestTxInfo::DXI_DIRECTION_IN;
                    destTxInfoTo.nBlockNumber = block.nNumber;
                    destTxInfoTo.txid = txid;
                    destTxInfoTo.nTxType = tx.nType;
                    destTxInfoTo.nTimeStamp = tx.nTimeStamp;
                    destTxInfoTo.destPeer = tx.from;
                    destTxInfoTo.nAmount = tx.nAmount;
                    destTxInfoTo.nTxFee = 0; // When nDirection is DXI_DIRECTION_IN, nTxFee is equal to 0

                    mapAddressTxInfo[tx.to].push_back(destTxInfoTo);
                }
            }
        }

        auto mt = mapContractTransferIn.find(txid);
        if (mt != mapContractTransferIn.end())
        {
            for (const CContractTransfer& vd : mt->second)
            {
                if (!vd.destFrom.IsNull() && !vd.destFrom.IsNullContract() && vd.destFrom == vd.destTo)
                {
                    CDestTxInfo destTxInfoFrom;
                    destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_CINOUT;
                    destTxInfoFrom.nBlockNumber = block.nNumber;
                    destTxInfoFrom.txid = txid;
                    destTxInfoFrom.nTxType = tx.nType;
                    destTxInfoFrom.nTimeStamp = tx.nTimeStamp;
                    destTxInfoFrom.destPeer = vd.destTo;
                    destTxInfoFrom.nAmount = vd.nAmount;
                    destTxInfoFrom.nTxFee = 0;

                    mapAddressTxInfo[vd.destFrom].push_back(destTxInfoFrom);
                }
                else
                {
                    if (!vd.destFrom.IsNull() && !vd.destFrom.IsNullContract())
                    {
                        CDestTxInfo destTxInfoFrom;
                        destTxInfoFrom.nDirection = CDestTxInfo::DXI_DIRECTION_COUT;
                        destTxInfoFrom.nBlockNumber = block.nNumber;
                        destTxInfoFrom.txid = txid;
                        destTxInfoFrom.nTxType = tx.nType;
                        destTxInfoFrom.nTimeStamp = tx.nTimeStamp;
                        destTxInfoFrom.destPeer = vd.destTo;
                        destTxInfoFrom.nAmount = vd.nAmount;
                        destTxInfoFrom.nTxFee = 0;

                        mapAddressTxInfo[vd.destFrom].push_back(destTxInfoFrom);
                    }
                    if (!vd.destTo.IsNull() && !vd.destTo.IsNullContract())
                    {
                        CDestTxInfo destTxInfoTo;
                        destTxInfoTo.nDirection = CDestTxInfo::DXI_DIRECTION_CIN;
                        destTxInfoTo.nBlockNumber = block.nNumber;
                        destTxInfoTo.txid = txid;
                        destTxInfoTo.nTxType = tx.nType;
                        destTxInfoTo.nTimeStamp = tx.nTimeStamp;
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
                destTxInfo.nTxType = tx.nType;
                destTxInfo.nTimeStamp = tx.nTimeStamp;
                destTxInfo.destPeer = tx.from;
                destTxInfo.nAmount = kv.second;
                destTxInfo.nTxFee = 0;

                mapAddressTxInfo[kv.first].push_back(destTxInfo);

                StdLog("BlockBase", "Update block address tx info: code reward: dest: %s, amount: %s, block: %s",
                       kv.first.ToString().c_str(), CoinToTokenBigFloat(kv.second).c_str(), block.GetHash().GetHex().c_str());
            }
        }
    }

    if (!dbBlock.AddAddressTxInfo(hashFork, block.hashPrev, hashBlock, block.nNumber, mapAddressTxInfo, hashNewRoot))
    {
        StdLog("BlockBase", "Update block address tx info: Add block address tx info fail, block: %s", block.GetHash().GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<uint256, CAddressContext>& mapAddressContext, uint256& hashNewRoot)
{
    std::map<uint256, CAddressContext> mapAddress;

    if (hashBlock == hashGenesisBlock)
    {
        bytes btTempData;
        if (block.txMint.to.IsTemplate() && block.txMint.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
        {
            mapAddress.insert(make_pair(block.txMint.to.data, CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), block.GetBlockNumber())));
        }
        else
        {
            mapAddress.insert(make_pair(block.txMint.to.data, CAddressContext(block.GetBlockNumber())));
        }
    }
    else
    {
        CAddressContext ctxtMintAddress;
        if (!dbBlock.RetrieveAddressContext(hashFork, block.hashPrev, block.txMint.to.data, ctxtMintAddress))
        {
            if (block.txMint.to.IsTemplate())
            {
                bytes btTempData;
                if (!block.txMint.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
                {
                    StdLog("BlockBase", "Update Block Address: Mint to address error, block: %s", hashBlock.GetHex().c_str());
                    return false;
                }
                mapAddress.insert(make_pair(block.txMint.to.data, CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), block.GetBlockNumber())));
            }
            else
            {
                mapAddress.insert(make_pair(block.txMint.to.data, CAddressContext(block.GetBlockNumber())));
            }
        }
    }

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const auto& tx = block.vtx[i];

        CDestination destTo;
        if (tx.to.IsNullContract())
        {
            destTo.SetContractId(tx.from.data, tx.nTxNonce);
        }
        else
        {
            destTo = tx.to;
        }

        auto it = mapAddress.find(destTo.data);
        if (it == mapAddress.end())
        {
            CAddressContext ctxtToAddress;
            if (!dbBlock.RetrieveAddressContext(hashFork, block.hashPrev, destTo.data, ctxtToAddress))
            {
                if (destTo.IsPubKey())
                {
                    mapAddress.insert(make_pair(destTo.data, CAddressContext(block.GetBlockNumber())));
                }
                else if (destTo.IsTemplate())
                {
                    bytes btTempData;
                    if (!tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
                    {
                        StdLog("BlockBase", "Update Block Address: To address template data error, to: %s, block: %s",
                               destTo.ToString().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                    mapAddress.insert(make_pair(destTo.data, CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), block.GetBlockNumber())));
                }
                else if (tx.to.IsNullContract())
                {
                    bytes btTempData;
                    if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
                    {
                        StdLog("BlockBase", "Update Block Address: tx data error, to: %s, txid: %s, block: %s",
                               destTo.ToString().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                    CTxContractData txcd;
                    try
                    {
                        hcode::CBufStream ss(btTempData);
                        ss >> txcd;
                    }
                    catch (std::exception& e)
                    {
                        StdLog("BlockBase", "Update Block Address: Parse tx data fail, to: %s, block: %s",
                               destTo.ToString().c_str(), hashBlock.GetHex().c_str());
                        return false;
                    }
                    if (txcd.IsCreate() || txcd.IsSetup())
                    {
                        auto mt = mapAddressContext.find(destTo.data);
                        if (mt != mapAddressContext.end())
                        {
                            CContractAddressContext contractAddressContext;
                            if (mt->second.GetContractAddressContext(contractAddressContext))
                            {
                                txcd.UncompressCode();
                                if (txcd.IsCreate())
                                {
                                    mapAddress[destTo.data] = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), tx.GetHash(), txcd.GetSourceCodeHash(),
                                                                                                      contractAddressContext.hashWasmCreateCode, contractAddressContext.hashWasmRunCode),
                                                                              block.GetBlockNumber());
                                }
                                else
                                {
                                    mapAddress[destTo.data] = CAddressContext(CContractAddressContext(txcd.GetType(), txcd.GetName(), txcd.GetDescribe(), tx.GetHash(), txcd.GetSourceCodeHash(),
                                                                                                      contractAddressContext.hashWasmCreateCode, contractAddressContext.hashWasmRunCode),
                                                                              block.GetBlockNumber());
                                }
                            }
                        }
                    }
                }
                else
                {
                    StdLog("BlockBase", "Update Block Address: To address type error, to: %s, block: %s",
                           destTo.ToString().c_str(), hashBlock.GetHex().c_str());
                    return false;
                }
            }
        }
    }

    for (const auto& kv : mapAddressContext)
    {
        if (mapAddress.find(kv.first) == mapAddress.end())
        {
            mapAddress.insert(make_pair(kv.first, kv.second));
        }
    }

    if (!dbBlock.AddAddressContext(hashFork, block.hashPrev, hashBlock, mapAddress, hashNewRoot))
    {
        StdLog("BlockBase", "Update Block Address: Add address context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    mapAddressContext.clear();
    mapAddressContext = mapAddress;
    return true;
}

bool CBlockBase::UpdateBlockCode(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, const uint32 nFile, const uint32 nBlockOffset,
                                 const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCodeContextIn, uint256& hashCodeRoot)
{
    std::map<uint256, CContracSourceCodeContext> mapSourceCode;
    std::map<uint256, CWasmCreateCodeContext> mapWasmCreateCode;
    std::map<uint256, CWasmRunCodeContext> mapWasmRunCode;

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nOffset = nBlockOffset + block.GetTxSerializedOffset()
                     + ss.GetSerializeSize(block.txMint)
                     + ss.GetSerializeSize(var);
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        auto& tx = block.vtx[i];

        if (tx.to.IsNullContract() && tx.GetTxDataCount() > 0)
        {
            bytes btTempData;
            if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
            {
                StdLog("BlockBase", "Update Block Code: Tx data error1, block: %s", hashBlock.GetHex().c_str());
                return false;
            }

            CTxContractData txcd;
            try
            {
                hcode::CBufStream ss(btTempData);
                ss >> txcd;
            }
            catch (std::exception& e)
            {
                StdLog("BlockBase", "Update Block Code: Tx data error2, block: %s", hashBlock.GetHex().c_str());
                return false;
            }
            txcd.UncompressCode();

            uint256 hashSourceCode = txcd.GetSourceCodeHash();
            if (hashSourceCode != 0)
            {
                auto it = mapSourceCode.find(hashSourceCode);
                if (it == mapSourceCode.end())
                {
                    CContracSourceCodeContext ctxtCode;
                    if (!dbBlock.RetrieveSourceCodeContext(hashFork, block.hashPrev, hashSourceCode, ctxtCode))
                    {
                        ctxtCode.Clear();
                    }
                    it = mapSourceCode.insert(make_pair(hashSourceCode, ctxtCode)).first;
                }
                it->second.AddPos(txcd.GetName(), txcd.GetCodeOwner(), nFile, nOffset);
            }

            if (txcd.IsCreate() || txcd.IsUpcode())
            {
                uint256 hashWasmCreateCode = txcd.GetWasmCreateCodeHash();
                if (hashWasmCreateCode != 0)
                {
                    CWasmCreateCodeContext ctxtCode;
                    if (!dbBlock.RetrieveWasmCreateCodeContext(hashFork, block.hashPrev, hashWasmCreateCode, ctxtCode))
                    {
                        mapWasmCreateCode[hashWasmCreateCode] = CWasmCreateCodeContext(txcd.GetName(), txcd.GetCodeOwner(), nFile, nOffset);
                    }
                }
            }
        }

        nOffset += ss.GetSerializeSize(tx);
    }

    for (const auto& kv : mapWasmRunCodeContextIn)
    {
        CWasmRunCodeContext ctxtCode;
        if (!dbBlock.RetrieveWasmRunCodeContext(hashFork, block.hashPrev, kv.first, ctxtCode)
            || ctxtCode.hashWasmCreateCode != kv.second.hashWasmCreateCode
            || ctxtCode.btWasmRunCode != kv.second.btWasmRunCode)
        {
            mapWasmRunCode[kv.first] = kv.second;
        }
    }

    if (!dbBlock.AddCodeContext(hashFork, block.hashPrev, hashBlock, mapSourceCode, mapWasmCreateCode, mapWasmRunCode, hashCodeRoot))
    {
        StdLog("BlockBase", "Update Block Code: Add code context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value)
{
    return dbBlock.RetrieveWasmState(hashFork, hashWasmRoot, key, value);
}

bool CBlockBase::AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState)
{
    return dbBlock.AddBlockWasmState(hashFork, hashPrevRoot, hashWasmRoot, mapWasmState);
}

bool CBlockBase::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress)
{
    return dbBlock.RetrieveAddressContext(hashFork, hashBlock, dest, ctxtAddress);
}

bool CBlockBase::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    return dbBlock.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockBase::CallWasmCode(const uint256& hashFork, const uint32 nHeight, const CDestination& destMint, const uint256& nBlockGasLimit,
                              const CDestination& from, const CDestination& to, const uint256& nGasPrice, const uint256& nGasLimit, const uint256& nAmount,
                              const bytes& data, const uint32 nTimeStamp, const uint256& hashPrevBlock, const uint256& hashPrevStateRoot, int& nStatus, bytes& btResult)
{
    if (!to.IsContract() || to.IsNullContract())
    {
        StdLog("BlockBase", "Call wasm code: to not is wasm address, to: %s", to.ToString().c_str());
        return false;
    }

    CBlockState blockState(*this, hashFork, hashPrevBlock, hashPrevStateRoot);

    uint256 hashWasmCreateCode;
    CDestination destCodeOwner;
    bytes btWasmRunCode;
    if (!blockState.GetWasmRunCode(to, hashWasmCreateCode, destCodeOwner, btWasmRunCode))
    {
        StdLog("BlockBase", "Call wasm code: Get wasm run code fail, to: %s", to.ToString().c_str());
        return false;
    }

    CWasmHostDB dbHost(blockState, to);
    CWasmRun wasmRun(dbHost, hashFork);
    CTxContractData txcd;
    if (!wasmRun.RunWasm(from.data, to.data, to.data, destCodeOwner, nGasLimit.Get64(),
                         nGasPrice, nAmount, destMint.data, nTimeStamp,
                         nHeight, nBlockGasLimit.Get64(), btWasmRunCode, data, txcd))
    {
        StdLog("BlockBase", "Call wasm code: Run wasm fail, to: %s", to.ToString().c_str());
    }
    nStatus = wasmRun.nStatusCode;
    btResult = wasmRun.vResult;

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

void CBlockBase::UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
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
        pIndexNew = new CBlockIndex(block, nFile, nOffset, nCrc);
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
                        if (!profile.Load(block.vchProof))
                        {
                            StdError("BlockBase", "Add new index: Load proof fail, block: %s", hash.GetHex().c_str());
                            delete pIndexNew;
                            pIndexNew = nullptr;
                            return nullptr;
                        }
                        if (!profile.IsIsolated())
                        {
                            pIndexNew->nMoneySupply += pIndexPrev->nMoneySupply;
                            pIndexNew->nMoneyDestroy += pIndexPrev->nMoneyDestroy;
                            pIndexNew->nTxCount += pIndexPrev->nTxCount;
                        }
                    }
                    else
                    {
                        pIndexNew->pOrigin = pIndexPrev->pOrigin;
                        pIndexNew->nRandBeacon ^= pIndexNew->pOrigin->nRandBeacon;
                        pIndexNew->nMoneySupply += pIndexPrev->nMoneySupply;
                        pIndexNew->nMoneyDestroy += pIndexPrev->nMoneyDestroy;
                        pIndexNew->nTxCount += pIndexPrev->nTxCount;
                    }
                    pIndexNew->nChainTrust += pIndexPrev->nChainTrust;
                }
            }

            UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, block.nTimeStamp, block.txMint.to, pIndexNew->GetRefBlock());
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

bool CBlockBase::UpdateDelegate(const uint256& hashBlock, const CBlockEx& block, const uint32 nFile, const uint32 nOffset,
                                const uint256& nMinEnrollAmount, uint256& hashDelegateRoot)
{
    StdTrace("BlockBase", "Update delegate: height: %d, block: %s", block.GetBlockHeight(), hashBlock.GetHex().c_str());

    std::map<CDestination, uint256> mapDelegateVote;
    std::map<int, std::map<CDestination, CDiskPos>> mapEnrollTx;

    auto addVote = [&](const CDestination& destVote, const uint256& nVote, const bool fAdd) -> bool
    {
        auto it = mapDelegateVote.find(destVote);
        if (it == mapDelegateVote.end())
        {
            uint256 nGetVote;
            if (!dbBlock.RetrieveDestDelegateVote(block.hashPrev, destVote, nGetVote))
            {
                nGetVote = 0;
            }
            it = mapDelegateVote.insert(make_pair(destVote, nGetVote)).first;
        }
        if (fAdd)
        {
            it->second += nVote;
        }
        else
        {
            it->second -= nVote;
        }
        return true;
    };

    std::map<CDestination, CDestination> mapDelegateAddress;
    if (!GetBlockDelegateAddress(hashGenesisBlock, hashBlock, block, mapDelegateAddress))
    {
        StdLog("BlockBase", "Update delegate: Get block delegate address fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (block.txMint.to.IsTemplate() && block.txMint.to.GetTemplateId().GetType() == TEMPLATE_DELEGATE)
    {
        if (!addVote(block.txMint.to, block.txMint.nAmount, true))
        {
            StdLog("BlockBase", "Update delegate: Add vote fail, mint dest: %s, block: %s, mint txid: %s",
                   block.txMint.to.ToString().c_str(), hashBlock.GetHex().c_str(), block.txMint.GetHash().GetHex().c_str());
            return false;
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
        CDestination destFromDelegateTemplate;
        CDestination destToDelegateTemplate;
        auto fit = mapDelegateAddress.find(tx.from);
        if (fit != mapDelegateAddress.end())
        {
            destFromDelegateTemplate = fit->second;
        }
        auto tit = mapDelegateAddress.find(tx.to);
        if (tit != mapDelegateAddress.end())
        {
            destToDelegateTemplate = tit->second;
        }
        if (!destToDelegateTemplate.IsNull())
        {
            if (!addVote(destToDelegateTemplate, tx.nAmount, true))
            {
                StdLog("BlockBase", "Update delegate: Add vote fail, to dest: %s, to delegate: %s, block: %s, txid: %s",
                       tx.to.ToString().c_str(), destToDelegateTemplate.ToString().c_str(),
                       hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
        }
        if (!destFromDelegateTemplate.IsNull())
        {
            if (!addVote(destFromDelegateTemplate, tx.nAmount + tx.GetTxFee(), false))
            {
                StdLog("BlockBase", "Update delegate: Add vote fail, from dest: %s, from delegate: %s, block: %s, txid: %s",
                       tx.from.ToString().c_str(), destFromDelegateTemplate.ToString().c_str(),
                       hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
        }

        if (tx.nType == CTransaction::TX_CERT)
        {
            if (destFromDelegateTemplate.IsNull())
            {
                StdLog("BlockBase", "Update delegate: TX_CERT destFromDelegate is null, destFromDelegate: %s, from: %s, to: %s, block: %s, txid: %s",
                       destFromDelegateTemplate.ToString().c_str(),
                       tx.from.ToString().c_str(),
                       tx.to.ToString().c_str(),
                       hashBlock.GetHex().c_str(),
                       tx.GetHash().GetHex().c_str());
                return false;
            }
            uint256 nDelegateVote = mapDelegateVote[destFromDelegateTemplate];
            if (nDelegateVote < nMinEnrollAmount)
            {
                StdLog("BlockBase", "Update delegate: TX_CERT not enough votes, destFromDelegate: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destFromDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }

            bytes btTempData;
            if (!tx.GetTxData(CTransaction::DF_CERTTXDATA, btTempData))
            {
                StdLog("BlockBase", "Update delegate: TX_CERT tx data error1, destFromDelegate: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destFromDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            int nCertAnchorHeight = 0;
            bytes btCertData;
            try
            {
                hcode::CBufStream ss(btTempData);
                ss >> nCertAnchorHeight >> btCertData;
            }
            catch (...)
            {
                StdLog("BlockBase", "Update delegate: TX_CERT tx data error2, destFromDelegate: %s, delegate vote: %s, weight ratio: %s, txid: %s",
                       destFromDelegateTemplate.ToString().c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(),
                       CoinToTokenBigFloat(nMinEnrollAmount).c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            mapEnrollTx[nCertAnchorHeight].insert(make_pair(destFromDelegateTemplate, CDiskPos(nFile, nSetOffset)));
            StdTrace("BlockBase", "Update delegate: Enroll cert tx, anchor height: %d, nAmount: %s, vote: %s, destFromDelegate: %s, txid: %s",
                     nCertAnchorHeight, CoinToTokenBigFloat(tx.nAmount).c_str(), CoinToTokenBigFloat(nDelegateVote).c_str(), destFromDelegateTemplate.ToString().c_str(), tx.GetHash().GetHex().c_str());
        }
        nSetOffset += ss.GetSerializeSize(tx);
    }

    for (auto it = mapDelegateVote.begin(); it != mapDelegateVote.end(); ++it)
    {
        StdTrace("BlockBase", "Update delegate: destDelegate: %s, votes: %s",
                 it->first.ToString().c_str(), CoinToTokenBigFloat(it->second).c_str());
    }

    if (!dbBlock.UpdateDelegateContext(block.hashPrev, hashBlock, mapDelegateVote, mapEnrollTx, hashDelegateRoot))
    {
        StdError("BlockBase", "Update delegate: Update delegate context failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateVote(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const std::map<uint256, CAddressContext>& mapBlockAddressContext, uint256& hashVoteRoot)
{
    std::map<CDestination, CVoteContext> mapBlockVote;
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        if (tx.nType == CTransaction::TX_CERT)
        {
            continue;
        }
        uint16 nToTemplateType = 0;
        uint16 nFromTemplateType = 0;
        if (tx.to.IsTemplate())
        {
            nToTemplateType = tx.to.GetTemplateId().GetType();
        }
        if (tx.from.IsTemplate())
        {
            nFromTemplateType = tx.from.GetTemplateId().GetType();
        }
        if (nToTemplateType == TEMPLATE_VOTE || nToTemplateType == TEMPLATE_DELEGATE)
        {
            auto it = mapBlockVote.find(tx.to);
            if (it == mapBlockVote.end())
            {
                CVoteContext ctxtVote;
                if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, tx.to, ctxtVote))
                {
                    if (nToTemplateType == TEMPLATE_VOTE)
                    {
                        bytes btTemplateData;
                        if (!GetTxToAddressTemplateData(hashFork, block.hashPrev, tx, mapBlockAddressContext, btTemplateData))
                        {
                            StdError("BlockBase", "Update vote: Get address template data failed, to: %s, block: %s",
                                    tx.to.ToString().c_str(), hashBlock.ToString().c_str());
                            return false;
                        }
                        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, btTemplateData);
                        if (ptr == nullptr)
                        {
                            StdError("BlockBase", "Update vote: Create template failed, block: %s", hashBlock.ToString().c_str());
                            return false;
                        }
                        auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
                        ctxtVote.destDelegate = objVote->destDelegate;
                        ctxtVote.destOwner = objVote->destOwner;
                        ctxtVote.nRewardMode = objVote->nRewardMode;
                    }
                    else
                    {
                        ctxtVote.destDelegate = tx.to;
                        ctxtVote.nRewardMode = CVoteContext::REWARD_MODE_VOTE;
                    }
                }
                it = mapBlockVote.insert(make_pair(tx.to, ctxtVote)).first;
            }
            if (!(tx.IsRewardTx() || tx.nType == CTransaction::TX_CERT))
            {
                it->second.nLastVoteHeight = block.GetBlockHeight();
            }
        }
        if (nFromTemplateType == TEMPLATE_VOTE || nFromTemplateType == TEMPLATE_DELEGATE)
        {
            auto it = mapBlockVote.find(tx.from);
            if (it == mapBlockVote.end())
            {
                CVoteContext ctxtVote;
                if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, tx.from, ctxtVote))
                {
                    StdError("BlockBase", "Update vote: Redeem find failed, dest: %s, block: %s",
                             tx.from.ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                it = mapBlockVote.insert(make_pair(tx.from, ctxtVote)).first;
            }
        }
    }

    if (block.txMint.to.IsTemplate() && block.txMint.to.GetTemplateId().GetType() == TEMPLATE_DELEGATE)
    {
        auto it = mapBlockVote.find(block.txMint.to);
        if (it == mapBlockVote.end())
        {
            CVoteContext ctxtVote;
            if (!dbBlock.RetrieveDestVoteContext(block.hashPrev, block.txMint.to, ctxtVote))
            {
                ctxtVote.destDelegate = block.txMint.to;
                ctxtVote.nRewardMode = CVoteContext::REWARD_MODE_VOTE;
            }
            it = mapBlockVote.insert(make_pair(block.txMint.to, ctxtVote)).first;
        }
    }

    for (auto& kv : mapBlockVote)
    {
        CDestState state;
        if (!dbBlock.RetrieveDestState(hashGenesisBlock, block.hashStateRoot, kv.first, state))
        {
            StdError("BlockBase", "Update vote: Retrieve dest state failed, dest: %s, block: %s",
                     kv.first.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
        if (state.nBalance < DELEGATE_PROOF_OF_STAKE_MIN_VOTE_AMOUNT)
        {
            kv.second.nVoteAmount = 0;
        }
        else
        {
            kv.second.nVoteAmount = state.nBalance;
        }
    }

    if (!dbBlock.AddBlockVote(block.hashPrev, hashBlock, mapBlockVote, hashVoteRoot))
    {
        StdError("BlockBase", "Update vote: Add block vote failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateVoteRedeem(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const std::map<uint256, CAddressContext>& mapBlockAddressContext, uint256& hashVoteRedeemRoot)
{
    std::map<CDestination, CVoteRedeemContext> mapBlockVoteRedeem;
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        if (tx.to.IsTemplate() && tx.to.GetTemplateId().GetType() == TEMPLATE_REDEEM)
        {
            auto it = mapBlockVoteRedeem.find(tx.to);
            if (it == mapBlockVoteRedeem.end())
            {
                CVoteRedeemContext ctxtVoteRedeem;
                if (!dbBlock.RetrieveDestVoteRedeemContext(block.hashPrev, tx.to, ctxtVoteRedeem))
                {
                    bytes btTemplateData;
                    if (!GetTxToAddressTemplateData(hashFork, block.hashPrev, tx, mapBlockAddressContext, btTemplateData))
                    {
                        StdError("BlockBase", "Update vote redeem: Get address template data failed, to: %s, block: %s",
                                tx.to.ToString().c_str(), hashBlock.ToString().c_str());
                        return false;
                    }
                    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_REDEEM, btTemplateData);
                    if (ptr == nullptr)
                    {
                        StdError("BlockBase", "Update vote redeem: Create template failed, block: %s", hashBlock.ToString().c_str());
                        return false;
                    }
                    auto objRedeem = boost::dynamic_pointer_cast<CTemplateRedeem>(ptr);
                    ctxtVoteRedeem.destOwner = objRedeem->destOwner;
                }
                it = mapBlockVoteRedeem.insert(make_pair(tx.to, ctxtVoteRedeem)).first;
            }
            it->second.nLastRedeemHeight = block.GetBlockHeight();
        }
    }

    for (auto& kv : mapBlockVoteRedeem)
    {
        CDestState state;
        if (!dbBlock.RetrieveDestState(hashGenesisBlock, block.hashStateRoot, kv.first, state))
        {
            StdError("BlockBase", "Update vote redeem: Retrieve dest state failed, dest: %s, block: %s",
                     kv.first.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
        kv.second.nRedeemAmount = state.nBalance;
    }

    if (!dbBlock.AddBlockVoteRedeem(block.hashPrev, hashBlock, mapBlockVoteRedeem, hashVoteRedeemRoot))
    {
        StdError("BlockBase", "Update vote redeem: Add block vote redeem failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateBlockInvite(const uint256& hashFork, const uint256& hashBlock, const CBlockEx& block, const std::map<uint256, CAddressContext>& mapBlockAddressContext, uint256& hashNewRoot)
{
    std::map<CDestination, CInviteContext> mapInviteContext;     // key: sub address, value: parent address, amount
    std::map<CDestination, std::set<CDestination>> mapVoteOwner; // key: owner address, value: vote address list

    for (const auto& tx : block.vtx)
    {
        if (tx.nType == CTransaction::TX_DEFI_RELATION)
        {
            auto it = mapInviteContext.find(tx.to);
            if (it == mapInviteContext.end())
            {
                CInviteContext ctxInvite;
                if (!dbBlock.RetrieveInviteParent(hashFork, block.hashPrev, tx.to, ctxInvite))
                {
                    ctxInvite.destParent.SetNull();
                    ctxInvite.destReward.SetNull();
                    ctxInvite.nVoteAmount = 0;
                }
                it = mapInviteContext.insert(std::make_pair(tx.to, ctxInvite)).first;
            }
            it->second.destParent = tx.from;
        }
        else
        {
            if (tx.to.IsTemplate() && tx.to.GetTemplateId().GetType() == TEMPLATE_VOTE)
            {
                bytes btTemplateData;
                if (!GetTxToAddressTemplateData(hashFork, block.hashPrev, tx, mapBlockAddressContext, btTemplateData))
                {
                    StdError("BlockBase", "Update block invite: Get address template data failed, to: %s, block: %s",
                             tx.to.ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, btTemplateData);
                if (ptr == nullptr)
                {
                    StdError("BlockBase", "Update block invite: Create template failed, to: %s, block: %s",
                             tx.to.ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);

                auto it = mapInviteContext.find(objVote->destOwner);
                if (it == mapInviteContext.end())
                {
                    CInviteContext ctxInvite;
                    if (!dbBlock.RetrieveInviteParent(hashFork, block.hashPrev, objVote->destOwner, ctxInvite))
                    {
                        ctxInvite.destParent.SetNull();
                        ctxInvite.destReward.SetNull();
                        ctxInvite.nVoteAmount = 0;
                    }
                    it = mapInviteContext.insert(std::make_pair(objVote->destOwner, ctxInvite)).first;
                }
                if (it->second.destReward.IsNull())
                {
                    it->second.destReward = tx.to;
                }

                auto& setVote = mapVoteOwner[objVote->destOwner];
                if (setVote.find(tx.to) == setVote.end())
                {
                    setVote.insert(tx.to);
                }
            }
            if (tx.from.IsTemplate() && tx.from.GetTemplateId().GetType() == TEMPLATE_VOTE)
            {
                bytes btTemplateData;
                if (!GetTxFromAddressTemplateData(hashFork, block.hashPrev, tx, mapBlockAddressContext, btTemplateData))
                {
                    StdError("BlockBase", "Update block invite: Get address template data failed, from: %s, block: %s",
                             tx.from.ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, btTemplateData);
                if (ptr == nullptr)
                {
                    StdError("BlockBase", "Update block invite: Create template failed, from: %s, block: %s",
                             tx.from.ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);

                auto& setVote = mapVoteOwner[objVote->destOwner];
                if (setVote.find(tx.from) == setVote.end())
                {
                    setVote.insert(tx.from);
                }
            }
        }
    }

    uint256 hashPrevStateRoot;
    if (block.hashPrev != 0)
    {
        CBlockIndex* pPrevIndex = GetIndex(block.hashPrev);
        if (pPrevIndex == nullptr)
        {
            StdLog("BlockBase", "Update block invite: Get prev index fail, prev block: %s, block: %s",
                   block.hashPrev.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
        hashPrevStateRoot = pPrevIndex->GetStateRoot();
    }

    for (auto& kv : mapVoteOwner)
    {
        auto it = mapInviteContext.find(kv.first);
        if (it == mapInviteContext.end())
        {
            CInviteContext ctxInvite;
            if (!dbBlock.RetrieveInviteParent(hashFork, block.hashPrev, kv.first, ctxInvite))
            {
                ctxInvite.destParent.SetNull();
                ctxInvite.destReward.SetNull();
                ctxInvite.nVoteAmount = 0;
            }
            it = mapInviteContext.insert(std::make_pair(kv.first, ctxInvite)).first;
        }
        for (auto& destVote : kv.second)
        {
            CDestState statePrev;
            CDestState stateBlock;
            if (hashPrevStateRoot != 0 && !dbBlock.RetrieveDestState(hashFork, hashPrevStateRoot, destVote, statePrev))
            {
                statePrev.nBalance = 0;
            }
            if (!dbBlock.RetrieveDestState(hashFork, block.hashStateRoot, destVote, stateBlock))
            {
                StdLog("BlockBase", "Update block invite: Retrieve block state fail, destVote: %s, block: %s",
                       destVote.ToString().c_str(), hashBlock.GetHex().c_str());
                return false;
            }
            if (stateBlock.nBalance > statePrev.nBalance)
            {
                it->second.nVoteAmount += (stateBlock.nBalance - statePrev.nBalance);
            }
            else if (stateBlock.nBalance < statePrev.nBalance)
            {
                it->second.nVoteAmount -= (statePrev.nBalance - stateBlock.nBalance);
            }
        }
    }

    if (!dbBlock.AddInviteRelation(hashFork, block.hashPrev, hashBlock, mapInviteContext, hashNewRoot))
    {
        StdLog("BlockBase", "Update block invite: Add invite context fail, block: %s", hashBlock.GetHex().c_str());
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
        if (!block.vchProof.empty())
        {
            CProofOfPiggyback proof;
            if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
            {
                hashRefBlock = proof.hashRefBlock;
            }
        }
        if (hashRefBlock == 0)
        {
            return false;
        }
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
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

bool CBlockBase::GetBlockDelegateAddress(const uint256& hashFork, const uint256& hashBlock, const CBlock& block, std::map<CDestination, CDestination>& mapDelegateAddress)
{
    auto getAddress = [&](const CDestination& dest, const bytes& btTemplateData) -> bool
    {
        if (mapDelegateAddress.count(dest) == 0)
        {
            CTemplateId tid;
            if (dest.GetTemplateId(tid))
            {
                if (tid.GetType() == TEMPLATE_DELEGATE)
                {
                    mapDelegateAddress.insert(make_pair(dest, dest));
                }
                else if (tid.GetType() == TEMPLATE_VOTE)
                {
                    CTemplatePtr ptr = nullptr;
                    CAddressContext ctxtAddress;
                    if (!dbBlock.RetrieveAddressContext(hashFork, block.hashPrev, dest.data, ctxtAddress))
                    {
                        if (btTemplateData.empty())
                        {
                            StdLog("BlockBase", "Get Block Delegate Address: Retrieve address context failed, dest: %s", dest.ToString().c_str());
                            return false;
                        }
                        ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, btTemplateData);
                    }
                    else
                    {
                        CTemplateAddressContext ctxtTemplate;
                        if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
                        {
                            StdError("BlockBase", "Get Block Delegate Address: Get address context failed, dest: %s", dest.ToString().c_str());
                            return false;
                        }
                        ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, ctxtTemplate.btData);
                    }
                    if (ptr == nullptr)
                    {
                        StdLog("BlockBase", "Get Block Delegate Address: Create template failed, dest: %s", dest.ToString().c_str());
                        return false;
                    }
                    auto vote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
                    mapDelegateAddress.insert(make_pair(dest, vote->destDelegate));
                }
            }
        }
        return true;
    };

    if (!getAddress(block.txMint.to, bytes()))
    {
        StdLog("BlockBase", "Get Block Delegate Address: Get mint to failed, mint to: %s", block.txMint.to.ToString().c_str());
        return false;
    }
    for (const CTransaction& tx : block.vtx)
    {
        if (!getAddress(tx.from, bytes()))
        {
            StdLog("BlockBase", "Get Block Delegate Address: Get from failed, txid: %s, from: %s",
                   tx.GetHash().GetHex().c_str(), tx.from.ToString().c_str());
            return false;
        }
        bytes btTempData;
        tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData);
        if (!getAddress(tx.to, btTempData))
        {
            StdLog("BlockBase", "Get Block Delegate Address: Get to failed, txid: %s, to: %s",
                   tx.GetHash().GetHex().c_str(), tx.to.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::GetTxContractData(const uint32 nTxFile, const uint32 nTxOffset, CTxContractData& txcdCode, uint256& txidCreate)
{
    CTransaction tx;
    if (!tsBlock.Read(tx, nTxFile, nTxOffset, false, true))
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    if (!tx.to.IsNullContract())
    {
        StdLog("BlockBase", "Get tx contract data: Read fail, to not is null contract address, to: %s, nTxFile: %d, nTxOffset: %d",
               tx.to.ToString().c_str(), nTxFile, nTxOffset);
        return false;
    }
    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
    {
        StdLog("BlockBase", "Get tx contract data: tx data error, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    try
    {
        hcode::CBufStream ss(btTempData);
        ss >> txcdCode;
    }
    catch (std::exception& e)
    {
        StdLog("BlockBase", "Get tx contract data: Parse tx data fail, nTxFile: %d, nTxOffset: %d", nTxFile, nTxOffset);
        return false;
    }
    txcdCode.UncompressAll();
    txidCreate = tx.GetHash();
    return true;
}

bool CBlockBase::GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode)
{
    CContracSourceCodeContext ctxtCode;
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

bool CBlockBase::GetBlockWasmCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CTxContractData& txcdCode)
{
    CWasmCreateCodeContext ctxtCode;
    if (!dbBlock.RetrieveWasmCreateCodeContext(hashFork, hashBlock, hashWasmCreateCode, ctxtCode))
    {
        StdLog("BlockBase", "Get block wasm create code data: Retrieve wasm create code context fail, code: %s, block: %s",
               hashWasmCreateCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    uint256 txidCreate;
    if (!GetTxContractData(ctxtCode.nFile, ctxtCode.nOffset, txcdCode, txidCreate))
    {
        StdLog("BlockBase", "Get block wasm create code data: Get tx contract data fail, code: %s, block: %s",
               hashWasmCreateCode.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode)
{
    return dbBlock.RetrieveWasmCreateCodeContext(hashFork, hashBlock, hashWasmCreateCode, ctxtCode);
}

bool CBlockBase::RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode)
{
    return dbBlock.RetrieveWasmRunCodeContext(hashFork, hashBlock, hashWasmRunCode, ctxtCode);
}

bool CBlockBase::GetWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CContractCodeContext& ctxtContractCode)
{
    CWasmCreateCodeContext ctxtCode;
    if (!dbBlock.RetrieveWasmCreateCodeContext(hashFork, hashBlock, hashWasmCreateCode, ctxtCode))
    {
        return false;
    }
    CTxContractData txcdCode;
    uint256 txidCreate;
    if (!GetTxContractData(ctxtCode.nFile, ctxtCode.nOffset, txcdCode, txidCreate))
    {
        return false;
    }
    ctxtContractCode.hashCode = txcdCode.GetWasmCreateCodeHash();
    ctxtContractCode.hashSource = txcdCode.GetSourceCodeHash();
    ctxtContractCode.strType = txcdCode.GetType();
    ctxtContractCode.strName = txcdCode.GetName();
    ctxtContractCode.destOwner = txcdCode.GetCodeOwner();
    ctxtContractCode.strDescribe = txcdCode.GetDescribe();
    ctxtContractCode.hashCreateTxid = txidCreate;
    return true;
}

bool CBlockBase::ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode)
{
    std::map<uint256, CWasmCreateCodeContext> mapWasmCreateCode;
    if (!dbBlock.ListWasmCreateCodeContext(hashFork, hashBlock, mapWasmCreateCode))
    {
        return false;
    }
    for (const auto& kv : mapWasmCreateCode)
    {
        CTxContractData txcdCode;
        uint256 txidCreate;
        if (!GetTxContractData(kv.second.nFile, kv.second.nOffset, txcdCode, txidCreate))
        {
            return false;
        }
        if (txid != 0 && txid != txidCreate)
        {
            continue;
        }

        CContractCodeContext ctxt;

        ctxt.hashCode = txcdCode.GetWasmCreateCodeHash();
        ctxt.hashSource = txcdCode.GetSourceCodeHash();
        ctxt.strType = txcdCode.GetType();
        ctxt.strName = txcdCode.GetName();
        ctxt.destOwner = txcdCode.GetCodeOwner();
        ctxt.strDescribe = txcdCode.GetDescribe();
        ctxt.hashCreateTxid = txidCreate;

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
    if (!RetrieveWasmState(hashFork, stateDest.hashStorageRoot, destContract.data, btDestCodeData))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve wasm state fail, storage root: %s, block: %s, destContract: %s",
               stateDest.hashStorageRoot.GetHex().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CContractDestCodeContext ctxtDestCode;
    try
    {
        CBufStream ss(btDestCodeData);
        ss >> ctxtDestCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockBase", "Verify contract address: Parse wasm code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CWasmRunCodeContext ctxtRunCode;
    if (!RetrieveWasmRunCodeContext(hashFork, hashBlock, ctxtDestCode.hashWasmRunCode, ctxtRunCode))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve wasm run code fail, block: %s, destContract: %s",
               hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }

    CWasmCreateCodeContext ctxtCreateCode;
    if (!RetrieveWasmCreateCodeContext(hashFork, hashBlock, ctxtDestCode.hashWasmCreateCode, ctxtCreateCode))
    {
        StdLog("CBlockBase", "Verify contract address: Retrieve wasm create code fail, hashWasmCreateCode: %s, block: %s, destContract: %s",
               ctxtDestCode.hashWasmCreateCode.ToString().c_str(), hashBlock.GetHex().c_str(), destContract.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::VerifyCreateContractTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx)
{
    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_CONTRACTCODE, btTempData))
    {
        StdLog("CBlockBase", "Verify create contract tx: Get tx data fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    CTxContractData txcdCode;
    try
    {
        hcode::CBufStream ss(btTempData);
        ss >> txcdCode;
    }
    catch (std::exception& e)
    {
        StdLog("CBlockBase", "Verify create contract tx: Parse tx data fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    uint256 hashWasmCreateCode;
    if (txcdCode.IsCreate() || txcdCode.IsUpcode())
    {
        txcdCode.UncompressCode();
        hashWasmCreateCode = txcdCode.GetWasmCreateCodeHash();
    }
    else if (txcdCode.IsSetup())
    {
        CTxContractData txcd;
        hashWasmCreateCode = txcdCode.GetWasmCreateCodeHash();
        if (!GetBlockWasmCreateCodeData(hashFork, hashBlock, hashWasmCreateCode, txcd))
        {
            StdLog("CBlockBase", "Verify create contract tx: Get create code data fail, code hash: %s, tx: %s, block: %s",
                   hashWasmCreateCode.GetHex().c_str(), tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }
    else
    {
        StdLog("CBlockBase", "Verify create contract tx: Code flag error, flag: %d, tx: %s, block: %s",
               txcdCode.nMuxType, tx.GetHash().GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
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

bool CBlockBase::ListDestState(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestState>& mapBlockState)
{
    CBlockIndex* pIndex = nullptr;
    if (!RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    return dbBlock.ListDestState(hashFork, pIndex->hashStateRoot, mapBlockState);
}

bool CBlockBase::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    return dbBlock.ListAddressTxInfo(hashFork, hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
}

bool CBlockBase::RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CInviteContext& ctxInvite)
{
    return dbBlock.RetrieveInviteParent(hashFork, hashBlock, destSub, ctxInvite);
}

bool CBlockBase::ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CInviteContext>& mapInviteContext)
{
    return dbBlock.ListInviteRelation(hashFork, hashBlock, mapInviteContext);
}

bool CBlockBase::GetTxIndex(const uint256& txid, CTxIndex& txIndex, uint256& hashAtFork)
{
    const uint16 nForkSn = CTxId(txid).GetForkSn();
    uint256 hashFork;
    if (!dbBlock.GetForkIdByForkSn(nForkSn, hashFork))
    {
        return false;
    }
    CBlockIndex* pLastIndex = nullptr;
    if (!RetrieveFork(hashFork, &pLastIndex))
    {
        return false;
    }
    if (!dbBlock.RetrieveTxIndex(hashFork, pLastIndex->GetBlockHash(), txid, txIndex))
    {
        return false;
    }
    hashAtFork = hashFork;
    return true;

    /*std::map<uint256, CBlockIndex*> mapForkIndex;
    ListForkIndex(mapForkIndex);

    for (const auto& kv : mapForkIndex)
    {
        if (kv.second)
        {
            if (dbBlock.RetrieveTxIndex(kv.first, kv.second->GetBlockHash(), txid, txIndex))
            {
                hashAtFork = kv.first;
                return true;
            }
        }
    }
    return false;*/
}

bool CBlockBase::GetTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex)
{
    if (hashFork == 0)
    {
        uint256 hashForkAt;
        return GetTxIndex(txid, txIndex, hashForkAt);
    }
    else
    {
        if (hashBlock == 0)
        {
            CBlockIndex* pLastIndex = nullptr;
            if (!RetrieveFork(hashFork, &pLastIndex))
            {
                return false;
            }
            return dbBlock.RetrieveTxIndex(hashFork, pLastIndex->GetBlockHash(), txid, txIndex);
        }
        else
        {
            return dbBlock.RetrieveTxIndex(hashFork, hashBlock, txid, txIndex);
        }
    }
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
    StdLog("BlockBase", "Start verify db...................................");
    if (!VerifyDB())
    {
        StdError("BlockBase", "Load DB: Verify DB fail.");
        return false;
    }
    StdLog("BlockBase", "Verify db success!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
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
        if (VerifyBlockDB(verifyBlock, outline, blockRoot, fVerify))
        {
            if (!LoadBlockIndex(outline, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Load block index fail, pos: %ld, block: %s.", i, verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (!fAllVerify)
            {
                fAllVerify = true;
            }
            StdLog("BlockBase", "Verify DB: Verify block db fail, pos: %ld, block: %s.", i, verifyBlock.hashBlock.GetHex().c_str());
            if (!RepairBlockDB(verifyBlock, blockRoot, &pIndexNew))
            {
                StdError("BlockBase", "Verify DB: Repair block db fail, pos: %ld, block: %s.", i, verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
        }

        if (pIndexNew->IsOrigin())
        {
            mapForkLast[pIndexNew->GetBlockHash()] = pIndexNew->GetBlockHash();
        }
        else
        {
            auto it = mapForkLast.find(pIndexNew->GetOriginHash());
            if (it == mapForkLast.end())
            {
                StdError("BlockBase", "Verify DB: Find fork last fail, fork: %s, block: %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            CBlockIndex* pIndexFork = GetIndex(it->second);
            if (pIndexFork == nullptr)
            {
                StdError("BlockBase", "Verify DB: Get last index fail, fork: %s, block: %s.",
                         pIndexNew->GetOriginHash().GetHex().c_str(), verifyBlock.hashBlock.GetHex().c_str());
                return false;
            }
            if (!(pIndexFork->nChainTrust > pIndexNew->nChainTrust
                  || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
            {
                UpdateBlockNext(pIndexNew);
                it->second = pIndexNew->GetBlockHash();
            }
        }

        if (i % 100000 == 0)
        {
            StdLog("BlockBase", "Verify DB: Verify block count: %ld, block: [%d] %s.",
                   i, CBlock::GetBlockHeightByHash(verifyBlock.hashBlock), verifyBlock.hashBlock.GetHex().c_str());
        }
    }

    for (const auto& kv : mapForkLast)
    {
        uint256 hashLastBlock;
        if (!dbBlock.RetrieveForkLast(kv.first, hashLastBlock) || hashLastBlock != kv.second)
        {
            StdError("BlockBase", "Verify DB: Fork last error, last: %s, fork: %s",
                     kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
            if (!dbBlock.UpdateForkLast(kv.first, kv.second))
            {
                StdError("BlockBase", "Verify DB: Repair fork last fail, last: %s, fork: %s",
                         kv.second.GetHex().c_str(), kv.first.GetHex().c_str());
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
        StdError("BlockBase", "Verify block DB: Retrieve block index fail, block: %s.", verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    if (outline.GetCrc() != verifyBlock.nIndexCrc)
    {
        StdError("BlockBase", "Verify block DB: Block index crc error, db index crc: 0x%8.8x, verify index crc: 0x%8.8x, block: %s.",
                 outline.GetCrc(), verifyBlock.nIndexCrc, verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerify || outline.IsOrigin())
    {
        if (!dbBlock.VerifyBlockRoot(outline.IsPrimary(), outline.hashOrigin, outline.hashPrev,
                                     verifyBlock.hashBlock, outline.hashStateRoot, blockRoot, false))
        {
            StdError("BlockBase", "Verify block DB: Verify block root fail, block: %s.", verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
        if (blockRoot.GetRootCrc() != verifyBlock.nRootCrc)
        {
            StdError("BlockBase", "Verify block DB: Root crc error, db root crc: 0x%8.8x, verify root crc: 0x%8.8x, block: %s.",
                     blockRoot.GetRootCrc(), verifyBlock.nRootCrc, verifyBlock.hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::RepairBlockDB(const CBlockVerify& verifyBlock, CBlockRoot& blockRoot, CBlockIndex** ppIndexNew)
{
    CBlockEx block;
    if (!tsBlock.Read(block, verifyBlock.nFile, verifyBlock.nOffset, true, true))
    {
        StdError("BlockBase", "Repair block DB: Read block fail, file: %d, offset: %d, block: %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, verifyBlock.hashBlock.GetHex().c_str());
        return false;
    }
    uint256 hashBlock = block.GetHash();
    if (hashBlock != verifyBlock.hashBlock)
    {
        StdError("BlockBase", "Repair block DB: Block error, file: %d, offset: %d, calc block: %s, block: %s.",
                 verifyBlock.nFile, verifyBlock.nOffset, hashBlock.GetHex().c_str(), verifyBlock.hashBlock.GetHex().c_str());
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
            StdError("BlockBase", "Repair block DB: Get prev index fail, block: %s, prev block: %s.",
                     verifyBlock.hashBlock.GetHex().c_str(), block.hashPrev.GetHex().c_str());
            return false;
        }
        hashFork = pPrevIndex->GetOriginHash();
    }

    if (!SaveBlock(hashFork, hashBlock, block, ppIndexNew, blockRoot, true))
    {
        StdError("BlockBase", "Repair block DB: Save block failed, block: %s", hashBlock.ToString().c_str());
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

} // namespace storage
} // namespace metabasenet

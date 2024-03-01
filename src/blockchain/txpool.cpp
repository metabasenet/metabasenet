// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txpool.h"

#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>
#include <deque>

using namespace std;
using namespace mtbase;

namespace metabasenet
{

//////////////////////////////
// CAddressTxState

void CAddressTxState::AddAddressTx(const uint256& txid, const CPooledTxPtr& ptx)
{
    mapDestTx.insert(std::make_pair(txid, ptx));
    RemoveMissTx(txid);
}

void CAddressTxState::RemoveAddressTx(const uint256& txid)
{
    mapDestTx.erase(txid);
}

bool CAddressTxState::IsEmptyAddressTx() const
{
    return (mapDestTx.empty() && mapMissTx.empty());
}

CDestState& CAddressTxState::GetAddressState()
{
    return stateAddress;
}

void CAddressTxState::SetAddressState(const CDestState& state)
{
    stateAddress = state;
    for (auto it = mapMissNonce.begin(); it != mapMissNonce.end();)
    {
        if (it->first > stateAddress.GetTxNonce())
        {
            break;
        }
        mapMissTx.erase(it->second);
        mapMissNonce.erase(it++);
    }
}

void CAddressTxState::AddMissTx(const uint256& txid, const CTransaction& tx)
{
    if (tx.GetNonce() <= stateAddress.GetTxNonce())
    {
        return;
    }
    if (mapMissTx.size() >= MAX_CACHE_MISSING_PREV_TX_COUNT)
    {
        if (mapMissNonce.size() > 0)
        {
            mapMissTx.erase(mapMissNonce.begin()->second);
            mapMissNonce.erase(mapMissNonce.begin());
        }
    }
    if (mapMissTx.count(txid) == 0)
    {
        auto it = mapMissNonce.find(tx.GetNonce());
        if (it != mapMissNonce.end())
        {
            mapMissTx.erase(it->second);
            mapMissNonce.erase(it);
        }
        mapMissTx.insert(std::make_pair(txid, tx));
        mapMissNonce.insert(std::make_pair(tx.GetNonce(), txid));
    }
}

void CAddressTxState::RemoveMissTx(const uint256& txid)
{
    auto it = mapMissTx.find(txid);
    if (it != mapMissTx.end())
    {
        mapMissNonce.erase(it->second.GetNonce());
        mapMissTx.erase(it);
    }
}

bool CAddressTxState::FetchNextMissTx(uint256& txid, CTransaction& tx)
{
    auto it = mapMissNonce.find(stateAddress.GetNextTxNonce());
    if (it != mapMissNonce.end())
    {
        auto mt = mapMissTx.find(it->second);
        if (mt != mapMissTx.end())
        {
            txid = mt->first;
            tx = mt->second;
            return true;
        }
    }
    return false;
}

//////////////////////////////
// CForkTxPool

bool CForkTxPool::GetSaveTxList(vector<std::pair<uint256, CTransaction>>& vTx)
{
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        if (kv.ptx)
        {
            const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx.get());
            vTx.push_back(make_pair(kv.txid, tx));
        }
    }
    return true;
}

void CForkTxPool::ClearTxPool()
{
    nTxSequenceNumber = INIT_TX_SEQUENCE_NUMBER;
    setTxLinkIndex.clear();
    mapTx.clear();
    mapAddressTxState.clear();
    StdLog("CForkTxPool", "Clear Tx Pool: Clear tx pool success, fork: %s", hashFork.ToString().c_str());
}

bool CForkTxPool::AddPooledTx(const uint256& txid, const CTransaction& tx, const int64 nTxSeq)
{
    CPooledTxPtr ptx(new CPooledTx(tx, nTxSeq));
    if (ptx == nullptr)
    {
        StdLog("CForkTxPool", "Add Pooled Tx: new tx fail, txid: %s", txid.ToString().c_str());
        return false;
    }
    auto it = mapTx.insert(make_pair(txid, ptx)).first;
    if (it == mapTx.end())
    {
        StdLog("CForkTxPool", "Add Pooled Tx: add tx fail, txid: %s", txid.ToString().c_str());
        return false;
    }

    if (!setTxLinkIndex.insert(CPooledTxLink(ptx)).second)
    {
        StdLog("CForkTxPool", "Add Pooled Tx: add link fail, txid: %s", txid.ToString().c_str());
        mapTx.erase(it);
        return false;
    }

    if (!tx.GetFromAddress().IsNull())
    {
        mapAddressTxState[tx.GetFromAddress()].AddAddressTx(txid, ptx);
    }
    if (!tx.GetToAddress().IsNull() && tx.GetFromAddress() != tx.GetToAddress())
    {
        mapAddressTxState[tx.GetToAddress()].AddAddressTx(txid, ptx);
    }
    return true;
}

bool CForkTxPool::RemovePooledTx(const uint256& txid, const CTransaction& tx, const bool fInvalidTx)
{
    auto it = mapTx.find(txid);
    if (it != mapTx.end())
    {
        auto removeDestTx = [&](const CDestination& dest, const uint256& txidc) -> bool {
            auto mt = mapAddressTxState.find(dest);
            if (mt == mapAddressTxState.end())
            {
                return false;
            }
            mt->second.RemoveAddressTx(txidc);
            if (mt->second.IsEmptyAddressTx())
            {
                // StdLog("CForkTxPool", "Remove Pooled Tx: Remove address state, address: %s, txid: %s",
                //        dest.ToString().c_str(), txid.GetHex().c_str());
                mapAddressTxState.erase(mt);
            }
            return true;
        };

        if (!tx.GetFromAddress().IsNull())
        {
            if (!removeDestTx(tx.GetFromAddress(), txid))
            {
                StdError("CForkTxPool", "Remove Pooled Tx: Remove dest tx error, from: %s, txid: %s",
                         tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
                return false;
            }
        }
        if (!tx.GetToAddress().IsNull() && tx.GetToAddress() != tx.GetFromAddress())
        {
            if (!removeDestTx(tx.GetToAddress(), txid))
            {
                StdError("CForkTxPool", "Remove Pooled Tx: Remove dest tx error, to: %s, txid: %s",
                         tx.GetToAddress().ToString().c_str(), txid.GetHex().c_str());
                return false;
            }
        }

        setTxLinkIndex.erase(txid);
        mapTx.erase(it);

        if (mapTx.empty() && nTxSequenceNumber > 0xFFFFFFFFFFFFFFFL)
        {
            nTxSequenceNumber = INIT_TX_SEQUENCE_NUMBER;
        }
    }
    else
    {
        if (fInvalidTx)
        {
            StdError("CForkTxPool", "Remove Pooled Tx: Remove invalud tx error, from: %s, txid: %s",
                     tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
            return false;
        }

        if (!tx.GetFromAddress().IsNull())
        {
            vector<pair<uint256, CPooledTxPtr>> vDelTx;
            auto mt = mapAddressTxState.find(tx.GetFromAddress());
            if (mt != mapAddressTxState.end())
            {
                for (const auto& kv : mt->second.mapDestTx)
                {
                    if (kv.second == nullptr)
                    {
                        continue;
                    }
                    const uint256& txidn = kv.first;
                    const CPooledTxPtr& ptx = kv.second;
                    if (ptx->GetFromAddress() == tx.GetFromAddress() /*&& ptx->GetNonce() >= tx.GetNonce()*/)
                    {
                        vDelTx.push_back(make_pair(txidn, ptx));
                    }
                }
            }
            for (auto& kv : vDelTx)
            {
                const uint256& txidn = kv.first;
                const CTransaction& txh = *static_cast<CTransaction*>(kv.second.get());
                if (!RemovePooledTx(txidn, txh, true))
                {
                    StdLog("CForkTxPool", "Remove Pooled Tx: Remove invalid tx fail, from: %s, txid: %s",
                           txh.GetFromAddress().ToString().c_str(), txidn.GetHex().c_str());
                    return false;
                }
                StdDebug("CForkTxPool", "Remove Pooled Tx: Remove invalid tx success, nonce: %ld, from: %s, txid: %s",
                         txh.GetNonce(), txh.GetFromAddress().ToString().c_str(), txidn.GetHex().c_str());
            }
        }
    }
    return true;
}

Errno CForkTxPool::AddTx(const uint256& txid, const CTransaction& tx)
{
    if (mapTx.find(txid) != mapTx.end())
    {
        StdDebug("CForkTxPool", "Add Tx: Tx exist, from: %s, txid: %s",
                 tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_ALREADY_HAVE;
    }
    if (pBlockChain->ExistsTx(hashFork, txid))
    {
        StdDebug("CForkTxPool", "Add Tx: Tx blockchain exist, from: %s, txid: %s",
                 tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_ALREADY_HAVE;
    }
    if (tx.GetFromAddress().IsNull())
    {
        StdLog("CForkTxPool", "Add Tx: From or to address is null, from: %s, to: %s, txid: %s",
               tx.GetFromAddress().ToString().c_str(), tx.GetToAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_TRANSACTION_INPUT_INVALID;
    }
    if (pBlockChain->IsExistBlacklistAddress(tx.GetFromAddress()))
    {
        StdLog("CForkTxPool", "Add Tx: From address at blacklist, from: %s, txid: %s",
               tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_TRANSACTION_AT_BLACKLIST;
    }

    CDestState stateFrom;
    if (!GetDestState(tx.GetFromAddress(), stateFrom))
    {
        StdLog("CForkTxPool", "Add Tx: Get from state fail, from: %s, txid: %s",
               tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_MISSING_PREV;
    }

    CAddressContext ctxAddressFrom;
    CAddressContext ctxAddressTo;

    std::map<CDestination, CAddressContext> mapBlockAddress;
    if (!GetAddressContext(tx.GetFromAddress(), ctxAddressFrom))
    {
        StdLog("CForkTxPool", "Add Tx: Get from address fail, from: %s, txid: %s",
               tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    mapBlockAddress.insert(make_pair(tx.GetFromAddress(), ctxAddressFrom));
    if (!tx.GetToAddress().IsNull())
    {
        if (!GetAddressContext(tx.GetToAddress(), ctxAddressTo))
        {
            if (!pBlockChain->GetTxToAddressContext(hashFork, hashLastBlock, tx, ctxAddressTo))
            {
                ctxAddressTo = CAddressContext(CPubkeyAddressContext());
            }
        }
        mapBlockAddress.insert(make_pair(tx.GetToAddress(), ctxAddressTo));
    }

    Errno err;
    if ((err = pCoreProtocol->VerifyTransaction(txid, tx, hashFork, hashLastBlock, CBlock::GetBlockHeightByHash(hashLastBlock) + 1, stateFrom, mapBlockAddress)) != OK)
    {
        StdLog("CForkTxPool", "Add Tx: Verify transaction fail, from: %s, txid: %s", tx.GetFromAddress().ToString().c_str(), txid.GetHex().c_str());
        if (err == ERR_MISSING_PREV)
        {
            auto it = mapAddressTxState.find(tx.GetFromAddress());
            if (it == mapAddressTxState.end())
            {
                SetDestState(tx.GetFromAddress(), stateFrom);
                mapAddressTxState[tx.GetFromAddress()].ctxAddress = ctxAddressFrom;
                it = mapAddressTxState.find(tx.GetFromAddress());
            }
            if (it != mapAddressTxState.end())
            {
                it->second.AddMissTx(txid, tx);
            }
        }
        return err;
    }

    if (hashFork == pCoreProtocol->GetGenesisBlockHash() && tx.GetTxType() == CTransaction::TX_CERT)
    {
        if (!VerifyRepeatCertTx(tx))
        {
            StdLog("CForkTxPool", "Add Tx: Verify cert tx fail, delegate address: %s, txid: %s", tx.GetToAddress().ToString().c_str(), txid.GetHex().c_str());
            return ERR_ALREADY_HAVE;
        }
        // if (!pBlockChain->VerifyDelegateMinVote(hashLastBlock, tx.GetNonce(), tx.GetToAddress()))
        // {
        //     StdLog("CForkTxPool", "Add Tx: Not enough votes, delegate address: %s, txid: %s", tx.GetToAddress().ToString().c_str(), txid.GetHex().c_str());
        //     return ERR_ALREADY_HAVE;
        // }
    }

    if (!AddPooledTx(txid, tx, nTxSequenceNumber++))
    {
        StdLog("CForkTxPool", "Add Tx: Add pooled tx fail, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.GetFromAddress() == tx.GetToAddress())
    {
        stateFrom.IncBalance(tx.GetAmount());
    }
    else if (tx.GetToAddress() != 0)
    {
        CDestState stateTo;
        if (!GetDestState(tx.GetToAddress(), stateTo))
        {
            stateTo.SetNull();
            stateTo.SetType(ctxAddressTo.GetDestType(), ctxAddressTo.GetTemplateType());
        }
        stateTo.IncBalance(tx.GetAmount());
        SetDestState(tx.GetToAddress(), stateTo);
    }

    if (tx.GetTxType() != CTransaction::TX_CERT)
    {
        stateFrom.IncTxNonce();
    }
    stateFrom.DecBalance(tx.GetAmount() + tx.GetTxFee());
    SetDestState(tx.GetFromAddress(), stateFrom);

    mapAddressTxState[tx.GetFromAddress()].ctxAddress = ctxAddressFrom;
    if (!tx.GetToAddress().IsNull())
    {
        mapAddressTxState[tx.GetToAddress()].ctxAddress = ctxAddressTo;
    }
    return OK;
}

bool CForkTxPool::Exists(const uint256& txid)
{
    return (mapTx.count(txid) > 0);
}

bool CForkTxPool::CheckTxNonce(const CDestination& destFrom, const uint64 nTxNonce)
{
    CDestState state;
    if (!GetDestState(destFrom, state))
    {
        return false;
    }
    if (state.GetTxNonce() < nTxNonce)
    {
        return false;
    }
    return true;
}

std::size_t CForkTxPool::GetTxCount() const
{
    return mapTx.size();
}

bool CForkTxPool::GetTx(const uint256& txid, CTransaction& tx) const
{
    auto it = mapTx.find(txid);
    if (it != mapTx.end() && it->second)
    {
        tx = *static_cast<CTransaction*>(it->second.get());
        return true;
    }
    return false;
}

uint64 CForkTxPool::GetDestNextTxNonce(const CDestination& dest)
{
    CDestState state;
    if (!GetDestState(dest, state))
    {
        return 0;
    }
    return state.GetNextTxNonce();
}

bool CForkTxPool::GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashRefBlock)
{
    if (hashRefBlock == 0)
    {
        auto it = mapAddressTxState.find(dest);
        if (it == mapAddressTxState.end() || it->second.ctxAddress.IsNull())
        {
            if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest, ctxAddress))
            {
                return false;
            }
        }
        else
        {
            ctxAddress = it->second.ctxAddress;
        }
    }
    else
    {
        if (!pBlockChain->RetrieveAddressContext(hashFork, hashRefBlock, dest, ctxAddress))
        {
            return false;
        }
    }
    return true;
}

bool CForkTxPool::FetchArrangeBlockTx(const uint256& hashPrev, const int64 nBlockTime,
                                      const size_t nMaxSize, vector<CTransaction>& vtx, uint256& nTotalTxFee)
{
    if (hashPrev != hashLastBlock)
    {
        return true;
    }

    uint256 nMintMinGasPrice = pBlockChain->GetForkMintMinGasPrice(hashFork);

    set<CDestination> setDisable;
    size_t nTotalSize = 0;

    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        set<pair<CDestination, int>> setDelegateEnroll;
        CPooledTxLinkSetByTxType& typeTx = setTxLinkIndex.get<2>();
        auto mt = typeTx.find(CTransaction::TX_CERT);
        for (; mt != typeTx.end(); ++mt)
        {
            if (mt->nType != CTransaction::TX_CERT)
            {
                break;
            }
            if (mt->ptx)
            {
                const CTransaction& tx = *static_cast<CTransaction*>(mt->ptx.get());
                if (nTotalSize + mt->ptx->nSerializeSize > nMaxSize)
                {
                    break;
                }

                if (tx.GetNonce() + CONSENSUS_ENROLL_INTERVAL < CBlock::GetBlockHeightByHash(hashPrev) || tx.GetNonce() > CBlock::GetBlockHeightByHash(hashPrev))
                {
                    continue;
                }
                if (!pBlockChain->VerifyDelegateMinVote(hashPrev, tx.GetNonce(), tx.GetToAddress()))
                {
                    continue;
                }
                if (!pBlockChain->VerifyDelegateEnroll(hashPrev, tx.GetNonce(), tx.GetToAddress()))
                {
                    continue;
                }
                if (setDelegateEnroll.find(make_pair(tx.GetToAddress(), (int)(tx.GetNonce()))) != setDelegateEnroll.end())
                {
                    continue;
                }
                setDelegateEnroll.insert(make_pair(tx.GetToAddress(), (int)(tx.GetNonce())));

                vtx.push_back(tx);
                nTotalSize += mt->ptx->nSerializeSize;
                nTotalTxFee += mt->ptx->GetTxFee();
            }
        }
    }

    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        if (kv.ptx == nullptr)
        {
            continue;
        }
        if (kv.nType == CTransaction::TX_CERT)
        {
            continue;
        }
        const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx.get());
        if (setDisable.find(tx.GetFromAddress()) != setDisable.end())
        {
            continue;
        }
        if (tx.GetGasPrice() < nMintMinGasPrice)
        {
            setDisable.insert(tx.GetFromAddress());
            continue;
        }
        if (nTotalSize + kv.ptx->nSerializeSize > nMaxSize)
        {
            break;
        }
        vtx.push_back(tx);
        nTotalSize += kv.ptx->nSerializeSize;
        nTotalTxFee += kv.ptx->GetTxFee();
    }
    return true;
}

bool CForkTxPool::SynchronizeBlockChain(const CBlockChainUpdate& update)
{
    std::set<CDestination> setAddAddress;
    for (const CBlockEx& block : update.vBlockAddNew)
    {
        for (const auto& tx : block.vtx)
        {
            if (tx.GetTxType() != CTransaction::TX_VOTE_REWARD)
            {
                const uint256 txid = tx.GetHash();
                if (!RemovePooledTx(txid, tx, false))
                {
                    StdLog("CForkTxPool", "Synchronize Block Chain: Remove pooled tx fail, new block: %s, txid: %s",
                           block.GetHash().GetHex().c_str(), txid.GetHex().c_str());
                    return false;
                }
                if (!tx.GetFromAddress().IsNull())
                {
                    setAddAddress.insert(tx.GetFromAddress());
                }
            }
        }
    }

    for (const CBlockEx& block : boost::adaptors::reverse(update.vBlockRemove))
    {
        for (const auto& tx : block.vtx)
        {
            if (tx.GetTxType() != CTransaction::TX_VOTE_REWARD && tx.GetTxType() != CTransaction::TX_CERT)
            {
                if (!AddTx(tx.GetHash(), tx))
                {
                    StdLog("CForkTxPool", "Synchronize Block Chain: Add tx fail, remove block: %s, txid: %s",
                           block.GetHash().GetHex().c_str(), tx.GetHash().GetHex().c_str());
                }
            }
        }
    }

    for (auto& dest : setAddAddress)
    {
        CDestState stateDb;
        if (!pBlockChain->RetrieveDestState(hashFork, update.hashLastBlock, dest, stateDb))
        {
            StdLog("CForkTxPool", "Synchronize Block Chain: Get address state fail, dest: %s, last block: %s",
                   dest.ToString().c_str(), update.hashLastBlock.GetHex().c_str());
            return false;
        }
        auto it = mapAddressTxState.find(dest);
        if (it != mapAddressTxState.end())
        {
            if (it->second.GetAddressState().GetTxNonce() < stateDb.GetTxNonce())
            {
                it->second.SetAddressState(stateDb);
            }
            do
            {
                uint256 txid;
                CTransaction tx;
                if (!it->second.FetchNextMissTx(txid, tx))
                {
                    break;
                }
                if (AddTx(txid, tx) != OK)
                {
                    break;
                }
            } while (true);
        }
    }

    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        RemoveObsoletedCertTx();
    }

    hashLastBlock = update.hashLastBlock;
    nLastBlockTime = update.nLastBlockTime;
    return true;
}

void CForkTxPool::ListTx(vector<pair<uint256, size_t>>& vTxPool)
{
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        vTxPool.push_back(make_pair(kv.txid, kv.ptx->nSerializeSize));
    }
}

void CForkTxPool::ListTx(vector<uint256>& vTxPool)
{
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        vTxPool.push_back(kv.txid);
    }
}

bool CForkTxPool::ListTx(const CDestination& dest, vector<CTxInfo>& vTxPool, const int64 nGetOffset, const int64 nGetCount)
{
    uint64 nTxSeq = 0;
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        if (nTxSeq >= nGetOffset && kv.ptx)
        {
            CTxInfo txInfo;
            const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx.get());

            txInfo.txid = kv.txid;
            txInfo.hashFork = hashFork;
            txInfo.nTxType = tx.GetTxType();
            txInfo.nTxNonce = tx.GetNonce();
            txInfo.nBlockHeight = -1;
            txInfo.nTxSeq = nTxSeq;
            txInfo.destFrom = tx.GetFromAddress();
            txInfo.destTo = tx.GetToAddress();
            txInfo.nAmount = tx.GetAmount();
            txInfo.nGasPrice = tx.GetGasPrice();
            txInfo.nGas = tx.GetGasLimit();
            txInfo.nSize = kv.ptx->nSerializeSize;

            vTxPool.push_back(txInfo);
            if (vTxPool.size() >= nGetCount)
            {
                break;
            }
        }
        nTxSeq++;
    }
    return true;
}

void CForkTxPool::GetDestBalance(const CDestination& dest, uint8& nDestType, uint8& nTemplateType, uint64& nTxNonce, uint256& nAvail,
                                 uint256& nUnconfirmedIn, uint256& nUnconfirmedOut, CAddressContext& ctxAddress, const uint256& hashBlock)
{
    CDestState state;
    if (!GetDestState(dest, state, hashBlock))
    {
        state.SetNull();
    }
    nDestType = state.GetDestType();
    nTemplateType = state.GetTemplateType();
    nTxNonce = state.GetTxNonce();
    nAvail = state.GetBalance();

    nUnconfirmedIn = 0;
    nUnconfirmedOut = 0;

    if (hashBlock == 0)
    {
        // pending
        auto mt = mapAddressTxState.find(dest);
        if (mt != mapAddressTxState.end())
        {
            for (const auto& kv : mt->second.mapDestTx)
            {
                if (kv.second)
                {
                    const CPooledTxPtr& ptx = kv.second;
                    if (ptx->GetFromAddress() == dest)
                    {
                        nUnconfirmedOut += (ptx->GetAmount() + ptx->GetTxFee());
                    }
                    if (ptx->GetToAddress() == dest)
                    {
                        nUnconfirmedIn += ptx->GetAmount();
                    }
                }
            }
        }
    }

    if (!GetAddressContext(dest, ctxAddress, hashBlock))
    {
        ctxAddress = CAddressContext(CPubkeyAddressContext());
    }
}

////////////////////////////////////
bool CForkTxPool::GetDestState(const CDestination& dest, CDestState& state, const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        // pending
        auto it = mapAddressTxState.find(dest);
        if (it == mapAddressTxState.end() || it->second.GetAddressState().IsNull())
        {
            if (!pBlockChain->RetrieveDestState(hashFork, hashLastBlock, dest, state))
            {
                return false;
            }
        }
        else
        {
            state = it->second.GetAddressState();
        }
    }
    else
    {
        if (!pBlockChain->RetrieveDestState(hashFork, hashBlock, dest, state))
        {
            return false;
        }
    }
    return true;
}

void CForkTxPool::SetDestState(const CDestination& dest, const CDestState& state)
{
    mapAddressTxState[dest].SetAddressState(state);
}

int64 CForkTxPool::GetMinTxSequenceNumber()
{
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    auto it = idxTx.begin();
    if (it != idxTx.end())
    {
        return it->nSequenceNumber;
    }
    return nTxSequenceNumber;
}

bool CForkTxPool::VerifyRepeatCertTx(const CTransaction& tx)
{
    if (tx.GetNonce() + CONSENSUS_ENROLL_INTERVAL < CBlock::GetBlockHeightByHash(hashLastBlock)
        || tx.GetNonce() > CBlock::GetBlockHeightByHash(hashLastBlock) + 2)
    {
        StdLog("CForkTxPool", "Verify Repeat Cert Tx: Enroll height error, Enroll height %lu, last height: %d, delegate address: %s, txid: %s",
               tx.GetNonce(), CBlock::GetBlockHeightByHash(hashLastBlock), tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str());
        return false;
    }
    auto it = mapAddressTxState.find(tx.GetToAddress());
    if (it != mapAddressTxState.end())
    {
        for (auto& kv : it->second.mapDestTx)
        {
            const CPooledTxPtr& ptx = kv.second;
            if (ptx->GetTxType() == CTransaction::TX_CERT && ptx->GetNonce() == tx.GetNonce())
            {
                StdLog("CForkTxPool", "Verify Repeat Cert Tx: Repeat enroll error, Enroll height %lu, delegate address: %s, txid: %s",
                       tx.GetNonce(), tx.GetToAddress().ToString().c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
        }
    }
    return true;
}

void CForkTxPool::RemoveObsoletedCertTx()
{
    uint32 nLastHeight = CBlock::GetBlockHeightByHash(hashLastBlock);
    vector<pair<uint256, CPooledTxPtr>> vRemoveTxid;

    CPooledTxLinkSetByTxType& typeTx = setTxLinkIndex.get<2>();
    auto mt = typeTx.find(CTransaction::TX_CERT);
    for (; mt != typeTx.end(); ++mt)
    {
        if (mt->nType != CTransaction::TX_CERT)
        {
            break;
        }
        if (mt->ptx)
        {
            if ((uint32)(mt->ptx->GetNonce()) + CONSENSUS_ENROLL_INTERVAL < nLastHeight)
            {
                vRemoveTxid.push_back(make_pair(mt->txid, mt->ptx));
            }
        }
    }

    for (auto& kv : vRemoveTxid)
    {
        const uint256& txid = kv.first;
        const CPooledTxPtr& ptx = kv.second;
        const CTransaction& tx = *static_cast<CTransaction*>(ptx.get());
        if (!RemovePooledTx(txid, tx, false))
        {
            StdLog("CForkTxPool", "Remove Obsoleted Cert Tx: Remove pooled tx fail, txid: %s", txid.GetHex().c_str());
        }
    }
}

//////////////////////////////
// CTxPool

CTxPool::CTxPool()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pDataStat = nullptr;
    pCertTxChannel = nullptr;
    pUserTxChannel = nullptr;
}

CTxPool::~CTxPool()
{
}

bool CTxPool::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    if (!GetObject("datastat", pDataStat))
    {
        Error("Failed to request datastat");
        return false;
    }

    if (!GetObject("certtxchannel", pCertTxChannel))
    {
        Error("Failed to request certtxchannel");
        return false;
    }

    if (!GetObject("usertxchannel", pUserTxChannel))
    {
        Error("Failed to request usertxchannel");
        return false;
    }
    return true;
}

void CTxPool::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pDataStat = nullptr;
    pCertTxChannel = nullptr;
    pUserTxChannel = nullptr;
}

bool CTxPool::HandleInvoke()
{
    if (!datTxPool.Initialize(Config()->pathData))
    {
        Error("Failed to initialize txpool data");
        return false;
    }

    if (!LoadData())
    {
        Error("Failed to load txpool data");
        return false;
    }
    return true;
}

void CTxPool::HandleHalt()
{
    if (!SaveData())
    {
        Error("Failed to save txpool data");
    }
    //Clear();
}

bool CTxPool::LoadData()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);

    std::map<uint256, CForkStatus> mapForkStatus;
    pBlockChain->GetForkStatus(mapForkStatus);
    for (const auto& kv : mapForkStatus)
    {
        mapForkPool.insert(make_pair(kv.first, CForkTxPool(pCoreProtocol, pBlockChain, kv.first, kv.second.hashLastBlock, kv.second.nLastBlockTime)));
    }

    std::map<uint256, std::vector<std::pair<uint256, CTransaction>>> mapSaveTx;
    if (!datTxPool.Load(mapSaveTx))
    {
        Error("Load Data failed");
        return false;
    }

    for (const auto& kv : mapSaveTx)
    {
        const uint256& hashFork = kv.first;
        CForkTxPool* pFork = GetForkTxPool(hashFork);
        if (pFork == nullptr)
        {
            Error("Get fork tx pool failed");
            return false;
        }

        for (const auto& vd : kv.second)
        {
            const uint256& txid = vd.first;
            const CTransaction& tx = vd.second;
            if (pFork->AddTx(txid, tx) != OK)
            {
                StdLog("CTxPool", "LoadData: Add tx fail, txid: %s", txid.GetHex().c_str());
            }
        }
    }
    return true;
}

bool CTxPool::SaveData()
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    std::map<uint256, std::vector<std::pair<uint256, CTransaction>>> mapSaveTx;
    for (auto& kv : mapForkPool)
    {
        kv.second.GetSaveTxList(mapSaveTx[kv.first]);
    }
    return datTxPool.Save(mapSaveTx);
}

CForkTxPool* CTxPool::GetForkTxPool(const uint256& hashFork)
{
    auto it = mapForkPool.find(hashFork);
    if (it == mapForkPool.end())
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            return nullptr;
        }
        it = mapForkPool.insert(make_pair(hashFork, CForkTxPool(pCoreProtocol, pBlockChain, hashFork, status.hashBlock, status.nBlockTime))).first;
    }
    return &(it->second);
}

///////////////////////////////////////////////////////////
void CTxPool::ClearTxPool(const uint256& hashFork)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        it->second.ClearTxPool();
    }
}

bool CTxPool::Exists(const uint256& hashFork, const uint256& txid)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        return it->second.Exists(txid);
    }
    return false;
}

bool CTxPool::CheckTxNonce(const uint256& hashFork, const CDestination& destFrom, const uint64 nTxNonce)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        return it->second.CheckTxNonce(destFrom, nTxNonce);
    }
    return false;
}

size_t CTxPool::Count(const uint256& hashFork) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        return it->second.GetTxCount();
    }
    return 0;
}

Errno CTxPool::Push(const uint256& hashFork, const CTransaction& tx)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
    uint256 txid = tx.GetHash();

    if (tx.IsRewardTx())
    {
        StdError("CTxPool", "Push: tx is mint, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork == nullptr)
    {
        StdError("CTxPool", "Push: Get fork tx pool failed, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    return pFork->AddTx(txid, tx);
}

bool CTxPool::Get(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    if (hashFork == 0)
    {
        for (const auto& kv : mapForkPool)
        {
            if (kv.second.GetTx(txid, tx))
            {
                hashAtFork = kv.first;
                return true;
            }
        }
    }
    else
    {
        auto it = mapForkPool.find(hashFork);
        if (it != mapForkPool.end())
        {
            if (it->second.GetTx(txid, tx))
            {
                hashAtFork = hashFork;
                return true;
            }
        }
    }
    return false;
}

void CTxPool::ListTx(const uint256& hashFork, vector<pair<uint256, size_t>>& vTxPool)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        it->second.ListTx(vTxPool);
    }
}

void CTxPool::ListTx(const uint256& hashFork, vector<uint256>& vTxPool)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        it->second.ListTx(vTxPool);
    }
}

bool CTxPool::ListTx(const uint256& hashFork, const CDestination& dest, vector<CTxInfo>& vTxPool, const int64 nGetOffset, const int64 nGetCount)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapForkPool.find(hashFork);
    if (it != mapForkPool.end())
    {
        return it->second.ListTx(dest, vTxPool, nGetOffset, nGetCount);
    }
    return false;
}

bool CTxPool::FetchArrangeBlockTx(const uint256& hashFork, const uint256& hashPrev, const int64 nBlockTime,
                                  const size_t nMaxSize, vector<CTransaction>& vtx, uint256& nTotalTxFee)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork)
    {
        return pFork->FetchArrangeBlockTx(hashPrev, nBlockTime, nMaxSize, vtx, nTotalTxFee);
    }
    return false;
}

bool CTxPool::SynchronizeBlockChain(const CBlockChainUpdate& update)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(update.hashFork);
    if (pFork)
    {
        return pFork->SynchronizeBlockChain(update);
    }
    return false;
}

bool CTxPool::PushCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx)
{
    const uint256 hashFork = pCoreProtocol->GetGenesisBlockHash();
    std::vector<CTransaction> vBroadTx;
    {
        boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
        CForkTxPool* pFork = GetForkTxPool(hashFork);
        if (!pFork)
        {
            return false;
        }
        for (auto& tx : vtx)
        {
            Errno err = pFork->AddTx(tx.GetHash(), tx);
            if (err == OK)
            {
                vBroadTx.push_back(tx);
            }
        }
    }
    pDataStat->AddP2pSynTxSynStatData(hashFork, vtx.size(), true);
    if (!vBroadTx.empty())
    {
        pCertTxChannel->BroadcastCertTx(nRecvNetNonce, vBroadTx);
        pDataStat->AddP2pSynTxSynStatData(hashFork, vBroadTx.size(), false);
    }
    return true;
}

bool CTxPool::PushUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx)
{
    std::vector<CTransaction> vBroadTx;
    {
        boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
        CForkTxPool* pFork = GetForkTxPool(hashFork);
        if (!pFork)
        {
            return false;
        }
        for (auto& tx : vtx)
        {
            Errno err = pFork->AddTx(tx.GetHash(), tx);
            if (err == OK /*|| err == ERR_MISSING_PREV*/)
            {
                vBroadTx.push_back(tx);
            }
        }
    }
    pDataStat->AddP2pSynTxSynStatData(hashFork, vtx.size(), true);
    if (!vBroadTx.empty())
    {
        pUserTxChannel->BroadcastUserTx(nRecvNetNonce, hashFork, vBroadTx);
        pDataStat->AddP2pSynTxSynStatData(hashFork, vBroadTx.size(), false);
    }
    return true;
}

void CTxPool::GetDestBalance(const uint256& hashFork, const CDestination& dest, uint8& nDestType, uint8& nTemplateType, uint64& nTxNonce, uint256& nAvail,
                             uint256& nUnconfirmedIn, uint256& nUnconfirmedOut, CAddressContext& ctxAddress, const uint256& hashBlock)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork)
    {
        pFork->GetDestBalance(dest, nDestType, nTemplateType, nTxNonce, nAvail, nUnconfirmedIn, nUnconfirmedOut, ctxAddress, hashBlock);
    }
}

uint64 CTxPool::GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork)
    {
        return pFork->GetDestNextTxNonce(dest);
    }
    return 0;
}

bool CTxPool::GetAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashRefBlock)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork)
    {
        return pFork->GetAddressContext(dest, ctxAddress, hashRefBlock);
    }
    return false;
}

} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txpool.h"

#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>
#include <deque>

using namespace std;
using namespace hcode;

namespace metabasenet
{

//////////////////////////////
// CForkTxPool

bool CForkTxPool::GetSaveTxList(vector<std::pair<uint256, CTransaction>>& vTx)
{
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx);
        vTx.push_back(make_pair(kv.txid, tx));
    }
    return true;
}

bool CForkTxPool::AddPooledTx(const uint256& txid, const CTransaction& tx, const int64 nTxSeq)
{
    auto it = mapTx.insert(make_pair(txid, CPooledTx(tx, nTxSeq))).first;
    if (it == mapTx.end())
    {
        return false;
    }

    if (!setTxLinkIndex.insert(CPooledTxLink(&it->second)).second)
    {
        mapTx.erase(it);
        return false;
    }

    if (!tx.from.IsNull())
    {
        mapDestTx[tx.from].insert(txid);
    }
    if (!tx.to.IsNull() && tx.from != tx.to)
    {
        mapDestTx[tx.to].insert(txid);
    }
    return true;
}

bool CForkTxPool::RemovePooledTx(const uint256& txid, const CTransaction& tx, const bool fInvalidTx)
{
    auto it = mapTx.find(txid);
    if (it != mapTx.end())
    {
        auto removeDestTx = [&](const CDestination& dest, const uint256& txidc) -> bool
        {
            auto mt = mapDestTx.find(dest);
            if (mt == mapDestTx.end())
            {
                return false;
            }
            mt->second.erase(txidc);
            if (mt->second.empty())
            {
                mapDestTx.erase(dest);
                mapDestState.erase(dest);
            }
            return true;
        };

        if (!tx.from.IsNull())
        {
            if (!removeDestTx(tx.from, txid))
            {
                StdError("CForkTxPool", "Remove Pooled Tx: Remove dest tx error, from: %s, txid: %s",
                         tx.from.ToString().c_str(), txid.GetHex().c_str());
                return false;
            }
        }
        if (!tx.to.IsNull() && tx.to != tx.from)
        {
            if (!removeDestTx(tx.to, txid))
            {
                StdError("CForkTxPool", "Remove Pooled Tx: Remove dest tx error, to: %s, txid: %s",
                         tx.to.ToString().c_str(), txid.GetHex().c_str());
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
                     tx.from.ToString().c_str(), txid.GetHex().c_str());
            return false;
        }

        if (!tx.from.IsNull())
        {
            vector<pair<uint256, CPooledTx>> vDelTx;
            auto mt = mapDestTx.find(tx.from);
            if (mt != mapDestTx.end())
            {
                for (const uint256& txidn : mt->second)
                {
                    auto nt = mapTx.find(txidn);
                    if (nt != mapTx.end())
                    {
                        const CPooledTx& txn = nt->second;
                        if (txn.from == tx.from && txn.nTxNonce >= tx.nTxNonce)
                        {
                            vDelTx.push_back(make_pair(txidn, txn));
                        }
                    }
                }
            }
            for (const auto& txs : vDelTx)
            {
                const uint256& txids = txs.first;
                const CDestination& destFrom = txs.second.from;
                const uint64 nTxNonce = txs.second.nTxNonce;
                if (!RemovePooledTx(txids, txs.second, true))
                {
                    StdLog("CForkTxPool", "Remove Pooled Tx: Remove invalid tx fail, from: %s, txid: %s",
                           destFrom.ToString().c_str(), txids.GetHex().c_str());
                    return false;
                }
                StdLog("CForkTxPool", "Remove Pooled Tx: Remove invalid tx success, nonce: %ld, count: %ld, from: %s, txid: %s",
                       nTxNonce, vDelTx.size(), destFrom.ToString().c_str(), txids.GetHex().c_str());
            }
        }
    }
    return true;
}

Errno CForkTxPool::AddTx(const uint256& txid, const CTransaction& tx)
{
    if (mapTx.find(txid) != mapTx.end())
    {
        StdLog("CForkTxPool", "Add Tx: Tx exist, from: %s, txid: %s",
               tx.from.ToString().c_str(), txid.GetHex().c_str());
        return ERR_ALREADY_HAVE;
    }
    if (tx.from.IsNull() || tx.to.IsNull())
    {
        StdLog("CForkTxPool", "Add Tx: From or to address is null, from: %s, to: %s, txid: %s",
               tx.from.ToString().c_str(), tx.to.ToString().c_str(), txid.GetHex().c_str());
        return ERR_TRANSACTION_INPUT_INVALID;
    }

    CDestState stateFrom;
    if (!GetDestState(tx.from, stateFrom))
    {
        StdLog("CForkTxPool", "Add Tx: Get from state fail, from: %s, txid: %s",
               tx.from.ToString().c_str(), txid.GetHex().c_str());
        return ERR_MISSING_PREV;
    }

    CAddressContext ctxtAddressFrom;
    CAddressContext ctxtAddressTo;
    if (!GetAddressContext(tx.from, ctxtAddressFrom))
    {
        StdLog("CForkTxPool", "Add Tx: Get from address context fail, from: %s, txid: %s",
               tx.from.ToString().c_str(), txid.GetHex().c_str());
        return ERR_MISSING_PREV;
    }
    if (tx.to.data != 0 && !GetAddressContext(tx.to, ctxtAddressTo))
    {
        bytes btTempData;
        if (tx.to.IsTemplate() && tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
        {
            ctxtAddressTo = CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), CBlock::GetBlockHeightByHash(hashLastBlock) + 1);
        }
        else
        {
            StdLog("CForkTxPool", "Add Tx: Get to address context fail, to: %s, txid: %s",
                   tx.to.ToString().c_str(), txid.GetHex().c_str());
            return ERR_MISSING_PREV;
        }
    }

    std::map<uint256, CAddressContext> mapBlockAddress;
    mapBlockAddress.insert(make_pair(tx.from.data, ctxtAddressFrom));
    if (tx.to.data != 0)
    {
        mapBlockAddress.insert(make_pair(tx.to.data, ctxtAddressTo));
    }

    Errno err;
    if ((err = pCoreProtocol->VerifyTransaction(tx, hashLastBlock, CBlock::GetBlockHeightByHash(hashLastBlock) + 1, stateFrom, mapBlockAddress)) != OK)
    {
        StdLog("CForkTxPool", "Add Tx: Verify transaction fail, txid: %s", txid.GetHex().c_str());
        return err;
    }

    if (!AddPooledTx(txid, tx, nTxSequenceNumber++))
    {
        StdLog("CForkTxPool", "Add Tx: Add pooled tx fail, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.from == tx.to)
    {
        stateFrom.nBalance += tx.nAmount;
    }
    else if (tx.to.data != 0)
    {
        CDestState stateTo;
        if (!GetDestState(tx.to, stateTo))
        {
            stateTo.SetNull();
        }
        stateTo.nBalance += tx.nAmount;
        SetDestState(tx.to, stateTo);
    }

    stateFrom.nTxNonce++;
    stateFrom.nBalance -= (tx.nAmount + tx.GetTxFee());
    SetDestState(tx.from, stateFrom);
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
    if (state.nTxNonce < nTxNonce)
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
    if (it != mapTx.end())
    {
        tx = static_cast<CTransaction>(it->second);
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

uint64 CForkTxPool::GetCertTxNextNonce(const CDestination& dest)
{
    int nTxCount = 0;
    auto mt = mapDestTx.find(dest);
    if (mt != mapDestTx.end())
    {
        for (const uint256& txid : mt->second)
        {
            auto nt = mapTx.find(txid);
            if (nt != mapTx.end())
            {
                const CPooledTx& tx = nt->second;
                if (tx.nType == CTransaction::TX_CERT && tx.from == dest)
                {
                    nTxCount++;
                }
            }
        }
    }
    if (nTxCount >= CONSENSUS_ENROLL_INTERVAL)
    {
        StdLog("CForkTxPool", "Get Cert Tx Next Nonce: nTxCount >= CONSENSUS_ENROLL_INTERVAL, nTxCount: %d, dest: %s", nTxCount, dest.ToString().c_str());
        return 0;
    }

    CDestState state;
    if (!GetDestState(dest, state))
    {
        StdLog("CForkTxPool", "Get Cert Tx Next Nonce: Get dest state fail, dest: %s", dest.ToString().c_str());
        return 0;
    }
    return state.GetNextTxNonce();
}

bool CForkTxPool::FetchArrangeBlockTx(const uint256& hashPrev, const int64 nBlockTime,
                                      const size_t nMaxSize, vector<CTransaction>& vtx, uint256& nTotalTxFee)
{
    if (hashPrev != hashLastBlock)
    {
        return true;
    }

    map<CDestination, int> mapVoteCert;
    if (!pBlockChain->GetDelegateCertTxCount(hashPrev, mapVoteCert))
    {
        StdError("CForkTxPool", "Fetch Arrange Block Tx: Get delegate cert tx count fail, prev: %s", hashPrev.GetHex().c_str());
        return false;
    }

    set<CDestination> setDisable;
    size_t nTotalSize = 0;
    CPooledTxLinkSetBySequenceNumber& idxTx = setTxLinkIndex.get<1>();
    for (const auto& kv : idxTx)
    {
        const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx);
        if (setDisable.find(tx.from) != setDisable.end())
        {
            continue;
        }
        if (tx.nTimeStamp > nBlockTime)
        {
            setDisable.insert(tx.from);
            continue;
        }
        if (tx.nType == CTransaction::TX_CERT)
        {
            auto it = mapVoteCert.find(tx.from);
            if (it != mapVoteCert.end())
            {
                if (it->second <= 0)
                {
                    setDisable.insert(tx.from);
                    continue;
                }
            }
            else
            {
                it = mapVoteCert.insert(make_pair(tx.from, CONSENSUS_ENROLL_INTERVAL * 2)).first;
            }
            it->second--;
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
    int64 nTxCount = 0;
    for (const CBlockEx& block : update.vBlockRemove)
    {
        nTxCount += block.vtx.size();
    }

    int64 nRemoveTxSeq = GetMinTxSequenceNumber() - nTxCount;

    uint256 hashBranchBlock;
    set<CDestination> setNewDest;
    for (const CBlockEx& block : boost::adaptors::reverse(update.vBlockRemove))
    {
        if (hashBranchBlock == 0)
        {
            hashBranchBlock = block.hashPrev;
        }
        for (const auto& tx : block.vtx)
        {
            if (!tx.IsNoMintRewardTx())
            {
                if (!AddSynchronizeTx(hashBranchBlock, tx.GetHash(), tx, nRemoveTxSeq++, setNewDest))
                {
                    StdLog("CForkTxPool", "Synchronize Block Chain: Add synchronize tx fail, remove block: %s, txid: %s",
                           block.GetHash().GetHex().c_str(), tx.GetHash().GetHex().c_str());
                    return false;
                }
            }
        }
    }

    for (const CBlockEx& block : update.vBlockAddNew)
    {
        for (const auto& tx : block.vtx)
        {
            if (!tx.IsNoMintRewardTx())
            {
                const uint256 txid = tx.GetHash();
                if (!RemovePooledTx(txid, tx, false))
                {
                    StdLog("CForkTxPool", "Synchronize Block Chain: Remove pooled tx fail, new block: %s, txid: %s",
                           block.GetHash().GetHex().c_str(), txid.GetHex().c_str());
                    return false;
                }
            }
        }
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
        if (nTxSeq >= nGetOffset)
        {
            CTxInfo txInfo;
            const CTransaction& tx = *static_cast<CTransaction*>(kv.ptx);

            txInfo.txid = kv.txid;
            txInfo.hashFork = tx.hashFork;
            txInfo.nTxType = tx.nType;
            txInfo.nTimeStamp = tx.nTimeStamp;
            txInfo.nTxNonce = tx.nTxNonce;
            txInfo.nBlockHeight = -1;
            txInfo.nTxSeq = nTxSeq;
            txInfo.destFrom = tx.from;
            txInfo.destTo = tx.to;
            txInfo.nAmount = tx.nAmount;
            txInfo.nGasPrice = tx.nGasPrice;
            txInfo.nGas = tx.nGasLimit;
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

void CForkTxPool::GetDestBalance(const CDestination& dest, uint64& nTxNonce, uint256& nAvail, uint256& nUnconfirmedIn, uint256& nUnconfirmedOut)
{
    CDestState state;
    if (!GetDestState(dest, state))
    {
        state.SetNull();
    }
    nTxNonce = state.nTxNonce;
    nAvail = state.nBalance;

    nUnconfirmedIn = 0;
    nUnconfirmedOut = 0;

    auto mt = mapDestTx.find(dest);
    if (mt != mapDestTx.end())
    {
        for (const uint256& txid : mt->second)
        {
            auto nt = mapTx.find(txid);
            if (nt != mapTx.end())
            {
                const CPooledTx& tx = nt->second;
                if (tx.from == dest)
                {
                    nUnconfirmedOut += (tx.nAmount + tx.GetTxFee());
                }
                if (tx.to == dest)
                {
                    nUnconfirmedIn += tx.nAmount;
                }
            }
        }
    }
}

bool CForkTxPool::GetAddressContext(const CDestination& dest, CAddressContext& ctxtAddress)
{
    if (dest.IsPubKey())
    {
        ctxtAddress.nType = CDestination::PREFIX_PUBKEY;
        ctxtAddress.btData.clear();
        ctxtAddress.nBlockNumber = CBlock::GetBlockHeightByHash(hashLastBlock) + 1;
        return true;
    }
    else if (dest.IsContract())
    {
        ctxtAddress.nType = CDestination::PREFIX_CONTRACT;
        ctxtAddress.btData.clear();
        ctxtAddress.nBlockNumber = CBlock::GetBlockHeightByHash(hashLastBlock) + 1;
        return true;
    }
    else if (dest.IsTemplate())
    {
        if (pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest.data, ctxtAddress))
        {
            if (ctxtAddress.nType != dest.prefix)
            {
                StdLog("CForkTxPool", "Get Address Context: Address type error, db address type: %d, query address type: %d, dest: %s",
                       ctxtAddress.nType, dest.prefix, dest.ToString().c_str());
                return false;
            }
            return true;
        }

        auto mt = mapDestTx.find(dest);
        if (mt != mapDestTx.end())
        {
            for (const uint256& txid : mt->second)
            {
                auto nt = mapTx.find(txid);
                if (nt != mapTx.end())
                {
                    const CPooledTx& tx = nt->second;
                    if (tx.to == dest)
                    {
                        bytes btTempData;
                        if (tx.GetTxData(CTransaction::DF_TEMPLATEDATA, btTempData))
                        {
                            ctxtAddress = CAddressContext(CTemplateAddressContext(std::string(), std::string(), btTempData), CBlock::GetBlockHeightByHash(hashLastBlock) + 1);
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

////////////////////////////////////
bool CForkTxPool::GetDestState(const CDestination& dest, CDestState& state)
{
    auto it = mapDestState.find(dest);
    if (it == mapDestState.end())
    {
        if (!pBlockChain->RetrieveDestState(hashFork, hashLastBlock, dest, state))
        {
            return false;
        }
    }
    else
    {
        state = it->second;
    }
    return true;
}

void CForkTxPool::SetDestState(const CDestination& dest, const CDestState& state)
{
    mapDestState[dest] = state;
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

bool CForkTxPool::AddSynchronizeTx(const uint256& hashBranchBlock, const uint256& txid, const CTransaction& tx, const int64 nTxSeq, std::set<CDestination>& setNewDest)
{
    auto isNewDest = [&](const CDestination& dest) -> bool
    {
        if (setNewDest.count(dest) > 0)
        {
            return true;
        }
        if (mapDestState.find(dest) == mapDestState.end())
        {
            setNewDest.insert(dest);
            return true;
        }
        return false;
    };

    auto getDestState = [&](const CDestination& dest) -> CDestState&
    {
        auto it = mapDestState.find(dest);
        if (it == mapDestState.end())
        {
            CDestState state;
            if (!pBlockChain->RetrieveDestState(hashFork, hashBranchBlock, dest, state))
            {
                state.SetNull();
            }
            it = mapDestState.insert(make_pair(dest, state)).first;
        }
        return it->second;
    };

    if (!AddPooledTx(txid, tx, nTxSeq))
    {
        return false;
    }

    if (isNewDest(tx.from))
    {
        CDestState& stateFrom = getDestState(tx.from);
        stateFrom.nTxNonce++;
        stateFrom.nBalance -= (tx.nAmount + tx.GetTxFee());
    }
    if (isNewDest(tx.to))
    {
        getDestState(tx.to).nBalance += tx.nAmount;
    }
    return true;
}

//////////////////////////////
// CTxPool

CTxPool::CTxPool()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
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
    return true;
}

void CTxPool::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
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

Errno CTxPool::Push(const CTransaction& tx)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
    uint256 txid = tx.GetHash();

    if (tx.IsRewardTx())
    {
        StdError("CTxPool", "Push: tx is mint, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CForkTxPool* pFork = GetForkTxPool(tx.hashFork);
    if (pFork == nullptr)
    {
        StdError("CTxPool", "Push: Get fork tx pool failed, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    return pFork->AddTx(txid, tx);
}

bool CTxPool::Get(const uint256& txid, CTransaction& tx) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    for (const auto& kv : mapForkPool)
    {
        if (kv.second.GetTx(txid, tx))
        {
            return true;
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

void CTxPool::AddDestDelegate(const CDestination& destDeleage)
{
}

int CTxPool::GetDestTxpoolTxCount(const CDestination& dest)
{
    return 0;
}

void CTxPool::GetDestBalance(const uint256& hashFork, const CDestination& dest, uint64& nTxNonce, uint256& nAvail, uint256& nUnconfirmedIn, uint256& nUnconfirmedOut)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(hashFork);
    if (pFork)
    {
        pFork->GetDestBalance(dest, nTxNonce, nAvail, nUnconfirmedIn, nUnconfirmedOut);
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

uint64 CTxPool::GetCertTxNextNonce(const CDestination& dest)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    CForkTxPool* pFork = GetForkTxPool(pCoreProtocol->GetGenesisBlockHash());
    if (pFork)
    {
        return pFork->GetCertTxNextNonce(dest);
    }
    return 0;
}

} // namespace metabasenet

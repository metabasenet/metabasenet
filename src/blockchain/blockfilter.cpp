// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockfilter.h"

#include "event.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

//////////////////////////////
// CBlockLogsFilter

bool CBlockLogsFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockLogsFilter::AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (logFilter.hashFromBlock != 0 && CBlock::GetBlockHeightByHash(hashBlock) < CBlock::GetBlockHeightByHash(logFilter.hashFromBlock))
    {
        return true;
    }
    if (logFilter.hashToBlock != 0 && CBlock::GetBlockHeightByHash(hashBlock) > CBlock::GetBlockHeightByHash(logFilter.hashToBlock))
    {
        return true;
    }

    MatchLogsVec vLogs;
    logFilter.matchesLogs(receipt, vLogs);
    if (!vLogs.empty())
    {
        CReceiptLogs r(vLogs);
        r.txid = receipt.txid;
        r.nTxIndex = receipt.nTxIndex;
        r.nBlockNumber = receipt.nBlockNumber;
        r.hashBlock = hashBlock;
        vReceiptLogs.push_back(r);

        nLogsCount += r.matchLogs.size();
        if (nLogsCount > MAX_FILTER_CACHE_COUNT * 2)
        {
            return false;
        }
    }
    return true;
}

void CBlockLogsFilter::GetTxReceiptLogs(const bool fAll, ReceiptLogsVec& receiptLogs)
{
    if (fAll)
    {
        for (auto& r : vHisReceiptLogs)
        {
            receiptLogs.push_back(r);
        }
        for (auto& r : vReceiptLogs)
        {
            receiptLogs.push_back(r);
        }
    }
    else
    {
        for (auto& r : vReceiptLogs)
        {
            receiptLogs.push_back(r);
            vHisReceiptLogs.push_back(r);
            nHisLogsCount += r.matchLogs.size();
        }

        vReceiptLogs.clear();
        nLogsCount = 0;
        nPrevGetChangesTime = GetTime();

        while (vHisReceiptLogs.size() > 0 && nHisLogsCount > MAX_FILTER_CACHE_COUNT * 2)
        {
            auto it = vHisReceiptLogs.begin();
            nHisLogsCount -= it->matchLogs.size();
            vHisReceiptLogs.erase(it);
        }
    }
}

//////////////////////////////
// CBlockMakerFilter

bool CBlockMakerFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockMakerFilter::AddBlockHash(const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (mapBlockHash.size() >= MAX_FILTER_CACHE_COUNT * 2)
    {
        return false;
    }
    mapBlockHash[hashBlock] = hashPrev;
    return true;
}

bool CBlockMakerFilter::GetFilterBlockHashs(const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockhash)
{
    if (fAll)
    {
        uint256 hash = hashLastBlock;
        while (true)
        {
            auto it = mapBlockHash.find(hash);
            if (it == mapBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            hash = it->second;
        }
        while (true)
        {
            auto it = mapHisBlockHash.find(hash);
            if (it == mapHisBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            hash = it->second;
        }
    }
    else
    {
        if (mapBlockHash.count(hashLastBlock) == 0)
        {
            return true;
        }
        uint256 hash = hashLastBlock;
        while (true)
        {
            auto it = mapBlockHash.find(hash);
            if (it == mapBlockHash.end())
            {
                break;
            }
            vBlockhash.push_back(hash);
            mapHisBlockHash.insert(*it);
            hash = it->second;
        }
        mapBlockHash.clear();
        nPrevGetChangesTime = GetTime();
        std::reverse(vBlockhash.begin(), vBlockhash.end());

        while (mapHisBlockHash.size() > MAX_FILTER_CACHE_COUNT * 2)
        {
            mapHisBlockHash.erase(mapHisBlockHash.begin());
        }
    }
    return true;
}

//////////////////////////////
// CBlockPendingTxFilter

bool CBlockPendingTxFilter::isTimeout()
{
    if (GetTime() - nPrevGetChangesTime >= FILTER_DEFAULT_TIMEOUT)
    {
        return true;
    }
    return false;
}

bool CBlockPendingTxFilter::AddPendingTx(const uint256& hashFork, const uint256& txid)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (setTxid.size() >= MAX_FILTER_CACHE_COUNT * 2)
    {
        return false;
    }
    if (setTxid.find(txid) == setTxid.end())
    {
        setTxid.insert(txid);
        if (setHisTxid.find(txid) != setHisTxid.end())
        {
            setHisTxid.erase(txid);
        }
    }
    return true;
}

bool CBlockPendingTxFilter::GetFilterTxids(const uint256& hashFork, const bool fAll, std::vector<uint256>& vTxid)
{
    if (hashFork != hashFork)
    {
        return true;
    }
    if (fAll)
    {
        for (auto& txid : setTxid)
        {
            vTxid.push_back(txid);
        }
        for (auto& txid : setHisTxid)
        {
            vTxid.push_back(txid);
        }
    }
    else
    {
        for (auto& txid : setTxid)
        {
            vTxid.push_back(txid);
            setHisTxid.insert(txid);
            if (++nSeqCreate == 0)
            {
                nSeqCreate++;
                mapHisSeq.clear();
                setHisTxid.clear();
                setHisTxid.insert(txid);
            }
            mapHisSeq.insert(std::make_pair(nSeqCreate, txid));
        }
        setTxid.clear();
        nPrevGetChangesTime = GetTime();

        while (setHisTxid.size() > MAX_FILTER_CACHE_COUNT * 2 && mapHisSeq.size() > 0)
        {
            auto it = mapHisSeq.begin();
            setHisTxid.erase(it->second);
            mapHisSeq.erase(it);
        }
    }
    return true;
}

//////////////////////////////
// CBlockFilter

CBlockFilter::CBlockFilter()
{
    pWsService = nullptr;
}

CBlockFilter::~CBlockFilter()
{
}

bool CBlockFilter::HandleInitialize()
{
    if (!GetObject("wsservice", pWsService))
    {
        Error("Failed to request wsservice");
        return false;
    }
    return true;
}

void CBlockFilter::HandleDeinitialize()
{
    pWsService = nullptr;
}

bool CBlockFilter::HandleInvoke()
{
    return true;
}

void CBlockFilter::HandleHalt()
{
}

void CBlockFilter::RemoveFilter(const uint256& nFilterId)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    if (CFilterId::isLogsFilter(nFilterId))
    {
        mapLogFilter.erase(nFilterId);
    }
    else if (CFilterId::isBlockFilter(nFilterId))
    {
        mapBlockFilter.erase(nFilterId);
    }
    else if (CFilterId::isTxFilter(nFilterId))
    {
        mapTxFilter.erase(nFilterId);
    }
}

uint256 CBlockFilter::AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn, const std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapHisBlockReceiptsIn)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateLogsFilterId(hashClient);
    while (mapLogFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateLogsFilterId(hashClient);
    }
    auto it = mapLogFilter.insert(make_pair(nFilterId, CBlockLogsFilter(hashFork, logFilterIn))).first;
    if (it == mapLogFilter.end())
    {
        return 0;
    }
    CBlockLogsFilter& filter = it->second;
    for (const auto& kv : mapHisBlockReceiptsIn)
    {
        const uint256& hashBlock = kv.first;
        for (const auto& receipt : kv.second)
        {
            filter.AddTxReceipt(hashFork, hashBlock, receipt);
        }
    }
    return nFilterId;
}

void CBlockFilter::AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapLogFilter.begin(); it != mapLogFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapLogFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddTxReceipt(hashFork, hashBlock, receipt))
                {
                    mapLogFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushLogs* pPushEvent = new CEventWsServicePushLogs(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.hashBlock = hashBlock;
            pPushEvent->data.receipt = receipt;

            pWsService->PostEvent(pPushEvent);
        }
    }
}

bool CBlockFilter::GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapLogFilter.find(nFilterId);
    if (it == mapLogFilter.end())
    {
        return false;
    }
    it->second.GetTxReceiptLogs(fAll, receiptLogs);
    return true;
}

uint256 CBlockFilter::AddBlockFilter(const uint256& hashClient, const uint256& hashFork)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateBlockFilterId(hashClient);
    while (mapBlockFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateBlockFilterId(hashClient);
    }
    mapBlockFilter.insert(make_pair(nFilterId, CBlockMakerFilter(hashFork)));
    return nFilterId;
}

void CBlockFilter::AddNewBlockInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapBlockFilter.begin(); it != mapBlockFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapBlockFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddBlockHash(hashFork, hashBlock, block.hashPrev))
                {
                    mapBlockFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushNewBlock* pPushEvent = new CEventWsServicePushNewBlock(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.hashBlock = hashBlock;
            pPushEvent->data.block = block;

            pWsService->PostEvent(pPushEvent);
        }
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);

        CForkBlockStat& stat = mapForkBlockStat[hashFork];

        if (stat.nStartBlockNumber == 0)
        {
            stat.nStartBlockNumber = block.GetBlockNumber();
        }
        stat.nCurrBlockNumber = block.GetBlockNumber();
        if (stat.nMaxPeerBlockNumber < block.GetBlockNumber())
        {
            stat.nMaxPeerBlockNumber = block.GetBlockNumber();
        }

        if (GetTime() > stat.nPrevSendStatTime)
        {
            stat.nPrevSendStatTime = GetTime();

            CEventWsServicePushSyncing* pPushEvent = new CEventWsServicePushSyncing(0);
            if (pPushEvent != nullptr)
            {
                pPushEvent->data.hashFork = hashFork;
                pPushEvent->data.fSyncing = true;
                pPushEvent->data.nStartingBlockNumber = stat.nStartBlockNumber;
                pPushEvent->data.nCurrentBlockNumber = stat.nCurrBlockNumber;
                pPushEvent->data.nHighestBlockNumber = stat.nMaxPeerBlockNumber;
                pPushEvent->data.nPulledStates = 0;
                pPushEvent->data.nKnownStates = 0;

                pWsService->PostEvent(pPushEvent);
            }
        }
    }
}

bool CBlockFilter::GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapBlockFilter.find(nFilterId);
    if (it == mapBlockFilter.end())
    {
        return false;
    }
    it->second.GetFilterBlockHashs(hashLastBlock, fAll, vBlockHash);
    return true;
}

uint256 CBlockFilter::AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork)
{
    boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
    uint256 nFilterId = createFilterId.CreateTxFilterId(hashClient);
    while (mapTxFilter.count(nFilterId) > 0)
    {
        nFilterId = createFilterId.CreateTxFilterId(hashClient);
    }
    mapTxFilter.insert(make_pair(nFilterId, CBlockPendingTxFilter(hashFork)));
    return nFilterId;
}

void CBlockFilter::AddPendingTx(const uint256& hashFork, const uint256& txid)
{
    {
        boost::unique_lock<boost::shared_mutex> lock(mutexFilter);
        for (auto it = mapTxFilter.begin(); it != mapTxFilter.end();)
        {
            if (it->second.isTimeout())
            {
                mapTxFilter.erase(it++);
            }
            else
            {
                if (!it->second.AddPendingTx(hashFork, txid))
                {
                    mapTxFilter.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    {
        CEventWsServicePushNewPendingTx* pPushEvent = new CEventWsServicePushNewPendingTx(0);
        if (pPushEvent != nullptr)
        {
            pPushEvent->data.hashFork = hashFork;
            pPushEvent->data.txid = txid;

            pWsService->PostEvent(pPushEvent);
        }
    }
}

bool CBlockFilter::GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);
    auto it = mapTxFilter.find(nFilterId);
    if (it == mapTxFilter.end())
    {
        return false;
    }
    it->second.GetFilterTxids(hashFork, fAll, vTxid);
    return true;
}

void CBlockFilter::AddMaxPeerBlockNumber(const uint256& hashFork, const uint64 nMaxPeerBlockNumber)
{
    boost::shared_lock<boost::shared_mutex> lock(mutexFilter);

    CForkBlockStat& stat = mapForkBlockStat[hashFork];
    if (stat.nMaxPeerBlockNumber < nMaxPeerBlockNumber)
    {
        stat.nMaxPeerBlockNumber = nMaxPeerBlockNumber;
    }
}

} // namespace metabasenet

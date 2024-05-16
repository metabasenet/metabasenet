// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_BLOCKFILTER_H
#define METABASENET_BLOCKFILTER_H

#include "base.h"
#include "block.h"
#include "mtbase.h"

namespace metabasenet
{

//////////////////////////////
// CBlockLogsFilter

class CBlockLogsFilter
{
public:
    CBlockLogsFilter(const uint256& hashForkIn, const CLogsFilter& logFilterIn)
      : hashFork(hashForkIn), logFilter(logFilterIn), nLogsCount(0), nHisLogsCount(0), nPrevGetChangesTime(mtbase::GetTime()) {}

    bool isTimeout();

    bool AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt);
    void GetTxReceiptLogs(const bool fAll, ReceiptLogsVec& receiptLogs);

protected:
    const uint256 hashFork;
    const CLogsFilter logFilter;

    int64 nPrevGetChangesTime;
    uint64 nLogsCount;
    uint64 nHisLogsCount;

    ReceiptLogsVec vReceiptLogs;
    ReceiptLogsVec vHisReceiptLogs;
};

//////////////////////////////
// CBlockMakerFilter

class CBlockMakerFilter
{
public:
    CBlockMakerFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(mtbase::GetTime()) {}

    bool isTimeout();
    bool AddBlockHash(const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev);
    bool GetFilterBlockHashs(const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockhash);

protected:
    const uint256 hashFork;
    int64 nPrevGetChangesTime;

    std::map<uint256, uint256> mapBlockHash;
    std::map<uint256, uint256, CustomBlockHashCompare> mapHisBlockHash;
};

//////////////////////////////
// CBlockPendingTxFilter

class CBlockPendingTxFilter
{
public:
    CBlockPendingTxFilter(const uint256& hashForkIn)
      : hashFork(hashForkIn), nPrevGetChangesTime(mtbase::GetTime()), nSeqCreate(0) {}

    bool isTimeout();
    bool AddPendingTx(const uint256& hashFork, const uint256& txid);
    bool GetFilterTxids(const uint256& hashFork, const bool fAll, std::vector<uint256>& vTxid);

protected:
    const uint256 hashFork;
    int64 nPrevGetChangesTime;
    uint64 nSeqCreate;

    std::set<uint256> setTxid;
    std::set<uint256> setHisTxid;
    std::map<uint64, uint256> mapHisSeq;
};

//////////////////////////////
// CForkBlockStat

class CForkBlockStat
{
public:
    CForkBlockStat()
      : nPrevSendStatTime(0), nStartBlockNumber(0), nCurrBlockNumber(0), nMaxPeerBlockNumber(0) {}

public:
    int64 nPrevSendStatTime;

    uint64 nStartBlockNumber;
    uint64 nCurrBlockNumber;
    uint64 nMaxPeerBlockNumber;
};

/////////////////////////////
// CBlockFilter

class CBlockFilter : public IBlockFilter
{
public:
    CBlockFilter();
    ~CBlockFilter();

    void RemoveFilter(const uint256& nFilterId) override;

    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logFilterIn, const std::map<uint256, std::vector<CTransactionReceipt>, CustomBlockHashCompare>& mapHisBlockReceiptsIn) override;
    void AddTxReceipt(const uint256& hashFork, const uint256& hashBlock, const CTransactionReceipt& receipt) override;
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs) override;

    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork) override;
    void AddNewBlockInfo(const uint256& hashFork, const uint256& hashBlock, const CBlock& block) override;
    bool GetFilterBlockHashs(const uint256& nFilterId, const uint256& hashLastBlock, const bool fAll, std::vector<uint256>& vBlockHash) override;

    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork) override;
    void AddPendingTx(const uint256& hashFork, const uint256& txid) override;
    bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid) override;

    void AddMaxPeerBlockNumber(const uint256& hashFork, const uint64 nMaxPeerBlockNumber) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

protected:
    IWsService* pWsService;

    boost::shared_mutex mutexFilter;
    CFilterId createFilterId;

    std::map<uint256, CBlockLogsFilter> mapLogFilter;     // key: filter id
    std::map<uint256, CBlockMakerFilter> mapBlockFilter;  // key: filter id
    std::map<uint256, CBlockPendingTxFilter> mapTxFilter; // key: filter id

    std::map<uint256, CForkBlockStat> mapForkBlockStat; //key: fork hash
};

} // namespace metabasenet

#endif // METABASENET_BLOCKFILTER_H

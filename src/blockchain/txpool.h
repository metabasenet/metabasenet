// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_TXPOOL_H
#define METABASENET_TXPOOL_H

#include "base.h"
#include "block.h"
#include "peernet.h"
#include "txpooldata.h"
#include "util.h"

namespace metabasenet
{

#define INIT_TX_SEQUENCE_NUMBER 0x1000000
#define MAX_CACHE_MISSING_PREV_TX_COUNT 0x10000

class CPooledTx : public CTransaction
{
public:
    CPooledTx()
      : nSequenceNumber(0), nSerializeSize(0) {}
    CPooledTx(const CTransaction& txIn, const uint64 nSeqIn)
      : CTransaction(txIn), nSequenceNumber(nSeqIn)
    {
        nSerializeSize = mtbase::GetSerializeSize(txIn);
    }

public:
    uint64 nSequenceNumber;
    uint64 nSerializeSize;
};
typedef std::shared_ptr<CPooledTx> CPooledTxPtr;

class CAddressTxState
{
public:
    CAddressTxState() {}

    void AddAddressTx(const uint256& txid, const CPooledTxPtr& ptx);
    void RemoveAddressTx(const uint256& txid);
    bool IsEmptyAddressTx() const;
    CDestState& GetAddressState();
    void SetAddressState(const CDestState& state);
    void AddMissTx(const uint256& txid, const CTransaction& tx);
    void RemoveMissTx(const uint256& txid);
    bool FetchNextMissTx(uint256& txid, CTransaction& tx);

public:
    std::map<uint256, CPooledTxPtr> mapDestTx;
    CDestState stateAddress;
    CAddressContext ctxAddress;
    std::map<uint256, CTransaction> mapMissTx;
    std::map<uint64, uint256> mapMissNonce;
};

class CPooledTxLink
{
public:
    CPooledTxLink()
      : nType(0), nSequenceNumber(0), ptx(nullptr) {}
    CPooledTxLink(const CPooledTxPtr ptxin)
      : ptx(ptxin)
    {
        txid = ptx->GetHash();
        nSequenceNumber = ptx->nSequenceNumber;
        nType = ptx->GetTxType();
    }

public:
    uint256 txid;
    uint16 nType;
    uint64 nSequenceNumber;
    CPooledTxPtr ptx;
};

typedef boost::multi_index_container<CPooledTxLink,
                                     boost::multi_index::indexed_by<
                                         // sorted by Tx ID
                                         boost::multi_index::ordered_unique<boost::multi_index::member<CPooledTxLink, uint256, &CPooledTxLink::txid>>,
                                         // sorted by entry sequence
                                         boost::multi_index::ordered_non_unique<boost::multi_index::member<CPooledTxLink, uint64, &CPooledTxLink::nSequenceNumber>>,
                                         // sorted by Tx Type
                                         boost::multi_index::ordered_non_unique<boost::multi_index::member<CPooledTxLink, uint16, &CPooledTxLink::nType>>>>
    CPooledTxLinkSet;

typedef CPooledTxLinkSet::nth_index<0>::type CPooledTxLinkSetByTxHash;
typedef CPooledTxLinkSet::nth_index<1>::type CPooledTxLinkSetBySequenceNumber;
typedef CPooledTxLinkSet::nth_index<2>::type CPooledTxLinkSetByTxType;

class CForkTxPool
{
public:
    CForkTxPool(ICoreProtocol* pCoreProtocolIn, IBlockChain* pBlockChainIn,
                const uint256& hashForkIn, const uint256& hashLastBlockIn, const int64 nBlockTimeIn)
      : nTxSequenceNumber(INIT_TX_SEQUENCE_NUMBER), pCoreProtocol(pCoreProtocolIn), pBlockChain(pBlockChainIn),
        hashFork(hashForkIn), hashLastBlock(hashLastBlockIn), nLastBlockTime(nBlockTimeIn) {}

    bool GetSaveTxList(std::vector<std::pair<uint256, CTransaction>>& vTx);
    void ClearTxPool();

    bool AddPooledTx(const uint256& txid, const CTransaction& tx, const int64 nTxSeq);
    bool RemovePooledTx(const uint256& txid, const CTransaction& tx, const bool fInvalidTx);

    Errno AddTx(const uint256& txid, const CTransaction& tx);

    bool Exists(const uint256& txid);
    bool CheckTxNonce(const CDestination& destFrom, const uint64 nTxNonce);
    std::size_t GetTxCount() const;
    bool GetTx(const uint256& txid, CTransaction& tx) const;
    uint64 GetDestNextTxNonce(const CDestination& dest);
    bool GetAddressContext(const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashRefBlock = uint256());

    bool FetchArrangeBlockTx(const uint256& hashPrev, const int64 nBlockTime,
                             const size_t nMaxSize, vector<CTransaction>& vtx, uint256& nTotalTxFee);
    bool SynchronizeBlockChain(const CBlockChainUpdate& update);

    void ListTx(std::vector<std::pair<uint256, std::size_t>>& vTxPool);
    void ListTx(std::vector<uint256>& vTxPool);
    bool ListTx(const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset, const int64 nGetCount);
    void GetDestBalance(const CDestination& dest, uint8& nDestType, uint8& nTemplateType, uint64& nTxNonce, uint256& nAvail,
                        uint256& nUnconfirmedIn, uint256& nUnconfirmedOut, CAddressContext& ctxAddress, const uint256& hashBlock = uint256());

protected:
    bool GetDestState(const CDestination& dest, CDestState& state, const uint256& hashBlock = uint256());
    void SetDestState(const CDestination& dest, const CDestState& state);
    int64 GetMinTxSequenceNumber();
    bool VerifyRepeatCertTx(const CTransaction& tx);
    void RemoveObsoletedCertTx();

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    uint256 hashFork;

    int64 nTxSequenceNumber;
    CPooledTxLinkSet setTxLinkIndex;
    std::map<uint256, CPooledTxPtr> mapTx;
    std::map<CDestination, CAddressTxState> mapAddressTxState;

    uint256 hashLastBlock;
    int64 nLastBlockTime;
};

class CTxPool : public ITxPool
{
public:
    CTxPool();
    ~CTxPool();
    void ClearTxPool(const uint256& hashFork) override;
    bool Exists(const uint256& hashFork, const uint256& txid) override;
    bool CheckTxNonce(const uint256& hashFork, const CDestination& destFrom, const uint64 nTxNonce) override;
    std::size_t Count(const uint256& hashFork) const override;
    Errno Push(const uint256& hashFork, const CTransaction& tx) override;
    bool Get(const uint256& hashFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork) const override;
    void ListTx(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) override;
    void ListTx(const uint256& hashFork, std::vector<uint256>& vTxPool) override;
    bool ListTx(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) override;
    bool FetchArrangeBlockTx(const uint256& hashFork, const uint256& hashPrev, const int64 nBlockTime,
                             const std::size_t nMaxSize, std::vector<CTransaction>& vtx, uint256& nTotalTxFee) override;
    bool SynchronizeBlockChain(const CBlockChainUpdate& update) override;
    void GetDestBalance(const uint256& hashFork, const CDestination& dest, uint8& nDestType, uint8& nTemplateType, uint64& nTxNonce, uint256& nAvail,
                        uint256& nUnconfirmedIn, uint256& nUnconfirmedOut, CAddressContext& ctxAddress, const uint256& hashBlock = uint256()) override;
    uint64 GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest) override;
    bool GetAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashRefBlock = uint256()) override;
    bool PushCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx) override;
    bool PushUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool LoadData();
    bool SaveData();
    CForkTxPool* GetForkTxPool(const uint256& hashFork);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    IDataStat* pDataStat;
    network::ICertTxChannel* pCertTxChannel;
    network::IUserTxChannel* pUserTxChannel;

    storage::CTxPoolData datTxPool;

    mutable boost::shared_mutex rwAccess;
    std::map<uint256, CForkTxPool> mapForkPool;
};

} // namespace metabasenet

#endif //METABASENET_TXPOOL_H

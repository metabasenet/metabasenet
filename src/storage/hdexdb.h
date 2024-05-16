// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_HDEXDB_H
#define STORAGE_HDEXDB_H

#include <map>

#include "block.h"
#include "destination.h"
#include "mtbase.h"
#include "matchdex.h"
#include "timeseries.h"
#include "transaction.h"
#include "triedb.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

///////////////////////////////////
// CCacheBlockDexOrder

class CCacheBlockDexOrder
{
public:
    CCacheBlockDexOrder()
    {
        ptrMatchDex = MAKE_SHARED_MATCH_DEX();
    }

    void Clear()
    {
        hashLastBlock = 0;
    }

    void SetLastBlock(const uint256& hashBlock)
    {
        hashLastBlock = hashBlock;
    }
    const uint256& GetLastBlock() const
    {
        return hashLastBlock;
    }
    const SHP_MATCH_DEX GetMatchDex() const
    {
        return ptrMatchDex;
    }

    bool AddDexOrderCache(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder, const CChainId nOrderAtChainId,
                          const uint256& hashOrderAtBlock, const uint256& nCompAmount, const uint64 nCompCount, const uint256& nPrevCompletePrice);
    bool UpdateCompleteOrderCache(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount);
    void UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock);
    void UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice);

    bool GetMatchDexResult(std::map<uint256, CMatchOrderResult>& mapMatchResult);
    bool ListMatchDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder);

    friend bool operator==(const CCacheBlockDexOrder& a, const CCacheBlockDexOrder& b)
    {
        if (a.hashLastBlock != b.hashLastBlock)
        {
            return false;
        }
        if (*a.ptrMatchDex != *b.ptrMatchDex)
        {
            return false;
        }
        return true;
    }
    friend inline bool operator!=(const CCacheBlockDexOrder& a, const CCacheBlockDexOrder& b)
    {
        return (!(a == b));
    }

protected:
    uint256 hashLastBlock;

    SHP_MATCH_DEX ptrMatchDex;
};
typedef std::shared_ptr<CCacheBlockDexOrder> SHP_CACHE_BLOCK_DEX_ORDER;
#define MAKE_SHARED_CACHE_BLOCK_DEX_ORDER std::make_shared<CCacheBlockDexOrder>

///////////////////////////////////
// CHdexDB

class CHdexDB
{
public:
    CHdexDB() {}

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();

    bool AddDexOrder(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, CDexOrderBody>& mapDexOrder, const std::map<CChainId, std::vector<CBlockCoinTransferProve>>& mapCrossTransferProve,
                     const std::map<uint256, uint256>& mapCoinPairCompletePrice, const std::set<CChainId>& setPeerCrossChainId, const std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>>& mapCompDexOrderRecord, const std::map<CChainId, CBlockProve>& mapBlockProve, uint256& hashNewRoot);
    bool GetDexOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, CDexOrderBody& dexOrder);
    bool GetDexCompletePrice(const uint256& hashBlock, const uint256& hashCoinPair, uint256& nCompletePrice);
    bool GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount);
    bool GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount);
    bool ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                             const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder);
    bool GetDexOrderMaxNumber(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, uint64& nMaxOrderNumber);
    bool GetPeerCrossLastBlock(const uint256& hashBlock, const CChainId nPeerChainId, uint256& hashLastProveBlock);
    const SHP_MATCH_DEX GetBlockMatchDex(const uint256& hashBlock);
    bool GetMatchDexData(const uint256& hashBlock, std::map<uint256, CMatchOrderResult>& mapMatchResult);
    bool ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder);

    bool AddBlockCrosschainProve(const uint256& hashBlock, const CBlockStorageProve& proveBlockCrosschain);
    bool UpdateBlockAggSign(const uint256& hashBlock, const bytes& btAggBitmap, const bytes& btAggSig, std::map<CChainId, CBlockProve>& mapBlockProve);
    bool GetBlockCrosschainProve(const uint256& hashBlock, CBlockStorageProve& proveBlockCrosschain);
    bool GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve);

    bool AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve);
    bool GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve);

    bool VerifyDexOrder(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode = true);

protected:
    bool WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot);
    bool ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot);
    void AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv);
    bool GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock);

    bool GetDexOrderDb(const uint256& hashRoot, const CChainId nChainId, const CDestination& destOrder, const uint256& hashCoinPair, const uint8 nOwnerCoinFlag, const uint64 nOrderNumber, CDexOrderSave& dexOrder);
    bool GetDexCompletePriceDb(const uint256& hashRoot, const uint256& hashCoinPair, uint256& nCompletePrice);
    bool GetDexOrderCompleteDb(const uint256& hashRoot, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount);
    bool GetDexOrderMaxNumberDb(const uint256& hashRoot, const CChainId nChainId, const CDestination& destOrder, const uint256& hashCoinPair, const uint8 nOwnerCoinFlag, uint64& nMaxOrderNumber);
    bool GetPeerCrossLastBlockDb(const uint256& hashRoot, const CChainId nPeerChainId, uint256& hashLastProveBlock);
    bool GetPeerChainSendPrevBlockDb(const uint256& hashRoot, const CChainId nSendChainId, uint256& hashLastProveBlock);
    bool ListPeerChainSendLastProveBlockDb(const uint256& hashBlock, std::map<CChainId, uint256>& mapSendLastProveBlock);
    bool WriteBlockCrosschainProveDb(const uint256& hashBlock, const CBlockStorageProve& proveBlockCrosschain);
    bool GetBlockCrosschainProveDb(const uint256& hashBlock, CBlockStorageProve& proveBlockCrosschain);

    bool GetCoinPairCompletePriceCache(const uint256& hashLastBlock, std::map<uint256, uint256>& mapCompPriceCache);
    bool GetCompDexOrderCache(const uint256& hashLastBlock, std::map<uint256, std::pair<uint256, uint64>>& mapCompCache);
    bool GetRecvConfirmBlockListCache(const uint256& hashLastBlock, std::map<CChainId, std::pair<uint256, uint256>>& mapRecvConfirmBlock);
    bool LoadLastDexOrderCache(const uint256& hashLastBlock, const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache, const std::map<uint256, uint256>& mapCompPriceCache, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder);
    void AddBlockDexOrderCache(const uint256& hashBlock, const SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder);
    void RemoveBlockDexOrderCache(const uint256& hashBlock);
    bool LoadLastBlockProveCache(const uint256& hashLastBlock, const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache, const std::map<uint256, uint256>& mapCompPriceCache, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder);
    SHP_CACHE_BLOCK_DEX_ORDER LoadBlockDexOrderCache(const uint256& hashBlock);

    bool AddLinkFirstPrevBlock(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashBlock, const uint256& hashFirstPrevBlock);
    bool GetLinkFirstPrevBlock(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashBlock, uint256& hashFirstPrevBlock);

    bool UpdateDexOrderCache(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64>>& mapAddNewOrder,
                             const std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64, CChainId, uint256>>& mapAddBlockProveOrder,
                             const std::map<uint256, uint256>& mapCoinPairCompletePrice,
                             const std::map<uint256, std::tuple<uint256, uint256, uint64>>& mapUpdateCompOrder,
                             const std::map<CChainId, uint256>& mapPeerProveLastBlock);
    bool GetBlockProveDb(const uint256& hashBlock, const CBlockStorageProve& proveCrosschain, std::map<CChainId, CBlockProve>& mapBlockProve);

    bool AddSendChainProveLastBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& nLastProveBlock);
    bool GetSendChainProveLastBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, uint256& nLastProveBlock);
    bool ListSendChainProveLastBlockDb(const CChainId nRecvChainId, std::map<CChainId, uint256>& mapSendLastBlock);

    bool AddRecvCrosschainProveDb(const CChainId nRecvChainId, const CBlockProve& blockProve);
    bool AddRecvCrosschainProveDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashFirstPrevBlock, const CBlockProve& blockProve);
    bool GetRecvCrosschainProveDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashFirstPrevBlock, CBlockProve& blockProve);
    bool ListRecvCrosschainProveDb(const CChainId nRecvChainId, std::vector<std::tuple<CChainId, uint256, CBlockProve>>& vRecvCrossProve);
    bool GetCrosschainProveForPrevBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashLastProveBlock, const uint32 nLastHeight, const uint16 nLastSlot, CBlockProve& blockProve);

protected:
    enum
    {
        MAX_CACHE_BLOCK_DEX_ORDER_COUNT = 32
    };

    mtbase::CRWAccess rwAccess;
    CTrieDB dbTrie;

    std::map<uint32, SHP_CACHE_BLOCK_DEX_ORDER> mapCacheForkLastDexOrder;                       // key: chainid
    std::map<uint256, SHP_CACHE_BLOCK_DEX_ORDER, CustomBlockHashCompare> mapCacheBlockDexOrder; // key: block hash
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_HDEXDB_H

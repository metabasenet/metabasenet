// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "hdexdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "block.h"
#include "param.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

#define HDEX_OUT_TEST_LOG
#define HDEX_CACHE_DEX_ENABLE

#define MAX_FETCH_DEX_ORDER_COUNT 1000

const string DB_HDEX_KEY_ID_TRIEROOT("trieroot");
const string DB_HDEX_KEY_ID_PREVROOT("prevroot");

const uint8 DB_HDEX_ROOT_TYPE_TRIE = 0x10;
const uint8 DB_HDEX_ROOT_TYPE_EXT = 0x20;

const uint8 DB_HDEX_KEY_TYPE_TRIE_HEIGHT_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x01;
const uint8 DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER = DB_HDEX_ROOT_TYPE_TRIE | 0x02;
const uint8 DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER = DB_HDEX_ROOT_TYPE_TRIE | 0x03;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_SEND = DB_HDEX_ROOT_TYPE_TRIE | 0x04;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_RECV = DB_HDEX_ROOT_TYPE_TRIE | 0x05;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_MAX_NUMBER = DB_HDEX_ROOT_TYPE_TRIE | 0x06;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT = DB_HDEX_ROOT_TYPE_TRIE | 0x07;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_RECORD = DB_HDEX_ROOT_TYPE_TRIE | 0x08;
const uint8 DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE = DB_HDEX_ROOT_TYPE_TRIE | 0x09;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0A;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_SEND_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0B;
const uint8 DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK = DB_HDEX_ROOT_TYPE_TRIE | 0x0C;

const uint8 DB_HDEX_KEY_TYPE_EXT_BLOCK_CROSSCHAIN_PROVE = DB_HDEX_ROOT_TYPE_EXT | 0x01;
const uint8 DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE = DB_HDEX_ROOT_TYPE_EXT | 0x02;
const uint8 DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK = DB_HDEX_ROOT_TYPE_EXT | 0x03;
const uint8 DB_HDEX_KEY_TYPE_EXT_BLOCK_FOR_FIRST_PREV_BLOCK = DB_HDEX_ROOT_TYPE_EXT | 0x04;

///////////////////////////////////
// CCacheBlockDexOrder

bool CCacheBlockDexOrder::AddDexOrderCache(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder, const CChainId nOrderAtChainId,
                                           const uint256& hashOrderAtBlock, const uint256& nCompAmount, const uint64 nCompCount, const uint256& nPrevCompletePrice)
{
    return ptrMatchDex->AddMatchDexOrder(hashDexOrder, destOrder, nOrderNumber, dexOrder, nOrderAtChainId, hashOrderAtBlock, nCompAmount, nCompCount, nPrevCompletePrice);
}

bool CCacheBlockDexOrder::UpdateCompleteOrderCache(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount)
{
    return ptrMatchDex->UpdateMatchCompleteOrder(hashCoinPair, hashDexOrder, nCompleteAmount, nCompleteCount);
}

void CCacheBlockDexOrder::UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock)
{
    ptrMatchDex->UpdatePeerProveLastBlock(nPeerChainId, hashLastProveBlock);
}

void CCacheBlockDexOrder::UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice)
{
    ptrMatchDex->UpdateCompletePrice(hashCoinPair, nCompletePrice);
}

bool CCacheBlockDexOrder::GetMatchDexResult(std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    return ptrMatchDex->MatchDex(mapMatchResult);
}

bool CCacheBlockDexOrder::ListMatchDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    return ptrMatchDex->ListDexOrder(strCoinSymbolSell, strCoinSymbolBuy, nGetCount, realDexOrder);
}

////////////////////////////////////////////
// CHdexDB

bool CHdexDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData / "hcc"))
    {
        return false;
    }
    return true;
}

void CHdexDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

void CHdexDB::Clear()
{
    dbTrie.Clear();
}

bool CHdexDB::AddDexOrder(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, CDexOrderBody>& mapDexOrder, const std::map<CChainId, std::vector<CBlockCoinTransferProve>>& mapCrossTransferProve,
                          const std::map<uint256, uint256>& mapCoinPairCompletePrice, const std::set<CChainId>& setPeerCrossChainId, const std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>>& mapCompDexOrderRecord, const std::map<CChainId, CBlockProve>& mapBlockProve, uint256& hashNewRoot)
{
    CWriteLock wlock(rwAccess);

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CHdexDB", "Add dex order: Read trie root fail, prev block: %s, block: %s",
                   hashPrevBlock.GetHex().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
    }
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    const uint32 nHeight = CBlock::GetBlockHeightByHash(hashBlock);
    const uint16 nSlot = CBlock::GetBlockSlotByHash(hashBlock);

    std::map<std::tuple<CDestination, uint256, uint8>, uint64> mapMaxOrderNumber;                                   // key: address, coin pair hash, owner coin flag, value: max order number
    std::map<CDexOrderHeader, std::pair<uint256, uint64>> mapCompOrderAmount;                                       // key: order header, value: complete amount, complete count
    std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64>> mapAddNewOrder;                           // key: order header, value: 1: order body, 2: complete amount, 3: complete count
    std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64, CChainId, uint256>> mapAddBlockProveOrder; // key: order header, value: 1: order body, 2: complete amount, 3: complete count, 4: at chainid, 5: at block
    std::map<uint256, std::tuple<uint256, uint256, uint64>> mapUpdateCompOrder;                                     // key: dex order hash, value: 1: coin pair hash, 2: complete amount, 3: complete count
    std::map<CChainId, uint256> mapPeerProveLastBlock;                                                              // key: peer chain id, value: last block

    for (const auto& kv : mapCompDexOrderRecord)
    {
        const CDexOrderHeader& orderHeader = kv.first;
        const std::vector<CCompDexOrderRecord>& vDexOrderRecord = kv.second;
        uint256 hashDexOrder = orderHeader.GetDexOrderHash();

        uint256 nCompOrderAmount;
        uint64 nCompleteOrderCount = 0;
        GetDexOrderCompleteDb(hashPrevRoot, hashDexOrder, nCompOrderAmount, nCompleteOrderCount);

        for (const CCompDexOrderRecord& orderRecord : vDexOrderRecord)
        {
            nCompOrderAmount += orderRecord.nCompleteAmount;
        }
        nCompleteOrderCount += vDexOrderRecord.size();

        mapCompOrderAmount.insert(std::make_pair(orderHeader, std::make_pair(nCompOrderAmount, nCompleteOrderCount)));
    }

    bytesmap mapKv;
    for (const auto& kv : mapDexOrder)
    {
        const CDexOrderHeader& orderHeader = kv.first;
        const CDexOrderBody& orderBody = kv.second;

        const CDestination& destOrder = orderHeader.GetOrderAddress();
        const uint256& hashCoinPair = orderHeader.GetCoinPairHash();
        const uint8 nOwnerCoinFlag = orderHeader.GetOwnerCoinFlag();
        const uint64 nOrderNumber = orderHeader.GetOrderNumber();
        const uint256 hashDexOrder = orderHeader.GetDexOrderHash();

        uint256 nCompleteAmount;
        uint64 nCompleteOrderCount = 0;

        auto it = mapCompOrderAmount.find(orderHeader);
        if (it != mapCompOrderAmount.end())
        {
            nCompleteAmount = it->second.first;
            nCompleteOrderCount = it->second.second;
        }
        else
        {
            if (!GetDexOrderCompleteDb(hashPrevRoot, hashDexOrder, nCompleteAmount, nCompleteOrderCount))
            {
                nCompleteAmount = 0;
                nCompleteOrderCount = 0;
            }
        }

        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag << BSwap64(nOrderNumber);
            ssKey.GetData(btKey);

            ssValue << CDexOrderSave(orderBody, nChainId, hashBlock);
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }
#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Dex order: chainid: %d, order address: %s, coin pair: %s, symble owner: %s, symble peer: %s, order number: %ld, order amount: %s, order price: %s, at block: %s",
                 nChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), orderBody.strCoinSymbolOwner.c_str(), orderBody.strCoinSymbolPeer.c_str(), nOrderNumber,
                 CoinToTokenBigFloat(orderBody.nOrderAmount).c_str(), CoinToTokenBigFloat(orderBody.nOrderPrice).c_str(), hashBlock.GetBhString().c_str());
#endif
        mapAddNewOrder[orderHeader] = std::make_tuple(orderBody, nCompleteAmount, nCompleteOrderCount);

        // Update max order number
        {
            auto it = mapMaxOrderNumber.find(std::make_tuple(destOrder, hashCoinPair, nOwnerCoinFlag));
            if (it == mapMaxOrderNumber.end())
            {
                uint64 nMaxOrderNumber;
                if (!GetDexOrderMaxNumberDb(hashPrevRoot, nChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nMaxOrderNumber))
                {
                    nMaxOrderNumber = 0;
                }
                if (nMaxOrderNumber < nOrderNumber)
                {
                    mapMaxOrderNumber.insert(std::make_pair(std::make_tuple(destOrder, hashCoinPair, nOwnerCoinFlag), nOrderNumber));
                }
            }
            else
            {
                if (it->second < nOrderNumber)
                {
                    it->second = nOrderNumber;
                }
            }
        }
    }

    for (const auto& kv : mapCrossTransferProve)
    {
        const CChainId nPeerChainId = kv.first;
        uint32 nIndex = 0;
        for (const CBlockCoinTransferProve& transProve : kv.second)
        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_SEND << BSwap32(nPeerChainId) << transProve.destTransfer << hashBlock << BSwap32(nIndex);
            nIndex++;
            ssKey.GetData(btKey);

            ssValue << transProve;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: cross transfer send: local chainid: %d, peer chainid: %d, address: %s, coin symbol: %s, ori chainid: %d, dest chainid: %d, amount: %s, at block: %s",
                     nChainId, nPeerChainId, transProve.destTransfer.ToString().c_str(), transProve.strCoinSymbol.c_str(), transProve.nOriChainId,
                     transProve.nDestChainId, CoinToTokenBigFloat(transProve.nTransferAmount).c_str(), hashBlock.GetBhString().c_str());
#endif
        }
    }

    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_HEIGHT_BLOCK << BSwap32(nHeight) << BSwap16(nSlot);
        ssKey.GetData(btKey);

        ssValue << hashBlock << hashRefBlock << hashPrevBlock;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }

    for (const auto& kv : mapCompOrderAmount)
    {
        const CDexOrderHeader& orderHeader = kv.first;

        const CChainId nAtChainId = orderHeader.GetChainId();
        const CDestination& destOrder = orderHeader.GetOrderAddress();
        const uint256& hashCoinPair = orderHeader.GetCoinPairHash();
        const uint8 nOwnerCoinFlag = orderHeader.GetOwnerCoinFlag();
        const uint64 nOrderNumber = orderHeader.GetOrderNumber();
        const uint256& nCompOrderAmount = kv.second.first;
        const uint64 nCompleteOrderCount = kv.second.second;

        uint256 hashDexOrder = orderHeader.GetDexOrderHash();

        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT << hashDexOrder;
        ssKey.GetData(btKey);

        ssValue << nCompOrderAmount << nCompleteOrderCount;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));

        mapUpdateCompOrder.insert(std::make_pair(hashDexOrder, std::make_tuple(hashCoinPair, nCompOrderAmount, nCompleteOrderCount)));

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Complete order amount: at chainid: %d, order address: %s, coin pair: %s, owner coin flag: %d, order number: %lu, complete amount: %s, complete count: %lu, dex order hash: %s, at block: %s",
                 nAtChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), nOwnerCoinFlag, nOrderNumber,
                 CoinToTokenBigFloat(nCompOrderAmount).c_str(), nCompleteOrderCount, hashDexOrder.ToString().c_str(), hashBlock.GetBhString().c_str());
#endif
    }

    for (const auto& kv : mapMaxOrderNumber)
    {
        const CDestination destOrder = std::get<0>(kv.first);
        const uint256 hashCoinPair = std::get<1>(kv.first);
        const uint8 nOwnerCoinFlag = std::get<2>(kv.first);

        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_MAX_NUMBER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Max order number: chainid: %d, order address: %s, coin pair: %s, owner coin flag: %d, max number: %ld, at block: %s",
                 nChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), nOwnerCoinFlag, kv.second, hashBlock.GetBhString().c_str());
#endif
    }

    for (const auto& kv : mapCoinPairCompletePrice)
    {
        const uint256& hashCoinPair = kv.first;
        const uint256& nCompletePrice = kv.second;

        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE << hashCoinPair;
        ssKey.GetData(btKey);

        ssValue << nCompletePrice;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Complete price: coin pair: %s, complete price: %s, at block: %s",
                 hashCoinPair.ToString().c_str(), CoinToTokenBigFloat(nCompletePrice).c_str(), hashBlock.GetBhString().c_str());
#endif
    }

    for (const CChainId nPeerChainId : setPeerCrossChainId)
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_SEND_LAST_PROVE_BLOCK << BSwap32(nPeerChainId);
        ssKey.GetData(btKey);

        ssValue << hashBlock;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Peer cross chainid: peer chainid: %u, block: %s",
                 nPeerChainId, hashBlock.GetBhString().c_str());
#endif
    }

    // for (const auto& kv : mapCompDexOrderRecord)
    // {
    //     const CDexOrderHeader& orderHeader = kv.first;

    //     const CChainId nAtChainId = orderHeader.GetChainId();
    //     const CDestination& destOrder = orderHeader.GetOrderAddress();
    //     const uint256& hashCoinPair = orderHeader.GetCoinPairHash();
    //     const uint8 nOwnerCoinFlag = orderHeader.GetOwnerCoinFlag();
    //     const uint64 nOrderNumber = orderHeader.GetOrderNumber();

    //     uint64 nCompRecordNumber = 0;
    //     auto it = mapCompOrderAmount.find(orderHeader);
    //     if (it != mapCompOrderAmount.end())
    //     {
    //         nCompRecordNumber = it->second.second - kv.second.size();
    //     }

    //     for (const CCompDexOrderRecord& orderRecord : kv.second)
    //     {
    //         mtbase::CBufStream ssKey, ssValue;
    //         bytes btKey, btValue;

    //         ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_RECORD << BSwap32(nAtChainId) << destOrder << hashCoinPair << nOwnerCoinFlag << BSwap64(nOrderNumber) << BSwap64(nCompRecordNumber);
    //         ssKey.GetData(btKey);

    //         ssValue << orderRecord << nHeight << nSlot;
    //         ssValue.GetData(btValue);

    //         mapKv.insert(make_pair(btKey, btValue));

    //         StdDebug("TEST", "HdexDB Add DexOrder: Complete record: at chainid: %d, order address: %s, coin pair: %s, owner coin flag: %d, order number: %lu, record number: %lu, peer address: %s, complete amount: %s, complete price: %s, at block: %s",
    //                  nAtChainId, destOrder.ToString().c_str(), hashCoinPair.ToString().c_str(), nOwnerCoinFlag, nOrderNumber, nCompRecordNumber,
    //                  orderRecord.destPeerOrder.ToString().c_str(), CoinToTokenBigFloat(orderRecord.nCompleteAmount).c_str(),
    //                  CoinToTokenBigFloat(orderRecord.nCompletePrice).c_str(), hashBlock.GetBhString().c_str());

    //         nCompRecordNumber++;
    //     }
    // }

    for (const auto& kv : mapBlockProve)
    {
        auto funcAddCrossTransferRecord = [&](const CBlockCoinTransferProve& transProve, const CChainId nPeerChainId, const uint256& hashPeerAtBlock, const uint32 nIndex) {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_TRANSFER_RECV << BSwap32(nPeerChainId) << transProve.destTransfer << hashPeerAtBlock << BSwap32(nIndex);
            ssKey.GetData(btKey);

            ssValue << transProve;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: cross transfer recv: local chainid: %d, peer chainid: %d, address: %s, coin symbol: %s, ori chainid: %d, dest chainid: %d, amount: %s, peer at block: %s",
                     nChainId, nPeerChainId, transProve.destTransfer.ToString().c_str(), transProve.strCoinSymbol.c_str(), transProve.nOriChainId,
                     transProve.nDestChainId, CoinToTokenBigFloat(transProve.nTransferAmount).c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif
        };
        auto funcAddOrderProve = [&](const CBlockDexOrderProve& orderProve, const CChainId nPeerChainId, const uint256& hashPeerAtBlock) {
            const CDestination& destOrder = orderProve.destOrder;
            const CChainId nChainIdPeer = orderProve.nChainIdPeer;
            const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer);
            const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer);
            const uint64 nOrderNumber = orderProve.nOrderNumber;

            CDexOrderHeader orderHeader(nPeerChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber);
            CDexOrderBody orderBody(orderProve.strCoinSymbolOwner, orderProve.strCoinSymbolPeer, nChainIdPeer, orderProve.nOrderAmount, orderProve.nOrderPrice);

            {
                mtbase::CBufStream ssKey, ssValue;
                bytes btKey, btValue;

                ssKey << DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER << BSwap32(nPeerChainId) << destOrder << hashCoinPair << nOwnerCoinFlag << BSwap64(nOrderNumber);
                ssKey.GetData(btKey);

                ssValue << CDexOrderSave(orderBody, nPeerChainId, hashPeerAtBlock);
                ssValue.GetData(btValue);

                mapKv[btKey] = btValue;
            }
#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: local chainid: %d, peer chainid: %d, order address: %s, symbol owner: %s, symbol peer: %s, owner coin flag: %d, order number: %lu, order amount: %s, order price: %s, at block: %s",
                     nChainId, nPeerChainId, destOrder.ToString().c_str(), orderProve.strCoinSymbolOwner.c_str(), orderProve.strCoinSymbolPeer.c_str(),
                     nOwnerCoinFlag, nOrderNumber, CoinToTokenBigFloat(orderProve.nOrderAmount).c_str(),
                     CoinToTokenBigFloat(orderProve.nOrderPrice).c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif

            uint256 nCompleteAmount;
            uint64 nCompleteOrderCount = 0;

            auto it = mapCompOrderAmount.find(orderHeader);
            if (it != mapCompOrderAmount.end())
            {
                nCompleteAmount = it->second.first;
                nCompleteOrderCount = it->second.second;
            }

            //key: order header, value: 1: order body, 2: complete amount, 3: complete count, 4: at chainid, 5: at block
            //std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64, CChainId, uint256>> mapAddBlockProveOrder;
            mapAddBlockProveOrder[orderHeader] = std::make_tuple(orderBody, nCompleteAmount, nCompleteOrderCount, nPeerChainId, hashPeerAtBlock);
        };
        auto funcAddConfirmRecvProve = [&](const uint256& hashPeerLastConfirmBlock, const CChainId nPeerChainId, const uint256& hashPeerAtBlock) {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK << BSwap32(nPeerChainId);
            ssKey.GetData(btKey);

            ssValue << hashPeerAtBlock << hashPeerLastConfirmBlock;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: Confirm recv prove block: peer chainid: %d, hashPeerAtBlock: %s, hashPeerLastConfirmBlock: %s, hashBlock: %s",
                     nPeerChainId, hashPeerAtBlock.GetBhString().c_str(), hashPeerLastConfirmBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
#endif
        };

        const CChainId nPeerChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

#ifdef HDEX_OUT_TEST_LOG
        StdDebug("TEST", "HdexDB Add DexOrder: Block prove: prove block: local chainid: %d ===============================", nChainId);
#endif

        uint256 hashLastProveBlock;
        if (blockProve.hashPrevBlock != 0 && !blockProve.vPrevBlockCcProve.empty())
        {
            std::vector<std::pair<uint256, CBlockPrevProve>> vCacheProve;
            uint256 hashAtBlock = blockProve.hashPrevBlock;

            for (const CBlockPrevProve& prevProve : blockProve.vPrevBlockCcProve)
            {
                vCacheProve.push_back(std::make_pair(hashAtBlock, prevProve));
                hashAtBlock = prevProve.hashPrevBlock;
            }

            for (const auto& vd : boost::adaptors::reverse(vCacheProve))
            {
                const uint256& hashPeerAtBlock = vd.first;
                const CBlockPrevProve& prevProve = vd.second;
                if (!prevProve.proveCrosschain.IsNull())
                {
                    hashLastProveBlock = hashPeerAtBlock;

#ifdef HDEX_OUT_TEST_LOG
                    StdDebug("TEST", "HdexDB Add DexOrder: Block prove: prove block: prev prove, prev prove block: %s, peer at block: %s",
                             prevProve.proveCrosschain.GetPrevProveBlock().GetBhString().c_str(), hashPeerAtBlock.GetBhString().c_str());
#endif

                    auto& transProve = prevProve.proveCrosschain.GetCoinTransferProve();
                    for (uint32 i = 0; i < transProve.size(); i++)
                    {
                        funcAddCrossTransferRecord(transProve[i], nPeerChainId, hashPeerAtBlock, i);
                    }
                    for (const auto& kv : prevProve.proveCrosschain.GetDexOrderProve())
                    {
                        funcAddOrderProve(kv.second, nPeerChainId, hashPeerAtBlock);
                    }
                    for (const uint256& hashPeerLastConfirmBlock : prevProve.proveCrosschain.GetCrossConfirmRecvBlock())
                    {
                        funcAddConfirmRecvProve(hashPeerLastConfirmBlock, nPeerChainId, hashPeerAtBlock);
                    }
                }
            }
        }

        if (!blockProve.proveCrosschain.IsNull())
        {
            hashLastProveBlock = blockProve.hashBlock;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: prove block: local prove, confirm block size: %lu, first prove block: %s, prev prove block: %s, local block: %s",
                     blockProve.proveCrosschain.GetCrossConfirmRecvBlock().size(), blockProve.GetFirstPrevBlockHash().GetBhString().c_str(),
                     blockProve.proveCrosschain.GetPrevProveBlock().GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
#endif

            auto& transProve = blockProve.proveCrosschain.GetCoinTransferProve();
            for (uint32 i = 0; i < transProve.size(); i++)
            {
                funcAddCrossTransferRecord(transProve[i], nPeerChainId, blockProve.hashBlock, i);
            }
            for (const auto& kv : blockProve.proveCrosschain.GetDexOrderProve())
            {
                funcAddOrderProve(kv.second, nPeerChainId, blockProve.hashBlock);
            }
            for (const uint256& hashPeerLastConfirmBlock : blockProve.proveCrosschain.GetCrossConfirmRecvBlock())
            {
                funcAddConfirmRecvProve(hashPeerLastConfirmBlock, nPeerChainId, blockProve.hashBlock);
            }
        }

        if (hashLastProveBlock != 0)
        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK << BSwap32(nPeerChainId);
            ssKey.GetData(btKey);

            ssValue << hashLastProveBlock;
            ssValue.GetData(btValue);

            mapKv[btKey] = btValue;

            mapPeerProveLastBlock[nPeerChainId] = hashLastProveBlock;

#ifdef HDEX_OUT_TEST_LOG
            StdDebug("TEST", "HdexDB Add DexOrder: Block prove: write last prove block: peer chainid: %d, last prove block: %s, prove block: %s",
                     nPeerChainId, hashLastProveBlock.GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
#endif
        }
    }

    AddPrevRoot(DB_HDEX_ROOT_TYPE_TRIE, hashPrevRoot, hashBlock, mapKv);

#ifdef HDEX_OUT_TEST_LOG
    StdDebug("TEST", "HdexDB Add DexOrder: mapKv.size: %lu =========================================", mapKv.size());
#endif
    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CHdexDB", "Add dex order: Add new trie fail, prev block: %s, block: %s",
               hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }

    if (!WriteTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashNewRoot))
    {
        StdLog("CHdexDB", "Add dex order: Write trie root fail, prev block: %s, block: %s",
               hashPrevBlock.GetHex().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }

    if (!UpdateDexOrderCache(hashPrevBlock, hashBlock, mapAddNewOrder, mapAddBlockProveOrder, mapCoinPairCompletePrice, mapUpdateCompOrder, mapPeerProveLastBlock))
    {
        StdLog("CHdexDB", "Add dex order: Update dex order cache fail,  block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, CDexOrderBody& dexOrder)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);

    CDexOrderSave dexOrderDb;
    if (!GetDexOrderDb(hashRoot, nChainIdOwner, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber, dexOrderDb))
    {
        return false;
    }
    dexOrder = dexOrderDb.dexOrder;
    return true;
}

bool CHdexDB::GetDexCompletePrice(const uint256& hashBlock, const uint256& hashCoinPair, uint256& nCompletePrice)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex complete price: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    return GetDexCompletePriceDb(hashRoot, hashCoinPair, nCompletePrice);
}

bool CHdexDB::GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get complete order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint256 hashDexOrder = CDexOrderHeader::GetDexOrderHashStatic(nChainIdOwner, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber);

    return GetDexOrderCompleteDb(hashRoot, hashDexOrder, nCompleteAmount, nCompleteOrderCount);
}

bool CHdexDB::GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get complete order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    return GetDexOrderCompleteDb(hashRoot, hashDexOrder, nCompleteAmount, nCompleteOrderCount);
}

bool CHdexDB::ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                                  const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder)
{
    CReadLock rlock(rwAccess);

    class CListAddressDexOrderTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListAddressDexOrderTrieDBWalker(const CChainId nChainIdIn, const CDestination& destOrderIn, const uint256& hashCoinPairIn, const uint8 nOwnerCoinFlagIn,
                                         const uint32 nGetCountIn, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrderIn)
          : nChainId(nChainIdIn), destOrder(destOrderIn), hashCoinPair(hashCoinPairIn), nOwnerCoinFlag(nOwnerCoinFlagIn), nGetCount(nGetCountIn), mapDexOrderOut(mapDexOrderIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListAddressDexOrderTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER)
                {
                    CChainId nChainIdDb;
                    CDestination destOrderDb;
                    uint256 hashCoinPairDb;
                    uint8 nOwnerCoinFlagDb;
                    uint64 nOrderNumberDb;

                    ssKey >> nChainIdDb >> destOrderDb >> hashCoinPairDb >> nOwnerCoinFlagDb >> nOrderNumberDb;

                    nChainIdDb = BSwap32(nChainIdDb);
                    nOrderNumberDb = BSwap64(nOrderNumberDb);

                    if (nChainIdDb == nChainId && destOrderDb == destOrder
                        && (hashCoinPair == 0 || (hashCoinPair == hashCoinPairDb && nOwnerCoinFlag == nOwnerCoinFlagDb)))
                    {
                        CDexOrderSave dexOrderSave;

                        mtbase::CBufStream ssValue(btValue);
                        ssValue >> dexOrderSave;

                        mapDexOrderOut.insert(std::make_pair(CDexOrderHeader(nChainIdDb, destOrderDb, hashCoinPairDb, nOwnerCoinFlagDb, nOrderNumberDb), dexOrderSave));
                        if (nGetCount > 0 && mapDexOrderOut.size() >= nGetCount)
                        {
                            fWalkOver = true;
                        }
                    }
                    else
                    {
                        fWalkOver = true;
                    }
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        const CChainId nChainId;
        const CDestination destOrder;
        const uint256 hashCoinPair;
        const uint8 nOwnerCoinFlag;
        const uint32 nGetCount;
        std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrderOut;
    };

    if (hashBlock == 0)
    {
        return true;
    }

    CChainId nChainId;
    uint256 hashCoinPair;
    uint8 nOwnerCoinFlag = 0;

    nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    if (!strCoinSymbolOwner.empty() && !strCoinSymbolPeer.empty())
    {
        hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
        nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "List dex order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    if (hashCoinPair == 0)
    {
        ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << BSwap32(nChainId) << destOrder;
    }
    else
    {
        ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag;
    }
    ssKeyPrefix.GetData(btKeyPrefix);

    bytes btBeginKeyTail;
    if (hashCoinPair != 0 && nBeginOrderNumber > 0)
    {
        mtbase::CBufStream ss;
        ss << BSwap64(nBeginOrderNumber);
        ss.GetData(btBeginKeyTail);
    }

    uint64 nGetCountInner = 0;
    if (nGetCount == 0 || nGetCount > MAX_FETCH_DEX_ORDER_COUNT)
    {
        nGetCountInner = MAX_FETCH_DEX_ORDER_COUNT;
    }
    else
    {
        nGetCountInner = nGetCount;
    }

    CListAddressDexOrderTrieDBWalker walker(nChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nGetCountInner, mapDexOrder);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix, btBeginKeyTail))
    {
        StdLog("CHdexDB", "List dex order: Walk through trie, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

#ifdef HDEX_OUT_TEST_LOG
    StdDebug("TEST", "====================== MATCHTEST: nChainId: %d =======================", nChainId);
    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (ptrDexOrderCache)
    {
        ptrDexOrderCache->GetMatchDex()->ShowDexOrderList();

        std::map<uint256, CMatchOrderResult> mapMatchResult;
        if (!ptrDexOrderCache->GetMatchDex()->MatchDex(mapMatchResult))
        {
            StdDebug("MATCHTEST", "MatchDex fail");
        }
        else
        {
            for (const auto& kv : mapMatchResult)
            {
                const CMatchOrderResult& result = kv.second;
                StdDebug("MATCHTEST", "######################### MATCH RESULT: symbol sell: %s, symbol buy: %s #########################",
                         result.strCoinSymbolSell.c_str(), result.strCoinSymbolBuy.c_str());
                for (const CMatchOrderRecord& record : result.vMatchOrderRecord)
                {
                    StdDebug("MATCHTEST", "MatchDex result: sell address: %s, buy address: %s", record.destSellOrder.ToString().c_str(), record.destBuyOrder.ToString().c_str());
                    StdDebug("MATCHTEST", "MatchDex result: sell complete amount: %s", CoinToTokenBigFloat(record.nSellCompleteAmount).c_str());
                    StdDebug("MATCHTEST", "MatchDex result: buy complete amount: %s", CoinToTokenBigFloat(record.nBuyCompleteAmount).c_str());
                }
            }
        }
    }

    StdDebug("TEST", "++++++++++++++++++ ListRecvCrosschainProveDb: nChainId: %d +++++++++++++++++++++++++", nChainId);
    std::vector<std::tuple<CChainId, uint256, CBlockProve>> vRecvCrossProve;
    if (ListRecvCrosschainProveDb(nChainId, vRecvCrossProve))
    {
        for (const auto& vd : vRecvCrossProve)
        {
            const CChainId nSendChainId = std::get<0>(vd);
            const uint256& hashFirstPrevProveBlock = std::get<1>(vd);
            const CBlockProve& blockProve = std::get<2>(vd);

            StdDebug("TEST", "ListRecvCrosschainProveDb: List: nRecvChainId: %d, nSendChainId: %d, hashFirstPrevProveBlock: %s, prove block: %s",
                     nChainId, nSendChainId, hashFirstPrevProveBlock.GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
        }
    }
#endif
    return true;
}

bool CHdexDB::GetDexOrderMaxNumber(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer, uint64& nMaxOrderNumber)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get dex order max number: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    const uint8 nOwnerCoinFlag = CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);

    return GetDexOrderMaxNumberDb(hashRoot, CBlock::GetBlockChainIdByHash(hashBlock), destOrder, hashCoinPair, nOwnerCoinFlag, nMaxOrderNumber);
}

bool CHdexDB::GetPeerCrossLastBlock(const uint256& hashBlock, const CChainId nPeerChainId, uint256& hashLastProveBlock)
{
    CReadLock rlock(rwAccess);

    uint256 hashRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Get peer cross last block: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return GetPeerCrossLastBlockDb(hashRoot, nPeerChainId, hashLastProveBlock);
}

const SHP_MATCH_DEX CHdexDB::GetBlockMatchDex(const uint256& hashBlock)
{
    CReadLock rlock(rwAccess);

    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (!ptrDexOrderCache)
    {
        return nullptr;
    }
    return ptrDexOrderCache->GetMatchDex();
}

bool CHdexDB::GetMatchDexData(const uint256& hashBlock, std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    CReadLock rlock(rwAccess);

    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (!ptrDexOrderCache)
    {
        return false;
    }
    return ptrDexOrderCache->GetMatchDexResult(mapMatchResult);
}

bool CHdexDB::ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    CReadLock rlock(rwAccess);

    SHP_CACHE_BLOCK_DEX_ORDER ptrDexOrderCache = LoadBlockDexOrderCache(hashBlock);
    if (!ptrDexOrderCache)
    {
        StdLog("CHdexDB", "List match dex order: Load block dex order cache fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return ptrDexOrderCache->ListMatchDexOrder(strCoinSymbolSell, strCoinSymbolBuy, nGetCount, realDexOrder);
}

bool CHdexDB::AddBlockCrosschainProve(const uint256& hashBlock, const CBlockStorageProve& proveBlockCrosschain)
{
    CWriteLock wlock(rwAccess);

#ifdef HDEX_OUT_TEST_LOG
    for (const auto& kv : proveBlockCrosschain.mapCrossProve)
    {
        for (const auto& kvs : kv.second.first.GetDexOrderProve())
        {
            StdDebug("TEST", "AddBlockCrosschainProve: dex order prove, peer chainid: %d, nOrderNumber: %lu, prev prove block: %s, block: %s",
                     kv.first, kvs.second.nOrderNumber, kv.second.first.GetPrevProveBlock().GetBhString().c_str(), hashBlock.GetBhString().c_str());
        }
    }
#endif

    return WriteBlockCrosschainProveDb(hashBlock, proveBlockCrosschain);
}

bool CHdexDB::UpdateBlockAggSign(const uint256& hashBlock, const bytes& btAggBitmap, const bytes& btAggSig, std::map<CChainId, CBlockProve>& mapBlockProve)
{
    CWriteLock wlock(rwAccess);

    CBlockStorageProve proveCrosschain;
    if (!GetBlockCrosschainProveDb(hashBlock, proveCrosschain))
    {
        StdLog("CHdexDB", "Update block agg sign: Get block crosschain prove fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    proveCrosschain.btAggSigBitmap = btAggBitmap;
    proveCrosschain.btAggSigData = btAggSig;

    if (!WriteBlockCrosschainProveDb(hashBlock, proveCrosschain))
    {
        StdLog("CHdexDB", "Update block agg sign: Write block crosschain prove fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    if (!GetBlockProveDb(hashBlock, proveCrosschain, mapBlockProve))
    {
        StdLog("CHdexDB", "Update block agg sign: Get block prove fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    for (const auto& kv : mapBlockProve)
    {
        const CChainId nRecvChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        if (!AddRecvCrosschainProveDb(nRecvChainId, blockProve))
        {
            StdLog("CHdexDB", "Update block agg sign: Add recv crosschain prove fail, recv chainid: %d, send block: %s, ref block: %s",
                   nRecvChainId, blockProve.hashBlock.GetBhString().c_str(), proveCrosschain.hashRefBlock.GetBhString().c_str());
            return false;
        }
    }
    StdDebug("CHdexDB", "Update block agg sign: block cross prove: %lu, prove count: %lu, block: %s",
             proveCrosschain.mapCrossProve.size(), mapBlockProve.size(), hashBlock.GetBhString().c_str());
    return true;
}

bool CHdexDB::GetBlockCrosschainProve(const uint256& hashBlock, CBlockStorageProve& proveBlockCrosschain)
{
    CReadLock rlock(rwAccess);
    return GetBlockCrosschainProveDb(hashBlock, proveBlockCrosschain);
}

bool CHdexDB::GetCrosschainProveForPrevBlock(const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve)
{
    CReadLock rlock(rwAccess);

    std::map<CChainId, uint256> mapSendLastProveBlock;
    if (!ListPeerChainSendLastProveBlockDb(hashRecvPrevBlock, mapSendLastProveBlock))
    {
        StdLog("CHdexDB", "Get crosschain prove for prevblock: List send last prove block fail, recv chainid: %d, recv prev block: %s", nRecvChainId, hashRecvPrevBlock.GetBhString().c_str());
        return false;
    }

    std::map<CChainId, uint256> mapSendLastBlock;
    if (!ListSendChainProveLastBlockDb(nRecvChainId, mapSendLastBlock))
    {
        StdLog("CHdexDB", "Get crosschain prove for prevblock: List send last block fail, recv chainid: %d, recv prev block: %s", nRecvChainId, hashRecvPrevBlock.GetBhString().c_str());
        return false;
    }

    for (const auto& kv : mapSendLastBlock)
    {
        if (mapSendLastProveBlock.find(kv.first) == mapSendLastProveBlock.end())
        {
            mapSendLastProveBlock.insert(std::make_pair(kv.first, kv.second));
        }
    }

    const uint32 nLastHeight = CBlock::GetBlockHeightByHash(hashRecvPrevBlock);
    const uint16 nLastSlot = CBlock::GetBlockSlotByHash(hashRecvPrevBlock);

    for (const auto& kv : mapSendLastProveBlock)
    {
        const CChainId nSendChainId = kv.first;
        const uint256& hashLastProveBlock = kv.second;
        CBlockProve blockProve;
        if (GetCrosschainProveForPrevBlockDb(nRecvChainId, nSendChainId, hashLastProveBlock, nLastHeight, nLastSlot, blockProve))
        {
            mapBlockCrosschainProve.insert(std::make_pair(nSendChainId, blockProve));
        }
        // else
        // {
        //     StdDebug("TEST", "Get crosschain prove for prevblock: Get fail, nRecvChainId: %d, nSendChainId: %d, hashLastProveBlock: %s", nRecvChainId, nSendChainId, hashLastProveBlock.GetBhString().c_str());
        // }
    }

    if (mapBlockCrosschainProve.empty())
    {
        return false;
    }
    return true;
}

bool CHdexDB::AddRecvCrosschainProve(const CChainId nRecvChainId, const CBlockProve& blockProve)
{
    CWriteLock wlock(rwAccess);

    return AddRecvCrosschainProveDb(nRecvChainId, blockProve);
}

bool CHdexDB::GetRecvCrosschainProve(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashSendProvePrevBlock, CBlockProve& blockProve)
{
    CReadLock rlock(rwAccess);

    return GetRecvCrosschainProveDb(nRecvChainId, nSendChainId, hashSendProvePrevBlock, blockProve);
}

bool CHdexDB::VerifyDexOrder(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashRoot))
    {
        StdLog("CHdexDB", "Verify dex order: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CHdexDB", "Verify dex order: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetBhString().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CHdexDB", "Verify dex order: Read prev trie root fail, prev block: %s", hashPrevBlock.GetBhString().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_HDEX_ROOT_TYPE_TRIE, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CHdexDB", "Verify dex order: Get prev root fail, hashRoot: %s, hashPrevRoot: %s, hashBlock: %s",
               hashRoot.GetHex().c_str(), hashPrevRoot.GetHex().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CHdexDB", "Verify dex order: Root error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, hashBlock: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetBhString().c_str(), hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------
bool CHdexDB::WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot)
{
    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_HDEX_KEY_ID_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CHdexDB::ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_HDEX_KEY_ID_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CHdexDB::AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << nRootType << DB_HDEX_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CHdexDB::GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << nRootType << DB_HDEX_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashPrevRoot >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrderDb(const uint256& hashRoot, const CChainId nChainId, const CDestination& destOrder, const uint256& hashCoinPair, const uint8 nOwnerCoinFlag, const uint64 nOrderNumber, CDexOrderSave& dexOrder)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag << BSwap64(nOrderNumber);
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> dexOrder;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexCompletePriceDb(const uint256& hashRoot, const uint256& hashCoinPair, uint256& nCompletePrice)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE << hashCoinPair;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nCompletePrice;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrderCompleteDb(const uint256& hashRoot, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT << hashDexOrder;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nCompleteAmount >> nCompleteOrderCount;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetDexOrderMaxNumberDb(const uint256& hashRoot, const CChainId nChainId, const CDestination& destOrder, const uint256& hashCoinPair, const uint8 nOwnerCoinFlag, uint64& nMaxOrderNumber)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_MAX_NUMBER << BSwap32(nChainId) << destOrder << hashCoinPair << nOwnerCoinFlag;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nMaxOrderNumber;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetPeerCrossLastBlockDb(const uint256& hashRoot, const CChainId nPeerChainId, uint256& hashLastProveBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_SEND_LAST_PROVE_BLOCK << BSwap32(nPeerChainId);
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashLastProveBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetPeerChainSendPrevBlockDb(const uint256& hashRoot, const CChainId nSendChainId, uint256& hashLastProveBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK << BSwap32(nSendChainId);
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashLastProveBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::ListPeerChainSendLastProveBlockDb(const uint256& hashBlock, std::map<CChainId, uint256>& mapSendLastProveBlock)
{
    class CListPeerChainSendLastProveBlockTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListPeerChainSendLastProveBlockTrieDBWalker(std::map<CChainId, uint256>& mapSendLastProveBlockIn)
          : mapSendLastProveBlock(mapSendLastProveBlockIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListPeerChainSendLastProveBlockTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK)
                {
                    CChainId nSendChainId;
                    uint256 hashLastProveBlock;

                    ssKey >> nSendChainId;
                    nSendChainId = BSwap32(nSendChainId);

                    mtbase::CBufStream ssValue(btValue);
                    ssValue >> hashLastProveBlock;

                    mapSendLastProveBlock.insert(std::make_pair(nSendChainId, hashLastProveBlock));
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        std::map<CChainId, uint256>& mapSendLastProveBlock; // key: send chainid, value: last prove block hash
    };

    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "List peer chain send last prove block: Read trie root fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_CROSS_LAST_PROVE_BLOCK;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListPeerChainSendLastProveBlockTrieDBWalker walker(mapSendLastProveBlock);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "List peer chain send last prove block: Walk through trie, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::WriteBlockCrosschainProveDb(const uint256& hashBlock, const CBlockStorageProve& proveBlockCrosschain)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_BLOCK_CROSSCHAIN_PROVE << hashBlock;
    ssValue << proveBlockCrosschain;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CHdexDB::GetBlockCrosschainProveDb(const uint256& hashBlock, CBlockStorageProve& proveBlockCrosschain)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_BLOCK_CROSSCHAIN_PROVE << hashBlock;

    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> proveBlockCrosschain;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::GetCoinPairCompletePriceCache(const uint256& hashLastBlock, std::map<uint256, uint256>& mapCompPriceCache)
{
    class CListCoinPairCompPriceTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListCoinPairCompPriceTrieDBWalker(std::map<uint256, uint256>& mapCompPriceCacheIn)
          : mapCompPriceCache(mapCompPriceCacheIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListCoinPairCompPriceTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE)
                {
                    uint256 hashCoinPair;
                    uint256 nCompletePrice;

                    ssKey >> hashCoinPair;

                    mtbase::CBufStream ssValue(btValue);
                    ssValue >> nCompletePrice;

                    mapCompPriceCache.insert(std::make_pair(hashCoinPair, nCompletePrice));
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        std::map<uint256, uint256>& mapCompPriceCache;
    };

    if (hashLastBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashLastBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "Get coin pair complete price: Read trie root fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_PRICE;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListCoinPairCompPriceTrieDBWalker walker(mapCompPriceCache);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "Get coin pair complete price: Walk through trie, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::GetCompDexOrderCache(const uint256& hashLastBlock, std::map<uint256, std::pair<uint256, uint64>>& mapCompCache)
{
    class CListCompDexOrderTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListCompDexOrderTrieDBWalker(std::map<uint256, std::pair<uint256, uint64>>& mapCompCacheIn)
          : mapCompCache(mapCompCacheIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListCompDexOrderTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT)
                {
                    uint256 hashDexOrder;
                    uint256 nCompleteAmount;
                    uint64 nCompleteCount;

                    ssKey >> hashDexOrder;

                    mtbase::CBufStream ssValue(btValue);
                    ssValue >> nCompleteAmount >> nCompleteCount;

                    mapCompCache.insert(std::make_pair(hashDexOrder, std::make_pair(nCompleteAmount, nCompleteCount)));
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        std::map<uint256, std::pair<uint256, uint64>>& mapCompCache;
    };

    if (hashLastBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashLastBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "Get complete dex order: Read trie root fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_DEX_ORDER_COMPLETE_AMOUNT;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListCompDexOrderTrieDBWalker walker(mapCompCache);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "Get complete dex order: Walk through trie, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::GetRecvConfirmBlockListCache(const uint256& hashLastBlock, std::map<CChainId, std::pair<uint256, uint256>>& mapRecvConfirmBlock)
{
    class CListRecvConfirmBlockTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListRecvConfirmBlockTrieDBWalker(std::map<CChainId, std::pair<uint256, uint256>>& mapRecvConfirmBlockIn)
          : mapRecvConfirmBlock(mapRecvConfirmBlockIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListRecvConfirmBlockTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK)
                {
                    CChainId nPeerChainIdDb;
                    uint256 hashPeerAtBlockDb;
                    uint256 hashConfirmLocalBlockDb;

                    ssKey >> nPeerChainIdDb;
                    nPeerChainIdDb = BSwap32(nPeerChainIdDb);

                    mtbase::CBufStream ssValue(btValue);
                    ssValue >> hashPeerAtBlockDb >> hashConfirmLocalBlockDb;

                    mapRecvConfirmBlock.insert(std::make_pair(nPeerChainIdDb, std::make_pair(hashPeerAtBlockDb, hashConfirmLocalBlockDb)));
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        std::map<CChainId, std::pair<uint256, uint256>>& mapRecvConfirmBlock; // key: chainid, value: 1: peer at block, 2: confirm local block
    };

    if (hashLastBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashLastBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "Get recv confirm block list: Read trie root fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_CROSS_RECV_CONFIRM_BLOCK;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListRecvConfirmBlockTrieDBWalker walker(mapRecvConfirmBlock);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "Get recv confirm block list: Walk through trie, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CHdexDB::LoadLastDexOrderCache(const uint256& hashLastBlock, const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache, const std::map<uint256, uint256>& mapCompPriceCache, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder)
{
    class CListBlockDexOrderTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListBlockDexOrderTrieDBWalker(const CChainId nChainIdIn, const std::map<uint256, std::pair<uint256, uint64>>& mapCompCacheIn, const std::map<uint256, uint256>& mapCompPriceCacheIn, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrderIn)
          : nChainId(nChainIdIn), mapCompCache(mapCompCacheIn), mapCompPriceCache(mapCompPriceCacheIn), ptrCacheDexOrder(ptrCacheDexOrderIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListBlockDexOrderTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER)
                {
                    CChainId nChainIdDb;
                    CDestination destOrderDb;
                    uint256 hashCoinPairDb;
                    uint8 nOwnerCoinFlagDb;
                    uint64 nOrderNumberDb;

                    ssKey >> nChainIdDb >> destOrderDb >> hashCoinPairDb >> nOwnerCoinFlagDb >> nOrderNumberDb;

                    nChainIdDb = BSwap32(nChainIdDb);
                    nOrderNumberDb = BSwap64(nOrderNumberDb);

                    if (nChainIdDb == nChainId)
                    {
                        mtbase::CBufStream ssValue(btValue);
                        CDexOrderSave dexOrderSave;
                        ssValue >> dexOrderSave;

                        uint256 hashDexOrder = CDexOrderHeader::GetDexOrderHashStatic(nChainId, destOrderDb, hashCoinPairDb, nOwnerCoinFlagDb, nOrderNumberDb);

                        uint256 nCompAmount;
                        uint64 nCompCount = 0;
                        auto it = mapCompCache.find(hashDexOrder);
                        if (it != mapCompCache.end())
                        {
                            nCompAmount = it->second.first;
                            nCompCount = it->second.second;
                        }

                        uint256 nPrevCompletePrice;
                        auto mt = mapCompPriceCache.find(hashCoinPairDb);
                        if (mt != mapCompPriceCache.end())
                        {
                            nPrevCompletePrice = mt->second;
                        }

                        if (!ptrCacheDexOrder->AddDexOrderCache(hashDexOrder, destOrderDb, nOrderNumberDb, dexOrderSave.dexOrder, dexOrderSave.nAtChainId,
                                                                dexOrderSave.hashAtBlock, nCompAmount, nCompCount, nPrevCompletePrice))
                        {
                            fWalkOver = true;
                        }
                    }
                    else
                    {
                        fWalkOver = true;
                    }
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        const CChainId nChainId;
        const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache;
        const std::map<uint256, uint256>& mapCompPriceCache;
        SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder;
    };

    if (hashLastBlock == 0)
    {
        return true;
    }

    CChainId nChainId = CBlock::GetBlockChainIdByHash(hashLastBlock);

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashLastBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "Load dex order: Read trie root fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_LOCAL_DEX_ORDER << BSwap32(nChainId);
    ssKeyPrefix.GetData(btKeyPrefix);

    CListBlockDexOrderTrieDBWalker walker(nChainId, mapCompCache, mapCompPriceCache, ptrCacheDexOrder);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "Load dex order: Walk through trie, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

void CHdexDB::AddBlockDexOrderCache(const uint256& hashBlock, const SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder)
{
    while (mapCacheBlockDexOrder.size() >= MAX_CACHE_BLOCK_DEX_ORDER_COUNT)
    {
        mapCacheBlockDexOrder.erase(mapCacheBlockDexOrder.begin());
    }
    mapCacheBlockDexOrder[hashBlock] = ptrCacheDexOrder;
}

void CHdexDB::RemoveBlockDexOrderCache(const uint256& hashBlock)
{
    mapCacheBlockDexOrder.erase(hashBlock);
}

bool CHdexDB::LoadLastBlockProveCache(const uint256& hashLastBlock, const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache, const std::map<uint256, uint256>& mapCompPriceCache, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder)
{
    class CListBlockProveTrieDBWalker : public CTrieDBWalker
    {
    public:
        CListBlockProveTrieDBWalker(const std::map<uint256, std::pair<uint256, uint64>>& mapCompCacheIn, const std::map<uint256, uint256>& mapCompPriceCacheIn, SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrderIn)
          : mapCompCache(mapCompCacheIn), mapCompPriceCache(mapCompPriceCacheIn), ptrCacheDexOrder(ptrCacheDexOrderIn) {}

        bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
        {
            if (btKey.size() == 0 || btValue.size() == 0)
            {
                mtbase::StdError("CListBlockProveTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
                return false;
            }
            try
            {
                mtbase::CBufStream ssKey(btKey);
                uint8 nKeyType;
                ssKey >> nKeyType;
                if (nKeyType == DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER)
                {
                    CChainId nPeerChainIdDb;
                    CDestination destOrderDb;
                    uint256 hashCoinPairDb;
                    uint8 nOwnerCoinFlagDb;
                    uint64 nOrderNumberDb;

                    ssKey >> nPeerChainIdDb >> destOrderDb >> hashCoinPairDb >> nOwnerCoinFlagDb >> nOrderNumberDb;

                    nPeerChainIdDb = BSwap32(nPeerChainIdDb);
                    nOrderNumberDb = BSwap64(nOrderNumberDb);

                    mtbase::CBufStream ssValue(btValue);
                    CDexOrderSave dexOrderSave;
                    ssValue >> dexOrderSave;

                    uint256 hashDexOrder = CDexOrderHeader::GetDexOrderHashStatic(nPeerChainIdDb, destOrderDb, hashCoinPairDb, nOwnerCoinFlagDb, nOrderNumberDb);

                    uint256 nCompAmount;
                    uint64 nCompCount = 0;
                    auto it = mapCompCache.find(hashDexOrder);
                    if (it != mapCompCache.end())
                    {
                        nCompAmount = it->second.first;
                        nCompCount = it->second.second;
                    }

                    uint256 nPrevCompletePrice;
                    auto nt = mapCompPriceCache.find(hashCoinPairDb);
                    if (nt != mapCompPriceCache.end())
                    {
                        nPrevCompletePrice = nt->second;
                    }

                    if (!ptrCacheDexOrder->AddDexOrderCache(hashDexOrder, destOrderDb, nOrderNumberDb, dexOrderSave.dexOrder, dexOrderSave.nAtChainId,
                                                            dexOrderSave.hashAtBlock, nCompAmount, nCompCount, nPrevCompletePrice))
                    {
                        fWalkOver = true;
                    }
                }
            }
            catch (std::exception& e)
            {
                mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
            return true;
        }

    public:
        const std::map<uint256, std::pair<uint256, uint64>>& mapCompCache;
        const std::map<uint256, uint256>& mapCompPriceCache;
        SHP_CACHE_BLOCK_DEX_ORDER ptrCacheDexOrder;
    };

    if (hashLastBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(DB_HDEX_ROOT_TYPE_TRIE, hashLastBlock, hashTrieRoot))
    {
        StdLog("CHdexDB", "Load block prove: Read trie root fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    bytes btKeyPrefix;
    mtbase::CBufStream ssKeyPrefix;
    ssKeyPrefix << DB_HDEX_KEY_TYPE_TRIE_PEER_DEX_ORDER;
    ssKeyPrefix.GetData(btKeyPrefix);

    CListBlockProveTrieDBWalker walker(mapCompCache, mapCompPriceCache, ptrCacheDexOrder);
    if (!dbTrie.WalkThroughTrie(hashTrieRoot, walker, btKeyPrefix))
    {
        StdLog("CHdexDB", "Load block prove: Walk through trie, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    std::map<CChainId, uint256> mapPeerLastProveBlock; // key: peer chainid, value: last prove block hash
    if (!ListPeerChainSendLastProveBlockDb(hashLastBlock, mapPeerLastProveBlock))
    {
        StdLog("CHdexDB", "Load block prove: List send last prove block fail, block: %s", hashLastBlock.GetBhString().c_str());
        return false;
    }

    for (const auto& kv : mapPeerLastProveBlock)
    {
        ptrCacheDexOrder->UpdatePeerProveLastBlock(kv.first, kv.second);
    }
    return true;
}

SHP_CACHE_BLOCK_DEX_ORDER CHdexDB::LoadBlockDexOrderCache(const uint256& hashBlock)
{
    CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);

#ifdef HDEX_CACHE_DEX_ENABLE
    auto it = mapCacheForkLastDexOrder.find(nChainId);
    if (it != mapCacheForkLastDexOrder.end() && it->second && it->second->GetLastBlock() == hashBlock)
    {
        StdDebug("CHdexDB", "Load block dex order: Load cache dex order success, block: %s", hashBlock.GetBhString().c_str());
        return it->second;
    }

    auto mt = mapCacheBlockDexOrder.find(hashBlock);
    if (mt != mapCacheBlockDexOrder.end())
    {
        return mt->second;
    }
#endif

    std::map<uint256, std::pair<uint256, uint64>> mapCompCache; // key: dex order hash, value: 1: complete amount, 2: complete count
    if (!GetCompDexOrderCache(hashBlock, mapCompCache))
    {
        StdLog("CHdexDB", "Load block dex order: Get complete dex order fail, block: %s", hashBlock.GetBhString().c_str());
        return nullptr;
    }
    std::map<uint256, uint256> mapCompPriceCache; // key: coin pair hash, value: complete price
    if (!GetCoinPairCompletePriceCache(hashBlock, mapCompPriceCache))
    {
        StdLog("CHdexDB", "Load block dex order: Get coin pair complete price fail, block: %s", hashBlock.GetBhString().c_str());
        return nullptr;
    }

    SHP_CACHE_BLOCK_DEX_ORDER ptrCacheBlockDexOrder = MAKE_SHARED_CACHE_BLOCK_DEX_ORDER();
    if (!LoadLastDexOrderCache(hashBlock, mapCompCache, mapCompPriceCache, ptrCacheBlockDexOrder))
    {
        StdLog("CHdexDB", "Load block dex order: Load last dex order fail, block: %s", hashBlock.GetBhString().c_str());
        return nullptr;
    }
    if (!LoadLastBlockProveCache(hashBlock, mapCompCache, mapCompPriceCache, ptrCacheBlockDexOrder))
    {
        StdLog("CHdexDB", "Load block dex order: Load last block prove fail, block: %s", hashBlock.GetBhString().c_str());
        return nullptr;
    }
    ptrCacheBlockDexOrder->SetLastBlock(hashBlock);

#ifdef HDEX_OUT_TEST_LOG
    {
        auto it = mapCacheForkLastDexOrder.find(nChainId);
        if (it != mapCacheForkLastDexOrder.end() && it->second)
        {
            if (it->second->GetLastBlock() == hashBlock)
            {
                if (*ptrCacheBlockDexOrder != *(it->second))
                {
                    StdLog("CHdexDB", "Load block dex order: Cache dex order error, block: %s", hashBlock.GetBhString().c_str());
                    StdDebug("CHdexDB", "Load data: +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
                    ptrCacheBlockDexOrder->GetMatchDex()->ShowDexOrderList();
                    StdDebug("CHdexDB", "Cache data: +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
                    it->second->GetMatchDex()->ShowDexOrderList();
                    StdDebug("CHdexDB", "End data: +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
                    mapCacheForkLastDexOrder.erase(it);
                }
                else
                {
                    StdDebug("CHdexDB", "Load block dex order: Cache dex order success, block: %s", hashBlock.GetBhString().c_str());
                }
            }
            else
            {
                StdLog("CHdexDB", "Load block dex order: Cache dex order block error, cache block: %s, block: %s", it->second->GetLastBlock().GetBhString().c_str(), hashBlock.GetBhString().c_str());
                //mapCacheForkLastDexOrder.erase(it);
            }
        }
    }
#endif

#ifdef HDEX_CACHE_DEX_ENABLE
    AddBlockDexOrderCache(hashBlock, ptrCacheBlockDexOrder);
#endif
    return ptrCacheBlockDexOrder;
}

bool CHdexDB::AddLinkFirstPrevBlock(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashBlock, const uint256& hashFirstPrevBlock)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_BLOCK_FOR_FIRST_PREV_BLOCK << BSwap32(nRecvChainId) << BSwap32(nSendChainId) << hashBlock;
    ssValue << hashFirstPrevBlock;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CHdexDB::GetLinkFirstPrevBlock(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashBlock, uint256& hashFirstPrevBlock)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_BLOCK_FOR_FIRST_PREV_BLOCK << BSwap32(nRecvChainId) << BSwap32(nSendChainId) << hashBlock;

    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashFirstPrevBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::UpdateDexOrderCache(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64>>& mapAddNewOrder,
                                  const std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64, CChainId, uint256>>& mapAddBlockProveOrder,
                                  const std::map<uint256, uint256>& mapCoinPairCompletePrice,
                                  const std::map<uint256, std::tuple<uint256, uint256, uint64>>& mapUpdateCompOrder,
                                  const std::map<CChainId, uint256>& mapPeerProveLastBlock)
{
    if (hashPrevBlock == 0 || CBlock::GetBlockChainIdByHash(hashPrevBlock) != CBlock::GetBlockChainIdByHash(hashBlock))
    {
        StdLog("CHdexDB", "Update dext order cache: Prev block is null, prev: %s, block: %s", hashPrevBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str());
        return true;
    }

    SHP_CACHE_BLOCK_DEX_ORDER ptrCacheBlockDexOrder = nullptr;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashPrevBlock);

    auto it = mapCacheForkLastDexOrder.find(nChainId);
    if (it != mapCacheForkLastDexOrder.end() && it->second && it->second->GetLastBlock() == hashPrevBlock)
    {
        ptrCacheBlockDexOrder = it->second;
    }
    else
    {
        if (it != mapCacheForkLastDexOrder.end())
        {
            if (it->second)
            {
                AddBlockDexOrderCache(it->second->GetLastBlock(), it->second);
            }
            mapCacheForkLastDexOrder.erase(it);
        }

        ptrCacheBlockDexOrder = LoadBlockDexOrderCache(hashPrevBlock);
        if (!ptrCacheBlockDexOrder)
        {
            StdLog("CHdexDB", "Update dext order cache: Load block dex order cache fail, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
        mapCacheForkLastDexOrder.insert(std::make_pair(nChainId, ptrCacheBlockDexOrder));

        RemoveBlockDexOrderCache(hashPrevBlock);
    }

    std::map<uint256, uint256> mapCompPriceCache; // key: coin pair hash, value: complete price
    if (!GetCoinPairCompletePriceCache(hashBlock, mapCompPriceCache))
    {
        StdLog("CHdexDB", "Update dext order cache: Get coin pair complete price fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    for (const auto& kv : mapCoinPairCompletePrice)
    {
        mapCompPriceCache[kv.first] = kv.second;
    }

    for (const auto& kv : mapAddNewOrder)
    {
        // key: order header, value: 1: order body, 2: complete amount, 3: complete count
        // std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64>> mapAddNewOrder;
        const CDestination& destOrder = kv.first.GetOrderAddress();
        const uint256& hashCoinPair = kv.first.GetCoinPairHash();
        const uint8 nOwnerCoinFlag = kv.first.GetOwnerCoinFlag();
        const uint64 nOrderNumber = kv.first.GetOrderNumber();
        const CDexOrderBody& dexOrder = std::get<0>(kv.second);
        const uint256& nCompAmount = std::get<1>(kv.second);
        const uint64 nCompCount = std::get<2>(kv.second);

        uint256 hashDexOrder = kv.first.GetDexOrderHash();

        uint256 nPrevCompletePrice;
        auto mt = mapCompPriceCache.find(hashCoinPair);
        if (mt != mapCompPriceCache.end())
        {
            nPrevCompletePrice = mt->second;
        }

        if (!ptrCacheBlockDexOrder->AddDexOrderCache(hashDexOrder, destOrder, nOrderNumber, dexOrder, nChainId, hashBlock, nCompAmount, nCompCount, nPrevCompletePrice))
        {
            StdLog("CHdexDB", "Update dext order cache: Add dex order cache fail, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
    }
    for (const auto& kv : mapAddBlockProveOrder)
    {
        //key: order header, value: 1: order body, 2: complete amount, 3: complete count, 4: at chainid, 5: at block
        //std::map<CDexOrderHeader, std::tuple<CDexOrderBody, uint256, uint64, CChainId, uint256>> mapAddBlockProveOrder;
        const CDestination& destOrder = kv.first.GetOrderAddress();
        const uint256& hashCoinPair = kv.first.GetCoinPairHash();
        const uint8 nOwnerCoinFlag = kv.first.GetOwnerCoinFlag();
        const uint64 nOrderNumber = kv.first.GetOrderNumber();
        const CDexOrderBody& dexOrder = std::get<0>(kv.second);
        const uint256& nCompAmount = std::get<1>(kv.second);
        const uint64 nCompCount = std::get<2>(kv.second);
        const CChainId nAtChainId = std::get<3>(kv.second);
        const uint256& hashAtBlock = std::get<4>(kv.second);

        uint256 hashDexOrder = CDexOrderHeader::GetDexOrderHashStatic(nAtChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber);

        uint256 nPrevCompletePrice;
        auto mt = mapCompPriceCache.find(hashCoinPair);
        if (mt != mapCompPriceCache.end())
        {
            nPrevCompletePrice = mt->second;
        }

        if (!ptrCacheBlockDexOrder->AddDexOrderCache(hashDexOrder, destOrder, nOrderNumber, dexOrder, nAtChainId, hashAtBlock, nCompAmount, nCompCount, nPrevCompletePrice))
        {
            StdLog("CHdexDB", "Update dext order cache: Add dex order cache fail, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
    }

    for (const auto& kv : mapUpdateCompOrder)
    {
        const uint256& hashDexOrder = kv.first;
        const uint256& hashCoinPair = std::get<0>(kv.second);
        const uint256& nCompleteAmount = std::get<1>(kv.second);
        const uint64 nCompleteCount = std::get<2>(kv.second);

        if (!ptrCacheBlockDexOrder->UpdateCompleteOrderCache(hashCoinPair, hashDexOrder, nCompleteAmount, nCompleteCount))
        {
            StdLog("CHdexDB", "Update dext order cache: Update complete order cache fail, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
    }

    for (const auto& kv : mapPeerProveLastBlock)
    {
        ptrCacheBlockDexOrder->UpdatePeerProveLastBlock(kv.first, kv.second);
    }

    for (const auto& kv : mapCoinPairCompletePrice)
    {
        ptrCacheBlockDexOrder->UpdateCompletePrice(kv.first, kv.second);
    }

    ptrCacheBlockDexOrder->SetLastBlock(hashBlock);
    return true;
}

bool CHdexDB::GetBlockProveDb(const uint256& hashBlock, const CBlockStorageProve& proveCrosschain, std::map<CChainId, CBlockProve>& mapBlockProve)
{
    CBlockProve blockProveReserve;

    blockProveReserve.hashBlock = hashBlock;
    blockProveReserve.btAggSigBitmap = proveCrosschain.btAggSigBitmap;
    blockProveReserve.btAggSigData = proveCrosschain.btAggSigData;
    blockProveReserve.hashRefBlock = proveCrosschain.hashRefBlock;
    blockProveReserve.vRefBlockMerkleProve = proveCrosschain.vRefBlockMerkleProve;
    blockProveReserve.hashPrevBlock = proveCrosschain.hashPrevBlock;
    blockProveReserve.vPrevBlockMerkleProve = proveCrosschain.vPrevBlockMerkleProve;

    const uint256& hashRefBlock = proveCrosschain.hashRefBlock;
    for (const auto& kv : proveCrosschain.mapCrossProve)
    {
        const CChainId nRecvChainId = kv.first;
        const CBlockCrosschainProve& crossProve = kv.second.first;
        const MERKLE_PROVE_DATA& merkleProveData = kv.second.second;

        CBlockProve blockProve = blockProveReserve;

        blockProve.proveCrosschain = crossProve;
        blockProve.vCrosschainMerkleProve = merkleProveData;

        mapBlockProve.insert(std::make_pair(nRecvChainId, blockProve));
    }

    uint256 hashAtBlock = proveCrosschain.hashPrevBlock;
    while (hashAtBlock != 0 && blockProveReserve.vPrevBlockCcProve.size() < 1024)
    {
        CBlockStorageProve proveCrossPrev;
        if (!GetBlockCrosschainProveDb(hashAtBlock, proveCrossPrev))
        {
            StdLog("CHdexDB", "Get block prove db: Get block crosschain prove fail, at block: %s", hashAtBlock.GetBhString().c_str());
            return false;
        }
        if (!proveCrossPrev.btAggSigBitmap.empty())
        {
            // The proof has been extracted
            break;
        }
        // StdDebug("TEST", "Get block prove db: xxxxxxxxxxxxxxxx, cp size: %lu, prev block: %s, prove size: %lu, at block: %s, cc size: %lu",
        //          proveCrossPrev.mapCrossProve.size(), proveCrossPrev.hashPrevBlock.GetBhString().c_str(), proveCrossPrev.vPrevBlockMerkleProve.size(),
        //          hashAtBlock.GetBhString().c_str(), blockProveReserve.vPrevBlockCcProve.size());

        // if (proveCrossPrev.hashPrevBlock != 0)
        // {
        //     if (!CBlock::VerifyBlockMerkleProve(hashAtBlock, proveCrossPrev.vPrevBlockMerkleProve, proveCrossPrev.hashPrevBlock))
        //     {
        //         StdLog("TEST", "Get block prove db: Verify prev block merkle prove fail, at block: %s, merkle prove size: %lu, prev block: %s",
        //                hashAtBlock.GetBhString().c_str(), proveCrossPrev.vPrevBlockMerkleProve.size(), proveCrossPrev.hashPrevBlock.GetBhString().c_str());
        //     }
        // }

        for (const auto& kv : proveCrossPrev.mapCrossProve)
        {
            const CChainId nRecvChainId = kv.first;
            if (mapBlockProve.find(nRecvChainId) == mapBlockProve.end())
            {
                mapBlockProve.insert(std::make_pair(nRecvChainId, blockProveReserve));
                // auto it = mapBlockProve.insert(std::make_pair(nRecvChainId, blockProveReserve)).first;
                // if (it->second.proveCrosschain.GetPrevProveBlock() == 0)
                // {
                //     StdDebug("TEST", "Get block prove db: add prove: prev prove block is 0, recv chainid: %d, block: %s", nRecvChainId, hashBlock.GetBhString().c_str());
                // }
            }
        }

        CBlockPrevProve prevProve;
        if (proveCrossPrev.hashPrevBlock != 0)
        {
            prevProve.hashPrevBlock = proveCrossPrev.hashPrevBlock;
            prevProve.vPrevBlockMerkleProve = proveCrossPrev.vPrevBlockMerkleProve;
        }
        for (auto& kv : mapBlockProve)
        {
            kv.second.vPrevBlockCcProve.push_back(prevProve);
        }
        blockProveReserve.vPrevBlockCcProve.push_back(prevProve);

        for (const auto& kv : proveCrossPrev.mapCrossProve)
        {
            const CChainId nRecvChainId = kv.first;
            const CBlockCrosschainProve& crossProve = kv.second.first;
            const MERKLE_PROVE_DATA& merkleProveData = kv.second.second;

            if (!crossProve.IsNull() && !merkleProveData.empty())
            {
                auto it = mapBlockProve.find(nRecvChainId);
                if (it != mapBlockProve.end() && it->second.vPrevBlockCcProve.size() > 0)
                {
                    auto& ccProve = it->second.vPrevBlockCcProve[it->second.vPrevBlockCcProve.size() - 1];
                    ccProve.proveCrosschain = crossProve;
                    ccProve.vCrosschainMerkleProve = merkleProveData;
                }
                else
                {
                    StdLog("CHdexDB", "Get block prove db: add prove: Block prove fill fail, recv chainid: %d, block: %s", nRecvChainId, hashBlock.GetBhString().c_str());
                    return false;
                }
            }
        }

        hashAtBlock = proveCrossPrev.hashPrevBlock;
    }

    // for (auto& kv : mapBlockProve)
    // {
    //     CBlockProve& blockProve = kv.second;
    //     if (!blockProve.vPrevBlockCcProve.empty())
    //     {
    //         for (int64 i = blockProve.vPrevBlockCcProve.size() - 1; i >= 0; --i)
    //         {
    //             if (!blockProve.vPrevBlockCcProve[i].proveCrosschain.IsNull())
    //             {
    //                 break;
    //             }
    //             blockProve.vPrevBlockCcProve.erase(blockProve.vPrevBlockCcProve.begin() + i);
    //             StdDebug("TEST", "Get block prove db: Erase cc prove, index: %d", i);
    //         }
    //     }
    // }
    return true;
}

bool CHdexDB::AddSendChainProveLastBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& nLastProveBlock)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK << BSwap32(nRecvChainId) << BSwap32(nSendChainId);
    ssValue << nLastProveBlock;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CHdexDB::GetSendChainProveLastBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, uint256& nLastProveBlock)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK << BSwap32(nRecvChainId) << BSwap32(nSendChainId);

    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> nLastProveBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::ListSendChainProveLastBlockDb(const CChainId nRecvChainId, std::map<CChainId, uint256>& mapSendLastBlock)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            uint8 nExtKey;
            uint8 nKeyType;
            CChainId nRecvChainIdDb;
            CChainId nSendChainIdDb;

            ssKey >> nExtKey >> nKeyType >> nRecvChainIdDb >> nSendChainIdDb;
            nRecvChainIdDb = BSwap32(nRecvChainIdDb);
            nSendChainIdDb = BSwap32(nSendChainIdDb);

            if (nKeyType == DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK && nRecvChainIdDb == nRecvChainId)
            {
                uint256 nLastProveBlock;
                ssValue >> nLastProveBlock;

                mapSendLastBlock[nSendChainIdDb] = nLastProveBlock;
                return true;
            }
        }
        catch (std::exception& e)
        {
            mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK << BSwap32(nRecvChainId);
    ssKeyPrefix << DB_HDEX_KEY_TYPE_EXT_SEND_CHAIN_LAST_PROVE_BLOCK << BSwap32(nRecvChainId);

    return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CHdexDB::AddRecvCrosschainProveDb(const CChainId nRecvChainId, const CBlockProve& blockProve)
{
    const CChainId nSendChainId = CBlock::GetBlockChainIdByHash(blockProve.hashBlock);
    const uint256 hashFirstPrevBlock = blockProve.GetFirstPrevBlockHash();
    if (hashFirstPrevBlock == 0)
    {
        StdLog("CHdexDB", "Add block crosschain prove db: prev prove block is 0, send chainid: %d, recv chainid: %d, prove block: %s",
               nSendChainId, nRecvChainId, blockProve.hashBlock.GetBhString().c_str());
        return false;
    }

    std::vector<uint256> vAtBlockHash;
    blockProve.GetBlockHashList(vAtBlockHash);

    for (const auto& hashAtBlock : vAtBlockHash)
    {
        uint256 hashFirstBlock;
        if (GetLinkFirstPrevBlock(nRecvChainId, nSendChainId, hashAtBlock, hashFirstBlock))
        {
            return true;
        }
    }

    if (!AddRecvCrosschainProveDb(nRecvChainId, nSendChainId, hashFirstPrevBlock, blockProve))
    {
        StdLog("CHdexDB", "Add block crosschain prove db: Add recv crosschain prove fail, recv chainid: %d, send chainid: %d, first prev block: %s, prove block: %s",
               nRecvChainId, nSendChainId, hashFirstPrevBlock.GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
        return false;
    }

    for (const auto& hashAtBlock : vAtBlockHash)
    {
        if (!AddLinkFirstPrevBlock(nRecvChainId, nSendChainId, hashAtBlock, hashFirstPrevBlock))
        {
            StdLog("CHdexDB", "Add block crosschain prove db: Add list first prev block fail, recv chainid: %d, send chainid: %d, first prev block: %s, prove block: %s",
                   nRecvChainId, nSendChainId, hashFirstPrevBlock.GetBhString().c_str(), blockProve.hashBlock.GetBhString().c_str());
            return false;
        }
    }

    uint256 nLastProveBlock;
    if (!GetSendChainProveLastBlockDb(nRecvChainId, nSendChainId, nLastProveBlock))
    {
        if (!AddSendChainProveLastBlockDb(nRecvChainId, nSendChainId, hashFirstPrevBlock))
        {
            StdLog("CHdexDB", "Add block crosschain prove db: Add send chain prove last block fail, recv chainid: %d, send chainid: %d, first prev block: %s, prove block: %s",
                   nRecvChainId, nSendChainId, hashFirstPrevBlock, blockProve.hashBlock.GetBhString().c_str());
            return false;
        }
    }
    return true;
}

bool CHdexDB::AddRecvCrosschainProveDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashFirstPrevBlock, const CBlockProve& blockProve)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE << BSwap32(nRecvChainId) << BSwap32(nSendChainId) << hashFirstPrevBlock;
    ssValue << blockProve;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CHdexDB::GetRecvCrosschainProveDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashFirstPrevBlock, CBlockProve& blockProve)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE << BSwap32(nRecvChainId) << BSwap32(nSendChainId) << hashFirstPrevBlock;

    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> blockProve;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CHdexDB::ListRecvCrosschainProveDb(const CChainId nRecvChainId, std::vector<std::tuple<CChainId, uint256, CBlockProve>>& vRecvCrossProve)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            uint8 nExtKey;
            uint8 nKeyType;
            CChainId nRecvChainIdDb;
            CChainId nSendChainIdDb;
            uint256 hashFirstPrevBlockDb;

            ssKey >> nExtKey >> nKeyType >> nRecvChainIdDb >> nSendChainIdDb >> hashFirstPrevBlockDb;
            nRecvChainIdDb = BSwap32(nRecvChainIdDb);
            nSendChainIdDb = BSwap32(nSendChainIdDb);

            if (nKeyType == DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE && nRecvChainIdDb == nRecvChainId)
            {
                CBlockProve blockProve;
                ssValue >> blockProve;

                vRecvCrossProve.push_back(std::tuple(nSendChainIdDb, hashFirstPrevBlockDb, blockProve));
                return true;
            }
        }
        catch (std::exception& e)
        {
            mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE << BSwap32(nRecvChainId);
    ssKeyPrefix << DB_HDEX_KEY_TYPE_EXT_RECV_CROSSCHAIN_PROVE << BSwap32(nRecvChainId);

    return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
}

bool CHdexDB::GetCrosschainProveForPrevBlockDb(const CChainId nRecvChainId, const CChainId nSendChainId, const uint256& hashLastProveBlock, const uint32 nLastHeight, const uint16 nLastSlot, CBlockProve& blockProve)
{
    if (!GetRecvCrosschainProveDb(nRecvChainId, nSendChainId, hashLastProveBlock, blockProve))
    {
        return false;
    }

    const uint32 nProveBlockHeight = CBlock::GetBlockHeightByHash(blockProve.hashBlock);
    const uint16 nProveBlockSlot = CBlock::GetBlockSlotByHash(blockProve.hashBlock);
    if (nProveBlockHeight > nLastHeight || (nProveBlockHeight == nLastHeight && nProveBlockSlot > nLastSlot))
    {
        StdDebug("CHdexDB", "Get crosschain prove for prev block db: height out, prove height: %d, last height: %d", nProveBlockHeight, nLastHeight);
        return false;
    }

    // std::vector<CBlockProve> vProve;
    // uint256 hashPrevBlock = hashLastProveBlock;
    // while (true)
    // {
    //     CBlockProve getProve;
    //     if (!GetRecvCrosschainProveDb(nRecvChainId, nSendChainId, hashPrevBlock, getProve))
    //     {
    //         break;
    //     }

    //     const uint32 nProveBlockHeight = CBlock::GetBlockHeightByHash(getProve.hashBlock);
    //     const uint16 nProveBlockSlot = CBlock::GetBlockSlotByHash(getProve.hashBlock);
    //     if (nProveBlockHeight > nLastHeight || (nProveBlockHeight == nLastHeight && nProveBlockSlot > nLastSlot))
    //     {
    //         StdDebug("CHdexDB", "Get crosschain prove for prev block db: height out, prove height: %d, last height: %d", nProveBlockHeight, nLastHeight);
    //         break;
    //     }

    //     uint256 hashFirstPrevBlock = getProve.hashPrevBlock;
    //     if (!getProve.vPrevBlockCcProve.empty())
    //     {
    //         hashFirstPrevBlock = getProve.vPrevBlockCcProve[getProve.vPrevBlockCcProve.size() - 1].hashPrevBlock;
    //     }
    //     if (hashFirstPrevBlock != hashPrevBlock)
    //     {
    //         StdDebug("CHdexDB", "Get crosschain prove for prev block db: prove block discontinuous, first prev block: %s, prev block: %s, last prove block: %s",
    //                  hashFirstPrevBlock.GetBhString().c_str(), hashPrevBlock.GetBhString().c_str(), hashLastProveBlock.GetBhString().c_str());
    //         break;
    //     }

    //     vProve.push_back(getProve);
    //     hashPrevBlock = getProve.hashBlock;
    // }
    // if (vProve.empty())
    // {
    //     return false;
    // }

    // blockProve = vProve[vProve.size() - 1];
    // for (int64 i = vProve.size() - 2; i >= 0; i--)
    // {
    //     const CBlockProve& proveCurr = vProve[i];
    //     blockProve.vPrevBlockCcProve.push_back(CBlockPrevProve(proveCurr.hashPrevBlock, proveCurr.vPrevBlockMerkleProve, proveCurr.proveCrosschain, proveCurr.vCrosschainMerkleProve));
    //     if (!proveCurr.vPrevBlockCcProve.empty())
    //     {
    //         blockProve.vPrevBlockCcProve.insert(blockProve.vPrevBlockCcProve.end(), proveCurr.vPrevBlockCcProve.begin(), proveCurr.vPrevBlockCcProve.end());
    //     }
    // }
    return true;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "matchdex.h"

#include "block.h"
#include "param.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

///////////////////////////////////
// CDexOrderKey

bool operator==(const CDexOrderKey& a, const CDexOrderKey& b)
{
    if (a.nPrice != b.nPrice)
    {
        StdLog("CDexOrderKey", "Compare: nPrice error, a.nPrice: %s, b.nPrice: %s", CoinToTokenBigFloat(a.nPrice).c_str(), CoinToTokenBigFloat(b.nPrice).c_str());
        return false;
    }
    if (a.nHeight != b.nHeight)
    {
        StdLog("CDexOrderKey", "Compare: nHeight error, a.nHeight: %d, b.nHeight: %d", a.nHeight, b.nHeight);
        return false;
    }
    if (a.nSlot != b.nSlot)
    {
        StdLog("CDexOrderKey", "Compare: nSlot error, a.nSlot: %d, b.nSlot: %d", a.nSlot, b.nSlot);
        return false;
    }
    if (a.hashOrderRandom != b.hashOrderRandom)
    {
        StdLog("CDexOrderKey", "Compare: hashOrderRandom error, a.hashOrderRandom: %s, b.hashOrderRandom: %s", a.hashOrderRandom.ToString().c_str(), b.hashOrderRandom.ToString().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
// CDexOrderValue

bool operator==(const CDexOrderValue& a, const CDexOrderValue& b)
{
    if (a.destOrder != b.destOrder)
    {
        StdLog("CDexOrderValue", "Compare: destOrder error, a.destOrder: %s, b.destOrder: %s", a.destOrder.ToString().c_str(), b.destOrder.ToString().c_str());
        return false;
    }
    if (a.nOrderNumber != b.nOrderNumber)
    {
        StdLog("CDexOrderValue", "Compare: nOrderNumber error, a.nOrderNumber: %lu, b.nOrderNumber: %lu", a.nOrderNumber, b.nOrderNumber);
        return false;
    }
    if (a.nOrderAmount != b.nOrderAmount)
    {
        StdLog("CDexOrderKey", "Compare: nOrderAmount error, a.nOrderAmount: %s, b.nOrderAmount: %s", CoinToTokenBigFloat(a.nOrderAmount).c_str(), CoinToTokenBigFloat(b.nOrderAmount).c_str());
        return false;
    }
    if (a.nCompleteAmount != b.nCompleteAmount)
    {
        StdLog("CDexOrderKey", "Compare: nOrderAmount error, a.nOrderAmount: %s, b.nOrderAmount: %s", CoinToTokenBigFloat(a.nOrderAmount).c_str(), CoinToTokenBigFloat(b.nOrderAmount).c_str());
        return false;
    }
    if (a.nCompleteCount != b.nCompleteCount)
    {
        StdLog("CDexOrderValue", "Compare: nCompleteCount error, a.nCompleteCount: %lu, b.nCompleteCount: %lu", a.nCompleteCount, b.nCompleteCount);
        return false;
    }
    if (a.nOrderAtChainId != b.nOrderAtChainId)
    {
        StdLog("CDexOrderValue", "Compare: nOrderAtChainId error, a.nOrderAtChainId: %u, b.nOrderAtChainId: %u", a.nOrderAtChainId, b.nOrderAtChainId);
        return false;
    }
    if (a.hashOrderAtBlock != b.hashOrderAtBlock)
    {
        StdLog("CDexOrderValue", "Compare: hashOrderAtBlock error, a.hashOrderAtBlock: %s, b.hashOrderAtBlock: %s", a.hashOrderAtBlock.GetBhString().c_str(), b.hashOrderAtBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
// CCoinDexPair

bool CCoinDexPair::AddOrder(const uint256& hashDexOrder, const std::string& strCoinSymbolOwner, const CDestination& destOrder, const uint64 nOrderNumber, const uint256& nOrderAmount, const uint256& nOrderPrice, const uint256& nCompleteAmount,
                            const uint64 nCompleteCount, const CChainId nOrderAtChainId, const uint256& hashOrderAtBlock, const uint32 nHeight, const uint16 nSlot, const uint256& hashOrderRandom)
{
    CDexOrderKey keyOrder(nOrderPrice, nHeight, nSlot, hashOrderRandom);
    CDexOrderValue* pOrderValue = nullptr;
    uint8 nOrderType = 0;
    if (strCoinSymbolOwner == strCoinSymbolSell)
    {
        pOrderValue = &mapSellOrder[keyOrder];
        nOrderType = CDP_ORDER_TYPE_SELL;
    }
    else if (strCoinSymbolOwner == strCoinSymbolBuy)
    {
        pOrderValue = &mapBuyOrder[keyOrder];
        nOrderType = CDP_ORDER_TYPE_BUY;
    }
    else
    {
        StdLog("CCoinDexPair", "Add Order: Coin symbol owner error, Coin symbol owner: %s, symbol sell: %s, symbol buy: %s",
               strCoinSymbolOwner.c_str(), strCoinSymbolSell.c_str(), strCoinSymbolBuy.c_str());
        return false;
    }

    pOrderValue->destOrder = destOrder;
    pOrderValue->nOrderNumber = nOrderNumber;
    pOrderValue->nOrderAmount = nOrderAmount;
    pOrderValue->nCompleteAmount = nCompleteAmount;
    pOrderValue->nCompleteCount = nCompleteCount;
    pOrderValue->nOrderAtChainId = nOrderAtChainId;
    pOrderValue->hashOrderAtBlock = hashOrderAtBlock;

    mapDexOrderIndex[hashDexOrder] = std::make_pair(keyOrder, nOrderType); // key: dex order hash, value: 1: order key, 2: order type

    if (nSellChainId != 0 && nSellChainId == nBuyChainId && hashOrderAtBlock != 0)
    {
        uint64 nHeightSlot = CDexOrderKey::GetHeightSlotStatic(CBlock::GetBlockHeightByHash(hashOrderAtBlock), CBlock::GetBlockSlotByHash(hashOrderAtBlock));
        if (nHeightSlot > nMatchHeightSlot)
        {
            nMatchHeightSlot = nHeightSlot;
        }
    }
    return true;
}

void CCoinDexPair::SetPrevCompletePrice(const uint256& nPrevPrice)
{
    nPrevCompletePrice = nPrevPrice;
}

bool CCoinDexPair::UpdateCompleteOrder(const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount)
{
    auto it = mapDexOrderIndex.find(hashDexOrder);
    if (it != mapDexOrderIndex.end())
    {
        const CDexOrderKey& keyOrder = it->second.first;
        const uint8 nOrderType = it->second.second;
        if (nOrderType == CDP_ORDER_TYPE_SELL)
        {
            auto mt = mapSellOrder.find(keyOrder);
            if (mt != mapSellOrder.end())
            {
                mt->second.nCompleteAmount = nCompleteAmount;
                mt->second.nCompleteCount = nCompleteCount;
                if (mt->second.GetSurplusAmount() == 0)
                {
                    mapSellOrder.erase(mt);
                    mapDexOrderIndex.erase(it);
                }
                return true;
            }
            StdLog("CCoinDexPair", "Update complete order: Sell order find fail, dex order hash: %s", hashDexOrder.ToString().c_str());
        }
        else if (nOrderType == CDP_ORDER_TYPE_BUY)
        {
            auto mt = mapBuyOrder.find(keyOrder);
            if (mt != mapBuyOrder.end())
            {
                mt->second.nCompleteAmount = nCompleteAmount;
                mt->second.nCompleteCount = nCompleteCount;
                if (mt->second.GetSurplusAmount() == 0)
                {
                    mapBuyOrder.erase(mt);
                    mapDexOrderIndex.erase(it);
                }
                return true;
            }
            StdLog("CCoinDexPair", "Update complete order: Buy order find fail, dex order hash: %s", hashDexOrder.ToString().c_str());
        }
    }
    return false;
}

void CCoinDexPair::UpdatePeerCoinPairLastBlock(const uint256& hashLastProveBlock)
{
    if (hashLastProveBlock != 0)
    {
        uint64 nHeightSlot = CDexOrderKey::GetHeightSlotStatic(CBlock::GetBlockHeightByHash(hashLastProveBlock), CBlock::GetBlockSlotByHash(hashLastProveBlock));
        if (nHeightSlot > nMatchHeightSlot)
        {
            nMatchHeightSlot = nHeightSlot;
        }
    }
}

bool CCoinDexPair::MatchOrder(CMatchOrderResult& matchResult)
{
    std::set<uint64> setMatchHeightSlot;
    std::map<CDexOrderKey, uint256, CustomCompareSellOrder> mapSellCompleteAmount;
    std::map<CDexOrderKey, uint256, CustomCompareBuyOrder> mapBuyCompleteAmount;

    auto funcMatch = [&](const uint64 nMatchEndHeightSlot) {
        for (const auto& kv : mapSellOrder)
        {
            const CDexOrderKey& keyDexOrderSell = kv.first;
            const CDexOrderValue& valueDexOrderSell = kv.second;

            // nMatchEndHeightSlot == 0: all in some chain
            if (nMatchEndHeightSlot != 0 && keyDexOrderSell.GetHeightSlotValue() > nMatchEndHeightSlot)
            {
                continue;
            }

            auto mt = mapSellCompleteAmount.find(keyDexOrderSell);
            if (mt == mapSellCompleteAmount.end())
            {
                mt = mapSellCompleteAmount.insert(std::make_pair(keyDexOrderSell, valueDexOrderSell.nCompleteAmount)).first;
            }
            uint256& nSellCompAmount = mt->second;

            uint256 nCalcSellOrderAmount;
            if (valueDexOrderSell.nOrderAmount > nSellCompAmount)
            {
                nCalcSellOrderAmount = valueDexOrderSell.nOrderAmount - nSellCompAmount;
            }
            else
            {
                continue;
            }

            for (auto it = mapBuyOrder.begin(); it != mapBuyOrder.end(); ++it)
            {
                const CDexOrderKey& keyDexOrderBuy = it->first;
                const CDexOrderValue& valueDexOrderBuy = it->second;

                if (nMatchEndHeightSlot != 0 && keyDexOrderBuy.GetHeightSlotValue() > nMatchEndHeightSlot)
                {
                    continue;
                }

                auto nt = mapBuyCompleteAmount.find(keyDexOrderBuy);
                if (nt == mapBuyCompleteAmount.end())
                {
                    nt = mapBuyCompleteAmount.insert(std::make_pair(keyDexOrderBuy, valueDexOrderBuy.nCompleteAmount)).first;
                }
                uint256& nBuyCompAmount = nt->second;

                uint256 nCalcBuyOrderAmount;
                if (valueDexOrderBuy.nOrderAmount > nBuyCompAmount)
                {
                    nCalcBuyOrderAmount = valueDexOrderBuy.nOrderAmount - nBuyCompAmount;
                }
                else
                {
                    continue;
                }
                if (keyDexOrderSell.nPrice > keyDexOrderBuy.nPrice)
                {
                    break;
                }

                uint256 nCompletePrice = CMatchTools::CalcCompletePrice(keyDexOrderBuy.nPrice, keyDexOrderSell.nPrice, nPrevCompletePrice);
                uint256 nSellCompleteAmount = nCalcSellOrderAmount;
                uint256 nBuyCompleteAmount = CMatchTools::CalcBuyAmount(nSellCompleteAmount, nCompletePrice, nSellPriceAnchor);
                if (nBuyCompleteAmount > nCalcBuyOrderAmount)
                {
                    nBuyCompleteAmount = nCalcBuyOrderAmount;
                    nSellCompleteAmount = CMatchTools::CalcSellAmount(nBuyCompleteAmount, nCompletePrice, nSellPriceAnchor);
                }

                CMatchOrderRecord matchOrder;

                matchOrder.destSellOrder = valueDexOrderSell.destOrder;
                matchOrder.nSellOrderNumber = valueDexOrderSell.nOrderNumber;
                matchOrder.nSellCompleteAmount = nSellCompleteAmount;
                matchOrder.nSellCompletePrice = keyDexOrderSell.nPrice;
                matchOrder.nSellOrderAtChainId = valueDexOrderSell.nOrderAtChainId;

                matchOrder.destBuyOrder = valueDexOrderBuy.destOrder;
                matchOrder.nBuyOrderNumber = valueDexOrderBuy.nOrderNumber;
                matchOrder.nBuyCompleteAmount = nBuyCompleteAmount;
                matchOrder.nBuyCompletePrice = keyDexOrderBuy.nPrice;
                matchOrder.nBuyOrderAtChainId = valueDexOrderBuy.nOrderAtChainId;

                matchOrder.nCompletePrice = nCompletePrice;

                matchResult.vMatchOrderRecord.push_back(matchOrder);

                nCalcSellOrderAmount -= nSellCompleteAmount;
                nSellCompAmount += nSellCompleteAmount;
                nBuyCompAmount += nBuyCompleteAmount;
                nPrevCompletePrice = nCompletePrice;

                if (nCalcSellOrderAmount == 0)
                {
                    break;
                }
            }

            if (nCalcSellOrderAmount != 0)
            {
                break;
            }
        }
    };

    for (const auto& kv : mapSellOrder)
    {
        const CDexOrderKey& keyDexOrderSell = kv.first;
        const CDexOrderValue& valueDexOrderSell = kv.second;

        uint64 nHeightSlot = keyDexOrderSell.GetHeightSlotValue();
        if (nMatchHeightSlot != 0 && nHeightSlot > nMatchHeightSlot)
        {
            continue;
        }
        if (valueDexOrderSell.nOrderAmount > valueDexOrderSell.nCompleteAmount)
        {
            if (setMatchHeightSlot.find(nHeightSlot) == setMatchHeightSlot.end())
            {
                setMatchHeightSlot.insert(nHeightSlot);
            }
        }
    }
    for (const auto& kv : mapBuyOrder)
    {
        const CDexOrderKey& keyDexOrderBuy = kv.first;
        const CDexOrderValue& valueDexOrderBuy = kv.second;

        uint64 nHeightSlot = keyDexOrderBuy.GetHeightSlotValue();
        if (nMatchHeightSlot != 0 && nHeightSlot > nMatchHeightSlot)
        {
            continue;
        }
        if (valueDexOrderBuy.nOrderAmount > valueDexOrderBuy.nCompleteAmount)
        {
            if (setMatchHeightSlot.find(nHeightSlot) == setMatchHeightSlot.end())
            {
                setMatchHeightSlot.insert(nHeightSlot);
            }
        }
    }

    for (const uint64 nHeightSlot : setMatchHeightSlot)
    {
        funcMatch(nHeightSlot);
    }

    matchResult.nSellPriceAnchor = nSellPriceAnchor;
    matchResult.strCoinSymbolSell = strCoinSymbolSell;
    matchResult.strCoinSymbolBuy = strCoinSymbolBuy;
    return true;
}

bool operator==(const CCoinDexPair& a, const CCoinDexPair& b)
{
    if (a.strCoinSymbolSell != b.strCoinSymbolSell)
    {
        StdLog("CCoinDexPair", "Compare: strCoinSymbolSell error, a.strCoinSymbolSell: %s, b.strCoinSymbolSell: %s", a.strCoinSymbolSell.c_str(), b.strCoinSymbolSell.c_str());
        return false;
    }
    if (a.strCoinSymbolBuy != b.strCoinSymbolBuy)
    {
        StdLog("CCoinDexPair", "Compare: strCoinSymbolBuy error, a.strCoinSymbolBuy: %s, b.strCoinSymbolBuy: %s", a.strCoinSymbolBuy.c_str(), b.strCoinSymbolBuy.c_str());
        return false;
    }
    if (a.nSellPriceAnchor != b.nSellPriceAnchor)
    {
        StdLog("CCoinDexPair", "Compare: nSellPriceAnchor error, a.nSellPriceAnchor: %s, b.nSellPriceAnchor: %s", CoinToTokenBigFloat(a.nSellPriceAnchor).c_str(), CoinToTokenBigFloat(b.nSellPriceAnchor).c_str());
        return false;
    }
    if (a.nPrevCompletePrice != b.nPrevCompletePrice)
    {
        StdLog("CCoinDexPair", "Compare: nPrevCompletePrice error, a.nPrevCompletePrice: %s, b.nPrevCompletePrice: %s", CoinToTokenBigFloat(a.nPrevCompletePrice).c_str(), CoinToTokenBigFloat(b.nPrevCompletePrice).c_str());
        return false;
    }
    if (a.nMatchHeightSlot != b.nMatchHeightSlot)
    {
        StdLog("CCoinDexPair", "Compare: nMatchHeightSlot error, a.nMatchHeightSlot: %lx, b.nMatchHeightSlot: %lx", a.nMatchHeightSlot, b.nMatchHeightSlot);
        return false;
    }

    if (a.mapSellOrder.size() != b.mapSellOrder.size())
    {
        StdLog("CCoinDexPair", "Compare: mapSellOrder size error, a.mapSellOrder.size: %lu, b.mapSellOrder.size: %lu", a.mapSellOrder.size(), b.mapSellOrder.size());
        return false;
    }
    if (a.mapBuyOrder.size() != b.mapBuyOrder.size())
    {
        StdLog("CCoinDexPair", "Compare: mapBuyOrder size error, a.mapBuyOrder.size: %lu, b.mapBuyOrder.size: %lu", a.mapBuyOrder.size(), b.mapBuyOrder.size());
        return false;
    }

    {
        auto it = a.mapSellOrder.begin();
        auto mt = b.mapSellOrder.begin();
        for (; it != a.mapSellOrder.end() && mt != b.mapSellOrder.end(); ++it, ++mt)
        {
            if (it->first != mt->first)
            {
                StdLog("CCoinDexPair", "Compare: mapSellOrder key error");
                return false;
            }
            if (it->second != mt->second)
            {
                StdLog("CCoinDexPair", "Compare: mapSellOrder value error");
                return false;
            }
        }
    }

    {
        auto it = a.mapBuyOrder.begin();
        auto mt = b.mapBuyOrder.begin();
        for (; it != a.mapBuyOrder.end() && mt != b.mapBuyOrder.end(); ++it, ++mt)
        {
            if (it->first != mt->first)
            {
                StdLog("CCoinDexPair", "Compare: mapBuyOrder key error");
                return false;
            }
            if (it->second != mt->second)
            {
                StdLog("CCoinDexPair", "Compare: mapBuyOrder value error");
                return false;
            }
        }
    }
    return true;
}

///////////////////////////////////
// CMatchDex

bool CMatchDex::AddMatchDexOrder(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder, const CChainId nOrderAtChainId,
                                 const uint256& hashOrderAtBlock, const uint256& nCompAmount, const uint64 nCompCount, const uint256& nPrevCompletePrice)
{
    const uint256 hashCoinPair = CDexOrderHeader::GetCoinPairHashStatic(dexOrder.strCoinSymbolOwner, dexOrder.strCoinSymbolPeer);
    auto it = mapCoinDex.find(hashCoinPair);
    if (it == mapCoinDex.end())
    {
        CChainId nSellChainId, nBuyChainId;
        if (dexOrder.GetSellCoinSymbol() == dexOrder.strCoinSymbolOwner)
        {
            nSellChainId = nOrderAtChainId;
            nBuyChainId = dexOrder.nCoinAtChainIdPeer;
        }
        else
        {
            nSellChainId = dexOrder.nCoinAtChainIdPeer;
            nBuyChainId = nOrderAtChainId;
        }
        it = mapCoinDex.insert(std::make_pair(hashCoinPair, CCoinDexPair(dexOrder.GetSellCoinSymbol(), dexOrder.GetBuyCoinSymbol(), nSellChainId, nBuyChainId, COIN, nPrevCompletePrice))).first;
    }
    else
    {
        if (nPrevCompletePrice != 0)
        {
            it->second.SetPrevCompletePrice(nPrevCompletePrice);
        }
    }

    if (dexOrder.nOrderAmount > nCompAmount)
    {
        const uint32 nHeight = CBlock::GetBlockHeightByHash(hashOrderAtBlock);
        const uint16 nSlot = CBlock::GetBlockSlotByHash(hashOrderAtBlock);
        const uint256 hashOrderRandom = CDexOrderHeader::GetOrderRandomHashStatic(hashOrderAtBlock, hashDexOrder);

        if (!it->second.AddOrder(hashDexOrder, dexOrder.strCoinSymbolOwner, destOrder, nOrderNumber, dexOrder.nOrderAmount, dexOrder.nOrderPrice,
                                 nCompAmount, nCompCount, nOrderAtChainId, hashOrderAtBlock, nHeight, nSlot, hashOrderRandom))
        {
            return false;
        }
    }

    auto& setCoinPair = mapChainIdLinkCoinDexPair[nOrderAtChainId];
    if (setCoinPair.find(hashCoinPair) == setCoinPair.end())
    {
        setCoinPair.insert(hashCoinPair);
    }
    return true;
}

bool CMatchDex::UpdateMatchCompleteOrder(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount)
{
    auto it = mapCoinDex.find(hashCoinPair);
    if (it != mapCoinDex.end())
    {
        return it->second.UpdateCompleteOrder(hashDexOrder, nCompleteAmount, nCompleteCount);
    }
    return false;
}

void CMatchDex::UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock)
{
    auto it = mapChainIdLinkCoinDexPair.find(nPeerChainId);
    if (it != mapChainIdLinkCoinDexPair.end())
    {
        for (const uint256& hashCoinPair : it->second)
        {
            auto mt = mapCoinDex.find(hashCoinPair);
            if (mt != mapCoinDex.end())
            {
                mt->second.UpdatePeerCoinPairLastBlock(hashLastProveBlock);
            }
        }
    }
}

void CMatchDex::UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice)
{
    auto it = mapCoinDex.find(hashCoinPair);
    if (it != mapCoinDex.end())
    {
        it->second.SetPrevCompletePrice(nCompletePrice);
    }
}

bool CMatchDex::ListDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder)
{
    const uint256 hashDexCoinPair = CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolSell, strCoinSymbolBuy);
    const bool fReverse = (CDexOrderHeader::GetSellCoinSymbolStatic(strCoinSymbolSell, strCoinSymbolBuy) != strCoinSymbolSell);

    auto it = mapCoinDex.find(hashDexCoinPair);
    if (it == mapCoinDex.end())
    {
        StdDebug("CMatchDex", "List dex order: Get dex coin pair fail, symbol sell: %s, symbol buy: %s", strCoinSymbolSell.c_str(), strCoinSymbolBuy.c_str());
        realDexOrder.strCoinSymbolSell = strCoinSymbolSell;
        realDexOrder.strCoinSymbolBuy = strCoinSymbolBuy;
        return true;
    }
    const CCoinDexPair& coinDexPair = it->second;

    auto funcGetMsOrder = [&](const CDexOrderKey& key, const CDexOrderValue& value, CMsOrder& msOrder) -> bool {
        if (fReverse)
        {
            msOrder.nPrice = CMatchTools::CalcPeerPrice(key.nPrice, coinDexPair.nSellPriceAnchor);
        }
        else
        {
            msOrder.nPrice = key.nPrice;
        }
        if (value.nOrderAmount > value.nCompleteAmount)
        {
            msOrder.nOrderAmount = value.nOrderAmount - value.nCompleteAmount;
            if (coinDexPair.nSellPriceAnchor == 0)
            {
                msOrder.nDealAmount = 0;
            }
            else
            {
                msOrder.nDealAmount = msOrder.nPrice * msOrder.nOrderAmount / coinDexPair.nSellPriceAnchor;
            }
            msOrder.destOrder = value.destOrder;
            msOrder.nOrderNumber = value.nOrderNumber;
            msOrder.nOriOrderAmount = value.nOrderAmount;
            msOrder.nAtHeight = CBlock::GetBlockHeightByHash(value.hashOrderAtBlock);
            msOrder.nAtSlot = CBlock::GetBlockSlotByHash(value.hashOrderAtBlock);
            return true;
        }
        return false;
    };

    for (const auto& kv : coinDexPair.mapSellOrder)
    {
        const CDexOrderKey& key = kv.first;
        const CDexOrderValue& value = kv.second;

        CMsOrder msOrder;
        if (funcGetMsOrder(key, value, msOrder))
        {
            if (fReverse)
            {
                realDexOrder.vBuy.push_back(msOrder);
                if (realDexOrder.vBuy.size() >= nGetCount)
                {
                    break;
                }
            }
            else
            {
                realDexOrder.vSell.push_back(msOrder);
                if (realDexOrder.vSell.size() >= nGetCount)
                {
                    break;
                }
            }
        }
    }

    for (const auto& kv : coinDexPair.mapBuyOrder)
    {
        const CDexOrderKey& key = kv.first;
        const CDexOrderValue& value = kv.second;

        CMsOrder msOrder;
        if (funcGetMsOrder(key, value, msOrder))
        {
            if (fReverse)
            {
                realDexOrder.vSell.push_back(msOrder);
                if (realDexOrder.vSell.size() >= nGetCount)
                {
                    break;
                }
            }
            else
            {
                realDexOrder.vBuy.push_back(msOrder);
                if (realDexOrder.vBuy.size() >= nGetCount)
                {
                    break;
                }
            }
        }
    }

    if (fReverse)
    {
        realDexOrder.strCoinSymbolSell = coinDexPair.strCoinSymbolBuy;
        realDexOrder.strCoinSymbolBuy = coinDexPair.strCoinSymbolSell;
        realDexOrder.nSellChainId = coinDexPair.nBuyChainId;
        realDexOrder.nBuyChainId = coinDexPair.nSellChainId;
        realDexOrder.nPrevCompletePrice = CMatchTools::CalcPeerPrice(coinDexPair.nPrevCompletePrice, coinDexPair.nSellPriceAnchor);
    }
    else
    {
        realDexOrder.strCoinSymbolSell = coinDexPair.strCoinSymbolSell;
        realDexOrder.strCoinSymbolBuy = coinDexPair.strCoinSymbolBuy;
        realDexOrder.nSellChainId = coinDexPair.nSellChainId;
        realDexOrder.nBuyChainId = coinDexPair.nBuyChainId;
        realDexOrder.nPrevCompletePrice = coinDexPair.nPrevCompletePrice;
    }
    realDexOrder.nSellPriceAnchor = coinDexPair.nSellPriceAnchor;
    realDexOrder.nMaxMatchHeight = CDexOrderKey::GetHeightByHsStatic(coinDexPair.nMatchHeightSlot);
    realDexOrder.nMaxMatchSlot = CDexOrderKey::GetSlotByHsStatic(coinDexPair.nMatchHeightSlot);
    return true;
}

bool CMatchDex::MatchDex(std::map<uint256, CMatchOrderResult>& mapMatchResult)
{
    for (auto& kv : mapCoinDex)
    {
        if (!kv.second.MatchOrder(mapMatchResult[kv.first]))
        {
            return false;
        }
    }
    return true;
}

void CMatchDex::ShowDexOrderList()
{
    for (const auto& kv : mapCoinDex)
    {
        const uint256& hashCoinPair = kv.first;
        const CCoinDexPair& coinDexPair = kv.second;
        const uint32 nMatchHeight = CDexOrderKey::GetHeightByHsStatic(coinDexPair.nMatchHeightSlot);
        const uint16 nMatchSlot = CDexOrderKey::GetSlotByHsStatic(coinDexPair.nMatchHeightSlot);

        StdDebug("MATCHTEST", "++++++++++++++ Coin pair hash: %s ++++++++++++", hashCoinPair.ToString().c_str());
        StdDebug("MATCHTEST", "Coin symbol sell: %s", coinDexPair.strCoinSymbolSell.c_str());
        StdDebug("MATCHTEST", "Coin symbol buy: %s", coinDexPair.strCoinSymbolBuy.c_str());
        StdDebug("MATCHTEST", "Prev complete price: %s", CoinToTokenBigFloat(coinDexPair.nPrevCompletePrice).c_str());
        StdDebug("MATCHTEST", "Match height slot: [%d-%d]", nMatchHeight, nMatchSlot);
        StdDebug("MATCHTEST", "Sell chainid: %d", coinDexPair.nSellChainId);
        StdDebug("MATCHTEST", "Buy chainid: %d", coinDexPair.nBuyChainId);

        for (const auto& kv : coinDexPair.mapSellOrder)
        {
            const CDexOrderKey& key = kv.first;
            const CDexOrderValue& value = kv.second;

            StdDebug("MATCHTEST", "----------- Sell.destOrder: [%d-%d] %s ---------------", key.nHeight, key.nSlot, value.destOrder.ToString().c_str());
            StdDebug("MATCHTEST", "Sell.nPrice: %s", CoinToTokenBigFloat(key.nPrice).c_str());
            StdDebug("MATCHTEST", "Sell.nOrderAmount: %s", CoinToTokenBigFloat(value.nOrderAmount).c_str());
            StdDebug("MATCHTEST", "Sell.nCompleteAmount: %s", CoinToTokenBigFloat(value.nCompleteAmount).c_str());
            StdDebug("MATCHTEST", "Sell.nSurplusAmount: %s", CoinToTokenBigFloat(value.GetSurplusAmount()).c_str());
        }
        for (const auto& kv : coinDexPair.mapBuyOrder)
        {
            const CDexOrderKey& key = kv.first;
            const CDexOrderValue& value = kv.second;

            StdDebug("MATCHTEST", "----------- Buy.destOrder: [%d-%d] %s ---------------", key.nHeight, key.nSlot, value.destOrder.ToString().c_str());
            StdDebug("MATCHTEST", "Buy.nPrice: %s", CoinToTokenBigFloat(key.nPrice).c_str());
            StdDebug("MATCHTEST", "Buy.nOrderAmount: %s", CoinToTokenBigFloat(value.nOrderAmount).c_str());
            StdDebug("MATCHTEST", "Buy.nCompleteAmount: %s", CoinToTokenBigFloat(value.nCompleteAmount).c_str());
            StdDebug("MATCHTEST", "Buy.nSurplusAmount: %s", CoinToTokenBigFloat(value.GetSurplusAmount()).c_str());
        }
    }
}

bool operator==(const CMatchDex& a, const CMatchDex& b)
{
    if (a.mapCoinDex.size() != b.mapCoinDex.size())
    {
        StdLog("CMatchDex", "Compare: mapCoinDex size error, a.mapCoinDex.size: %lu, b.mapCoinDex.size: %lu", a.mapCoinDex.size(), b.mapCoinDex.size());
        return false;
    }
    auto it = a.mapCoinDex.begin();
    auto mt = b.mapCoinDex.begin();
    for (; it != a.mapCoinDex.end() && mt != b.mapCoinDex.end(); ++it, ++mt)
    {
        if (it->first != mt->first)
        {
            StdLog("CMatchDex", "Compare: mapCoinDex key error");
            return false;
        }
        if (it->second != mt->second)
        {
            StdLog("CMatchDex", "Compare: mapCoinDex value error");
            return false;
        }
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_MATCHDEX_H
#define STORAGE_MATCHDEX_H

#include <map>

#include "block.h"
#include "destination.h"
#include "mtbase.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{
namespace storage
{

///////////////////////////////////
// CMatchTools

class CMatchTools
{
public:
    CMatchTools() {}

public:
    static inline uint256 CalcPeerPrice(const uint256& nSelfPrice, const uint256& nPriceAnchor)
    {
        if (nSelfPrice == 0)
        {
            return 0;
        }
        return nPriceAnchor * nPriceAnchor / nSelfPrice;
    }
    static inline uint256 CalcCompletePrice(const uint256& nBuyPrice, const uint256& nSellPrice, const uint256& nPrevCompletePrice)
    {
        if (nSellPrice >= nPrevCompletePrice)
        {
            return nSellPrice;
        }
        if (nBuyPrice >= nPrevCompletePrice)
        {
            return nPrevCompletePrice;
        }
        return nBuyPrice;
    }
    static inline uint256 CalcBuyAmount(const uint256& nSellAmount, const uint256& nCompletePrice, const uint256& nSellPriceAnchor)
    {
        if (nSellPriceAnchor == 0)
        {
            return 0;
        }
        return nSellAmount * nCompletePrice / nSellPriceAnchor;
    }
    static inline uint256 CalcSellAmount(const uint256& nBuyAmount, const uint256& nCompletePrice, const uint256& nSellPriceAnchor)
    {
        if (nCompletePrice == 0)
        {
            return 0;
        }
        return nBuyAmount * nSellPriceAnchor / nCompletePrice;
    }
};

///////////////////////////////////
// CDexOrderKey

class CDexOrderKey
{
public:
    CDexOrderKey()
      : nHeight(0), nSlot(0) {}
    CDexOrderKey(const uint256 nPriceIn, const uint32 nHeightIn, const uint16 nSlotIn, const uint256& hashOrderRandomIn)
      : nPrice(nPriceIn), nHeight(nHeightIn), nSlot(nSlotIn), hashOrderRandom(hashOrderRandomIn) {}

    uint64 GetHeightSlotValue() const
    {
        return GetHeightSlotStatic(nHeight, nSlot);
    }

    static inline uint64 GetHeightSlotStatic(const uint32 nHeight, const uint16 nSlot)
    {
        return (((uint64)nHeight << 32) | nSlot);
    }
    static inline uint32 GetHeightByHsStatic(const uint64 nHeightSlot)
    {
        return (uint32)(nHeightSlot >> 32);
    }
    static inline uint16 GetSlotByHsStatic(const uint64 nHeightSlot)
    {
        return (uint16)(nHeightSlot & 0xFFFF);
    }

    friend bool operator==(const CDexOrderKey& a, const CDexOrderKey& b);
    friend inline bool operator!=(const CDexOrderKey& a, const CDexOrderKey& b)
    {
        return (!(a == b));
    }

public:
    uint256 nPrice;
    uint32 nHeight;
    uint16 nSlot;
    uint256 hashOrderRandom;
};

///////////////////////////////////
// CDexOrderValue

class CDexOrderValue
{
public:
    CDexOrderValue()
      : nOrderNumber(0), nCompleteCount(0) {}
    CDexOrderValue(const CDestination& destOrderIn, const uint64 nOrderNumberIn, const uint256& nOrderAmountIn, const uint256& nCompleteAmountIn,
                   const uint64 nCompleteCountIn, const CChainId nOrderAtChainIdIn, const uint256& hashOrderAtBlockIn)
      : destOrder(destOrderIn), nOrderNumber(nOrderNumberIn), nOrderAmount(nOrderAmountIn), nCompleteAmount(nCompleteAmountIn),
        nCompleteCount(nCompleteCountIn), nOrderAtChainId(nOrderAtChainIdIn), hashOrderAtBlock(hashOrderAtBlockIn) {}

    uint256 GetSurplusAmount() const
    {
        if (nOrderAmount > nCompleteAmount)
        {
            return nOrderAmount - nCompleteAmount;
        }
        return 0;
    }

    friend bool operator==(const CDexOrderValue& a, const CDexOrderValue& b);
    friend inline bool operator!=(const CDexOrderValue& a, const CDexOrderValue& b)
    {
        return (!(a == b));
    }

public:
    CDestination destOrder;
    uint64 nOrderNumber;
    uint256 nOrderAmount;
    uint256 nCompleteAmount;
    uint64 nCompleteCount;
    CChainId nOrderAtChainId;
    uint256 hashOrderAtBlock;
};

///////////////////////////////////
// CustomCompareSellOrder

struct CustomCompareSellOrder
{
    bool operator()(const CDexOrderKey& a, const CDexOrderKey& b) const
    {
        if (a.nPrice < b.nPrice)
        {
            return true;
        }
        else if (a.nPrice == b.nPrice)
        {
            if (a.nHeight < b.nHeight)
            {
                return true;
            }
            else if (a.nHeight == b.nHeight)
            {
                if (a.nSlot < b.nSlot)
                {
                    return true;
                }
                else if (a.nSlot == b.nSlot)
                {
                    if (a.hashOrderRandom < b.hashOrderRandom)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

///////////////////////////////////
// CustomCompareBuyOrder

struct CustomCompareBuyOrder
{
    bool operator()(const CDexOrderKey& a, const CDexOrderKey& b) const
    {
        if (b.nPrice < a.nPrice)
        {
            return true;
        }
        else if (b.nPrice == a.nPrice)
        {
            if (a.nHeight < b.nHeight)
            {
                return true;
            }
            else if (a.nHeight == b.nHeight)
            {
                if (a.nSlot < b.nSlot)
                {
                    return true;
                }
                else if (a.nSlot == b.nSlot)
                {
                    if (a.hashOrderRandom < b.hashOrderRandom)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

///////////////////////////////////
// CCoinDexPair

class CCoinDexPair
{
public:
    CCoinDexPair(const std::string& strCoinSymbolSellIn, const std::string& strCoinSymbolBuyIn,
                 const CChainId nSellChainIdIn, const CChainId nBuyChainIdIn,
                 const uint256& nSellPriceAnchorIn, const uint256& nPrevCompletePriceIn)
      : strCoinSymbolSell(strCoinSymbolSellIn), strCoinSymbolBuy(strCoinSymbolBuyIn), nSellChainId(nSellChainIdIn), nBuyChainId(nBuyChainIdIn),
        nSellPriceAnchor(nSellPriceAnchorIn), nPrevCompletePrice(nPrevCompletePriceIn), nMatchHeightSlot(0) {}

    bool AddOrder(const uint256& hashDexOrder, const std::string& strCoinSymbolOwner, const CDestination& destOrder, const uint64 nOrderNumber, const uint256& nOrderAmount, const uint256& nOrderPrice, const uint256& nCompleteAmount,
                  const uint64 nCompleteCount, const CChainId nOrderAtChainId, const uint256& hashOrderAtBlock, const uint32 nHeight, const uint16 nSlot, const uint256& hashOrderRandom);
    void SetPrevCompletePrice(const uint256& nPrevPrice);
    bool UpdateCompleteOrder(const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount);
    void UpdatePeerCoinPairLastBlock(const uint256& hashLastProveBlock);

    bool MatchOrder(CMatchOrderResult& matchResult);

    friend bool operator==(const CCoinDexPair& a, const CCoinDexPair& b);
    friend inline bool operator!=(const CCoinDexPair& a, const CCoinDexPair& b)
    {
        return (!(a == b));
    }

public:
    enum
    {
        CDP_ORDER_TYPE_SELL = 1,
        CDP_ORDER_TYPE_BUY = 2
    };

    const std::string strCoinSymbolSell; // min symbol
    const std::string strCoinSymbolBuy;  // max symbol
    const CChainId nSellChainId;
    const CChainId nBuyChainId;
    const uint256 nSellPriceAnchor; // 1 sell token of coin amount
    uint256 nPrevCompletePrice;     // sell price, 1 sell token == n buy coin
    uint64 nMatchHeightSlot;

    std::map<CDexOrderKey, CDexOrderValue, CustomCompareSellOrder> mapSellOrder;
    std::map<CDexOrderKey, CDexOrderValue, CustomCompareBuyOrder> mapBuyOrder;

    std::map<uint256, std::pair<CDexOrderKey, uint8>> mapDexOrderIndex; // key: dex order hash, value: 1: order key, 2: order type, 1-sell, 2-buy
};

///////////////////////////////////
// CMatchDex

class CMatchDex
{
public:
    CMatchDex() {}

    bool AddMatchDexOrder(const uint256& hashDexOrder, const CDestination& destOrder, const uint64 nOrderNumber, const CDexOrderBody& dexOrder, const CChainId nOrderAtChainId,
                          const uint256& hashOrderAtBlock, const uint256& nCompAmount, const uint64 nCompCount, const uint256& nPrevCompletePrice);
    bool UpdateMatchCompleteOrder(const uint256& hashCoinPair, const uint256& hashDexOrder, const uint256& nCompleteAmount, const uint64 nCompleteCount);
    void UpdatePeerProveLastBlock(const CChainId nPeerChainId, const uint256& hashLastProveBlock);
    void UpdateCompletePrice(const uint256& hashCoinPair, const uint256& nCompletePrice);
    bool ListDexOrder(const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder);

    bool MatchDex(std::map<uint256, CMatchOrderResult>& mapMatchResult);

    void ShowDexOrderList();

    friend bool operator==(const CMatchDex& a, const CMatchDex& b);
    friend inline bool operator!=(const CMatchDex& a, const CMatchDex& b)
    {
        return (!(a == b));
    }

public:
    std::map<uint256, CCoinDexPair> mapCoinDex;                      // key: coin dex pair hash
    std::map<CChainId, std::set<uint256>> mapChainIdLinkCoinDexPair; // key: at chain id, value: coin dex pair hash
};
typedef std::shared_ptr<CMatchDex> SHP_MATCH_DEX;
#define MAKE_SHARED_MATCH_DEX std::make_shared<CMatchDex>

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_MATCHDEX_H

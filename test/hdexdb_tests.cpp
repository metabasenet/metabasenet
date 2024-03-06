// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "hdexdb.h"

#include <boost/test/unit_test.hpp>

#include "base_tests.h"
#include "crypto.h"
#include "test_big.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet;
using namespace metabasenet::storage;

//./build-release/test/test_big --log_level=all --run_test=hdexdb_tests/basetest
//./build-release/test/test_big --log_level=all --run_test=hdexdb_tests/dexordertest

BOOST_FIXTURE_TEST_SUITE(hdexdb_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(basetest)
{
    //InitLogDir();
    NointLogOut();
    StdDebug("TEST", "Start base test!");

    std::string fullpath = GetOutPath("hdexdb_tests");
    if (boost::filesystem::exists(fullpath))
    {
        boost::filesystem::remove_all(fullpath);
    }
    boost::filesystem::create_directories(fullpath);

    CHdexDB hdb;
    BOOST_CHECK(hdb.Initialize(fullpath));

    const CChainId nChainIdOwner = 201;
    const CChainId nChainIdPeer = 202;
    const uint256 hashFork = CBlock::CreateBlockHash(nChainIdOwner, 0, 0, uint256(123));
    const uint256 hashPrevBlock = 0;    //CBlock::CreateBlockHash(201, 5, 0, uint256(777));
    const uint256 hashBlock = hashFork; //CBlock::CreateBlockHash(201, 6, 0, uint256(888));
    std::map<CDexOrderHeader, CDexOrderBody> mapDexOrder;
    std::map<uint256, uint256> mapCoinPairCompletePrice;
    std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>> mapCompDexOrderRecord;
    std::map<CChainId, CBlockProve> mapBlockProve;

    const CDestination destOrder("0x5bc5c1726286ff0a8006b19312ca307210e0e658");
    const CDestination destPeer("0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5");
    const std::string strSymbolOwner = "THAH";
    const std::string strSymbolPeer = "TABC";
    const uint64 nOrderNumber = 1;
    const uint256 nOrderAmount = 555;
    const uint256 nOrderPrice = 666;
    const uint256 nCompleteAmount = 444;
    const uint256 nCompletePrice = 333;

    const CDexOrderHeader orderHeader(nChainIdOwner, destOrder, strSymbolOwner, strSymbolPeer, nOrderNumber);
    const uint256 hashCoinPair = orderHeader.GetCoinPairHash();

    mapDexOrder[orderHeader] = CDexOrderBody(strSymbolOwner, strSymbolPeer, nChainIdPeer, nOrderAmount, nOrderPrice);
    mapCoinPairCompletePrice[hashCoinPair] = uint256(222);
    auto& orderRecord = mapCompDexOrderRecord[orderHeader];
    orderRecord.push_back(CCompDexOrderRecord(destPeer, nCompleteAmount, nCompletePrice));

    uint256 hashNewRoot;
    BOOST_CHECK(hdb.AddDexOrder(hashFork, hashBlock, hashPrevBlock, hashBlock, mapDexOrder, {}, mapCoinPairCompletePrice, {}, mapCompDexOrderRecord, mapBlockProve, hashNewRoot));

    hdb.Deinitialize();
    boost::filesystem::remove_all(fullpath);
}

BOOST_AUTO_TEST_CASE(dexordertest)
{
    //InitLogDir();
    NointLogOut();
    StdDebug("TEST", "Start dex order test!");

    std::string fullpath = GetOutPath("hdexdb_tests");
    if (boost::filesystem::exists(fullpath))
    {
        boost::filesystem::remove_all(fullpath);
    }
    boost::filesystem::create_directories(fullpath);

    CHdexDB hdb;
    BOOST_CHECK(hdb.Initialize(fullpath));

    //----------------------------------------------------------------------------
    auto funcAddOrder = [&](const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashBlock, const CChainId nPeerChainId, const std::string& strSymbolOwner, const std::string& strSymbolPeer,
                            const CDestination& destOrderOwner, const CDestination& destOrderPeer, const uint256& nOrderAmount, const uint256& nOrderPrice) {
        // FORK
        {
            const uint256 hashBlockFork = hashFork;
            const uint256 hashPrevBlock = 0;
            uint256 hashNewRootFork;
            BOOST_CHECK(hdb.AddDexOrder(hashFork, hashBlockFork, hashPrevBlock, hashBlockFork, {}, {}, {}, {}, {}, {}, hashNewRootFork));

            CBlockStorageProve blockCrosschainTest;

            blockCrosschainTest.hashRefBlock = hashBlockFork;
            blockCrosschainTest.vRefBlockMerkleProve.push_back(std::make_pair(2, uint256(99)));

            blockCrosschainTest.hashPrevBlock = hashPrevBlock;
            blockCrosschainTest.vPrevBlockMerkleProve.push_back(std::make_pair(2, uint256(99)));

            BOOST_CHECK(hdb.AddBlockCrosschainProve(hashBlockFork, blockCrosschainTest));

            //----------------------------------------------------------------------------
            bytes btAggBitmap = uint256(111).GetBytes();
            bytes btAggSig = uint256(222).GetBytes();

            std::map<CChainId, CBlockProve> mapBlockProveOut;
            BOOST_CHECK(hdb.UpdateBlockAggSign(hashBlockFork, btAggBitmap, btAggSig, mapBlockProveOut));
        }

        //----------------------------------------------------------------------------
        // block 1
        {
            const CChainId nLocalChainId = CBlock::GetBlockChainIdByHash(hashFork);
            const uint256 hashPrevBlock = hashFork;
            std::map<CDexOrderHeader, CDexOrderBody> mapDexOrder;
            std::map<uint256, uint256> mapCoinPairCompletePrice;
            std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>> mapCompDexOrderRecord;
            std::map<CChainId, CBlockProve> mapBlockProve;
            std::set<CChainId> setPeerCrossChainId;

            const uint64 nOrderNumber = 1;

            const CDexOrderHeader orderHeader(nLocalChainId, destOrderOwner, strSymbolOwner, strSymbolPeer, nOrderNumber);
            //const uint256 hashCoinPair = orderHeader.GetCoinPairHash();

            mapDexOrder[orderHeader] = CDexOrderBody(strSymbolOwner, strSymbolPeer, nPeerChainId, nOrderAmount, nOrderPrice);
            // mapCoinPairCompletePrice[hashCoinPair] = uint256(222);
            // auto& orderRecord = mapCompDexOrderRecord[orderHeader];
            // orderRecord.push_back(CCompDexOrderRecord(destPeer, nCompleteAmount, nCompletePrice));

            setPeerCrossChainId.insert(nPeerChainId);

            uint256 hashNewRoot;
            BOOST_CHECK(hdb.AddDexOrder(hashFork, hashRefBlock, hashPrevBlock, hashBlock, mapDexOrder, {}, mapCoinPairCompletePrice, setPeerCrossChainId, mapCompDexOrderRecord, mapBlockProve, hashNewRoot));

            //----------------------------------------------------------------------------
            CBlockStorageProve blockCrosschainTest;

            blockCrosschainTest.hashRefBlock = hashRefBlock;
            blockCrosschainTest.vRefBlockMerkleProve.push_back(std::make_pair(2, uint256(99)));

            blockCrosschainTest.hashPrevBlock = hashPrevBlock;
            blockCrosschainTest.vPrevBlockMerkleProve.push_back(std::make_pair(2, uint256(99)));

            // std::map<CChainId, std::pair<CBlockCrosschainProve, mtbase::MERKLE_PROVE_DATA>> mapCrossProve; // key is peer chainid
            CBlockCrosschainProve ccProve;
            ccProve.AddDexOrderProve(destOrderOwner, nLocalChainId, nPeerChainId, strSymbolOwner, strSymbolPeer, nOrderNumber, nOrderAmount, nOrderPrice);
            mtbase::MERKLE_PROVE_DATA vCcProveMerkle = { { 2, uint256(99) } };
            blockCrosschainTest.mapCrossProve.insert(std::make_pair(nPeerChainId, std::make_pair(ccProve, vCcProveMerkle)));

            BOOST_CHECK(hdb.AddBlockCrosschainProve(hashBlock, blockCrosschainTest));

            //----------------------------------------------------------------------------
            bytes btAggBitmap = uint256(111).GetBytes();
            bytes btAggSig = uint256(222).GetBytes();

            std::map<CChainId, CBlockProve> mapBlockProveOut;
            BOOST_CHECK(hdb.UpdateBlockAggSign(hashBlock, btAggBitmap, btAggSig, mapBlockProveOut));

            //----------------------------------------------------------------------------
            StdDebug("TEST", "++++++++++++++++++++ [ send prove: local chainid: %d ] ++++++++++++++++++++", nLocalChainId);
            for (const auto& kv : mapBlockProveOut)
            {
                const CChainId nPeerChainId = kv.first;
                const CBlockProve& blockProve = kv.second;

                StdDebug("TEST", "nPeerChainId: %d", nPeerChainId);
                StdDebug("TEST", "hashBlock: %s", blockProve.hashBlock.ToString().c_str());
                StdDebug("TEST", "hashRefBlock: %s", blockProve.hashRefBlock.ToString().c_str());
                StdDebug("TEST", "hashPrevBlock: %s", blockProve.hashPrevBlock.ToString().c_str());

                const std::map<CDexOrderHeader, CBlockDexOrderProve>& mapDexOrderProve = blockProve.proveCrosschain.GetDexOrderProve();
                for (const auto& kv : mapDexOrderProve)
                {
                    const CBlockDexOrderProve& prove = kv.second;
                    StdDebug("TEST", "prove: destOrder: %s", prove.destOrder.ToString().c_str());
                    StdDebug("TEST", "prove: strCoinSymbolOwner: %s", prove.strCoinSymbolOwner.c_str());
                    StdDebug("TEST", "prove: strCoinSymbolPeer: %s", prove.strCoinSymbolPeer.c_str());
                    StdDebug("TEST", "prove: nOrderNumber: %lu", prove.nOrderNumber);
                    StdDebug("TEST", "prove: nOrderAmount: %s", CoinToTokenBigFloat(prove.nOrderAmount).c_str());
                    StdDebug("TEST", "prove: nOrderPrice: %s", CoinToTokenBigFloat(prove.nOrderPrice).c_str());
                }
                // std::vector<CBlockPrevProve> vPrevBlockCcProve;
            }
        }
    };

    //----------------------------------------------------------------------------
    auto funcGetBlockProve = [&](const CChainId nRecvChainId, const uint256& hashRecvPrevBlock, std::map<CChainId, CBlockProve>& mapBlockCrosschainProve) {
        BOOST_CHECK(hdb.GetCrosschainProveForPrevBlock(nRecvChainId, hashRecvPrevBlock, mapBlockCrosschainProve));

        StdDebug("TEST", "============ [ block prove: recv chainid: %d ] ===================", nRecvChainId);
        for (const auto& kv : mapBlockCrosschainProve)
        {
            const CChainId nPeerChainId = kv.first;
            const CBlockProve& blockProve = kv.second;

            StdDebug("TEST", "nPeerChainId: %d", nPeerChainId);
            StdDebug("TEST", "hashBlock: %s", blockProve.hashBlock.ToString().c_str());
            StdDebug("TEST", "hashRefBlock: %s", blockProve.hashRefBlock.ToString().c_str());
            StdDebug("TEST", "hashPrevBlock: %s", blockProve.hashPrevBlock.ToString().c_str());

            const std::map<CDexOrderHeader, CBlockDexOrderProve>& mapDexOrderProve = blockProve.proveCrosschain.GetDexOrderProve();
            for (const auto& kv : mapDexOrderProve)
            {
                const CBlockDexOrderProve& prove = kv.second;
                StdDebug("TEST", "prove: destOrder: %s", prove.destOrder.ToString().c_str());
                StdDebug("TEST", "prove: strCoinSymbolOwner: %s", prove.strCoinSymbolOwner.c_str());
                StdDebug("TEST", "prove: strCoinSymbolPeer: %s", prove.strCoinSymbolPeer.c_str());
                StdDebug("TEST", "prove: nOrderNumber: %lu", prove.nOrderNumber);
                StdDebug("TEST", "prove: nOrderAmount: %s", CoinToTokenBigFloat(prove.nOrderAmount).c_str());
                StdDebug("TEST", "prove: nOrderPrice: %s", CoinToTokenBigFloat(prove.nOrderPrice).c_str());
            }
            // std::vector<CBlockPrevProve> vPrevBlockCcProve;
        }
    };

    //----------------------------------------------------------------------------
    auto funcOutMatchDexInfo = [&](const uint256& hashBlock) {
        const SHP_MATCH_DEX ptrMatchDex = hdb.GetBlockMatchDex(hashBlock);
        BOOST_CHECK(ptrMatchDex != nullptr);

        const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
        const uint32 nHeight = CBlock::GetBlockHeightByHash(hashBlock);

        StdDebug("TEST", "============ [ match dex: chainid: %d, coin dex size: %lu, height: %d, block: %s ] ===================",
                 nChainId, ptrMatchDex->mapCoinDex.size(), nHeight, hashBlock.ToString().c_str());
        for (const auto& kv : ptrMatchDex->mapCoinDex)
        {
            const uint256& hashCoinPair = kv.first;
            const CCoinDexPair& dexPair = kv.second;

            StdDebug("TEST", "match pair: coin pair: %s", hashCoinPair.ToString().c_str());

            StdDebug("TEST", "match pair: symbol sell: %s", dexPair.strCoinSymbolSell.c_str());
            StdDebug("TEST", "match pair: symbol buy: %s", dexPair.strCoinSymbolBuy.c_str());
            StdDebug("TEST", "match pair: sell price anchor: %s", CoinToTokenBigFloat(dexPair.nSellPriceAnchor).c_str());
            StdDebug("TEST", "match pair: prev complete price: %s", CoinToTokenBigFloat(dexPair.nPrevCompletePrice).c_str());

            for (const auto& kv : dexPair.mapSellOrder)
            {
                const CDexOrderKey& key = kv.first;
                const CDexOrderValue& value = kv.second;

                StdDebug("TEST", "match pair: Sell: Key.nPrice: %s", CoinToTokenBigFloat(key.nPrice).c_str());
                StdDebug("TEST", "match pair: Sell: Key.nHeight: %d", key.nHeight);
                StdDebug("TEST", "match pair: Sell: Key.nSlot: %d", key.nSlot);
                StdDebug("TEST", "match pair: Sell: Key.hashOrderRandom: %s", key.hashOrderRandom.ToString().c_str());

                StdDebug("TEST", "match pair: Sell: Value.destOrder: %s", value.destOrder.ToString().c_str());
                StdDebug("TEST", "match pair: Sell: Value.nOrderNumber: %lu", value.nOrderNumber);
                StdDebug("TEST", "match pair: Sell: Value.nOrderAmount: %s", CoinToTokenBigFloat(value.nOrderAmount).c_str());
                StdDebug("TEST", "match pair: Sell: Value.nSurplusOrderAmount: %s", CoinToTokenBigFloat(value.GetSurplusAmount()).c_str());
                StdDebug("TEST", "match pair: Sell: Value.nCompleteAmount: %s", CoinToTokenBigFloat(value.nCompleteAmount).c_str());
                StdDebug("TEST", "match pair: Sell: Value.nCompleteCount: %lu", value.nCompleteCount);
                StdDebug("TEST", "match pair: Sell: Value.nOrderAtChainId: %u", value.nOrderAtChainId);
                StdDebug("TEST", "match pair: Sell: Value.hashOrderAtBlock: %s", value.hashOrderAtBlock.ToString().c_str());
            }

            for (const auto& kv : dexPair.mapBuyOrder)
            {
                const CDexOrderKey& key = kv.first;
                const CDexOrderValue& value = kv.second;

                StdDebug("TEST", "match pair: Buy: Key.nPrice: %s", CoinToTokenBigFloat(key.nPrice).c_str());
                StdDebug("TEST", "match pair: Buy: Key.nHeight: %d", key.nHeight);
                StdDebug("TEST", "match pair: Buy: Key.nSlot: %d", key.nSlot);
                StdDebug("TEST", "match pair: Buy: Key.hashOrderRandom: %s", key.hashOrderRandom.ToString().c_str());

                StdDebug("TEST", "match pair: Buy: Value.destOrder: %s", value.destOrder.ToString().c_str());
                StdDebug("TEST", "match pair: Buy: Value.nOrderNumber: %lu", value.nOrderNumber);
                StdDebug("TEST", "match pair: Buy: Value.nOrderAmount: %s", CoinToTokenBigFloat(value.nOrderAmount).c_str());
                StdDebug("TEST", "match pair: Buy: Value.nSurplusOrderAmount: %s", CoinToTokenBigFloat(value.GetSurplusAmount()).c_str());
                StdDebug("TEST", "match pair: Buy: Value.nCompleteAmount: %s", CoinToTokenBigFloat(value.nCompleteAmount).c_str());
                StdDebug("TEST", "match pair: Buy: Value.nCompleteCount: %lu", value.nCompleteCount);
                StdDebug("TEST", "match pair: Buy: Value.nOrderAtChainId: %u", value.nOrderAtChainId);
                StdDebug("TEST", "match pair: Buy: Value.hashOrderAtBlock: %s", value.hashOrderAtBlock.ToString().c_str());
            }
        }
    };

    //----------------------------------------------------------------------------
    auto funcOutMatchResultInfo = [&](const uint256& hashBlock, std::map<uint256, CMatchOrderResult>& mapMatchResult) {
        BOOST_CHECK(hdb.GetMatchDexData(hashBlock, mapMatchResult));

        // std::string strCoinSymbolSell; // min symbol
        // std::string strCoinSymbolBuy;  // max symbol
        // uint256 nSellPriceAnchor;      // 1 sell token of coin amount
        // std::vector<CMatchOrderRecord> vMatchOrderRecord;

        // CDestination destSellOrder;
        // uint64 nSellOrderNumber;
        // uint256 nSellCompleteAmount;
        // uint256 nSellCompletePrice;
        // CChainId nSellOrderAtChainId;
        // CDestination destBuyOrder;
        // uint64 nBuyOrderNumber;
        // uint256 nBuyCompleteAmount;
        // uint256 nBuyCompletePrice;
        // CChainId nBuyOrderAtChainId;
        // uint256 nCompletePrice;

        const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
        const uint32 nHeight = CBlock::GetBlockHeightByHash(hashBlock);

        StdDebug("TEST", "============ [ match result: chainid: %d, coin pair count: %lu, height: %d, block: %s ] ===================",
                 nChainId, mapMatchResult.size(), nHeight, hashBlock.ToString().c_str());
        for (const auto& kv : mapMatchResult)
        {
            const uint256& hashCoinPair = kv.first;
            const CMatchOrderResult& matchResult = kv.second;

            StdDebug("TEST", "match result: coin pair: %s", hashCoinPair.ToString().c_str());
            StdDebug("TEST", "match result: strCoinSymbolSell: %s", matchResult.strCoinSymbolSell.c_str());
            StdDebug("TEST", "match result: strCoinSymbolBuy: %s", matchResult.strCoinSymbolBuy.c_str());
            StdDebug("TEST", "match result: nSellPriceAnchor: %s", CoinToTokenBigFloat(matchResult.nSellPriceAnchor).c_str());

            for (const CMatchOrderRecord& orderRecord : matchResult.vMatchOrderRecord)
            {
                StdDebug("TEST", "match result: Order record: Sell.destSellOrder: %s", orderRecord.destSellOrder.ToString().c_str());
                StdDebug("TEST", "match result: Order record: Sell.nSellOrderNumber: %lu", orderRecord.nSellOrderNumber);
                StdDebug("TEST", "match result: Order record: Sell.nSellCompleteAmount: %s", CoinToTokenBigFloat(orderRecord.nSellCompleteAmount).c_str());
                StdDebug("TEST", "match result: Order record: Sell.nSellCompletePrice: %s", CoinToTokenBigFloat(orderRecord.nSellCompletePrice).c_str());
                StdDebug("TEST", "match result: Order record: Sell.nSellOrderAtChainId: %u", orderRecord.nSellOrderAtChainId);
                StdDebug("TEST", "match result: Order record: Buy.destBuyOrder: %s", orderRecord.destBuyOrder.ToString().c_str());
                StdDebug("TEST", "match result: Order record: Buy.nBuyOrderNumber: %lu", orderRecord.nBuyOrderNumber);
                StdDebug("TEST", "match result: Order record: Buy.nBuyCompleteAmount: %s", CoinToTokenBigFloat(orderRecord.nBuyCompleteAmount).c_str());
                StdDebug("TEST", "match result: Order record: Buy.nBuyCompletePrice: %s", CoinToTokenBigFloat(orderRecord.nBuyCompletePrice).c_str());
                StdDebug("TEST", "match result: Order record: Buy.nBuyOrderAtChainId: %u", orderRecord.nBuyOrderAtChainId);
                StdDebug("TEST", "match result: Order record: nCompletePrice: %s", CoinToTokenBigFloat(orderRecord.nCompletePrice).c_str());
            }
        }
    };

    //----------------------------------------------------------------------------
    auto funcAddMatchResult = [&](const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CMatchOrderResult>& mapMatchResult) {
        std::map<uint256, uint256> mapCoinPairCompletePrice; // key: coin pair, value: complete price
        std::map<CDexOrderHeader, std::vector<CCompDexOrderRecord>> mapCompDexOrderRecord;

        for (const auto& kv : mapMatchResult)
        {
            const uint256& hashCoinPair = kv.first;
            const CMatchOrderResult& orderResult = kv.second;

            if (!orderResult.vMatchOrderRecord.empty())
            {
                mapCoinPairCompletePrice[hashCoinPair] = orderResult.vMatchOrderRecord[orderResult.vMatchOrderRecord.size() - 1].nCompletePrice;
            }

            // std::string strCoinSymbolSell; // min symbol
            // std::string strCoinSymbolBuy;  // max symbol
            // uint256 nSellPriceAnchor;      // 1 sell token of coin amount
            // std::vector<CMatchOrderRecord> vMatchOrderRecord;

            // CDestination destSellOrder;
            // uint64 nSellOrderNumber;
            // uint256 nSellCompleteAmount;
            // uint256 nSellCompletePrice;
            // CChainId nSellOrderAtChainId;

            // CDestination destBuyOrder;
            // uint64 nBuyOrderNumber;
            // uint256 nBuyCompleteAmount;
            // uint256 nBuyCompletePrice;
            // CChainId nBuyOrderAtChainId;
            // uint256 nCompletePrice;

            for (const CMatchOrderRecord& orderRecord : orderResult.vMatchOrderRecord)
            {
                mapCompDexOrderRecord[CDexOrderHeader(orderRecord.nSellOrderAtChainId, orderRecord.destSellOrder, orderResult.strCoinSymbolSell, orderResult.strCoinSymbolBuy, orderRecord.nSellOrderNumber)].push_back(CCompDexOrderRecord(orderRecord.destBuyOrder, orderRecord.nSellCompleteAmount, orderRecord.nCompletePrice));
                mapCompDexOrderRecord[CDexOrderHeader(orderRecord.nBuyOrderAtChainId, orderRecord.destBuyOrder, orderResult.strCoinSymbolBuy, orderResult.strCoinSymbolSell, orderRecord.nBuyOrderNumber)].push_back(CCompDexOrderRecord(orderRecord.destSellOrder, orderRecord.nBuyCompleteAmount, orderRecord.nCompletePrice));
            }
        }

        uint256 hashNewRootDexOrder;
        BOOST_CHECK(hdb.AddDexOrder(hashFork, hashRefBlock, hashPrevBlock, hashBlock, {}, {}, mapCoinPairCompletePrice, {}, mapCompDexOrderRecord, {}, hashNewRootDexOrder));
    };

    //----------------------------------------------------------------------------
    const CChainId nChainIdA = 201;
    const CChainId nChainIdB = 202;
    const uint256 hashForkA = CBlock::CreateBlockHash(nChainIdA, 0, 0, uint256(uint256(BSwap32((uint32)1)).ToBigEndian()));
    const uint256 hashForkB = CBlock::CreateBlockHash(nChainIdB, 0, 0, uint256(uint256(BSwap32((uint32)2)).ToBigEndian()));
    const uint256 hashBlockA1 = CBlock::CreateBlockHash(nChainIdA, 1, 0, uint256(uint256(BSwap32((uint32)3)).ToBigEndian()));
    const uint256 hashBlockB1 = CBlock::CreateBlockHash(nChainIdB, 1, 0, uint256(uint256(BSwap32((uint32)4)).ToBigEndian()));
    const uint256 hashBlockA2 = CBlock::CreateBlockHash(nChainIdA, 2, 0, uint256(uint256(BSwap32((uint32)5)).ToBigEndian()));
    const uint256 hashBlockB2 = CBlock::CreateBlockHash(nChainIdB, 2, 0, uint256(uint256(BSwap32((uint32)6)).ToBigEndian()));
    const uint256 hashBlockA3 = CBlock::CreateBlockHash(nChainIdA, 3, 0, uint256(uint256(BSwap32((uint32)7)).ToBigEndian()));
    const uint256 hashBlockB3 = CBlock::CreateBlockHash(nChainIdB, 3, 0, uint256(uint256(BSwap32((uint32)8)).ToBigEndian()));
    const std::string strSymbolA = "AAA"; // sell
    const std::string strSymbolB = "BBB"; // buy
    const CDestination destOrderA("0x5bc5c1726286ff0a8006b19312ca307210e0e658");
    const CDestination destOrderB("0x0a9f6b9e0de14c2c9d02883904a69c7bee82c2a5");
    const uint256 nOrderAmountA = TokenBigFloatToCoin("200.12"); // sell
    const uint256 nOrderPriceA = TokenBigFloatToCoin("0.22");
    const uint256 nOrderAmountB = TokenBigFloatToCoin("30.66"); // buy
    const uint256 nOrderPriceB = TokenBigFloatToCoin("0.25");

    funcAddOrder(hashForkA, hashBlockA1, hashBlockA1, nChainIdB, strSymbolA, strSymbolB, destOrderA, destOrderB, nOrderAmountA, nOrderPriceA);
    funcAddOrder(hashForkB, hashBlockA1, hashBlockB1, nChainIdA, strSymbolB, strSymbolA, destOrderB, destOrderA, nOrderAmountB, nOrderPriceB);

    std::map<CChainId, CBlockProve> mapBlockCrosschainProveA;
    std::map<CChainId, CBlockProve> mapBlockCrosschainProveB;
    funcGetBlockProve(nChainIdA, hashForkA, mapBlockCrosschainProveA);
    funcGetBlockProve(nChainIdB, hashForkB, mapBlockCrosschainProveB);

    uint256 hashNewRootDexOrderA;
    uint256 hashNewRootDexOrderB;
    BOOST_CHECK(hdb.AddDexOrder(hashForkA, hashBlockA2, hashBlockA1, hashBlockA2, {}, {}, {}, {}, {}, mapBlockCrosschainProveA, hashNewRootDexOrderA));
    BOOST_CHECK(hdb.AddDexOrder(hashForkB, hashBlockA2, hashBlockB1, hashBlockB2, {}, {}, {}, {}, {}, mapBlockCrosschainProveB, hashNewRootDexOrderB));

    funcOutMatchDexInfo(hashBlockA2);
    funcOutMatchDexInfo(hashBlockB2);

    std::map<uint256, CMatchOrderResult> mapMatchResultA;
    std::map<uint256, CMatchOrderResult> mapMatchResultB;
    funcOutMatchResultInfo(hashBlockA2, mapMatchResultA);
    funcOutMatchResultInfo(hashBlockB2, mapMatchResultB);

    funcAddMatchResult(hashForkA, hashBlockA3, hashBlockA2, hashBlockA3, mapMatchResultA);
    funcAddMatchResult(hashForkB, hashBlockA3, hashBlockB2, hashBlockB3, mapMatchResultB);

    funcOutMatchDexInfo(hashBlockA3);
    funcOutMatchDexInfo(hashBlockB3);

    //----------------------------------------------------------------------------
    hdb.Deinitialize();
    boost::filesystem::remove_all(fullpath);
}

BOOST_AUTO_TEST_SUITE_END()

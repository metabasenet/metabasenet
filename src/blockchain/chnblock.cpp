// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblock.h"

#include <boost/bind.hpp>
#include <boost/range/adaptor/reversed.hpp>

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

#define MAX_CACHE_CHN_BLOCK_COUNT 2000
#define MAX_CACHE_CHN_BLOCK_SIZE (1024 * 1024 * 1024)
#define SINGLE_REQ_BLOCK_HASH_COUNT 32
#define BLOCK_SYNC_TIMER_TIME 300

namespace metabasenet
{

////////////////////////////////////
// CBlockChnFork

void CBlockChnFork::AddPrevBlockHash(const uint256& hashPrevBlock, const uint256& hashBlock)
{
    mapBlockHash[hashBlock] = hashPrevBlock;
    mapPrevHash[hashPrevBlock] = hashBlock;
}

bool CBlockChnFork::AddBlockHash(const std::vector<uint256>& vBlockHash)
{
    bool fAllExist = true;
    uint256 hashPrev;
    for (auto& hash : boost::adaptors::reverse(vBlockHash))
    {
        auto it = mapBlockHash.find(hash);
        if (it == mapBlockHash.end())
        {
            fAllExist = false;
            mapBlockHash[hash] = hashPrev;
        }
        else
        {
            if (it->second == 0)
            {
                it->second = hashPrev;
            }
        }

        if (hashPrev != 0 && mapPrevHash.count(hashPrev) == 0)
        {
            mapPrevHash[hashPrev] = hash;
        }
        hashPrev = hash;
    }
    return fAllExist;
}

bool CBlockChnFork::GetNextBlockHash(const uint256& hashPrevBlock, uint256& hashBlock)
{
    auto it = mapPrevHash.find(hashPrevBlock);
    if (it != mapPrevHash.end())
    {
        hashBlock = it->second;
        return (hashBlock != 0);
    }
    return false;
}

void CBlockChnFork::RemoveBlockHash(const uint256& hashBlock)
{
    auto it = mapBlockHash.find(hashBlock);
    if (it != mapBlockHash.end())
    {
        if (it->second != 0)
        {
            mapPrevHash.erase(it->second);
        }
        mapBlockHash.erase(it);
    }
}

void CBlockChnFork::ClearPrevBlockHash(const uint256& hashLastExistBlock)
{
    uint256 hashBlock = hashLastExistBlock;
    while (hashBlock != 0)
    {
        auto it = mapBlockHash.find(hashBlock);
        if (it == mapBlockHash.end())
        {
            break;
        }
        hashBlock = it->second;
        mapBlockHash.erase(it);
        if (hashBlock != 0)
        {
            mapPrevHash.erase(hashBlock);
        }
    }
}

bool CBlockChnFork::CheckPrevExist(const uint256& hashBlock)
{
    uint256 hash = hashBlock;
    for (uint32 i = 0; i < SINGLE_REQ_BLOCK_HASH_COUNT; i++)
    {
        auto it = mapBlockHash.find(hash);
        if (it == mapBlockHash.end())
        {
            return false;
        }
        hash = it->second;
    }
    return true;
}

////////////////////////////////////
// CBlockChannel

CBlockChannel::CBlockChannel()
  : nPrevCheckCacheTimeoutTime(0), nCacheBlockByteCount(0), nBlockSyncTimerId(0)
{
    pPeerNet = nullptr;
    pDispatcher = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pBlockFilter = nullptr;
}

CBlockChannel::~CBlockChannel()
{
}

bool CBlockChannel::HandleInitialize()
{
    if (!GetObject("peernet", pPeerNet))
    {
        Error("Failed to request peer net");
        return false;
    }

    if (!GetObject("dispatcher", pDispatcher))
    {
        Error("Failed to request dispatcher");
        return false;
    }

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

    if (!GetObject("blockfilter", pBlockFilter))
    {
        Error("Failed to request blockfilter");
        return false;
    }
    return true;
}

void CBlockChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pDispatcher = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pBlockFilter = nullptr;
}

bool CBlockChannel::HandleInvoke()
{
    nBlockSyncTimerId = SetTimer(BLOCK_SYNC_TIMER_TIME, boost::bind(&CBlockChannel::BlockSyncTimerFunc, this, _1));
    if (nBlockSyncTimerId == 0)
    {
        StdLog("CBlockChannel", "Handle Invoke: Set timer fail");
        return false;
    }
    return network::IBlockChannel::HandleInvoke();
}

void CBlockChannel::HandleHalt()
{
    if (nBlockSyncTimerId != 0)
    {
        CancelTimer(nBlockSyncTimerId);
        nBlockSyncTimerId = 0;
    }
    network::IBlockChannel::HandleHalt();
}

bool CBlockChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockChnPeer(eventActive.data.nService, eventActive.data);
    StdLog("CBlockChannel", "CEvent peer active: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    if (!mapChnFork.empty())
    {
        network::CEventPeerBlockSubscribe eventSubscribe(nNonce, pCoreProtocol->GetGenesisBlockHash());
        for (auto& kv : mapChnFork)
        {
            eventSubscribe.data.push_back(kv.first);
        }
        pPeerNet->DispatchEvent(&eventSubscribe);
        StdLog("CBlockChannel", "CEvent peer active: Send fork subscribe, peer: %s", GetPeerAddressInfo(nNonce).c_str());
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockChannel", "CEvent peer deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        mapChnPeer.erase(it);
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockSubscribe& eventSubscribe)
{
    const uint64 nNonce = eventSubscribe.nNonce;
    const uint256& hashFork = eventSubscribe.hashFork;

    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CBlockChannel", "CEvent peer block subscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetBhString().c_str());
        return false;
    }
    StdLog("CBlockChannel", "CEvent peer block subscribe: Recv fork subscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetBhString().c_str());

    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        for (auto& hashFork : eventSubscribe.data)
        {
            it->second.Subscribe(hashFork);
        }
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockUnsubscribe& eventUnsubscribe)
{
    const uint64 nNonce = eventUnsubscribe.nNonce;
    const uint256& hashFork = eventUnsubscribe.hashFork;

    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CBlockChannel", "CEvent peer block unsubscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetBhString().c_str());
        return false;
    }
    StdLog("CBlockChannel", "CEvent peer block unsubscribe: Recv fork unsubscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetBhString().c_str());

    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        for (auto& hashFork : eventUnsubscribe.data)
        {
            it->second.Unsubscribe(hashFork);
        }
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockBks& eventBks)
{
    const uint256& hashFork = eventBks.hashFork;
    const uint64 nRecvPeerNonce = eventBks.nNonce;

    StdDebug("CBlockChannel", "CEvent peer block bks: Recv block, block: %s, fork: %s",
             eventBks.data.hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());

    CBlock block;
    try
    {
        CBufStream ss(eventBks.data.btBlockData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    uint256 hashBlock = block.GetHash();
    if (eventBks.data.hashBlock != hashBlock)
    {
        StdLog("CBlockChannel", "CEvent peer block bks: Block hash error, block hash: %s, data block: %s, prev: %s",
               eventBks.data.hashBlock.GetBhString().c_str(), hashBlock.GetBhString().c_str(), eventBks.data.hashPrev.GetBhString().c_str());
        return false;
    }

    pBlockFilter->AddMaxPeerBlockNumber(hashFork, block.GetBlockNumber());

    if (!pBlockChain->Exists(eventBks.data.hashPrev))
    {
        StdDebug("CBlockChannel", "CEvent peer block bks: Prev block not exist, block: %s, prev: %s",
                 eventBks.data.hashBlock.GetBhString().c_str(), eventBks.data.hashPrev.GetBhString().c_str());

        AddCacheBlock(hashBlock, block, eventBks.data.btBlockData.size(), nRecvPeerNonce);

        auto it = mapChnFork.find(hashFork);
        if (it != mapChnFork.end())
        {
            it->second.AddPrevBlockHash(block.hashPrev, hashBlock);
            if (!it->second.CheckPrevExist(hashBlock))
            {
                SendNextPrevBlockReq(hashFork, hashBlock, nRecvPeerNonce);
            }
        }
    }
    else if (pBlockChain->Exists(eventBks.data.hashBlock))
    {
        StdDebug("CBlockChannel", "CEvent peer block bks: Block existed, block: %s, prev: %s",
                 eventBks.data.hashBlock.GetBhString().c_str(), eventBks.data.hashPrev.GetBhString().c_str());
    }
    else
    {
        if (pDispatcher->AddNewBlock(block, nRecvPeerNonce) != OK)
        {
            StdLog("CBlockChannel", "CEvent peer block bks: Add new block fail, block: %s, fork: %s",
                   eventBks.data.hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());
        }
        else
        {
            StdDebug("CBlockChannel", "CEvent peer block bks: Add block success, block: %s, fork: %s",
                     eventBks.data.hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());

            AddNextBlock(hashBlock);
        }
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockNextPrevBlock& eventData)
{
    const uint256& hashFork = eventData.hashFork;
    const uint64 nRecvPeerNonce = eventData.nNonce;
    uint256 hashBlock = eventData.data;

    network::CEventPeerBlockPrevBlocks eventPrevBlocks(nRecvPeerNonce, hashFork);
    std::vector<uint256>& vPrevBlockhash = eventPrevBlocks.data;

    do
    {
        uint256 hashLastBlock;
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            StdLog("CBlockChannel", "CEvent peer block next prev block: Retrieve fork last fail, fork: %s", hashFork.GetBhString().c_str());
            break;
        }
        if (hashBlock == 0)
        {
            hashBlock = hashLastBlock;
            vPrevBlockhash.push_back(hashLastBlock);
        }
        else
        {
            if (!pBlockChain->VerifySameChain(hashBlock, hashLastBlock))
            {
                StdLog("CBlockChannel", "CEvent peer block next prev block: Verify same chain fail, block: %s, last block: %s, fork: %s",
                       hashBlock.GetBhString().c_str(), hashLastBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());
                break;
            }
            vPrevBlockhash.push_back(hashBlock);
        }
        if (!pBlockChain->GetPrevBlockHashList(hashBlock, SINGLE_REQ_BLOCK_HASH_COUNT, vPrevBlockhash))
        {
            StdLog("CBlockChannel", "CEvent peer block next prev block: Get prev block hash fail, block: %s, fork: %s",
                   hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());
        }
    } while (0);

    pPeerNet->DispatchEvent(&eventPrevBlocks);
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockPrevBlocks& eventData)
{
    const uint256& hashFork = eventData.hashFork;
    const uint64 nRecvPeerNonce = eventData.nNonce;
    const std::vector<uint256>& vPrevBlockHash = eventData.data;

    if (vPrevBlockHash.size() > 1)
    {
        const uint256& hashFirstBlock = vPrevBlockHash[vPrevBlockHash.size() - 1];

        auto it = mapChnFork.find(hashFork);
        if (it == mapChnFork.end())
        {
            return false;
        }
        CBlockChnFork& chnFork = it->second;

        bool fAllExist = chnFork.AddBlockHash(vPrevBlockHash);

        uint256 hashExistBlock;
        for (auto& hash : vPrevBlockHash)
        {
            if (pBlockChain->Exists(hash))
            {
                hashExistBlock = hash;
                break;
            }
        }
        if (hashExistBlock == 0)
        {
            if (!fAllExist)
            {
                SendNextPrevBlockReq(hashFork, hashFirstBlock, nRecvPeerNonce);
            }
        }
        else
        {
            RequestNextBlockData(hashFork, hashExistBlock, nRecvPeerNonce);
        }
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockGetBlockReq& eventData)
{
    const uint256& hashFork = eventData.hashFork;
    const uint64 nRecvPeerNonce = eventData.nNonce;
    const uint256& hashBlock = eventData.data;

    CBlock block;
    if (pBlockChain->GetBlock(hashBlock, block))
    {
        CBufStream ss;
        ss << block;
        bytes btBlockData;
        ss.GetData(btBlockData);
        SendGetBlockRsp(hashFork, hashBlock, btBlockData, nRecvPeerNonce);
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerBlockGetBlockRsp& eventData)
{
    const uint256& hashFork = eventData.hashFork;
    const uint64 nRecvPeerNonce = eventData.nNonce;
    const bytes& btBlockData = eventData.data;

    if (!btBlockData.empty())
    {
        CBlock block;
        try
        {
            CBufStream ss(btBlockData);
            ss >> block;
        }
        catch (std::exception& e)
        {
            mtbase::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        uint256 hashBlock = block.GetHash();

        if (pDispatcher->AddNewBlock(block, nRecvPeerNonce) != OK)
        {
            StdLog("CBlockChannel", "CEvent peer block get block rsp: Add new block fail, block: %s, fork: %s",
                   hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());
            return true;
        }
        StdDebug("CBlockChannel", "CEvent peer block get block rsp: Add block success, block: %s, fork: %s",
                 hashBlock.GetBhString().c_str(), hashFork.GetBhString().c_str());

        AddNextBlock(hashBlock);

        RequestNextBlockData(hashFork, hashBlock, nRecvPeerNonce);
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventLocalBlockSyncTimer& eventData)
{
    for (auto& kv : mapChnFork)
    {
        const uint256& hashFork = kv.first;
        for (auto& kvs : mapChnPeer)
        {
            if (kvs.second.IsSubscribe(hashFork))
            {
                SendNextPrevBlockReq(hashFork, 0, kvs.first);
            }
        }
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventLocalBlockSubscribeFork& eventSubsFork)
{
    network::CEventPeerBlockSubscribe eventSubscribe(0, pCoreProtocol->GetGenesisBlockHash());
    for (auto& hashFork : eventSubsFork.data)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            StdLog("CBlockChannel", "CEvent local block subscribe fork: Fork is not enabled, fork: %s", hashFork.GetBhString().c_str());
            continue;
        }
        StdDebug("CBlockChannel", "CEvent local block subscribe fork: Subscribe fork, fork: %s", hashFork.GetBhString().c_str());

        if (mapChnFork.find(hashFork) == mapChnFork.end())
        {
            mapChnFork.insert(std::make_pair(hashFork, CBlockChnFork(hashFork)));
        }
        eventSubscribe.data.push_back(hashFork);
    }

    for (auto& kv : mapChnPeer)
    {
        eventSubscribe.nNonce = kv.first;
        pPeerNet->DispatchEvent(&eventSubscribe);
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventLocalBlockBroadcastBks& eventBroadBks)
{
    const uint64 nRecvPeerNonce = eventBroadBks.nNonce;
    const uint256& hashFork = eventBroadBks.hashFork;

    network::CEventPeerBlockBks eventBks(0, hashFork);

    eventBks.data.hashBlock = eventBroadBks.data.hashBlock;
    eventBks.data.hashPrev = eventBroadBks.data.hashPrev;
    CBufStream ss;
    ss << eventBroadBks.data.block;
    ss.GetData(eventBks.data.btBlockData);

    for (auto& kv : mapChnPeer)
    {
        if (kv.second.IsSubscribe(hashFork) && (nRecvPeerNonce == 0 || nRecvPeerNonce != kv.first))
        {
            eventBks.nNonce = kv.first;
            pPeerNet->DispatchEvent(&eventBks);
            StdDebug("CBlockChannel", "CEvent local block broadcast bks: Broadcast block, peer nonce: 0x%x, txs: %lu, block: %s",
                     kv.first, eventBroadBks.data.block.vtx.size(), eventBroadBks.data.hashBlock.GetBhString().c_str());
        }
    }

    AddNextBlock(eventBroadBks.data.hashBlock);
    return true;
}

//-------------------------------------------------------------------------------------------
void CBlockChannel::SubscribeFork(const uint256& hashFork, const uint64 nNonce)
{
    network::CEventLocalBlockSubscribeFork* pEvent = new network::CEventLocalBlockSubscribeFork(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.push_back(hashFork);
        PostEvent(pEvent);
    }
}

void CBlockChannel::BroadcastBlock(const uint64 nRecvNetNonce, const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev, const CBlock& block)
{
    network::CEventLocalBlockBroadcastBks* pEvent = new network::CEventLocalBlockBroadcastBks(nRecvNetNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.hashBlock = hashBlock;
        pEvent->data.hashPrev = hashPrev;
        pEvent->data.block = block;
        PostEvent(pEvent);
    }
}

const string CBlockChannel::GetPeerAddressInfo(const uint64 nNonce)
{
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        return it->second.GetRemoteAddress();
    }
    return string("0.0.0.0");
}

void CBlockChannel::BlockSyncTimerFunc(uint32 nTimerId)
{
    if (nTimerId == nBlockSyncTimerId)
    {
        nBlockSyncTimerId = SetTimer(BLOCK_SYNC_TIMER_TIME, boost::bind(&CBlockChannel::BlockSyncTimerFunc, this, _1));
        network::CEventLocalBlockSyncTimer* pEvent = new network::CEventLocalBlockSyncTimer(0, pCoreProtocol->GetGenesisBlockHash());
        if (pEvent)
        {
            pEvent->data = nTimerId;
            PostEvent(pEvent);
        }
    }
}

void CBlockChannel::AddCacheBlock(const uint256& hashBlock, const CBlock& block, const uint64 nBlockSize, const uint64 nRecvNonce)
{
    if (mapChnBlock.find(hashBlock) == mapChnBlock.end())
    {
        while (mapChnBlock.size() >= MAX_CACHE_CHN_BLOCK_COUNT || (nCacheBlockByteCount >= MAX_CACHE_CHN_BLOCK_SIZE && mapChnBlock.size() > 0))
        {
            RemoveCacheBlock(mapChnBlock.begin()->first);
        }
        if (mapChnBlock.insert(std::make_pair(hashBlock, CChnCacheBlock(block, nBlockSize, nRecvNonce))).second)
        {
            mapChnPrevBlock[block.hashPrev].insert(hashBlock);
            nCacheBlockByteCount += nBlockSize;

            StdDebug("CBlockChannel", "Add cache block: Add cache block success, cache count: %lu, bytes: %lu KB, type: %s, number: %lu, block: %s, prev: %s",
                     mapChnBlock.size(), nCacheBlockByteCount / 1024, GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), block.nNumber,
                     hashBlock.GetBhString().c_str(), block.hashPrev.GetBhString().c_str());
        }
    }
}

void CBlockChannel::RemoveCacheBlock(const uint256& hashBlock)
{
    auto it = mapChnBlock.find(hashBlock);
    if (it != mapChnBlock.end())
    {
        const uint256& hashPrev = it->second.block.hashPrev;
        auto mt = mapChnPrevBlock.find(hashPrev);
        if (mt != mapChnPrevBlock.end())
        {
            mt->second.erase(hashBlock);
            if (mt->second.empty())
            {
                mapChnPrevBlock.erase(mt);
            }
        }
        nCacheBlockByteCount -= it->second.nBlockSize;
        mapChnBlock.erase(it);

        if (mapChnBlock.empty())
        {
            mapChnPrevBlock.clear();
            nCacheBlockByteCount = 0;
        }
    }
}

void CBlockChannel::AddNextBlock(const uint256& hashPrev)
{
    std::vector<uint256> vRemoveHash;
    auto it = mapChnPrevBlock.find(hashPrev);
    if (it != mapChnPrevBlock.end())
    {
        for (const uint256& hashBlock : it->second)
        {
            auto mt = mapChnBlock.find(hashBlock);
            if (mt != mapChnBlock.end())
            {
                const CBlock& block = mt->second.block;
                Errno err = pDispatcher->AddNewBlock(block, mt->second.nRecvNonce);
                if (err != OK)
                {
                    StdLog("CBlockChannel", "Add next block: Add new block fail, block: %s, err: [%d] %s",
                           hashBlock.GetBhString().c_str(), err, ErrorString(err));
                }
                else
                {
                    StdDebug("CBlockChannel", "Add next block: Add new block success, type: %s, block: %s",
                             GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), hashBlock.GetBhString().c_str());
                }
                vRemoveHash.push_back(hashBlock);
            }
        }
    }
    for (auto& hashBlock : vRemoveHash)
    {
        RemoveCacheBlock(hashBlock);
    }
    if (vRemoveHash.size() > 0)
    {
        StdDebug("CBlockChannel", "Add next block: Remove count: %lu, cache count: %lu, bytes: %lu KB", vRemoveHash.size(), mapChnBlock.size(), nCacheBlockByteCount / 1024);
    }

    uint256 hashRemoveBlock = hashPrev;
    while (true)
    {
        auto nt = mapChnBlock.find(hashRemoveBlock);
        if (nt == mapChnBlock.end())
        {
            break;
        }
        uint256 hashRemovePrev = nt->second.block.hashPrev;
        RemoveCacheBlock(hashRemoveBlock);
        hashRemoveBlock = hashRemovePrev;
    }

    ClearCacheTimeout();
}

void CBlockChannel::ClearCacheTimeout()
{
    uint64 nCurrTime = GetTime();
    if (nPrevCheckCacheTimeoutTime + 60 > nCurrTime)
    {
        return;
    }
    nPrevCheckCacheTimeoutTime = nCurrTime;

    uint64 nCheckEndTime = nCurrTime - 3600 * 24;
    std::vector<uint256> vTimeoutBlock;
    for (auto& kv : mapChnBlock)
    {
        if (kv.second.nCacheTime < nCheckEndTime)
        {
            vTimeoutBlock.push_back(kv.first);
        }
    }
    for (auto& hashBlock : vTimeoutBlock)
    {
        RemoveCacheBlock(hashBlock);
    }
}

void CBlockChannel::RequestNextBlockData(const uint256& hashFork, const uint256& hashPrevBlock, const uint64 nNonce)
{
    auto it = mapChnFork.find(hashFork);
    if (it == mapChnFork.end())
    {
        return;
    }
    CBlockChnFork& chnFork = it->second;

    chnFork.ClearPrevBlockHash(hashPrevBlock);

    uint256 hashPrev = hashPrevBlock;
    while (true)
    {
        uint256 hashNextBlock;
        if (!chnFork.GetNextBlockHash(hashPrev, hashNextBlock))
        {
            break;
        }
        chnFork.RemoveBlockHash(hashNextBlock);
        if (!pBlockChain->Exists(hashNextBlock))
        {
            SendGetBlockReq(hashFork, hashNextBlock, nNonce);
            break;
        }
        hashPrev = hashNextBlock;
    }
}

void CBlockChannel::SendNextPrevBlockReq(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce)
{
    network::CEventPeerBlockNextPrevBlock eventData(nNonce, hashFork);
    eventData.data = hashBlock;
    pPeerNet->DispatchEvent(&eventData);
}

void CBlockChannel::SendGetBlockReq(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce)
{
    network::CEventPeerBlockGetBlockReq eventData(nNonce, hashFork);
    eventData.data = hashBlock;
    pPeerNet->DispatchEvent(&eventData);
}

void CBlockChannel::SendGetBlockRsp(const uint256& hashFork, const uint256& hashBlock, const bytes& btBlockData, const uint64 nNonce)
{
    network::CEventPeerBlockGetBlockRsp eventData(nNonce, hashFork);
    eventData.data = btBlockData;
    pPeerNet->DispatchEvent(&eventData);
}

} // namespace metabasenet

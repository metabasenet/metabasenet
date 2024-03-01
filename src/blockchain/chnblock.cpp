// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblock.h"

#include <boost/bind.hpp>

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

#define MAX_CACHE_CHN_BLOCK_COUNT 2000
#define MAX_CACHE_CHN_BLOCK_SIZE (1024 * 1024 * 1024)

namespace metabasenet
{

CBlockChannel::CBlockChannel()
  : nPrevCheckCacheTimeoutTime(0), nCacheBlockByteCount(0)
{
    pPeerNet = nullptr;
    pDispatcher = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
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
    return true;
}

void CBlockChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pDispatcher = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CBlockChannel::HandleInvoke()
{
    return network::IBlockChannel::HandleInvoke();
}

void CBlockChannel::HandleHalt()
{
    network::IBlockChannel::HandleHalt();
}

bool CBlockChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockChnPeer(eventActive.data.nService, eventActive.data);
    StdLog("CBlockChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    if (!mapChnFork.empty())
    {
        network::CEventPeerBlockSubscribe eventSubscribe(nNonce, pCoreProtocol->GetGenesisBlockHash());
        for (auto& kv : mapChnFork)
        {
            eventSubscribe.data.push_back(kv.first);
        }
        pPeerNet->DispatchEvent(&eventSubscribe);
        StdLog("CBlockChannel", "CEvent Peer Active: Send fork subscribe, peer: %s", GetPeerAddressInfo(nNonce).c_str());
    }
    return true;
}

bool CBlockChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

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
        StdLog("CBlockChannel", "CEvent Peer Block Subscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    StdLog("CBlockChannel", "CEvent Peer Block Subscribe: Recv fork subscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());

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
        StdLog("CBlockChannel", "CEvent Peer Block Unsubscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    StdLog("CBlockChannel", "CEvent Peer Block Unsubscribe: Recv fork unsubscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());

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

    StdDebug("CBlockChannel", "CEvent Peer Block Bks: Recv block, block: [%d] %s, fork: %s",
             CBlock::GetBlockHeightByHash(eventBks.data.hashBlock), eventBks.data.hashBlock.GetHex().c_str(), hashFork.ToString().c_str());

    if (!pBlockChain->Exists(eventBks.data.hashPrev))
    {
        StdDebug("CBlockChannel", "CEvent Peer Block Bks: Prev block not exist, block: [%d] %s, prev: %s",
                 CBlock::GetBlockHeightByHash(eventBks.data.hashBlock), eventBks.data.hashBlock.GetHex().c_str(), eventBks.data.hashPrev.GetHex().c_str());

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
        AddCacheBlock(block, eventBks.data.btBlockData.size(), nRecvPeerNonce);
    }
    else if (pBlockChain->Exists(eventBks.data.hashBlock))
    {
        StdDebug("CBlockChannel", "CEvent Peer Block Bks: Block existed, block: [%d] %s, prev: %s",
                 CBlock::GetBlockHeightByHash(eventBks.data.hashBlock), eventBks.data.hashBlock.GetHex().c_str(), eventBks.data.hashPrev.GetHex().c_str());
    }
    else
    {
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
        if (pDispatcher->AddNewBlock(block, nRecvPeerNonce) != OK)
        {
            StdLog("CBlockChannel", "CEvent Peer Block Bks: Add new block fail, block: [%d] %s, fork: %s",
                   CBlock::GetBlockHeightByHash(eventBks.data.hashBlock), eventBks.data.hashBlock.GetHex().c_str(), hashFork.ToString().c_str());
        }
        else
        {
            StdDebug("CBlockChannel", "CEvent Peer Block Bks: Add block success, block: [%d] %s, fork: %s",
                     CBlock::GetBlockHeightByHash(eventBks.data.hashBlock), eventBks.data.hashBlock.GetHex().c_str(), hashFork.ToString().c_str());

            AddNextBlock(block.GetHash());
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
            StdLog("CBlockChannel", "CEvent Local Block Subscribe Fork: Fork is not enabled, fork: %s", hashFork.GetHex().c_str());
            continue;
        }
        StdDebug("CBlockChannel", "CEvent Local Block Subscribe Fork: Subscribe fork, fork: %s", hashFork.GetHex().c_str());

        mapChnFork.insert(std::make_pair(hashFork, CBlockChnFork(hashFork)));
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
            StdDebug("CBlockChannel", "CEvent Local Block Broadcast Bks: Broadcast block, peer nonce: 0x%x, txs: %lu, block: [%d] %s",
                     kv.first, eventBroadBks.data.block.vtx.size(), CBlock::GetBlockHeightByHash(eventBroadBks.data.hashBlock),
                     eventBroadBks.data.hashBlock.GetHex().c_str());
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

void CBlockChannel::AddCacheBlock(const CBlock& block, const uint64 nBlockSize, const uint64 nRecvNonce)
{
    const uint256 hashBlock = block.GetHash();
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

            StdDebug("CBlockChannel", "Add cache block: Add cache block success, cache count: %lu, bytes: %lu KB, type: %s, number: %lu, block: [%d] %s, prev: %s",
                     mapChnBlock.size(), nCacheBlockByteCount / 1024, GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), block.nNumber,
                     CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(), block.hashPrev.ToString().c_str());
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
                    StdLog("CBlockChannel", "Add next block: Add new block fail, block: [%d] %s, err: [%d] %s",
                           CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(), err, ErrorString(err));
                }
                else
                {
                    StdDebug("CBlockChannel", "Add next block: Add new block success, type: %s, block: [%d-%lu] %s",
                             GetBlockTypeStr(block.nType, block.txMint.GetTxType()).c_str(), CBlock::GetBlockHeightByHash(hashBlock), block.nNumber, hashBlock.GetHex().c_str());
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

} // namespace metabasenet

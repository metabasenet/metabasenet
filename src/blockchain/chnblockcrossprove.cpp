// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblockcrossprove.h"

#include <boost/bind.hpp>

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

#define BLOCK_CROSS_PROVE_CHANNEL_THREAD_COUNT 1
#define BLOCK_CROSS_PROVE_TIMER_TIME 1000
#define BLOCK_CROSS_PROVE_CACHE_BROADCAST_PROVE_COUNT 10000

namespace metabasenet
{

////////////////////////////////////////////////////
// CBlockCrossProveChnFork

////////////////////////////////////////////////////
// CBlockCrossProveChannel

CBlockCrossProveChannel::CBlockCrossProveChannel()
  : network::IBlockCrossProveChannel(BLOCK_CROSS_PROVE_CHANNEL_THREAD_COUNT), nBlockCrossProveTimerId(0)
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CBlockCrossProveChannel::~CBlockCrossProveChannel()
{
}

bool CBlockCrossProveChannel::HandleInitialize()
{
    if (!GetObject("peernet", pPeerNet))
    {
        Error("Failed to request peer net");
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

    StdLog("CBlockCrossProveChannel", "HandleInitialize");
    return true;
}

void CBlockCrossProveChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CBlockCrossProveChannel::HandleInvoke()
{
    nBlockCrossProveTimerId = SetTimer(BLOCK_CROSS_PROVE_TIMER_TIME, boost::bind(&CBlockCrossProveChannel::BlockCrossProveTimerFunc, this, _1));
    if (nBlockCrossProveTimerId == 0)
    {
        StdLog("CBlockCrossProveChannel", "Handle Invoke: Set timer fail");
        return false;
    }
    return network::IBlockCrossProveChannel::HandleInvoke();
}

void CBlockCrossProveChannel::HandleHalt()
{
    if (nBlockCrossProveTimerId != 0)
    {
        CancelTimer(nBlockCrossProveTimerId);
        nBlockCrossProveTimerId = 0;
    }
    network::IBlockCrossProveChannel::HandleHalt();
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockCrossProveChnPeer(eventActive.data.nService, eventActive.data);

    StdLog("CBlockCrossProveChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());
    return true;
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    const uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockCrossProveChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    mapChnPeer.erase(nNonce);
    return true;
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventPeerBlockCrossProveData& eventBcp)
{
    const uint64 nRecvPeerNonce = eventBcp.nNonce;
    const uint256& hashFork = eventBcp.hashFork;
    const bytes& btProveData = eventBcp.data;
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashFork);

    CBlockProve blockProve;
    if (!blockProve.Load(btProveData))
    {
        StdLog("CBlockChannel", "CEvent Peer Block CrossProve: Load block prove fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    bytes btCacheProve;
    if (GetBroadcastProveData(hashFork, blockProve.hashBlock, btCacheProve))
    {
        return true;
    }
    AddBroadcastProveData(hashFork, blockProve.hashBlock, btProveData);

    //pBlockChain->AddRecvCrosschainProve(nChainId, blockProve);

    for (const auto& kv : mapChnPeer)
    {
        if (nRecvPeerNonce == 0 || nRecvPeerNonce != kv.first)
        {
            if (!SendBlockProveData(kv.first, btProveData, hashFork))
            {
                StdLog("CBlockChannel", "CEvent Peer Block CrossProve: Broadcast block prove fail, peer nonce: 0x%lx", kv.first);
            }
        }
    }
    return true;
}

//------------------------------------------
bool CBlockCrossProveChannel::HandleEvent(network::CEventLocalBlockcrossproveTimer& eventTimer)
{
    return true;
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventLocalBlockcrossproveSubscribeFork& eventSubsFork)
{
    for (auto& hashFork : eventSubsFork.data)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            StdLog("CBlockCrossProveChannel", "CEvent Local Crossprove Subscribe Fork: Fork is not enabled, fork: %s", hashFork.GetHex().c_str());
            continue;
        }
        StdLog("CBlockCrossProveChannel", "CEvent Local Crossprove Subscribe Fork: Subscribe fork, last block: %s, ref block: %s, fork: %s",
               status.hashBlock.ToString().c_str(), status.hashRefBlock.ToString().c_str(), hashFork.GetHex().c_str());

        if (mapChnFork.count(hashFork) == 0)
        {
            auto it = mapChnFork.insert(std::make_pair(hashFork, CBlockCrossProveChnFork(hashFork))).first;
            if (it != mapChnFork.end())
            {
                StdDebug("CBlockCrossProveChannel", "CEvent Local Crossprove Subscribe Fork: Update block vote success, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
            else
            {
                StdLog("CBlockCrossProveChannel", "CEvent Local Crossprove Subscribe Fork: Add fork fail, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
        }
        else
        {
            StdLog("CBlockCrossProveChannel", "CEvent Local Crossprove Subscribe Fork: Fork existed, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
        }
    }
    return true;
}

bool CBlockCrossProveChannel::HandleEvent(network::CEventLocalBlockcrossproveBroadcastProve& eventBroadcastProve)
{
    const uint64 nRecvNonce = eventBroadcastProve.nNonce;
    const uint256& hashFork = eventBroadcastProve.hashFork;
    const CBlockBroadcastProve& broadProve = eventBroadcastProve.data;

    CBlockStatus statusBlock;
    if (!pBlockChain->GetBlockStatus(broadProve.hashBlock, statusBlock))
    {
        StdLog("CBlockChannel", "CEvent Local CrossProve Broadcast prove: Get block status fail, block: %s", broadProve.hashBlock.ToString().c_str());
        return false;
    }

    for (const auto& kv : broadProve.mapBlockProve)
    {
        const CChainId nChainId = kv.first;
        const CBlockProve& blockProve = kv.second;

        bytes btProveData;
        blockProve.Save(btProveData);

        uint256 hashRecvFork;
        if (!pBlockChain->GetForkHashByChainId(nChainId, hashRecvFork, statusBlock.hashRefBlock))
        {
            StdLog("CBlockChannel", "CEvent Local CrossProve Broadcast prove: Get fork hash by chainid fail, chainid: %d, ref block: %s", nChainId, statusBlock.hashRefBlock.ToString().c_str());
            continue;
        }

        bytes btCacheProve;
        if (GetBroadcastProveData(hashRecvFork, broadProve.hashBlock, btCacheProve))
        {
            continue;
        }
        AddBroadcastProveData(hashRecvFork, broadProve.hashBlock, btProveData);

        for (const auto& mkv : mapChnPeer)
        {
            if (nRecvNonce == 0 || mkv.first != nRecvNonce)
            {
                if (!SendBlockProveData(mkv.first, btProveData, hashRecvFork))
                {
                    StdLog("CBlockChannel", "CEvent Local CrossProve Broadcast prove: Broadcast block prove fail, peer nonce: 0x%lx, chainid: %d", mkv.first, nChainId);
                }
            }
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------
bool CBlockCrossProveChannel::SendBlockProveData(const uint64 nNetId, const bytes& btData, const uint256& hashFork)
{
    if (pPeerNet)
    {
        network::CEventPeerBlockCrossProveData eventBvp(nNetId, hashFork);
        eventBvp.data = btData;
        return pPeerNet->DispatchEvent(&eventBvp);
    }
    return false;
}

void CBlockCrossProveChannel::SubscribeFork(const uint256& hashFork, const uint64 nNonce)
{
    network::CEventLocalBlockcrossproveSubscribeFork* pEvent = new network::CEventLocalBlockcrossproveSubscribeFork(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.push_back(hashFork);
        PostEvent(pEvent);
    }
}

void CBlockCrossProveChannel::BroadcastBlockProve(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce, const std::map<CChainId, CBlockProve>& mapBlockProve)
{
    network::CEventLocalBlockcrossproveBroadcastProve* pEvent = new network::CEventLocalBlockcrossproveBroadcastProve(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.hashBlock = hashBlock;
        pEvent->data.mapBlockProve = mapBlockProve;
        PostEvent(pEvent);
    }
}

//-------------------------------------------------------------------------------------------
const string CBlockCrossProveChannel::GetPeerAddressInfo(uint64 nNonce)
{
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        return it->second.GetRemoteAddress();
    }
    return string("0.0.0.0");
}

void CBlockCrossProveChannel::BlockCrossProveTimerFunc(uint32 nTimerId)
{
    if (nTimerId == nBlockCrossProveTimerId)
    {
        nBlockCrossProveTimerId = SetTimer(BLOCK_CROSS_PROVE_TIMER_TIME, boost::bind(&CBlockCrossProveChannel::BlockCrossProveTimerFunc, this, _1));
        network::CEventLocalBlockcrossproveTimer* pEvent = new network::CEventLocalBlockcrossproveTimer(0, pCoreProtocol->GetGenesisBlockHash());
        if (pEvent)
        {
            pEvent->data = nTimerId;
            PostEvent(pEvent);
        }
    }
}

void CBlockCrossProveChannel::AddBroadcastProveData(const uint256& hashRecvFork, const uint256& hashSrcBlock, const bytes& btProveData)
{
    auto& mapForkProve = mapBroadcastProve[hashRecvFork];
    mapForkProve[hashSrcBlock] = btProveData;

    while (mapForkProve.size() > BLOCK_CROSS_PROVE_CACHE_BROADCAST_PROVE_COUNT)
    {
        mapForkProve.erase(mapForkProve.begin());
    }
}

bool CBlockCrossProveChannel::GetBroadcastProveData(const uint256& hashRecvFork, const uint256& hashSrcBlock, bytes& btProveData)
{
    auto it = mapBroadcastProve.find(hashRecvFork);
    if (it != mapBroadcastProve.end())
    {
        auto mt = it->second.find(hashSrcBlock);
        if (mt != it->second.end())
        {
            btProveData = mt->second;
            return true;
        }
    }
    return false;
}

} // namespace metabasenet

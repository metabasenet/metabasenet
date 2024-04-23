// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnusertx.h"

#include <boost/bind.hpp>

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

#define USER_TX_CHANNEL_THREAD_COUNT 1

namespace metabasenet
{

CUserTxChannel::CUserTxChannel()
  : network::IUserTxChannel(USER_TX_CHANNEL_THREAD_COUNT)
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

CUserTxChannel::~CUserTxChannel()
{
}

bool CUserTxChannel::HandleInitialize()
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

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool");
        return false;
    }
    return true;
}

void CUserTxChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

bool CUserTxChannel::HandleInvoke()
{
    return network::IUserTxChannel::HandleInvoke();
}

void CUserTxChannel::HandleHalt()
{
    network::IUserTxChannel::HandleHalt();
}

bool CUserTxChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CUserTxChnPeer(eventActive.data.nService, eventActive.data);
    StdLog("CUserTxChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    if (!mapChnFork.empty())
    {
        network::CEventPeerUsertxSubscribe eventSubscribe(nNonce, pCoreProtocol->GetGenesisBlockHash());
        for (auto& kv : mapChnFork)
        {
            eventSubscribe.data.push_back(kv.first);
        }
        pPeerNet->DispatchEvent(&eventSubscribe);
        StdLog("CUserTxChannel", "CEvent Peer Active: Send fork subscribe, peer: %s", GetPeerAddressInfo(nNonce).c_str());
    }
    return true;
}

bool CUserTxChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    uint64 nNonce = eventDeactive.nNonce;
    StdLog("CUserTxChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        mapChnPeer.erase(it);
    }
    return true;
}

bool CUserTxChannel::HandleEvent(network::CEventPeerUsertxSubscribe& eventSubscribe)
{
    const uint64 nNonce = eventSubscribe.nNonce;
    const uint256& hashFork = eventSubscribe.hashFork;

    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CUserTxChannel", "CEvent Peer Usertx Subscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    StdLog("CUserTxChannel", "CEvent Peer Usertx Subscribe: Recv fork subscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());

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

bool CUserTxChannel::HandleEvent(network::CEventPeerUsertxUnsubscribe& eventUnsubscribe)
{
    const uint64 nNonce = eventUnsubscribe.nNonce;
    const uint256& hashFork = eventUnsubscribe.hashFork;

    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CUserTxChannel", "CEvent Peer Usertx Unsubscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    StdLog("CUserTxChannel", "CEvent Peer Usertx Unsubscribe: Recv fork unsubscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());

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

bool CUserTxChannel::HandleEvent(network::CEventPeerUsertxTxs& eventTxs)
{
    const uint64 nRecvPeerNonce = eventTxs.nNonce;
    const uint256& hashFork = eventTxs.hashFork;
    std::vector<CTransaction> vTxs;
    try
    {
        CBufStream ss(eventTxs.data);
        ss >> vTxs;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    if (!vTxs.empty())
    {
        pTxPool->PushUserTx(nRecvPeerNonce, hashFork, vTxs);
    }
    return true;
}

bool CUserTxChannel::HandleEvent(network::CEventLocalUsertxSubscribeFork& eventSubsFork)
{
    network::CEventPeerUsertxSubscribe eventSubscribe(0, pCoreProtocol->GetGenesisBlockHash());
    for (auto& hashFork : eventSubsFork.data)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            StdLog("CUserTxChannel", "CEvent Local Usertx Subscribe Fork: Fork is not enabled, fork: %s", hashFork.GetHex().c_str());
            continue;
        }
        StdLog("CUserTxChannel", "CEvent Local Usertx Subscribe Fork: Subscribe fork, fork: %s", hashFork.GetHex().c_str());

        mapChnFork.insert(std::make_pair(hashFork, CUserTxChnFork(hashFork)));
        eventSubscribe.data.push_back(hashFork);
    }

    for (auto& kv : mapChnPeer)
    {
        eventSubscribe.nNonce = kv.first;
        pPeerNet->DispatchEvent(&eventSubscribe);
    }
    return true;
}

bool CUserTxChannel::HandleEvent(network::CEventLocalUsertxBroadcastTxs& eventBroadTxs)
{
    const uint64 nRecvPeerNonce = eventBroadTxs.nNonce;
    const uint256& hashFork = eventBroadTxs.hashFork;

    network::CEventPeerUsertxTxs eventCertTxs(0, hashFork);

    CBufStream ss;
    ss << eventBroadTxs.data;
    ss.GetData(eventCertTxs.data);

    for (const auto& [nonce, peer] : mapChnPeer)
    {
        if (peer.IsSubscribe(hashFork) && (nRecvPeerNonce == 0 || nRecvPeerNonce != nonce))
        {
            eventCertTxs.nNonce = nonce;
            pPeerNet->DispatchEvent(&eventCertTxs);
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------
void CUserTxChannel::SubscribeFork(const uint256& hashFork, const uint64 nNonce)
{
    network::CEventLocalUsertxSubscribeFork* pEvent = new network::CEventLocalUsertxSubscribeFork(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.push_back(hashFork);
        PostEvent(pEvent);
    }
}

void CUserTxChannel::BroadcastUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx)
{
    network::CEventLocalUsertxBroadcastTxs* pEvent = new network::CEventLocalUsertxBroadcastTxs(nRecvNetNonce, hashFork);
    if (pEvent)
    {
        pEvent->data = vtx;
        PostEvent(pEvent);
    }
}

const string CUserTxChannel::GetPeerAddressInfo(uint64 nNonce)
{
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        return it->second.GetRemoteAddress();
    }
    return string("0.0.0.0");
}

} // namespace metabasenet

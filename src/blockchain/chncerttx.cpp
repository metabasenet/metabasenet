// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chncerttx.h"

#include <boost/bind.hpp>

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

namespace metabasenet
{

CCertTxChannel::CCertTxChannel()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

CCertTxChannel::~CCertTxChannel()
{
}

bool CCertTxChannel::HandleInitialize()
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

void CCertTxChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
}

bool CCertTxChannel::HandleInvoke()
{
    return network::ICertTxChannel::HandleInvoke();
}

void CCertTxChannel::HandleHalt()
{
    network::ICertTxChannel::HandleHalt();
}

bool CCertTxChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CCertTxChnPeer(eventActive.data.nService, eventActive.data);

    network::CEventPeerCerttxSubscribe eventSubscribe(nNonce, pCoreProtocol->GetGenesisBlockHash());
    eventSubscribe.data.push_back(pCoreProtocol->GetGenesisBlockHash());
    pPeerNet->DispatchEvent(&eventSubscribe);

    StdLog("CCertTxChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());
    return true;
}

bool CCertTxChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    uint64 nNonce = eventDeactive.nNonce;
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        mapChnPeer.erase(it);
    }
    StdLog("CCertTxChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());
    return true;
}

bool CCertTxChannel::HandleEvent(network::CEventPeerCerttxSubscribe& eventSubscribe)
{
    const uint64 nNonce = eventSubscribe.nNonce;
    const uint256& hashFork = eventSubscribe.hashFork;
    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CCertTxChannel", "CEvent Peer Certtx Subscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        it->second.Subscribe();
    }
    StdLog("CCertTxChannel", "CEvent Peer Certtx Subscribe: Recv subscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
    return true;
}

bool CCertTxChannel::HandleEvent(network::CEventPeerCerttxUnsubscribe& eventUnsubscribe)
{
    const uint64 nNonce = eventUnsubscribe.nNonce;
    const uint256& hashFork = eventUnsubscribe.hashFork;
    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        StdLog("CCertTxChannel", "CEvent Peer Certtx Unsubscribe: Fork error, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
        return false;
    }
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        it->second.Unsubscribe();
    }
    StdLog("CCertTxChannel", "CEvent Peer Certtx Unsubscribe: Recv unsubscribe, peer: %s, fork: %s", GetPeerAddressInfo(nNonce).c_str(), hashFork.GetHex().c_str());
    return true;
}

bool CCertTxChannel::HandleEvent(network::CEventPeerCerttxTxs& eventTxs)
{
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
        pTxPool->PushCertTx(eventTxs.nNonce, vTxs);
    }
    return true;
}

bool CCertTxChannel::HandleEvent(network::CEventLocalCerttxBroadcastTxs& eventBroadTxs)
{
    const uint64 nRecvPeerNonce = eventBroadTxs.nNonce;
    network::CEventPeerCerttxTxs eventCertTxs(0, eventBroadTxs.hashFork);

    CBufStream ss;
    ss << eventBroadTxs.data;
    ss.GetData(eventCertTxs.data);

    for (auto& kv : mapChnPeer)
    {
        if (kv.second.IsSubscribe() && (nRecvPeerNonce == 0 || nRecvPeerNonce != kv.first))
        {
            eventCertTxs.nNonce = kv.first;
            pPeerNet->DispatchEvent(&eventCertTxs);
        }
    }
    return true;
}

//-------------------------------------------------------------------------------------------
void CCertTxChannel::BroadcastCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx)
{
    network::CEventLocalCerttxBroadcastTxs* pEvent = new network::CEventLocalCerttxBroadcastTxs(nRecvNetNonce, pCoreProtocol->GetGenesisBlockHash());
    if (pEvent)
    {
        pEvent->data = vtx;
        PostEvent(pEvent);
    }
}

const string CCertTxChannel::GetPeerAddressInfo(uint64 nNonce)
{
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        return it->second.GetRemoteAddress();
    }
    return string("0.0.0.0");
}

} // namespace metabasenet

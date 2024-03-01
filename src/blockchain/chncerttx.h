// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CHNCERTTX_H
#define METABASENET_CHNCERTTX_H

#include "base.h"
#include "peernet.h"
#include "schedule.h"

namespace metabasenet
{

class CCertTxChnPeer
{
public:
    CCertTxChnPeer()
      : nService(0), fSubscribe(false) {}
    CCertTxChnPeer(uint64 nServiceIn, const network::CAddress& addr)
      : nService(nServiceIn), addressRemote(addr), fSubscribe(false) {}

    const std::string GetRemoteAddress()
    {
        std::string strRemoteAddress;
        boost::asio::ip::tcp::endpoint ep;
        boost::system::error_code ec;
        addressRemote.ssEndpoint.GetEndpoint(ep);
        if (ep.address().is_v6())
        {
            strRemoteAddress = string("[") + ep.address().to_string(ec) + "]:" + std::to_string(ep.port());
        }
        else
        {
            strRemoteAddress = ep.address().to_string(ec) + ":" + std::to_string(ep.port());
        }
        return strRemoteAddress;
    }
    void Subscribe()
    {
        fSubscribe = true;
    }
    void Unsubscribe()
    {
        fSubscribe = false;
    }
    bool IsSubscribe() const
    {
        return fSubscribe;
    }

public:
    uint64 nService;
    network::CAddress addressRemote;
    bool fSubscribe;
};

class CCertTxChannel : public network::ICertTxChannel
{
public:
    CCertTxChannel();
    ~CCertTxChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerCerttxSubscribe& eventSubscribe) override;
    bool HandleEvent(network::CEventPeerCerttxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(network::CEventPeerCerttxTxs& eventTxs) override;

    bool HandleEvent(network::CEventLocalCerttxBroadcastTxs& eventBroadTxs) override;

public:
    void BroadcastCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx) override;

protected:
    const string GetPeerAddressInfo(uint64 nNonce);

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;

    std::map<uint64, CCertTxChnPeer> mapChnPeer;
};

} // namespace metabasenet

#endif // METABASENET_CHNCERTTX_H

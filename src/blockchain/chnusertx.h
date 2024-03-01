// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CHNUSERTX_H
#define METABASENET_CHNUSERTX_H

#include "base.h"
#include "peernet.h"
#include "schedule.h"

namespace metabasenet
{

class CUserTxChnPeer
{
public:
    CUserTxChnPeer()
      : nService(0) {}
    CUserTxChnPeer(uint64 nServiceIn, const network::CAddress& addr)
      : nService(nServiceIn), addressRemote(addr) {}

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
    void Subscribe(const uint256& hashFork)
    {
        setSubscribeFork.insert(hashFork);
    }
    void Unsubscribe(const uint256& hashFork)
    {
        setSubscribeFork.erase(hashFork);
    }
    bool IsSubscribe(const uint256& hashFork) const
    {
        return (setSubscribeFork.count(hashFork) > 0);
    }

public:
    uint64 nService;
    network::CAddress addressRemote;
    std::set<uint256> setSubscribeFork;
};

class CUserTxChnFork
{
public:
    CUserTxChnFork() {}
    CUserTxChnFork(const uint256& hashForkIn)
      : hashFork(hashForkIn) {}

public:
    uint256 hashFork;
};

class CUserTxChannel : public network::IUserTxChannel
{
public:
    CUserTxChannel();
    ~CUserTxChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerUsertxSubscribe& eventSubscribe) override;
    bool HandleEvent(network::CEventPeerUsertxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(network::CEventPeerUsertxTxs& eventTxs) override;

    bool HandleEvent(network::CEventLocalUsertxSubscribeFork& eventSubsFork) override;
    bool HandleEvent(network::CEventLocalUsertxBroadcastTxs& eventBroadTxs) override;

public:
    void SubscribeFork(const uint256& hashFork, const uint64 nNonce) override;
    void BroadcastUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx) override;

protected:
    const string GetPeerAddressInfo(uint64 nNonce);

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;

    std::map<uint64, CUserTxChnPeer> mapChnPeer;
    std::map<uint256, CUserTxChnFork> mapChnFork;
};

} // namespace metabasenet

#endif // METABASENET_CHNUSERTX_H

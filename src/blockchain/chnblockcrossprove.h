// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CHNBLOCKCROSSPROVE_H
#define METABASENET_CHNBLOCKCROSSPROVE_H

#include "base.h"
#include "consblockvote.h"
#include "peernet.h"

using namespace consensus;

namespace metabasenet
{

class CBlockCrossProveChnPeer
{
public:
    CBlockCrossProveChnPeer()
      : nService(0) {}
    CBlockCrossProveChnPeer(uint64 nServiceIn, const network::CAddress& addr)
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
        //return (setSubscribeFork.count(hashFork) > 0);
        return true;
    }

public:
    uint64 nService;
    network::CAddress addressRemote;
    std::set<uint256> setSubscribeFork;
};

class CBlockCrossProveChnFork
{
public:
    CBlockCrossProveChnFork(const uint256& hashForkIn)
      : hashFork(hashForkIn) {}

protected:
    const uint256 hashFork;
};

class CBlockCrossProveChannel : public network::IBlockCrossProveChannel
{
public:
    CBlockCrossProveChannel();
    ~CBlockCrossProveChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerBlockCrossProveData& eventBcp) override;

    bool HandleEvent(network::CEventLocalBlockcrossproveTimer& eventTimer) override;
    bool HandleEvent(network::CEventLocalBlockcrossproveSubscribeFork& eventSubsFork) override;
    bool HandleEvent(network::CEventLocalBlockcrossproveBroadcastProve& eventBroadcastProve) override;

public:
    bool SendBlockProveData(const uint64 nNetId, const bytes& btData, const uint256& hashFork);

    void SubscribeFork(const uint256& hashFork, const uint64 nNonce) override;
    void BroadcastBlockProve(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce, const std::map<CChainId, CBlockProve>& mapBlockProve) override;

protected:
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(mtbase::IBase::Config());
    }
    const string GetPeerAddressInfo(uint64 nNonce);
    void BlockCrossProveTimerFunc(uint32 nTimerId);
    void AddBroadcastProveData(const uint256& hashRecvFork, const uint256& hashSrcBlock, const bytes& btProveData);
    bool GetBroadcastProveData(const uint256& hashRecvFork, const uint256& hashSrcBlock, bytes& btProveData);

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    std::map<uint64, CBlockCrossProveChnPeer> mapChnPeer;
    std::map<uint256, CBlockCrossProveChnFork> mapChnFork;
    uint32 nBlockCrossProveTimerId;

    std::map<uint256, std::map<uint256, bytes, CustomBlockHashCompare>> mapBroadcastProve; // key1: recv fork hash, key2: src block hash, value: prove data
};

} // namespace metabasenet

#endif // METABASENET_CHNBLOCKCROSSPROVE_H

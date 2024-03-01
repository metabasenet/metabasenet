// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CHNBLOCK_H
#define METABASENET_CHNBLOCK_H

#include "base.h"
#include "peernet.h"
#include "schedule.h"

namespace metabasenet
{

class CBlockChnPeer
{
public:
    CBlockChnPeer()
      : nService(0) {}
    CBlockChnPeer(uint64 nServiceIn, const network::CAddress& addr)
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

class CBlockChnFork
{
public:
    CBlockChnFork() {}
    CBlockChnFork(const uint256& hashForkIn)
      : hashFork(hashForkIn) {}

public:
    uint256 hashFork;
};

class CChnCacheBlock
{
public:
    CChnCacheBlock(const CBlock& blockIn, const uint64 nBlockSizeIn, const uint64 nRecvNonceIn)
      : block(blockIn), nCacheTime(GetTime()), nBlockSize(nBlockSizeIn), nRecvNonce(nRecvNonceIn) {}

public:
    const CBlock block;
    const uint64 nCacheTime;
    const uint64 nBlockSize;
    const uint64 nRecvNonce;
};

class CBlockChannel : public network::IBlockChannel
{
public:
    CBlockChannel();
    ~CBlockChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerBlockSubscribe& eventSubscribe) override;
    bool HandleEvent(network::CEventPeerBlockUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(network::CEventPeerBlockBks& eventBks) override;

    bool HandleEvent(network::CEventLocalBlockSubscribeFork& eventSubsFork) override;
    bool HandleEvent(network::CEventLocalBlockBroadcastBks& eventBroadBks) override;

public:
    void SubscribeFork(const uint256& hashFork, const uint64 nNonce) override;
    void BroadcastBlock(const uint64 nRecvNetNonce, const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev, const CBlock& block) override;

protected:
    const string GetPeerAddressInfo(const uint64 nNonce);
    void AddCacheBlock(const CBlock& block, const uint64 nBlockSize, const uint64 nRecvNonce);
    void RemoveCacheBlock(const uint256& hashBlock);
    void AddNextBlock(const uint256& hashPrev);
    void ClearCacheTimeout();

protected:
    network::CBbPeerNet* pPeerNet;
    IDispatcher* pDispatcher;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    std::map<uint64, CBlockChnPeer> mapChnPeer;
    std::map<uint256, CBlockChnFork> mapChnFork;
    std::map<uint256, CChnCacheBlock> mapChnBlock;        // key: block hash
    std::map<uint256, std::set<uint256>> mapChnPrevBlock; // key: prev block, value: next block

    uint64 nPrevCheckCacheTimeoutTime;
    uint64 nCacheBlockByteCount;
};

} // namespace metabasenet

#endif // METABASENET_CHNBLOCK_H

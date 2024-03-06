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

    void AddPrevBlockHash(const uint256& hashPrevBlock, const uint256& hashBlock);
    bool AddBlockHash(const std::vector<uint256>& vBlockHash);
    bool GetNextBlockHash(const uint256& hashPrevBlock, uint256& hashBlock);
    void RemoveBlockHash(const uint256& hashBlock);
    void ClearPrevBlockHash(const uint256& hashLastExistBlock);
    bool CheckPrevExist(const uint256& hashBlock);

public:
    uint256 hashFork;

    std::map<uint256, uint256, CustomBlockHashCompare> mapBlockHash; // key: block hash, value: prev block hash
    std::map<uint256, uint256, CustomBlockHashCompare> mapPrevHash;  // key: prev hash, value: block hash
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
    bool HandleEvent(network::CEventPeerBlockNextPrevBlock& eventData) override;
    bool HandleEvent(network::CEventPeerBlockPrevBlocks& eventData) override;
    bool HandleEvent(network::CEventPeerBlockGetBlockReq& eventData) override;
    bool HandleEvent(network::CEventPeerBlockGetBlockRsp& eventData) override;

    bool HandleEvent(network::CEventLocalBlockSyncTimer& eventData) override;
    bool HandleEvent(network::CEventLocalBlockSubscribeFork& eventSubsFork) override;
    bool HandleEvent(network::CEventLocalBlockBroadcastBks& eventBroadBks) override;

public:
    void SubscribeFork(const uint256& hashFork, const uint64 nNonce) override;
    void BroadcastBlock(const uint64 nRecvNetNonce, const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev, const CBlock& block) override;

protected:
    const string GetPeerAddressInfo(const uint64 nNonce);
    void BlockSyncTimerFunc(uint32 nTimerId);
    void AddCacheBlock(const uint256& hashBlock, const CBlock& block, const uint64 nBlockSize, const uint64 nRecvNonce);
    void RemoveCacheBlock(const uint256& hashBlock);
    void AddNextBlock(const uint256& hashPrev);
    void ClearCacheTimeout();
    void RequestNextBlockData(const uint256& hashFork, const uint256& hashPrevBlock, const uint64 nNonce);
    void SendNextPrevBlockReq(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce);
    void SendGetBlockReq(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce);
    void SendGetBlockRsp(const uint256& hashFork, const uint256& hashBlock, const bytes& btBlockData, const uint64 nNonce);

protected:
    network::CBbPeerNet* pPeerNet;
    IDispatcher* pDispatcher;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    IBlockFilter* pBlockFilter;

    std::map<uint64, CBlockChnPeer> mapChnPeer;                                   // key: peer nonce
    std::map<uint256, CBlockChnFork> mapChnFork;                                  // key: fork hash
    std::map<uint256, CChnCacheBlock, CustomBlockHashCompare> mapChnBlock;        // key: block hash
    std::map<uint256, std::set<uint256>, CustomBlockHashCompare> mapChnPrevBlock; // key: prev block, value: next block

    uint64 nPrevCheckCacheTimeoutTime;
    uint64 nCacheBlockByteCount;
    uint32 nBlockSyncTimerId;
};

} // namespace metabasenet

#endif // METABASENET_CHNBLOCK_H

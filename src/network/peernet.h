// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NETWORK_PEERNET_H
#define NETWORK_PEERNET_H

#include "mtbase.h"
#include "peerevent.h"
#include "proto.h"

namespace metabasenet
{
namespace network
{

class INetChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    INetChannel()
      : IIOModule("netchannel") {}
    virtual int GetPrimaryChainHeight() = 0;
    virtual bool IsForkSynchronized(const uint256& hashFork) const = 0;
    virtual void BroadcastBlockInv(const uint256& hashFork, const uint256& hashBlock) = 0;
    virtual void BroadcastTxInv(const uint256& hashFork) = 0;
    virtual void SubscribeFork(const uint256& hashFork, const uint64& nNonce) = 0;
    virtual void UnsubscribeFork(const uint256& hashFork) = 0;
    virtual bool SubmitCachePowBlock(const CConsensusParam& consParam) = 0;
    virtual bool IsLocalCachePowBlock(int nHeight, bool& fIsDpos) = 0;
    virtual bool AddCacheLocalPowBlock(const CBlock& block) = 0;
};

class IBlockChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockChannel()
      : IIOModule("blockchannel") {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastBlock(const uint64 nRecvNetNonce, const uint256& hashFork, const uint256& hashBlock, const uint256& hashPrev, const CBlock& block) = 0;
};

class ICertTxChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    ICertTxChannel()
      : IIOModule("certtxchannel") {}
    virtual void BroadcastCertTx(const uint64 nRecvNetNonce, const std::vector<CTransaction>& vtx) = 0;
};

class IUserTxChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IUserTxChannel(const uint32 nThreadCount = 1)
      : IIOModule("usertxchannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastUserTx(const uint64 nRecvNetNonce, const uint256& hashFork, const std::vector<CTransaction>& vtx) = 0;
};

class IBlockVoteChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockVoteChannel(const uint32 nThreadCount = 1)
      : IIOModule("blockvotechannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void UpdateNewBlock(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock, const int64 nBlockTime, const uint64 nNonce) = 0;
};

class IBlockCrossProveChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IBlockCrossProveChannel(const uint32 nThreadCount = 1)
      : IIOModule("blockcrossprovechannel", nThreadCount) {}
    virtual void SubscribeFork(const uint256& hashFork, const uint64 nNonce) = 0;
    virtual void BroadcastBlockProve(const uint256& hashFork, const uint256& hashBlock, const uint64 nNonce, const std::map<CChainId, CBlockProve>& mapBlockProve) = 0;
};

class IDelegatedChannel : public mtbase::IIOModule, virtual public CBbPeerEventListener
{
public:
    IDelegatedChannel()
      : IIOModule("delegatedchannel") {}
    virtual void PrimaryUpdate(int nStartHeight,
                               const std::vector<std::pair<uint256, std::map<CDestination, size_t>>>& vEnrolledWeight,
                               const std::vector<std::pair<uint256, std::map<CDestination, std::vector<unsigned char>>>>& vDistributeData,
                               const std::map<CDestination, std::vector<unsigned char>>& mapPublishData,
                               const uint256& hashDistributeOfPublish)
        = 0;
};

class CBbPeerNet : public mtbase::CPeerNet, virtual public CBbPeerEventListener
{
public:
    CBbPeerNet();
    ~CBbPeerNet();
    virtual void BuildHello(mtbase::CPeer* pPeer, mtbase::CBufStream& ssPayload);
    virtual uint32 BuildPing(mtbase::CPeer* pPeer, mtbase::CBufStream& ssPayload);
    void HandlePeerWriten(mtbase::CPeer* pPeer) override;
    virtual bool HandlePeerHandshaked(mtbase::CPeer* pPeer, uint32 nTimerId);
    virtual bool HandlePeerRecvMessage(mtbase::CPeer* pPeer, int nChannel, int nCommand,
                                       mtbase::CBufStream& ssPayload);
    uint32 SetPingTimer(uint32 nOldTimerId, uint64 nNonce, int64 nElapse);

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleEvent(CEventPeerSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerInv& eventInv) override;
    bool HandleEvent(CEventPeerGetData& eventGetData) override;
    bool HandleEvent(CEventPeerGetBlocks& eventGetBlocks) override;
    bool HandleEvent(CEventPeerTx& eventTx) override;
    bool HandleEvent(CEventPeerBlock& eventBlock) override;
    bool HandleEvent(CEventPeerGetFail& eventGetFail) override;
    bool HandleEvent(CEventPeerMsgRsp& eventMsgRsp) override;

    bool HandleEvent(CEventPeerBlockSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerBlockUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerBlockBks& eventBks) override;
    bool HandleEvent(CEventPeerBlockNextPrevBlock& eventData) override;
    bool HandleEvent(CEventPeerBlockPrevBlocks& eventData) override;
    bool HandleEvent(CEventPeerBlockGetBlockReq& eventData) override;
    bool HandleEvent(CEventPeerBlockGetBlockRsp& eventData) override;

    bool HandleEvent(CEventPeerCerttxSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerCerttxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerCerttxTxs& eventTxs) override;

    bool HandleEvent(CEventPeerUsertxSubscribe& eventSubscribe) override;
    bool HandleEvent(CEventPeerUsertxUnsubscribe& eventUnsubscribe) override;
    bool HandleEvent(CEventPeerUsertxTxs& eventTxs) override;

    bool HandleEvent(CEventPeerBlockVoteProtoData& eventBvp) override;

    bool HandleEvent(CEventPeerBlockCrossProveData& eventBcp) override;

    bool HandleEvent(CEventPeerBulletin& eventBulletin) override;
    bool HandleEvent(CEventPeerGetDelegated& eventGetDelegated) override;
    bool HandleEvent(CEventPeerDistribute& eventDistribute) override;
    bool HandleEvent(CEventPeerPublish& eventPublish) override;

    mtbase::CPeer* CreatePeer(mtbase::CIOClient* pClient, uint64 nNonce, bool fInBound) override;
    void DestroyPeer(mtbase::CPeer* pPeer) override;
    mtbase::CPeerInfo* GetPeerInfo(mtbase::CPeer* pPeer, mtbase::CPeerInfo* pInfo) override;
    CAddress GetGateWayAddress(const CNetHost& gateWayAddr);
    bool SendChannelMessage(int nChannel, uint64 nNonce, int nCommand, CBufStream& ssPayload);
    bool SendDataMessage(uint64 nNonce, int nCommand, mtbase::CBufStream& ssPayload);
    bool SendDelegatedMessage(uint64 nNonce, int nCommand, mtbase::CBufStream& ssPayload);
    bool SetInvTimer(uint64 nNonce, std::vector<CInv>& vInv);
    virtual void ProcessAskFor(mtbase::CPeer* pPeer);
    void Configure(uint32 nMagicNumIn, uint32 nVersionIn, uint64 nServiceIn,
                   const std::string& subVersionIn, bool fEnclosedIn, const uint256& hashGenesisIn)
    {
        nMagicNum = nMagicNumIn;
        nVersion = nVersionIn;
        nService = nServiceIn;
        subVersion = subVersionIn;
        fEnclosed = fEnclosedIn;
        hashGenesis = hashGenesisIn;
    }
    virtual bool CheckPeerVersion(uint32 nVersionIn, uint64 nServiceIn, const std::string& subVersionIn) = 0;
    uint32 CreateSeq(uint64 nNonce);

protected:
    INetChannel* pNetChannel;
    IDelegatedChannel* pDelegatedChannel;
    IBlockChannel* pBlockChannel;
    ICertTxChannel* pCertTxChannel;
    IUserTxChannel* pUserTxChannel;
    IBlockVoteChannel* pBlockVoteChannel;
    IBlockCrossProveChannel* pBlockCrossProveChannel;
    uint32 nMagicNum;
    uint32 nVersion;
    uint64 nService;
    bool fEnclosed;
    std::string subVersion;
    uint256 hashGenesis;
    std::set<boost::asio::ip::tcp::endpoint> setDNSeed;
    uint64 nSeqCreate;
};

} // namespace network
} // namespace metabasenet

#endif //NETWORK_PEERNET_H

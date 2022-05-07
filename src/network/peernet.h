// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NETWORK_PEERNET_H
#define NETWORK_PEERNET_H

#include "hcode.h"
#include "peerevent.h"
#include "proto.h"

namespace metabasenet
{
namespace network
{

class INetChannel : public hcode::IIOModule, virtual public CBbPeerEventListener
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

class IDelegatedChannel : public hcode::IIOModule, virtual public CBbPeerEventListener
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

class CBbPeerNet : public hcode::CPeerNet, virtual public CBbPeerEventListener
{
public:
    CBbPeerNet();
    ~CBbPeerNet();
    virtual void BuildHello(hcode::CPeer* pPeer, hcode::CBufStream& ssPayload);
    virtual uint32 BuildPing(hcode::CPeer* pPeer, hcode::CBufStream& ssPayload);
    void HandlePeerWriten(hcode::CPeer* pPeer) override;
    virtual bool HandlePeerHandshaked(hcode::CPeer* pPeer, uint32 nTimerId);
    virtual bool HandlePeerRecvMessage(hcode::CPeer* pPeer, int nChannel, int nCommand,
                                       hcode::CBufStream& ssPayload);
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
    bool HandleEvent(CEventPeerBulletin& eventBulletin) override;
    bool HandleEvent(CEventPeerGetDelegated& eventGetDelegated) override;
    bool HandleEvent(CEventPeerDistribute& eventDistribute) override;
    bool HandleEvent(CEventPeerPublish& eventPublish) override;
    hcode::CPeer* CreatePeer(hcode::CIOClient* pClient, uint64 nNonce, bool fInBound) override;
    void DestroyPeer(hcode::CPeer* pPeer) override;
    hcode::CPeerInfo* GetPeerInfo(hcode::CPeer* pPeer, hcode::CPeerInfo* pInfo) override;
    CAddress GetGateWayAddress(const CNetHost& gateWayAddr);
    bool SendDataMessage(uint64 nNonce, int nCommand, hcode::CBufStream& ssPayload);
    bool SendDelegatedMessage(uint64 nNonce, int nCommand, hcode::CBufStream& ssPayload);
    bool SetInvTimer(uint64 nNonce, std::vector<CInv>& vInv);
    virtual void ProcessAskFor(hcode::CPeer* pPeer);
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

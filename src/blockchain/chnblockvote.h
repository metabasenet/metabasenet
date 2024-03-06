// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CHNBLOCKVOTE_H
#define METABASENET_CHNBLOCKVOTE_H

#include "base.h"
#include "consblockvote.h"
#include "peernet.h"

using namespace consensus;
using namespace consensus::consblockvote;

namespace metabasenet
{

class CBlockVoteChnPeer
{
public:
    CBlockVoteChnPeer()
      : nService(0) {}
    CBlockVoteChnPeer(uint64 nServiceIn, const network::CAddress& addr)
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

public:
    uint64 nService;
    network::CAddress addressRemote;
};

class CBlockVoteResult
{
public:
    CBlockVoteResult()
      : nVoteBeginTimeMillis(GetTimeMillis()), nVoteCommitTimeMillis(0) {}

    void SetVoteResult(const bytes& btBitmapIn, const bytes& btAggSigIn)
    {
        btBitmap = btBitmapIn;
        btAggSig = btAggSigIn;
        nVoteCommitTimeMillis = GetTimeMillis();
    }
    void GetVoteResult(bytes& btBitmapOut, bytes& btAggSigOut) const
    {
        btBitmapOut = btBitmap;
        btAggSigOut = btAggSig;
    }
    int64 GetVoteBeginTime() const
    {
        return nVoteBeginTimeMillis;
    }
    int64 GetVoteEndTime() const
    {
        return nVoteCommitTimeMillis;
    }

protected:
    bytes btBitmap;
    bytes btAggSig;

    int64 nVoteBeginTimeMillis;
    int64 nVoteCommitTimeMillis;
};

class CBlockVoteChnFork
{
public:
    CBlockVoteChnFork(const uint256& hashForkIn, const int64 nEpochDurationIn, funcSendNetData sendNetDataIn, funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkeyIn, funcAddBlockLocalSignFlag addBlockLocalSignFlagIn, funcCommitVoteResult commitVoteResultIn)
      : hashFork(hashForkIn), consBlockVote(network::PROTO_CHN_BLOCK_VOTE, nEpochDurationIn, sendNetDataIn, getVoteBlockCandidatePubkeyIn, addBlockLocalSignFlagIn, commitVoteResultIn) {}

    bool AddLocalConsKey(const uint256& prikey, const uint384& pubkey);
    bool AddBlockCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, const vector<uint384>& vPubkey);
    bool GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig);
    bool AddPeerNode(const uint64 nPeerNonce);
    void RemovePeerNode(const uint64 nPeerNonce);
    void OnNetData(const uint64 nPeerNonce, const bytes& btNetData);
    void OnTimer();
    void SetVoteResult(const uint256& hashBlock, const bytes& btBitmapIn, const bytes& btAggSigIn);

protected:
    const uint256 hashFork;
    CConsBlockVote consBlockVote;
    std::map<uint256, CBlockVoteResult, CustomBlockHashCompare> mapVoteResult;
};

class CBlockVoteChannel : public network::IBlockVoteChannel
{
public:
    CBlockVoteChannel();
    ~CBlockVoteChannel();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool HandleEvent(network::CEventPeerActive& eventActive) override;
    bool HandleEvent(network::CEventPeerDeactive& eventDeactive) override;

    bool HandleEvent(network::CEventPeerBlockVoteProtoData& eventBvp) override;

    bool HandleEvent(network::CEventLocalBlockvoteTimer& eventTimer) override;
    bool HandleEvent(network::CEventLocalBlockvoteSubscribeFork& eventSubsFork) override;
    bool HandleEvent(network::CEventLocalBlockvoteUpdateBlock& eventUpdateBlock) override;
    bool HandleEvent(network::CEventLocalBlockvoteCommitVoteResult& eventVoteResult) override;

public:
    bool SendNetData(const uint64 nNetId, const uint8 nTunnelId, const bytes& btData, const uint256& hashFork);
    bool GetVoteBlockCandidatePubkey(const uint256& hashBlock, uint32& nBlockHeight, int64& nBlockTime, vector<uint384>& vPubkey, bytes& btAggBitmap, bytes& btAggSig, const uint256& hashFork);
    bool AddBlockLocalVoteSignFlag(const uint256& hashBlock, const uint256& hashFork);
    bool CommitBlockVoteResult(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig, const uint256& hashFork);
    bool AddBlockVoteCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, CBlockVoteChnFork& chnFork);

    void SubscribeFork(const uint256& hashFork, const uint64 nNonce) override;
    void UpdateNewBlock(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock, const int64 nBlockTime, const uint64 nNonce) override;

protected:
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(mtbase::IBase::Config());
    }
    const string GetPeerAddressInfo(uint64 nNonce);
    void BlockVoteTimerFunc(uint32 nTimerId);

protected:
    network::CBbPeerNet* pPeerNet;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    std::map<uint64, CBlockVoteChnPeer> mapChnPeer;
    std::map<uint256, CBlockVoteChnFork> mapChnFork;
    uint32 nBlockVoteTimerId;
};

} // namespace metabasenet

#endif // METABASENET_CHNBLOCKVOTE_H

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chnblockvote.h"

#include <boost/bind.hpp>

using namespace std;
using namespace mtbase;
using namespace consensus::consblockvote;
using boost::asio::ip::tcp;

#define BLOCK_VOTE_CHANNEL_THREAD_COUNT 1
#define BLOCK_VOTE_TIMER_TIME 100
#define MAX_BLOCK_VOTE_RESULT_COUNT 1000

namespace metabasenet
{

////////////////////////////////////////////////////
// CBlockVoteChnFork

bool CBlockVoteChnFork::AddLocalConsKey(const uint256& prikey, const uint384& pubkey)
{
    return consBlockVote.AddConsKey(prikey, pubkey);
}

bool CBlockVoteChnFork::AddBlockCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, const vector<uint384>& vPubkey)
{
    while (mapVoteResult.size() >= MAX_BLOCK_VOTE_RESULT_COUNT)
    {
        mapVoteResult.erase(mapVoteResult.begin());
    }
    mapVoteResult[hashBlock] = CBlockVoteResult();
    return consBlockVote.AddCandidatePubkey(hashBlock, nBlockHeight, nBlockTime, vPubkey);
}

bool CBlockVoteChnFork::GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig)
{
    return consBlockVote.GetBlockVoteResult(hashBlock, btBitmap, btAggSig);
}

bool CBlockVoteChnFork::AddPeerNode(const uint64 nPeerNonce)
{
    return consBlockVote.AddNetNode(nPeerNonce);
}

void CBlockVoteChnFork::RemovePeerNode(const uint64 nPeerNonce)
{
    consBlockVote.RemoveNetNode(nPeerNonce);
}

void CBlockVoteChnFork::OnNetData(const uint64 nPeerNonce, const bytes& btNetData)
{
    consBlockVote.OnEventNetData(nPeerNonce, btNetData);
}

void CBlockVoteChnFork::OnTimer()
{
    consBlockVote.OnTimer();
}

void CBlockVoteChnFork::SetVoteResult(const uint256& hashBlock, const bytes& btBitmapIn, const bytes& btAggSigIn)
{
    auto it = mapVoteResult.find(hashBlock);
    if (it != mapVoteResult.end())
    {
        it->second.SetVoteResult(btBitmapIn, btAggSigIn);

        CBitmap bmVoteBitmap;
        bmVoteBitmap.ImportBytes(btBitmapIn);
        StdDebug("TEST", "Commit Block Vote Result: vote bitmap: %s, agg sig: %s, vote duration: %lu ms, block: [%d] %s, fork: %s",
                 bmVoteBitmap.GetBitmapString().c_str(), ToHexString(btAggSigIn).c_str(), GetTimeMillis() - it->second.GetVoteBeginTime(),
                 CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
    }
}

////////////////////////////////////////////////////
// CBlockVoteChannel

CBlockVoteChannel::CBlockVoteChannel()
  : network::IBlockVoteChannel(BLOCK_VOTE_CHANNEL_THREAD_COUNT), nBlockVoteTimerId(0)
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

CBlockVoteChannel::~CBlockVoteChannel()
{
}

bool CBlockVoteChannel::HandleInitialize()
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

    StdLog("CBlockVoteChannel", "HandleInitialize");
    return true;
}

void CBlockVoteChannel::HandleDeinitialize()
{
    pPeerNet = nullptr;
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CBlockVoteChannel::HandleInvoke()
{
    nBlockVoteTimerId = SetTimer(BLOCK_VOTE_TIMER_TIME, boost::bind(&CBlockVoteChannel::BlockVoteTimerFunc, this, _1));
    if (nBlockVoteTimerId == 0)
    {
        StdLog("CBlockVoteChannel", "Handle Invoke: Set timer fail");
        return false;
    }
    return network::IBlockVoteChannel::HandleInvoke();
}

void CBlockVoteChannel::HandleHalt()
{
    if (nBlockVoteTimerId != 0)
    {
        CancelTimer(nBlockVoteTimerId);
        nBlockVoteTimerId = 0;
    }
    network::IBlockVoteChannel::HandleHalt();
}

bool CBlockVoteChannel::HandleEvent(network::CEventPeerActive& eventActive)
{
    const uint64 nNonce = eventActive.nNonce;
    mapChnPeer[nNonce] = CBlockVoteChnPeer(eventActive.data.nService, eventActive.data);

    StdLog("CBlockVoteChannel", "CEvent Peer Active: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    for (auto& kv : mapChnFork)
    {
        kv.second.AddPeerNode(nNonce);
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventPeerDeactive& eventDeactive)
{
    const uint64 nNonce = eventDeactive.nNonce;
    StdLog("CBlockVoteChannel", "CEvent Peer Deactive: peer: %s", GetPeerAddressInfo(nNonce).c_str());

    mapChnPeer.erase(nNonce);
    for (auto& kv : mapChnFork)
    {
        kv.second.RemovePeerNode(nNonce);
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventPeerBlockVoteProtoData& eventBvp)
{
    auto it = mapChnFork.find(eventBvp.hashFork);
    if (it != mapChnFork.end())
    {
        it->second.OnNetData(eventBvp.nNonce, eventBvp.data);
    }
    return true;
}

//------------------------------------------
bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteTimer& eventTimer)
{
    for (auto& kv : mapChnFork)
    {
        kv.second.OnTimer();
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteSubscribeFork& eventSubsFork)
{
    for (auto& hashFork : eventSubsFork.data)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(hashFork, status))
        {
            StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Fork is not enabled, fork: %s", hashFork.GetHex().c_str());
            continue;
        }
        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Subscribe fork, last block: %s, ref block: %s, fork: %s",
               status.hashBlock.ToString().c_str(), status.hashRefBlock.ToString().c_str(), hashFork.GetHex().c_str());

        if (mapChnFork.count(hashFork) == 0)
        {
            int64 nEpochDuration = 0;
            if (hashFork == pCoreProtocol->GetGenesisBlockHash())
            {
                nEpochDuration = BLOCK_TARGET_SPACING * 1000;
            }
            else
            {
                nEpochDuration = EXTENDED_BLOCK_SPACING * 1000;
            }

            auto it = mapChnFork.insert(std::make_pair(hashFork, CBlockVoteChnFork(hashFork, nEpochDuration,
                                                                                   boost::bind(&CBlockVoteChannel::SendNetData, this, _1, _2, _3, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::GetVoteBlockCandidatePubkey, this, _1, _2, _3, _4, _5, _6, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::AddBlockLocalVoteSignFlag, this, _1, hashFork),
                                                                                   boost::bind(&CBlockVoteChannel::CommitBlockVoteResult, this, _1, _2, _3, hashFork))))
                          .first;
            if (it != mapChnFork.end())
            {
                for (auto& kv : MintConfig()->mapMint)
                {
                    const uint256& keyMint = kv.first;
                    CCryptoBlsKey blskey;
                    if (!crypto::CryptoBlsMakeNewKey(blskey, keyMint))
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Make new key fail, fork: %s", hashFork.GetHex().c_str());
                        continue;
                    }
                    if (!it->second.AddLocalConsKey(blskey.secret, blskey.pubkey))
                    {
                        StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add local cons key fail, fork: %s", hashFork.GetHex().c_str());
                        continue;
                    }
                }
                for (auto& kv : mapChnPeer)
                {
                    it->second.AddPeerNode(kv.first);
                }

                if (!AddBlockVoteCandidatePubkey(status.hashBlock, status.nBlockHeight, status.nBlockTime, it->second))
                {
                    StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Update block vote fail, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
                    continue;
                }
                StdDebug("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Update block vote success, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
            else
            {
                StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Add fork fail, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
            }
        }
        else
        {
            StdLog("CBlockVoteChannel", "CEvent Local Blockvote Subscribe Fork: Fork existed, last block: %s, fork: %s", status.hashBlock.ToString().c_str(), hashFork.GetHex().c_str());
        }
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteUpdateBlock& eventUpdateBlock)
{
    const uint256& hashFork = eventUpdateBlock.hashFork;
    const std::vector<network::CUpdateBlockVote>& vUpdate = eventUpdateBlock.data;
    auto it = mapChnFork.find(hashFork);
    if (it != mapChnFork.end())
    {
        CBlockVoteChnFork& chnFork = it->second;
        for (const network::CUpdateBlockVote& updateBlockVote : vUpdate)
        {
            if (!AddBlockVoteCandidatePubkey(updateBlockVote.hashBlock, updateBlockVote.nBlockHeight, updateBlockVote.nBlockTime, chnFork))
            {
                StdLog("CBlockVoteChannel", "CEvent Local Blockvote Update Block: Add candidate pubkey fail, block: %s", updateBlockVote.hashBlock.GetBhString().c_str());
            }
        }
    }
    return true;
}

bool CBlockVoteChannel::HandleEvent(network::CEventLocalBlockvoteCommitVoteResult& eventVoteResult)
{
    const uint256& hashFork = eventVoteResult.hashFork;
    const network::CBlockCommitVoteResult& voteResult = eventVoteResult.data;
    StdDebug("CBlockVoteChannel", "Event commit block vote result: block: %s, fork: %s", voteResult.hashBlock.ToString().c_str(), hashFork.ToString().c_str());
    auto it = mapChnFork.find(hashFork);
    if (it != mapChnFork.end())
    {
        it->second.SetVoteResult(voteResult.hashBlock, voteResult.btBitmap, voteResult.btAggSig);

        bytes btDbBitmap;
        bytes btDbAggSig;
        bool fDbAtChain = false;
        uint256 hashDbAtBlock;
        if (!pBlockChain->RetrieveBlockVoteResult(voteResult.hashBlock, btDbBitmap, btDbAggSig, fDbAtChain, hashDbAtBlock))
        {
            if (!pBlockChain->AddBlockVoteResult(voteResult.hashBlock, true, voteResult.btBitmap, voteResult.btAggSig, false, uint256()))
            {
                StdLog("CBlockVoteChannel", "Event commit block vote result: Add block vote result fail, hashBlock: %s", voteResult.hashBlock.ToString().c_str());
            }
            else
            {
                StdDebug("CBlockVoteChannel", "Event commit block vote result: Add vote result success, block: %s, fork: %s", voteResult.hashBlock.ToString().c_str(), hashFork.ToString().c_str());
            }
        }
    }
    else
    {
        StdLog("CBlockVoteChannel", "Event commit block vote result: Fork not exist, block: %s, fork: %s", voteResult.hashBlock.ToString().c_str(), hashFork.ToString().c_str());
    }
    return true;
}

//-------------------------------------------------------------------------------------------
bool CBlockVoteChannel::SendNetData(const uint64 nNetId, const uint8 nTunnelId, const bytes& btData, const uint256& hashFork)
{
    if (pPeerNet)
    {
        network::CEventPeerBlockVoteProtoData eventBvp(nNetId, hashFork);
        eventBvp.data = btData;
        return pPeerNet->DispatchEvent(&eventBvp);
    }
    StdError("CBlockVoteChannel", "Send Net Data: pPeerNet is NULL, fork: %s", hashFork.ToString().c_str());
    return false;
}

bool CBlockVoteChannel::GetVoteBlockCandidatePubkey(const uint256& hashBlock, uint32& nBlockHeight, int64& nBlockTime, vector<uint384>& vPubkey, bytes& btAggBitmap, bytes& btAggSig, const uint256& hashFork)
{
    btAggBitmap.clear();
    btAggSig.clear();

    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CBlockVoteChannel", "Get vote block candidate pubkey: Retrieve fork last fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }
    if (!pBlockChain->VerifySameChain(hashBlock, hashLastBlock))
    {
        StdLog("CBlockVoteChannel", "Get vote block candidate pubkey: Verify same chain fail, block: %s, last block: %s, fork: %s",
               hashBlock.GetBhString().c_str(), hashLastBlock.GetBhString().c_str(), hashFork.ToString().c_str());
        return false;
    }

    CBlockStatus status;
    if (!pBlockChain->GetBlockStatus(hashBlock, status))
    {
        StdLog("CBlockVoteChannel", "Get vote block candidate pubkey: Get block status fail, block: %s, fork: %s",
               hashBlock.GetBhString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    if (!pBlockChain->GetPrevBlockCandidatePubkey(hashBlock, vPubkey))
    {
        StdLog("CBlockVoteChannel", "Get vote block candidate pubkey: Get prev block candidate pubkey fail, block: %s, fork: %s",
               hashBlock.GetBhString().c_str(), hashFork.ToString().c_str());
        return false;
    }
    nBlockHeight = status.nBlockHeight;
    nBlockTime = status.nBlockTime;

    bool fAtChain = false;
    uint256 hashAtBlock;
    if (!pBlockChain->RetrieveBlockVoteResult(hashBlock, btAggBitmap, btAggSig, fAtChain, hashAtBlock))
    {
        btAggBitmap.clear();
        btAggSig.clear();
    }
    return true;
}

bool CBlockVoteChannel::AddBlockLocalVoteSignFlag(const uint256& hashBlock, const uint256& hashFork)
{
    // uint256 hashLastBlock;
    // if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    // {
    //     StdLog("CBlockVoteChannel", "Add block local sign flag: Retrieve fork last fail, fork: %s", hashFork.ToString().c_str());
    //     return false;
    // }
    // if (!pBlockChain->VerifySameChain(hashBlock, hashLastBlock))
    // {
    //     StdLog("CBlockVoteChannel", "Add block local sign flag: Verify same chain fail, block: %s, last block: %s, fork: %s",
    //            hashBlock.GetBhString().c_str(), hashLastBlock.GetBhString().c_str(), hashFork.ToString().c_str());
    //     return false;
    // }
    // if (!pBlockChain->AddBlockLocalVoteSignFlag(hashBlock))
    // {
    //     StdLog("CBlockVoteChannel", "Add block local sign flag: Add block local sign flag fail, block: %s, fork: %s",
    //            hashBlock.GetBhString().c_str(), hashFork.ToString().c_str());
    //     return false;
    // }
    return true;
}

bool CBlockVoteChannel::CommitBlockVoteResult(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig, const uint256& hashFork)
{
    StdDebug("CBlockVoteChannel", "Commit block vote result: block: %s, fork: %s", hashBlock.ToString().c_str(), hashFork.ToString().c_str());
    network::CEventLocalBlockvoteCommitVoteResult* pEvent = new network::CEventLocalBlockvoteCommitVoteResult(0, hashFork);
    if (pEvent)
    {
        network::CBlockCommitVoteResult& voteResult = pEvent->data;
        voteResult.hashBlock = hashBlock;
        voteResult.btBitmap = btBitmap;
        voteResult.btAggSig = btAggSig;
        PostEvent(pEvent);
    }
    else
    {
        StdError("CBlockVoteChannel", "Commit block vote result: New fail, fork: %s", hashFork.ToString().c_str());
    }
    return true;
}

bool CBlockVoteChannel::AddBlockVoteCandidatePubkey(const uint256& hashBlock, const uint32 nBlockHeight, const int64 nBlockTime, CBlockVoteChnFork& chnFork)
{
    std::vector<uint384> vCandidatePubkey;
    if (!pBlockChain->GetPrevBlockCandidatePubkey(hashBlock, vCandidatePubkey))
    {
        StdLog("CBlockVoteChannel", "Add block vote candidate pubkey: Get candidate pubkey fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    if (vCandidatePubkey.empty())
    {
        StdDebug("CBlockVoteChannel", "Add block vote candidate pubkey: Candiadate pubkey is empty, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    if (!chnFork.AddBlockCandidatePubkey(hashBlock, nBlockHeight, nBlockTime * 1000, vCandidatePubkey))
    {
        StdLog("CBlockVoteChannel", "Add block vote candidate pubkey Add candidate pubkey fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

void CBlockVoteChannel::SubscribeFork(const uint256& hashFork, const uint64 nNonce)
{
    network::CEventLocalBlockvoteSubscribeFork* pEvent = new network::CEventLocalBlockvoteSubscribeFork(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.push_back(hashFork);
        PostEvent(pEvent);
    }
}

void CBlockVoteChannel::UpdateNewBlock(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock, const int64 nBlockTime, const uint64 nNonce)
{
    network::CEventLocalBlockvoteUpdateBlock* pEvent = new network::CEventLocalBlockvoteUpdateBlock(nNonce, hashFork);
    if (pEvent)
    {
        pEvent->data.push_back(network::CUpdateBlockVote(hashBlock, hashRefBlock, nBlockTime, CBlock::GetBlockHeightByHash(hashBlock)));
        PostEvent(pEvent);
    }
}

//-------------------------------------------------------------------------------------------
const string CBlockVoteChannel::GetPeerAddressInfo(uint64 nNonce)
{
    auto it = mapChnPeer.find(nNonce);
    if (it != mapChnPeer.end())
    {
        return it->second.GetRemoteAddress();
    }
    return string("0.0.0.0");
}

void CBlockVoteChannel::BlockVoteTimerFunc(uint32 nTimerId)
{
    if (nTimerId == nBlockVoteTimerId)
    {
        nBlockVoteTimerId = SetTimer(BLOCK_VOTE_TIMER_TIME, boost::bind(&CBlockVoteChannel::BlockVoteTimerFunc, this, _1));
        network::CEventLocalBlockvoteTimer* pEvent = new network::CEventLocalBlockvoteTimer(0, pCoreProtocol->GetGenesisBlockHash());
        if (pEvent)
        {
            pEvent->data = nTimerId;
            PostEvent(pEvent);
        }
    }
}

} // namespace metabasenet

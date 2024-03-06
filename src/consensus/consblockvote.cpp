// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consblockvote.h"

#include "msgblockvote.pb.h"

namespace consensus
{

namespace consblockvote
{

using namespace std;
using namespace metabasenet::crypto;

#define CBV_SHOW_DEBUG

/////////////////////////////////
#define PSD_SET_MSG(MSGID, PBMSG, OUTMSG)                              \
    OUTMSG.resize(PBMSG.ByteSizeLong() + 1);                           \
    if (!PBMSG.SerializeToArray(OUTMSG.data() + 1, OUTMSG.size() - 1)) \
    {                                                                  \
        StdError(__PRETTY_FUNCTION__, "SerializeToArray fail");        \
        return false;                                                  \
    }                                                                  \
    OUTMSG[0] = MSGID;

/////////////////////////////////
// CConsKey

bool CConsKey::Sign(const uint256& hash, bytes& btSig)
{
    return CryptoBlsSign(prikey, hash.GetBytes(), btSig);
}

bool CConsKey::Verify(const uint256& hash, const bytes& btSig)
{
    return CryptoBlsVerify(pubkey, hash.GetBytes(), btSig);
}

/////////////////////////////////
// CConsBlock

bool CConsBlock::SetCandidateNodeList(const vector<uint384>& vCandidateNodePubkeyIn)
{
    if (vCandidateNodePubkeyIn.empty())
    {
        StdLog("CConsBlock", "Set candidate node list: Candidate node is empty");
        return false;
    }

    mapCandidateNodeIndex.clear();
    vPreVoteCandidateNodePubkey.clear();
    vCommitVoteCandidateNodePubkey.clear();
    bmBlockPreVoteBitmap.Initialize(vCandidateNodePubkeyIn.size());
    bmBlockCommitVoteBitmap.Initialize(vCandidateNodePubkeyIn.size());

    for (uint32 i = 0; i < (uint32)(vCandidateNodePubkeyIn.size()); i++)
    {
        auto& pubkey = vCandidateNodePubkeyIn[i];
        auto it = mapCandidateNodeIndex.find(pubkey);
        if (it != mapCandidateNodeIndex.end())
        {
            StdLog("CConsBlock", "Set candidate node list: Candidate node index existed");
            mapCandidateNodeIndex.clear();
            vPreVoteCandidateNodePubkey.clear();
            vCommitVoteCandidateNodePubkey.clear();
            bmBlockPreVoteBitmap.Clear();
            bmBlockCommitVoteBitmap.Clear();
            return false;
        }
        mapCandidateNodeIndex.insert(make_pair(pubkey, i));
        vPreVoteCandidateNodePubkey.push_back(CNodePubkey(i, pubkey));
        vCommitVoteCandidateNodePubkey.push_back(CNodePubkey(i, pubkey));
    }
    return true;
}

bool CConsBlock::IsExistCandidateNodePubkey(const uint384& pubkeyNode) const
{
    return (mapCandidateNodeIndex.find(pubkeyNode) != mapCandidateNodeIndex.end());
}

bool CConsBlock::ExistPreVoteSign(const uint384& pubkeyNode)
{
    return (mapPreVoteSig.find(pubkeyNode) != mapPreVoteSig.end());
}

bool CConsBlock::ExistCommitVoteSign(const uint384& pubkeyNode)
{
    return (mapCommitVoteSig.find(pubkeyNode) != mapCommitVoteSig.end());
}

bool CConsBlock::AddPreVoteSign(const uint384& pubkeyNode, const bytes& btSig)
{
    auto it = mapPreVoteSig.find(pubkeyNode);
    if (it == mapPreVoteSig.end())
    {
        auto mt = mapCandidateNodeIndex.find(pubkeyNode);
        if (mt != mapCandidateNodeIndex.end() && mt->second < vPreVoteCandidateNodePubkey.size())
        {
            mapPreVoteSig.insert(make_pair(pubkeyNode, btSig));
            bmBlockPreVoteBitmap.SetBit(mt->second);

            vPreVoteCandidateNodePubkey[mt->second].SetStatus(CNodePubkey::ES_COMPLETED);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlock", "Add pre vote sig: Add pre vote sig success, pubkey: %s, block: %s",
                     pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
#endif
        }
        else
        {
            if (mt == mapCandidateNodeIndex.end())
            {
                StdLog("CConsBlock", "Add pre vote sig: Pubkey not exist, pubkey: %s", pubkeyNode.GetHex().c_str());
            }
            else
            {
                StdLog("CConsBlock", "Add pre vote sig: Pubkey index error, index: %d, candidate pubkey count: %lu, pubkey: %s",
                       mt->second, vPreVoteCandidateNodePubkey.size(), pubkeyNode.GetHex().c_str());
            }
        }
    }
    return true;
}

bool CConsBlock::AddCommitVoteSign(const uint384& pubkeyNode, const bytes& btSig)
{
    auto it = mapCommitVoteSig.find(pubkeyNode);
    if (it == mapCommitVoteSig.end())
    {
        auto mt = mapCandidateNodeIndex.find(pubkeyNode);
        if (mt != mapCandidateNodeIndex.end() && mt->second < vCommitVoteCandidateNodePubkey.size())
        {
            mapCommitVoteSig.insert(make_pair(pubkeyNode, btSig));
            bmBlockCommitVoteBitmap.SetBit(mt->second);

            vCommitVoteCandidateNodePubkey[mt->second].SetStatus(CNodePubkey::ES_COMPLETED);

#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlock", "Add commit vote sig: Add commit vote sig success, pubkey: %s, block: %s",
                     pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
#endif
        }
        else
        {
            if (mt == mapCandidateNodeIndex.end())
            {
                StdLog("CConsBlock", "Add commit vote sig: Pubkey not exist, pubkey: %s", pubkeyNode.GetHex().c_str());
            }
            else
            {
                StdLog("CConsBlock", "Add commit vote sig: Pubkey index error, index: %d, candidate pubkey count: %lu, pubkey: %s",
                       mt->second, vPreVoteCandidateNodePubkey.size(), pubkeyNode.GetHex().c_str());
            }
        }
    }
    return true;
}

bool CConsBlock::GetPreVoteBitmap(bytes& btBitmap)
{
    if (bmBlockPreVoteBitmap.HasValidBit())
    {
        bmBlockPreVoteBitmap.GetBytes(btBitmap);
        return true;
    }
    return false;
}

bool CConsBlock::GetCommitVoteBitmap(bytes& btBitmap)
{
    if (bmBlockCommitVoteBitmap.HasValidBit())
    {
        bmBlockCommitVoteBitmap.GetBytes(btBitmap);
        return true;
    }
    return false;
}

void CConsBlock::GetPreVoteSigByBitmap(const bytes& btGetBitmap, map<uint384, bytes>& mapSigOut)
{
    CBitmap btm;
    if (!btm.ImportBytes(btGetBitmap))
    {
        StdLog("CConsBlock", "Get pre vote sig by bitmap: Bitmap error");
        return;
    }

    vector<uint32> vIndexList;
    btm.GetIndexList(vIndexList);
    if (vIndexList.empty())
    {
        StdLog("CConsBlock", "Get pre vote sig by bitmap: Index list empty, max bit: %d, valid bit: %d", btm.GetMaxBits(), btm.HasValidBit());
        return;
    }

    for (auto& index : vIndexList)
    {
        if (index < (uint32)(vPreVoteCandidateNodePubkey.size()))
        {
            const uint384& pubkeyNode = vPreVoteCandidateNodePubkey[index].pubkey;
            auto it = mapPreVoteSig.find(pubkeyNode);
            if (it != mapPreVoteSig.end())
            {
                mapSigOut.insert(make_pair(pubkeyNode, it->second));
            }
            else
            {
                StdLog("CConsBlock", "Get pre vote sig by bitmap: Find pre vote fail, index: %d, pre vote size: %lu, pubkey: %s, block: %s",
                       index, mapPreVoteSig.size(), pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
            }
        }
    }
}

void CConsBlock::GetCommitVoteSigByBitmap(const CBitmap& bmGetBitmap, map<uint384, bytes>& mapSigOut)
{
    vector<uint32> vIndexList;
    bmGetBitmap.GetIndexList(vIndexList);
    if (vIndexList.empty())
    {
        StdLog("CConsBlock", "Get commit vote sig by bitmap: Index list empty, has bit: %d", bmGetBitmap.GetValidBits());
        return;
    }

    for (auto& index : vIndexList)
    {
        if (index < (uint32)(vCommitVoteCandidateNodePubkey.size()))
        {
            const uint384& pubkeyNode = vCommitVoteCandidateNodePubkey[index].pubkey;
            auto it = mapCommitVoteSig.find(pubkeyNode);
            if (it != mapCommitVoteSig.end())
            {
                mapSigOut.insert(make_pair(pubkeyNode, it->second));
            }
            else
            {
                StdLog("CConsBlock", "Get commit vote sig by bitmap: Find commit vote fail, index: %d, commit vote size: %lu, pubkey: %s, block: %s",
                       index, mapPreVoteSig.size(), pubkeyNode.GetHex().c_str(), hashBlock.GetBhString().c_str());
            }
        }
    }
}

bool CConsBlock::GetPreVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut)
{
    CBitmap btmPeer;
    if (!btmPeer.ImportBytes(btBitmapPeer))
    {
        return false;
    }
    if (btmPeer.GetMaxBits() != bmBlockPreVoteBitmap.GetMaxBits())
    {
        return false;
    }
    if (btmPeer == bmBlockPreVoteBitmap)
    {
        return false;
    }

    //(local | peer) ^ local
    btmPeer |= bmBlockPreVoteBitmap;
    btmPeer ^= bmBlockPreVoteBitmap;

    vector<uint32> vIndexList;
    btmPeer.GetIndexList(vIndexList);

    CBitmap bmOut;
    bmOut.Initialize(bmBlockPreVoteBitmap.GetMaxBits());

    uint32 nAddCount = 0;
    for (auto& index : vIndexList)
    {
        if (index < vPreVoteCandidateNodePubkey.size())
        {
            auto& node = vPreVoteCandidateNodePubkey[index];
            node.CheckStatus();
            if (node.GetStatus() == CNodePubkey::ES_INIT)
            {
                bmOut.SetBit(index);
                node.SetStatus(CNodePubkey::ES_WAITING);
                if (++nAddCount >= MAX_AWAIT_BIT_COUNT)
                {
                    break;
                }
            }
        }
    }
    if (nAddCount == 0)
    {
        return false;
    }
    bmOut.GetBytes(btBitmapOut);
    return true;
}

bool CConsBlock::GetCommitVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut)
{
    CBitmap btmPeer;
    if (!btmPeer.ImportBytes(btBitmapPeer))
    {
        return false;
    }
    if (btmPeer.GetMaxBits() != bmBlockCommitVoteBitmap.GetMaxBits())
    {
        return false;
    }
    if (btmPeer == bmBlockCommitVoteBitmap)
    {
        return false;
    }

    //(local | peer) ^ local
    btmPeer |= bmBlockCommitVoteBitmap;
    btmPeer ^= bmBlockCommitVoteBitmap;

    vector<uint32> vIndexList;
    btmPeer.GetIndexList(vIndexList);

    CBitmap bmOut;
    bmOut.Initialize(bmBlockCommitVoteBitmap.GetMaxBits());

    uint32 nAddCount = 0;
    for (auto& index : vIndexList)
    {
        if (index < vCommitVoteCandidateNodePubkey.size())
        {
            auto& node = vCommitVoteCandidateNodePubkey[index];
            node.CheckStatus();
            if (node.GetStatus() == CNodePubkey::ES_INIT)
            {
                bmOut.SetBit(index);
                node.SetStatus(CNodePubkey::ES_WAITING);
                if (++nAddCount >= MAX_AWAIT_BIT_COUNT)
                {
                    break;
                }
            }
        }
    }
    if (nAddCount == 0)
    {
        return false;
    }
    bmOut.GetBytes(btBitmapOut);
    return true;
}

bool CConsBlock::GetPubkeysByBitmap(const CBitmap& bmBitmap, vector<uint384>& vPubkeys)
{
    vector<uint32> vIndexList;
    bmBitmap.GetIndexList(vIndexList);
    for (auto& index : vIndexList)
    {
        if (index >= vPreVoteCandidateNodePubkey.size())
        {
            StdLog("CConsBlock", "Get pre vote pubkeys by bitmap: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.push_back(vPreVoteCandidateNodePubkey[index].pubkey);
    }
    if (vPubkeys.size() < (vPreVoteCandidateNodePubkey.size() * 2 / 3))
    {
        StdLog("CConsBlock", "Get pre vote pubkeys by bitmap: Sign not enough, sign count: %lu, candidate count: %lu, index count: %lu, block: %s",
               vPubkeys.size(), vPreVoteCandidateNodePubkey.size(), vIndexList.size(), hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CConsBlock::GetLocalPreVoteSign(const int64 nEpochDurationIn, CBitmap& bmPreVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs)
{
    // int64 nWaitTime = nEpochDurationIn / 10;
    // if (nWaitTime < 500)
    // {
    //     nWaitTime = 500;
    // }
    // else if (nWaitTime > 3000)
    // {
    //     nWaitTime = 3000;
    // }
    // if (GetTimeMillis() - nVoteBeginTime < nWaitTime)
    // {
    //     return false;
    // }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlock", "Get local pre vote sig: pre vote sig count: %lu, candidate node count: %lu, block: %s",
             mapPreVoteSig.size(), mapCandidateNodeIndex.size(), hashBlock.GetBhString().c_str());
#endif

    if (mapPreVoteSig.size() >= (mapCandidateNodeIndex.size() * 2 / 3))
    {
        vector<uint32> vIndexList;
        bmBlockPreVoteBitmap.GetIndexList(vIndexList);
        if (vIndexList.empty() || vIndexList.size() != mapPreVoteSig.size())
        {
            StdLog("CConsBlock", "Get local pre vote sig: Pre vote bitmap bits error, bits: %lu, sigs: %lu, block: %s",
                   vIndexList.size(), mapPreVoteSig.size(), hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.reserve(vIndexList.size());
        vSigs.reserve(vIndexList.size());
        for (auto& index : vIndexList)
        {
            if (index >= vPreVoteCandidateNodePubkey.size())
            {
                StdLog("CConsBlock", "Get local pre vote sig: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
                return false;
            }
            auto& pubkey = vPreVoteCandidateNodePubkey[index].pubkey;
            vPubkeys.push_back(pubkey);

            auto it = mapPreVoteSig.find(pubkey);
            if (it == mapPreVoteSig.end())
            {
                StdLog("CConsBlock", "Get local pre vote sig: Find sig fail, index: %d, pubkey: %s, block: %s",
                       index, pubkey.GetHex().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
            vSigs.push_back(it->second);
        }
        bmPreVoteBitmap = bmBlockPreVoteBitmap;
        return true;
    }
    return false;
}

bool CConsBlock::GetLocalCommitVoteSign(const int64 nEpochDurationIn, CBitmap& bmCommitVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs)
{
    // int64 nWaitTime = nEpochDurationIn / 10;
    // if (nWaitTime < 500)
    // {
    //     nWaitTime = 500;
    // }
    // else if (nWaitTime > 3000)
    // {
    //     nWaitTime = 3000;
    // }
    // if (GetTimeMillis() - nVoteBeginTime < nWaitTime)
    // {
    //     return false;
    // }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlock", "Get local commit vote sig: commit vote sig count: %lu, candidate node count: %lu, block: %s",
             mapCommitVoteSig.size(), mapCandidateNodeIndex.size(), hashBlock.GetBhString().c_str());
#endif

    if (mapCommitVoteSig.size() >= (mapCandidateNodeIndex.size() * 2 / 3))
    {
        vector<uint32> vIndexList;
        bmBlockCommitVoteBitmap.GetIndexList(vIndexList);
        if (vIndexList.empty() || vIndexList.size() != mapCommitVoteSig.size())
        {
            StdLog("CConsBlock", "Get local commit vote sig: Commit vote bitmap bits error, bits: %lu, sigs: %lu, block: %s",
                   vIndexList.size(), mapCommitVoteSig.size(), hashBlock.GetBhString().c_str());
            return false;
        }
        vPubkeys.reserve(vIndexList.size());
        vSigs.reserve(vIndexList.size());
        for (auto& index : vIndexList)
        {
            if (index >= vCommitVoteCandidateNodePubkey.size())
            {
                StdLog("CConsBlock", "Get local commit vote sig: Index error, index: %d, block: %s", index, hashBlock.GetBhString().c_str());
                return false;
            }
            auto& pubkey = vCommitVoteCandidateNodePubkey[index].pubkey;
            vPubkeys.push_back(pubkey);

            auto it = mapCommitVoteSig.find(pubkey);
            if (it == mapCommitVoteSig.end())
            {
                StdLog("CConsBlock", "Get local commit vote sig: Find sig fail, index: %d, pubkey: %s, block: %s",
                       index, pubkey.GetHex().c_str(), hashBlock.GetBhString().c_str());
                return false;
            }
            vSigs.push_back(it->second);
        }
        bmCommitVoteBitmap = bmBlockCommitVoteBitmap;
        return true;
    }
    return false;
}

void CConsBlock::SetAggCommitVoteSign(const bytes& btAggCommitVoteBitmapIn, const bytes& btAggCommitVoteSigIn)
{
    btAggCommitVoteBitmap = btAggCommitVoteBitmapIn;
    btAggCommitVoteSig = btAggCommitVoteSigIn;
}

bool CConsBlock::GetAggCommitVoteSign(bytes& btAggCommitVoteBitmapOut, bytes& btAggCommitVoteSigOut)
{
    if (btAggCommitVoteBitmap.empty())
    {
        return false;
    }
    btAggCommitVoteBitmapOut = btAggCommitVoteBitmap;
    btAggCommitVoteSigOut = btAggCommitVoteSig;
    return true;
}

bool CConsBlock::IsHasAggCommitVoteSign()
{
    return !(btAggCommitVoteBitmap.empty());
}

/////////////////////////////////
// CConsBlockVote

bool CConsBlockVote::AddConsKey(const uint256& prikey, const uint384& pubkey)
{
    if (mapConsKey.find(prikey) == mapConsKey.end())
    {
        mapConsKey.insert(make_pair(prikey, CConsKey(prikey, pubkey)));
    }
    return true;
}

bool CConsBlockVote::AddCandidatePubkey(const uint256& hashBlock, const uint32 nBlockEpoch, const int64 nVoteBeginTimeIn, const vector<uint384>& vPubkey)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it == mapConsBlock.end())
    {
        while (mapConsBlock.size() >= MAX_CONS_HEIGHT_COUNT && !mapAddTime.empty())
        {
            for (auto& hash : mapAddTime.begin()->second)
            {
                mapConsBlock.erase(hash);
            }
            mapAddTime.erase(mapAddTime.begin());
        }
        uint64 nCurTime = ((GetTime() << 32) | (GetTimeMillis() & 0xFFFFFFFF));
        it = mapConsBlock.insert(make_pair(hashBlock, CConsBlock(hashBlock, nBlockEpoch, nVoteBeginTimeIn, nCurTime))).first;

        CConsBlock& consHeight = it->second;
        if (!consHeight.SetCandidateNodeList(vPubkey))
        {
            StdLog("CConsBlockVote", "Add candidate pubkey: Set candidate node list fail, block: %s", hashBlock.GetBhString().c_str());
            mapConsBlock.erase(it);
            return false;
        }

        if (!AddLocalPreVoteSign(hashBlock, consHeight))
        {
            StdLog("CConsBlockVote", "Add candidate pubkey: Add local pre vote sign fail, block: %s", hashBlock.GetBhString().c_str());
            mapConsBlock.erase(it);
            return false;
        }

        mapAddTime[nCurTime].insert(hashBlock);

        BroadcastPreVoteBitmapPush(hashBlock, consHeight);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Add candidate pubkey: Add candidate pubkey success, cons block count: %lu, block: %s", mapConsBlock.size(), hashBlock.GetBhString().c_str());
#endif
    }
    return true;
}

bool CConsBlockVote::GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it == mapConsBlock.end())
    {
        uint32 nBlockEpoch = 0;
        int64 nBlockTime = 0;
        vector<uint384> vPubkey;
        if (getVoteBlockCandidatePubkey(hashBlock, nBlockEpoch, nBlockTime, vPubkey, btBitmap, btAggSig) && !btBitmap.empty() && !btAggSig.empty())
        {
            return true;
        }
        return false;
    }
    return it->second.GetAggCommitVoteSign(btBitmap, btAggSig);
}

bool CConsBlockVote::AddNetNode(const uint64 nNetId)
{
    auto it = mapNetNode.find(nNetId);
    if (it == mapNetNode.end())
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Add net node, net id: 0x%lx", nNetId);
#endif
        mapNetNode.insert(make_pair(nNetId, CNetNode()));
        SendSubscribeReq(nNetId);
    }
    return true;
}

void CConsBlockVote::RemoveNetNode(const uint64 nNetId)
{
    mapNetNode.erase(nNetId);
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "Remove net node, net id: 0x%lx", nNetId);
#endif
}

void CConsBlockVote::OnTimer()
{
    CheckPreVoteBitmapReq();

    int64 nWaitTime = nEpochDuration / 5;
    if (nWaitTime < 1000)
    {
        nWaitTime = 1000;
    }
    else if (nWaitTime > 5000)
    {
        nWaitTime = 5000;
    }
    int64 nCurTime = GetTimeMillis();
    if (nCurTime - nPrevCheckPreVoteBitmapTime >= nWaitTime)
    {
        nPrevCheckPreVoteBitmapTime = nCurTime;

        BroadcastPreVoteBitmap();
        BroadcastCommitVoteBitmap();
        BroadcastAggCommitVoteSign();

        CheckLocalVote();
    }
}

void CConsBlockVote::OnEventNetData(const uint64 nNetId, const bytes& btData)
{
    const uint8* pData = btData.data();
    size_t nDataLen = btData.size();

    if (pData == nullptr || nDataLen <= 1)
    {
        StdLog("CConsBlockVote", "On net data req: Net data error, net id: 0x%lx", nNetId);
        return;
    }
    uint8 nFuncId = *pData;
    pData++;
    nDataLen--;

    switch (nFuncId)
    {
    case PS_MSGID_BLOCKVOTE_SUBSCRIBE_REQ:
        OnNetMsgSubscribeReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_SUBSCRIBE_RSP:
        OnNetMsgSubscribeRsp(nNetId, pData, nDataLen);
        break;

    case PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_REQ:
        OnNetMsgPreVoteBitmapReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_PUSH:
        OnNetMsgPreVoteBitmapPush(nNetId, pData, nDataLen);
        break;

    case PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_REQ:
        OnNetMsgPreVoteSignReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_PUSH:
        OnNetMsgPreVoteSignPush(nNetId, pData, nDataLen);
        break;

    case PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_REQ:
        OnNetMsgCommitVoteBitmapReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_PUSH:
        OnNetMsgCommitVoteBitmapPush(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_COMMIT_VOTE_REQ:
        OnNetMsgCommitVoteSignReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_COMMIT_VOTE_PUSH:
        OnNetMsgCommitVoteSignPush(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_REQ:
        OnNetMsgAggCommitVoteReq(nNetId, pData, nDataLen);
        break;
    case PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_PUSH:
        OnNetMsgAggCommitVotePush(nNetId, pData, nDataLen);
        break;
    default:
        StdLog("CConsBlockVote", "On net data req: msg id error, id: %d, net id: 0x%lx", nFuncId, nNetId);
        break;
    }
}

bool CConsBlockVote::VerifyCommitVoteAggSig(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig, const vector<uint384>& vCandidatePubkeys)
{
    vector<uint384> vBitmapPubkeys;
    if (!GetBitPubkeysByBitmap(vCandidatePubkeys, btBitmap, vBitmapPubkeys))
    {
        return false;
    }
    return CryptoBlsFastAggregateVerify(vBitmapPubkeys, GetCommitVoteSignData(hashBlock).GetBytes(), btAggSig);
}

//------------------------------
void CConsBlockVote::OnNetMsgSubscribeReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::SubscribeReq msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On subscribe req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }
    uint32 version = msg.version();
    mapNetNode[nNetId].SetPeerVersion(version);
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On subscribe req, version: %d, net id: 0x%lx", version, nNetId);
#endif

    SendSubscribeRsp(nNetId, true);
}

void CConsBlockVote::OnNetMsgSubscribeRsp(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::SubscribeRsp msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On subscribe rsp: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }
    uint32 version = msg.version();
    uint32 result = msg.result();
    if (result != 0)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On subscribe rsp: Subscribe success, version: %d, net id: 0x%lx", version, nNetId);
#endif
        NodeSubscribe(nNetId, version);
    }
    else
    {
        StdLog("CConsBlockVote", "On subscribe rsp: Subscribe fail, result: %d, net id: 0x%lx", result, nNetId);
    }
}

void CConsBlockVote::OnNetMsgPreVoteBitmapReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::PreVoteBitmapReq msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On pre vote bitmap req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (hashBlock == 0)
    {
        StdLog("CConsBlockVote", "On pre vote bitmap req: Block is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On pre vote bitmap req: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On pre vote bitmap req, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    bytes btBitmap;
    if (pConsBlock->GetPreVoteBitmap(btBitmap))
    {
        bytes btMsg;
        if (MakePreVoteBitmapPush(btMsg, hashBlock, btBitmap))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On pre vote bitmap req: Send bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }
    }
}

void CConsBlockVote::OnNetMsgPreVoteBitmapPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::PreVoteBitmapPush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On pre vote bitmap push: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    auto mt = mapNetNode.find(nNetId);
    if (mt != mapNetNode.end())
    {
        mt->second.SetPrevBitmapReqTime();
    }

    uint256 hashBlock;
    bytes btBitmapPeer;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.bitmap().empty())
    {
        const string& nt = msg.bitmap();
        if (nt.size() > 0)
        {
            btBitmapPeer.assign(nt.data(), nt.data() + nt.size());
        }
    }

    if (hashBlock == 0 || btBitmapPeer.empty())
    {
        StdLog("CConsBlockVote", "On pre vote bitmap push: Block or bitmap is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On pre vote bitmap push: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On pre vote bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    bytes btGetBitmap;
    if (pConsBlock->GetPreVoteAwaitBitmap(btBitmapPeer, btGetBitmap) && !btGetBitmap.empty())
    {
        bytes btMsg;
        if (MakePreVoteSignReq(btMsg, hashBlock, btGetBitmap))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On pre vote bitmap push: Send pre vote sig req, net id: 0x%lx", nNetId);
#endif
        }
    }
}

void CConsBlockVote::OnNetMsgPreVoteSignReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::PreVoteSignReq msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On pre vote sig req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    bytes btGetBitmap;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.getbitmap().empty())
    {
        const string& nt = msg.getbitmap();
        if (nt.size() > 0)
        {
            btGetBitmap.assign(nt.data(), nt.data() + nt.size());
        }
    }

    if (hashBlock == 0 || btGetBitmap.empty())
    {
        StdLog("CConsBlockVote", "On pre vote sig req: Block or bitmap is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On pre vote sig req: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }

    map<uint384, bytes> mapSigOut;
    pConsBlock->GetPreVoteSigByBitmap(btGetBitmap, mapSigOut);

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On pre vote sig req: Get pre vote sig count: %lu, net id: 0x%lx, block: %s",
             mapSigOut.size(), nNetId, hashBlock.GetBhString().c_str());
#endif

    if (!mapSigOut.empty())
    {
        bytes btMsg;
        if (MakePreVoteSignPush(btMsg, hashBlock, mapSigOut))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On pre vote sig req: Send pre vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }

        bytes btPreVoteBitmap;
        btMsg.clear();
        if (pConsBlock->GetPreVoteBitmap(btPreVoteBitmap))
        {
            if (MakePreVoteBitmapPush(btMsg, hashBlock, btPreVoteBitmap))
            {
                SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
                StdDebug("CConsBlockVote", "On pre vote sig req: Send bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
            }
        }
    }
    else
    {
        StdLog("CConsBlockVote", "On pre vote sig req: Get pre vote sig fail, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
    }
}

void CConsBlockVote::OnNetMsgPreVoteSignPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::PreVoteSignPush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On pre vote sig push: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (hashBlock == 0)
    {
        StdLog("CConsBlockVote", "On pre vote sig push: block is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On pre vote sig push: Find block fail, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On pre vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    map<uint384, bytes> mapBroadcastSig;
    for (int i = 0; i < msg.datalist_size(); i++)
    {
        uint384 pubkeyNode;
        bytes btSig;

        auto item = msg.datalist(i);
        if (!item.pubkeynode().empty())
        {
            pubkeyNode.SetBytes(item.pubkeynode());
        }
        if (!item.sig().empty())
        {
            const string& str = item.sig();
            if (str.size() > 0)
            {
                btSig.assign(str.data(), str.data() + str.size());
            }
        }

        if (pubkeyNode != uint384((uint64)0) && !btSig.empty() && !pConsBlock->ExistPreVoteSign(pubkeyNode))
        {
            if (!VerifyPreVoteSign(hashBlock, pubkeyNode, btSig))
            {
                StdLog("CConsBlockVote", "On pre vote sig push: Verify pre vote sign fail, pubkey: %s, net id: 0x%lx, block: %s",
                       pubkeyNode.GetHex().c_str(), nNetId, hashBlock.GetBhString().c_str());
                continue;
            }
            if (!pConsBlock->AddPreVoteSign(pubkeyNode, btSig))
            {
                StdLog("CConsBlockVote", "On pre vote sig push: Add pre vote sig fail, pubkey: %s, net id: 0x%lx, block: %s",
                       pubkeyNode.GetHex().c_str(), nNetId, hashBlock.GetBhString().c_str());
                continue;
            }
            mapBroadcastSig.insert(make_pair(pubkeyNode, btSig));
        }
    }

    if (!mapBroadcastSig.empty())
    {
        bytes btMsg;
        if (MakePreVoteSignPush(btMsg, hashBlock, mapBroadcastSig))
        {
            BroadcastData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On pre vote sig push: Broadcast pre vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }
    }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On pre vote sig push: pre vote bitmap: %s, net id: 0x%lx, block: %s",
             pConsBlock->bmBlockPreVoteBitmap.GetBitmapString().c_str(), nNetId, hashBlock.GetBhString().c_str());
#endif

    CheckPreVote(hashBlock);
}

void CConsBlockVote::OnNetMsgCommitVoteBitmapReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::CommitVoteBitmapReq msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On commit vote bitmap req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (hashBlock == 0)
    {
        StdLog("CConsBlockVote", "On commit vote bitmap req: block is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
        StdLog("CConsBlockVote", "On commit vote bitmap req: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On commit vote bitmap req, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    bytes btBitmap;
    if (!pConsBlock->GetCommitVoteBitmap(btBitmap))
    {
        StdLog("CConsBlockVote", "On commit vote bitmap req: Pre vote bitmap is empty, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }

    if (!btBitmap.empty())
    {
        bytes btMsg;
        if (MakeCommitVoteBitmapPush(btMsg, hashBlock, btBitmap))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On commit vote bitmap req: Send pre vote bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }
    }
}

void CConsBlockVote::OnNetMsgCommitVoteBitmapPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::CommitVoteBitmapPush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On commit vote bitmap push: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    bytes btBitmapPeer;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.bitmap().empty())
    {
        const string& nt = msg.bitmap();
        if (nt.size() > 0)
        {
            btBitmapPeer.assign(nt.data(), nt.data() + nt.size());
        }
    }
    if (hashBlock == 0)
    {
        StdLog("CConsBlockVote", "On commit vote bitmap push: block is null, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
        StdDebug("CConsBlockVote", "On commit vote bitmap push: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On commit vote bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    bytes btGetBitmap;
    if (pConsBlock->GetCommitVoteAwaitBitmap(btBitmapPeer, btGetBitmap) && !btGetBitmap.empty())
    {
        bytes btMsg;
        if (MakeCommitVoteSignReq(btMsg, hashBlock, btGetBitmap))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On commit vote bitmap push: Send commit vote sig req, net id: 0x%lx", nNetId);
#endif
        }
    }
}

void CConsBlockVote::OnNetMsgCommitVoteSignReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::CommitVoteSignReq msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On commit vote sig req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    CBitmap bmGetBitmap;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.getbitmap().empty())
    {
        if (!bmGetBitmap.ImportString(msg.getbitmap()))
        {
            StdLog("CConsBlockVote", "On commit vote sig req: Get bitmap is error, net id: 0x%lx", nNetId);
            return;
        }
    }

    if (hashBlock == 0 || bmGetBitmap.IsNull())
    {
        StdLog("CConsBlockVote", "On commit vote sig req: Block or bitmap is error, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On commit vote sig req: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On commit vote sig req, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    if (pConsBlock->bmAggPreVoteBitmap.IsNull() || pConsBlock->btAggPreVoteSig.empty())
    {
        StdLog("CConsBlockVote", "On commit vote sig req: Agg sig or pre vote sig is empty, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
    map<uint384, bytes> mapCommitVoteSig;
    pConsBlock->GetCommitVoteSigByBitmap(bmGetBitmap, mapCommitVoteSig);
    if (mapCommitVoteSig.empty())
    {
        StdLog("CConsBlockVote", "On commit vote sig req: Agg sig or pre vote sig is empty, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
    bytes btPreVoteBitmap;
    pConsBlock->bmAggPreVoteBitmap.GetBytes(btPreVoteBitmap);

    bytes btMsg;
    if (MakeCommitVoteSignPush(btMsg, hashBlock, btPreVoteBitmap, pConsBlock->btAggPreVoteSig, mapCommitVoteSig))
    {
        SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On commit vote sig req: Send commit vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
    }

    bytes btBitmap;
    if (pConsBlock->GetCommitVoteBitmap(btBitmap) && !btBitmap.empty())
    {
        btMsg.clear();
        if (MakeCommitVoteBitmapPush(btMsg, hashBlock, btBitmap))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On commit vote sig req: Send commit vote bitmap push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }
    }
}

void CConsBlockVote::OnNetMsgCommitVoteSignPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::CommitVoteSignPush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On commit vote sig push: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    CBitmap bmPreVoteBitmap;
    bytes btPreVoteAggSig;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.prevotebitmap().empty())
    {
        if (!bmPreVoteBitmap.ImportString(msg.prevotebitmap()))
        {
            StdLog("CConsBlockVote", "On commit vote sig push: Hash bitmap error, net id: 0x%lx", nNetId);
            return;
        }
    }
    if (!msg.prevoteaggsig().empty())
    {
        const string& nt = msg.prevoteaggsig();
        if (nt.size() > 0)
        {
            btPreVoteAggSig.assign(nt.data(), nt.data() + nt.size());
        }
    }
    if (hashBlock == 0 || bmPreVoteBitmap.IsNull() || btPreVoteAggSig.empty())
    {
        StdLog("CConsBlockVote", "On commit vote sig push: Block or bitmap is error, net id: 0x%lx", nNetId);
        return;
    }

    CConsBlock* pConsBlock = LoadVoteBlock(hashBlock, nNetId);
    if (pConsBlock == nullptr)
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On commit vote sig push: Block not exist, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }

    if (!VerifyPreVoteAggSign(hashBlock, bmPreVoteBitmap, btPreVoteAggSig))
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On commit vote sig push: Verify pre vote agg sig fail, net id: 0x%lx", nNetId);
#endif
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On commit vote sig push: Verify pre vote agg sig success, net id: 0x%lx, block: %s",
             nNetId, hashBlock.GetBhString().c_str());
#endif

    map<uint384, bytes> mapBroadcastSig;
    for (int i = 0; i < msg.datalist_size(); i++)
    {
        uint384 pubkeyNode;
        bytes btSig;

        auto item = msg.datalist(i);
        if (!item.pubkeynode().empty())
        {
            pubkeyNode.SetBytes(item.pubkeynode());
        }
        if (!item.sig().empty())
        {
            const string& str = item.sig();
            if (str.size() > 0)
            {
                btSig.assign(str.data(), str.data() + str.size());
            }
        }

        if (pubkeyNode != uint384((uint64)0) && !btSig.empty() && !pConsBlock->ExistCommitVoteSign(pubkeyNode))
        {
            if (VerifyCommitVoteSign(hashBlock, pubkeyNode, btSig))
            {
                pConsBlock->AddCommitVoteSign(pubkeyNode, btSig);
                mapBroadcastSig.insert(make_pair(pubkeyNode, btSig));
            }
            else
            {
                StdLog("CConsBlockVote", "On commit vote sig push: Verify commit vote sig fail, pubkey: %s, net id: 0x%lx, block: %s",
                       pubkeyNode.GetHex().c_str(), nNetId, hashBlock.GetBhString().c_str());
            }
        }
    }

    if (!mapBroadcastSig.empty())
    {
        bytes btMsg;
        if (MakeCommitVoteSignPush(btMsg, hashBlock, bmPreVoteBitmap.GetBytes(), btPreVoteAggSig, mapBroadcastSig))
        {
            BroadcastData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "On commit vote sig push: Broadcast commit vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        }
    }

    CheckCommitVote(hashBlock);
}

void CConsBlockVote::OnNetMsgAggCommitVoteReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::AggCommitVotePush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On agg commit vote req: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (hashBlock == 0)
    {
        StdLog("CConsBlockVote", "On agg commit vote req: Block is null, net id: 0x%lx", nNetId);
        return;
    }
#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On agg commit vote req, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    bytes btCommitVoteBitmap;
    bytes btCommitVoteAggSig;
    if (GetBlockVoteResult(hashBlock, btCommitVoteBitmap, btCommitVoteAggSig))
    {
        bytes btMsg;
        if (MakeAggCommitVotePush(btMsg, hashBlock, btCommitVoteBitmap, btCommitVoteAggSig))
        {
            SendNetData(nNetId, btMsg);
        }
    }
}

void CConsBlockVote::OnNetMsgAggCommitVotePush(const uint64 nNetId, const uint8* pData, const size_t nDataLen)
{
    msgblockvote::AggCommitVotePush msg;
    if (!msg.ParseFromArray(pData, nDataLen))
    {
        StdLog("CConsBlockVote", "On agg commit vote push: Parse message fail, net id: 0x%lx", nNetId);
        return;
    }

    uint256 hashBlock;
    CBitmap bmCommitVoteBitmap;
    bytes btCommitVoteAggSig;

    if (!msg.blockhash().empty())
    {
        hashBlock.SetBytes(msg.blockhash());
    }
    if (!msg.commitvotebitmap().empty())
    {
        if (!bmCommitVoteBitmap.ImportString(msg.commitvotebitmap()))
        {
            StdLog("CConsBlockVote", "On agg commit vote push: Commit vote bitmap error, net id: 0x%lx", nNetId);
            return;
        }
    }
    if (!msg.commitvoteaggsig().empty())
    {
        const string& nt = msg.commitvoteaggsig();
        if (nt.size() > 0)
        {
            btCommitVoteAggSig.assign(nt.data(), nt.data() + nt.size());
        }
    }

    if (hashBlock == 0 || bmCommitVoteBitmap.IsNull() || btCommitVoteAggSig.empty())
    {
        StdLog("CConsBlockVote", "On agg commit vote push: Block or bitmap or aggsig error, net id: 0x%lx", nNetId);
        return;
    }

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On agg commit vote push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif

    uint32 nBlockEpoch = 0;
    int64 nBlockTime = 0;
    vector<uint384> vPubkey;
    bytes btAggBitmap;
    bytes btAggSig;
    if (getVoteBlockCandidatePubkey(hashBlock, nBlockEpoch, nBlockTime, vPubkey, btAggBitmap, btAggSig) && !btAggBitmap.empty() && !btAggSig.empty())
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On agg commit vote push: Agg commit vote sig is existed, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
        return;
    }

    if (!VerifyCommitVoteAggSign(hashBlock, bmCommitVoteBitmap, btCommitVoteAggSig))
    {
        StdLog("CConsBlockVote", "On agg commit vote push: Verify commit vote agg sig fail, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
    if (bmCommitVoteBitmap.IsNull())
    {
        StdLog("CConsBlockVote", "On agg commit vote push: Agg bitmap is null, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
        return;
    }
    btAggBitmap.clear();
    bmCommitVoteBitmap.GetBytes(btAggBitmap);

    commitVoteResult(hashBlock, btAggBitmap, btCommitVoteAggSig);

#ifdef CBV_SHOW_DEBUG
    StdDebug("CConsBlockVote", "On agg commit vote push: **************************************************, commit vote bitmap: %s, commit agg sig: %s, block: %s",
             bmCommitVoteBitmap.GetBitmapString().c_str(), ToHexString(btCommitVoteAggSig).c_str(), hashBlock.GetBhString().c_str());
#endif

    bytes btMsg;
    if (MakeAggCommitVotePush(btMsg, hashBlock, btAggBitmap, btCommitVoteAggSig))
    {
        BroadcastData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "On agg commit vote push: Broadcast agg commit vote sig push, net id: 0x%lx, block: %s", nNetId, hashBlock.GetBhString().c_str());
#endif
    }

    RemoveVoteBlock(hashBlock);
}

//---------------------------
bool CConsBlockVote::MakeSubscribeReq(bytes& btMsg, const uint32 nVersion)
{
    msgblockvote::SubscribeReq pbmsg;

    pbmsg.set_version(nVersion);

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_SUBSCRIBE_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeSubscribeRsp(bytes& btMsg, const uint32 nVersion, const uint32 nResult)
{
    msgblockvote::SubscribeRsp pbmsg;

    pbmsg.set_version(nVersion);
    pbmsg.set_result(nResult);

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_SUBSCRIBE_RSP, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakePreVoteBitmapReq(bytes& btMsg, const uint256& hashBlock)
{
    msgblockvote::PreVoteBitmapReq pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakePreVoteBitmapPush(bytes& btMsg, const uint256& hashBlock, const bytes& btBitmap)
{
    msgblockvote::PreVoteBitmapPush pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_bitmap(btBitmap.data(), btBitmap.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_PUSH, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakePreVoteSignReq(bytes& btMsg, const uint256& hashBlock, const bytes& btGetBitmap)
{
    msgblockvote::PreVoteSignReq pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_getbitmap(btGetBitmap.data(), btGetBitmap.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakePreVoteSignPush(bytes& btMsg, const uint256& hashBlock, const map<uint384, bytes>& mapPreVoteSigIn)
{
    msgblockvote::PreVoteSignPush pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    for (auto& kv : mapPreVoteSigIn)
    {
        auto pItem = pbmsg.add_datalist();
        if (pItem)
        {
            pItem->set_pubkeynode(kv.first.begin(), kv.first.size());
            pItem->set_sig(kv.second.data(), kv.second.size());
        }
    }

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_PUSH, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeCommitVoteBitmapReq(bytes& btMsg, const uint256& hashBlock)
{
    msgblockvote::CommitVoteBitmapReq pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeCommitVoteBitmapPush(bytes& btMsg, const uint256& hashBlock, const bytes& btBitmap)
{
    msgblockvote::CommitVoteBitmapPush pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_bitmap(btBitmap.data(), btBitmap.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_PUSH, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeCommitVoteSignReq(bytes& btMsg, const uint256& hashBlock, const bytes& btGetBitmap)
{
    msgblockvote::CommitVoteSignReq pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_getbitmap(btGetBitmap.data(), btGetBitmap.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_COMMIT_VOTE_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeCommitVoteSignPush(bytes& btMsg, const uint256& hashBlock, const bytes& btPreVoteBitmap, const bytes& btPreVoteAggSig, const map<uint384, bytes>& mapCommitVoteSig)
{
    msgblockvote::CommitVoteSignPush pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_prevotebitmap(btPreVoteBitmap.data(), btPreVoteBitmap.size());
    pbmsg.set_prevoteaggsig(btPreVoteAggSig.data(), btPreVoteAggSig.size());
    for (auto& kv : mapCommitVoteSig)
    {
        auto pItem = pbmsg.add_datalist();
        if (pItem)
        {
            pItem->set_pubkeynode(kv.first.begin(), kv.first.size());
            pItem->set_sig(kv.second.data(), kv.second.size());
        }
    }

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_COMMIT_VOTE_PUSH, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeAggCommitVoteReq(bytes& btMsg, const uint256& hashBlock)
{
    msgblockvote::AggCommitVoteReq pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_REQ, pbmsg, btMsg)
    return true;
}

bool CConsBlockVote::MakeAggCommitVotePush(bytes& btMsg, const uint256& hashBlock, const bytes& btCommitVoteBitmap, const bytes& btCommitVoteAggSig)
{
    msgblockvote::AggCommitVotePush pbmsg;

    pbmsg.set_blockhash(hashBlock.begin(), hashBlock.size());
    pbmsg.set_commitvotebitmap(btCommitVoteBitmap.data(), btCommitVoteBitmap.size());
    pbmsg.set_commitvoteaggsig(btCommitVoteAggSig.data(), btCommitVoteAggSig.size());

    PSD_SET_MSG(PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_PUSH, pbmsg, btMsg)
    return true;
}

//---------------------------
bool CConsBlockVote::SendNetData(const uint64 nNetId, const bytes& btData)
{
    return sendNetData(nNetId, nTunnelId, btData);
}

void CConsBlockVote::NodeSubscribe(const uint64 nNetId, const uint32 nPeerVersion)
{
    auto& node = mapNetNode[nNetId];
    node.Subscribe();
    node.SetPeerVersion(nPeerVersion);

    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        bytes btMsg;
        if (MakePreVoteBitmapReq(btMsg, hashBlock))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "Node subscribe: Send pre vote bitmap req, net id: 0x%lx", nNetId);
#endif
        }

        btMsg.clear();
        if (MakeCommitVoteBitmapReq(btMsg, hashBlock))
        {
            SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "Node subscribe: Send commit vote bitmap req, net id: 0x%lx", nNetId);
#endif
        }
    }
}

void CConsBlockVote::SendSubscribeReq(const uint64 nNetId)
{
    bytes btMsg;
    if (MakeSubscribeReq(btMsg, BLOCK_VOTE_PRO_VER_1))
    {
        SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Send msg: subscribe req, node id: 0x%lx", nNetId);
#endif
    }
}

void CConsBlockVote::SendSubscribeRsp(const uint64 nNetId, const bool fResult)
{
    bytes btMsg;
    if (MakeSubscribeRsp(btMsg, BLOCK_VOTE_PRO_VER_1, (fResult ? 1 : 0)))
    {
        SendNetData(nNetId, btMsg);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Send msg: subscribe rsp, node id: 0x%lx", nNetId);
#endif
    }
}

void CConsBlockVote::BroadcastData(const uint64 nExcludeNetId, const bytes& btMsg)
{
    for (auto& kv : mapNetNode)
    {
        if (kv.second.IsSubscribe() && (nExcludeNetId == 0 || kv.first != nExcludeNetId))
        {
            SendNetData(kv.first, btMsg);
        }
    }
}

CConsBlock* CConsBlockVote::LoadVoteBlock(const uint256& hashBlock, const uint64 nNetId)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it == mapConsBlock.end())
    {
        uint32 nBlockEpoch = 0;
        int64 nBlockTime = 0;
        vector<uint384> vPubkey;
        bytes btAggBitmap;
        bytes btAggSig;
        if (!getVoteBlockCandidatePubkey(hashBlock, nBlockEpoch, nBlockTime, vPubkey, btAggBitmap, btAggSig))
        {
            return nullptr;
        }

        if (!btAggBitmap.empty() && !btAggSig.empty())
        {
            if (nNetId != 0)
            {
                bytes btMsg;
                if (MakeAggCommitVotePush(btMsg, hashBlock, btAggBitmap, btAggSig))
                {
                    SendNetData(nNetId, btMsg);
                }
            }
            return nullptr;
        }

        if (!AddCandidatePubkey(hashBlock, nBlockEpoch, nBlockTime, vPubkey))
        {
            StdLog("CConsBlockVote", "Load vote block: Add candidate pubkey fail, block: %s", hashBlock.GetBhString().c_str());
            return nullptr;
        }
        it = mapConsBlock.find(hashBlock);
        if (it == mapConsBlock.end())
        {
            StdLog("CConsBlockVote", "Load vote block: Get cons block fail, block: %s", hashBlock.GetBhString().c_str());
            return nullptr;
        }
    }
    return &(it->second);
}

void CConsBlockVote::RemoveVoteBlock(const uint256& hashBlock)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it != mapConsBlock.end())
    {
        if (it->second.nAddTime != 0)
        {
            auto mt = mapAddTime.find(it->second.nAddTime);
            if (mt != mapAddTime.end())
            {
                mt->second.erase(hashBlock);
                if (mt->second.empty())
                {
                    mapAddTime.erase(mt);
                }
            }
        }
        mapConsBlock.erase(it);
    }
}

bool CConsBlockVote::AddLocalPreVoteSign(const uint256& hashBlock, CConsBlock& consHeight)
{
    if (addBlockLocalSignFlag(hashBlock))
    {
        map<uint384, bytes> mapSigList;
        if (!GetLocalKeySignData(consHeight, GetPreVoteSignData(hashBlock), mapSigList))
        {
            StdLog("CConsBlockVote", "Add local pre vote sign: Get local sig fail, block: %s", hashBlock.GetBhString().c_str());
            return false;
        }
        for (auto& kv : mapSigList)
        {
            if (!consHeight.AddPreVoteSign(kv.first, kv.second))
            {
                StdLog("CConsBlockVote", "Add local pre vote sign: Add pre vote sig fail, block: %s", hashBlock.GetBhString().c_str());
                return false;
            }
        }
    }
    return true;
}

uint256 CConsBlockVote::GetPreVoteSignData(const uint256& hashBlock)
{
    mtbase::CBufStream ms;
    ms << string("prevote") << hashBlock;
    return CryptoHash(ms.GetData(), ms.GetSize());
}

uint256 CConsBlockVote::GetCommitVoteSignData(const uint256& hashBlock)
{
    mtbase::CBufStream ms;
    ms << string("commitvote") << hashBlock;
    return CryptoHash(ms.GetData(), ms.GetSize());
}

bool CConsBlockVote::GetBitPubkeysByBitmap(const vector<uint384>& vCandidatePubkeys, const bytes& btBitmap, vector<uint384>& vBitmapPubkeys)
{
    CBitmap bmBitmap;
    if (!bmBitmap.ImportBytes(btBitmap))
    {
        StdLog("CConsBlockVote", "Get bit pubkey by bitmap: Import bitmap fail, btBitmap: %s", ToHexString(btBitmap).c_str());
        return false;
    }
    vector<uint32> vIndexList;
    bmBitmap.GetIndexList(vIndexList);
    for (auto& index : vIndexList)
    {
        if (index >= vCandidatePubkeys.size())
        {
            StdLog("CConsBlockVote", "Get bit pubkey by bitmap: Index error, index: %d, size: %lu", index, vCandidatePubkeys.size());
            return false;
        }
        vBitmapPubkeys.push_back(vCandidatePubkeys[index]);
    }
    if (vBitmapPubkeys.size() < (vCandidatePubkeys.size() * 2 / 3))
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Get bit pubkey by bitmap: Less than two-thirds, vote count: %lu, candidate count: %lu", vBitmapPubkeys.size(), vCandidatePubkeys.size());
#endif
        return false;
    }
    return true;
}

bool CConsBlockVote::GetLocalKeySignData(const CConsBlock& consHeight, const uint256& hash, map<uint384, bytes>& mapSigList)
{
    for (auto& kv : mapConsKey)
    {
        if (consHeight.IsExistCandidateNodePubkey(kv.second.pubkey))
        {
            bytes btSig;
            if (!kv.second.Sign(hash, btSig))
            {
                StdLog("CConsBlockVote", "Get local key sig data: Sign fail, pubkey: %s", kv.second.pubkey.GetHex().c_str());
                return false;
            }
            mapSigList.insert(make_pair(kv.second.pubkey, btSig));
        }
    }
    return true;
}

void CConsBlockVote::BroadcastPreVoteBitmapPush(const uint256& hashBlock, CConsBlock& consHeight)
{
    bytes btMsg;
    if (MakePreVoteBitmapReq(btMsg, hashBlock))
    {
        BroadcastData(0, btMsg);
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Broadcast pre vote bitmap req, block: %s", hashBlock.GetBhString().c_str());
#endif
    }

    bytes btBitmap;
    if (consHeight.GetPreVoteBitmap(btBitmap))
    {
        btMsg.clear();
        if (MakePreVoteBitmapPush(btMsg, hashBlock, btBitmap))
        {
            BroadcastData(0, btMsg);
#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "Broadcast pre vote bitmap push, block: %s", hashBlock.GetBhString().c_str());
#endif
        }
    }
}

bool CConsBlockVote::VerifyPreVoteSign(const uint256& hashBlock, const uint384& pubkeyNode, const bytes& btSig)
{
    return CryptoBlsVerify(pubkeyNode, GetPreVoteSignData(hashBlock).GetBytes(), btSig);
}

bool CConsBlockVote::VerifyCommitVoteSign(const uint256& hashBlock, const uint384& pubkeyNode, const bytes& btSig)
{
    return CryptoBlsVerify(pubkeyNode, GetCommitVoteSignData(hashBlock).GetBytes(), btSig);
}

bool CConsBlockVote::VerifyPreVoteAggSign(const uint256& hashBlock, const CBitmap& bmPreVoteBitmap, const bytes& btPreVoteAggSig)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it == mapConsBlock.end())
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Verify pre vote agg sig: Find block fail, block: %s", hashBlock.GetBhString().c_str());
#endif
        return false;
    }

    vector<uint384> vPubkeys;
    if (!it->second.GetPubkeysByBitmap(bmPreVoteBitmap, vPubkeys))
    {
        StdLog("CConsBlockVote", "Verify pre vote agg sig: Get pre vote pubkey fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    if (!CryptoBlsFastAggregateVerify(vPubkeys, GetPreVoteSignData(hashBlock).GetBytes(), btPreVoteAggSig))
    {
        StdLog("CConsBlockVote", "Verify pre vote agg sig: Aggregate verify fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

bool CConsBlockVote::VerifyCommitVoteAggSign(const uint256& hashBlock, const CBitmap& bmCommitVoteBitmap, const bytes& btCommitVoteAggSig)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it == mapConsBlock.end())
    {
#ifdef CBV_SHOW_DEBUG
        StdDebug("CConsBlockVote", "Verify commit vote agg sig: Find block fail, block: %s", hashBlock.GetBhString().c_str());
#endif
        return false;
    }

    vector<uint384> vPubkeys;
    if (!it->second.GetPubkeysByBitmap(bmCommitVoteBitmap, vPubkeys))
    {
        StdLog("CConsBlockVote", "Verify commit vote agg sig: Get vote pubkey fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }

    if (!CryptoBlsFastAggregateVerify(vPubkeys, GetCommitVoteSignData(hashBlock).GetBytes(), btCommitVoteAggSig))
    {
        StdLog("CConsBlockVote", "Verify commit vote agg sig: Aggregate verify fail, block: %s", hashBlock.GetBhString().c_str());
        return false;
    }
    return true;
}

void CConsBlockVote::GetMaxConsBlockHash(set<uint256>& setMaxBlockHash)
{
    uint32 nMaxHeight = 0;
    uint64 nMaxBlockTime = 0;
    for (auto& kv : mapConsBlock)
    {
        if (kv.second.nBlockEpoch > nMaxHeight || (kv.second.nBlockEpoch == nMaxHeight && kv.second.nVoteBeginTime > nMaxBlockTime))
        {
            nMaxHeight = kv.second.nBlockEpoch;
            nMaxBlockTime = kv.second.nVoteBeginTime;
        }
    }
    for (auto& kv : mapConsBlock)
    {
        if (kv.second.nBlockEpoch == nMaxHeight && kv.second.nVoteBeginTime == nMaxBlockTime)
        {
            setMaxBlockHash.insert(kv.first);
        }
    }
    if (!mapAddTime.empty())
    {
        for (auto& hash : mapAddTime.rbegin()->second)
        {
            setMaxBlockHash.insert(hash);
        }
    }
}

void CConsBlockVote::BroadcastPreVoteBitmap()
{
    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        auto it = mapConsBlock.find(hashBlock);
        if (it != mapConsBlock.end())
        {
            if (!it->second.IsHasAggCommitVoteSign())
            {
                bytes btBitmap;
                if (it->second.GetPreVoteBitmap(btBitmap) && !btBitmap.empty())
                {
                    bytes btMsg;
                    if (MakePreVoteBitmapPush(btMsg, hashBlock, btBitmap))
                    {
                        BroadcastData(0, btMsg);
                    }
                }
            }
        }
    }
}

void CConsBlockVote::BroadcastCommitVoteBitmap()
{
    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        auto it = mapConsBlock.find(hashBlock);
        if (it != mapConsBlock.end())
        {
            if (!it->second.IsHasAggCommitVoteSign())
            {
                bytes btBitmap;
                if (it->second.GetCommitVoteBitmap(btBitmap) && !btBitmap.empty())
                {
                    bytes btMsg;
                    if (MakeCommitVoteBitmapPush(btMsg, hashBlock, btBitmap))
                    {
                        BroadcastData(0, btMsg);
                    }
                }
            }
        }
    }
}

void CConsBlockVote::BroadcastAggCommitVoteSign()
{
    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        auto it = mapConsBlock.find(hashBlock);
        if (it != mapConsBlock.end())
        {
            bytes btAggBitmap;
            bytes btAggSig;
            if (it->second.GetAggCommitVoteSign(btAggBitmap, btAggSig))
            {
                bytes btMsg;
                if (MakeAggCommitVotePush(btMsg, hashBlock, btAggBitmap, btAggSig))
                {
                    BroadcastData(0, btMsg);
#ifdef CBV_SHOW_DEBUG
                    StdDebug("CConsBlockVote", "Broadcast agg commit vote sign: Broadcast agg commit vote sig push, block: %s", hashBlock.GetBhString().c_str());
#endif
                }
            }
            else
            {
                bytes btMsg;
                if (MakeAggCommitVoteReq(btMsg, hashBlock))
                {
                    BroadcastData(0, btMsg);
                }
            }
        }
    }
}

void CConsBlockVote::CheckPreVoteBitmapReq()
{
    int64 nWaitTime = nEpochDuration / 5;
    if (nWaitTime < 2000)
    {
        nWaitTime = 2000;
    }
    else if (nWaitTime > 5000)
    {
        nWaitTime = 5000;
    }

    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        for (auto& kv : mapNetNode)
        {
            if (kv.second.CheckPrevBitmapReqTime(nWaitTime))
            {
                bytes btMsg;
                if (MakePreVoteBitmapReq(btMsg, hashBlock))
                {
                    SendNetData(kv.first, btMsg);
#ifdef CBV_SHOW_DEBUG
                    StdDebug("CConsBlockVote", "Check pre vote bitmap req: Send pre vote bitmap req, net id: 0x%lx, block: %s", kv.first, hashBlock.GetBhString().c_str());
#endif
                }
            }
        }
    }
}

void CConsBlockVote::CheckPreVote(const uint256& hashBlock)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it != mapConsBlock.end() && it->second.bmAggPreVoteBitmap.IsNull())
    {
        CBitmap bmPreVoteBitmap;
        vector<uint384> vPreVotePubkeys;
        vector<bytes> vPreVoteSigs;
        if (it->second.GetLocalPreVoteSign(nEpochDuration, bmPreVoteBitmap, vPreVotePubkeys, vPreVoteSigs))
        {
            bytes btPreVoteAggSig;
            if (!CryptoBlsAggregateSig(vPreVoteSigs, btPreVoteAggSig))
            {
                StdLog("CConsBlockVote", "Check pre vote: Aggregate sig fail, block: %s", hashBlock.GetBhString().c_str());
                return;
            }
            it->second.bmAggPreVoteBitmap = bmPreVoteBitmap;
            it->second.btAggPreVoteSig = btPreVoteAggSig;

            map<uint384, bytes> mapBroadcastSig;
            if (!GetLocalKeySignData(it->second, GetCommitVoteSignData(hashBlock), mapBroadcastSig))
            {
                StdLog("CConsBlockVote", "Check pre vote: Get local sig fail, block: %s", hashBlock.GetBhString().c_str());
                return;
            }
            for (auto& kv : mapBroadcastSig)
            {
                if (!it->second.AddCommitVoteSign(kv.first, kv.second))
                {
                    StdLog("CConsBlockVote", "Check pre vote: Add commit vote sig fail, block: %s", hashBlock.GetBhString().c_str());
                    return;
                }
            }

            if (!mapBroadcastSig.empty())
            {
                bytes btMsg;
                if (MakeCommitVoteSignPush(btMsg, hashBlock, bmPreVoteBitmap.GetBytes(), btPreVoteAggSig, mapBroadcastSig))
                {
                    BroadcastData(0, btMsg);
#ifdef CBV_SHOW_DEBUG
                    StdDebug("CConsBlockVote", "Check pre vote: Broadcast commit vote sig push, block: %s", hashBlock.GetBhString().c_str());
#endif
                }
            }
        }
    }
}

void CConsBlockVote::CheckCommitVote(const uint256& hashBlock)
{
    auto it = mapConsBlock.find(hashBlock);
    if (it != mapConsBlock.end() && !it->second.IsHasAggCommitVoteSign())
    {
        CBitmap bmCommitVoteBitmap;
        vector<uint384> vCommitVotePubkeys;
        vector<bytes> vCommitVoteSigs;
        if (it->second.GetLocalCommitVoteSign(nEpochDuration, bmCommitVoteBitmap, vCommitVotePubkeys, vCommitVoteSigs))
        {
            bytes btCommitVoteAggSig;
            if (!CryptoBlsAggregateSig(vCommitVoteSigs, btCommitVoteAggSig))
            {
                StdLog("CConsBlockVote", "Check commit vote: Aggregate sig fail, block: %s", hashBlock.GetBhString().c_str());
                return;
            }
            if (bmCommitVoteBitmap.IsNull())
            {
                StdLog("CConsBlockVote", "Check commit vote: Agg bitmap is null, block: %s", hashBlock.GetBhString().c_str());
                return;
            }
            bytes btAggBitmap;
            bmCommitVoteBitmap.GetBytes(btAggBitmap);
            it->second.SetAggCommitVoteSign(btAggBitmap, btCommitVoteAggSig);

            commitVoteResult(hashBlock, btAggBitmap, btCommitVoteAggSig);

#ifdef CBV_SHOW_DEBUG
            StdDebug("CConsBlockVote", "Check commit vote: ##################################, epoch: %d, commit vote bitmap: %s, commit agg sig: %s, vote duration: %lu ms, block: %s",
                     it->second.nBlockEpoch, bmCommitVoteBitmap.GetBitmapString().c_str(), ToHexString(btCommitVoteAggSig).c_str(),
                     GetTimeMillis() - it->second.nBeginTimeMillis, hashBlock.GetBhString().c_str());
#endif

            bytes btMsg;
            if (MakeAggCommitVotePush(btMsg, hashBlock, btAggBitmap, btCommitVoteAggSig))
            {
                BroadcastData(0, btMsg);
#ifdef CBV_SHOW_DEBUG
                StdDebug("CConsBlockVote", "Check commit vote: Broadcast agg commit vote sig push, block: %s", hashBlock.GetBhString().c_str());
#endif
            }

            RemoveVoteBlock(hashBlock);
        }
    }
}

void CConsBlockVote::CheckLocalVote()
{
    set<uint256> setMaxBlockHash;
    GetMaxConsBlockHash(setMaxBlockHash);

    for (auto& hashBlock : setMaxBlockHash)
    {
        auto it = mapConsBlock.find(hashBlock);
        if (it != mapConsBlock.end())
        {
            CheckPreVote(hashBlock);
            CheckCommitVote(hashBlock);
        }
    }
}

} // namespace consblockvote
} // namespace consensus

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __CONSENSUS_CONSBLOCKVOTE_H
#define __CONSENSUS_CONSBLOCKVOTE_H

#include "conscommon.h"

namespace consensus
{

namespace consblockvote
{

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

/////////////////////////////////
// Protocol version

#define BLOCK_VOTE_PRO_VER_1 1

/////////////////////////////////
// const define

#define MAX_AWAIT_BIT_COUNT 20
#define MAX_CONS_HEIGHT_COUNT 2048

/////////////////////////////////
// Message id

#define PS_MSGID_BLOCKVOTE_SUBSCRIBE_REQ 1
#define PS_MSGID_BLOCKVOTE_SUBSCRIBE_RSP 2

#define PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_REQ 3
#define PS_MSGID_BLOCKVOTE_PRE_VOTE_BITMAP_PUSH 4

#define PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_REQ 5
#define PS_MSGID_BLOCKVOTE_PRE_VOTE_SIG_PUSH 6

#define PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_REQ 7
#define PS_MSGID_BLOCKVOTE_COMMIT_VOTE_BITMAP_PUSH 8

#define PS_MSGID_BLOCKVOTE_COMMIT_VOTE_REQ 9
#define PS_MSGID_BLOCKVOTE_COMMIT_VOTE_PUSH 10

#define PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_REQ 11
#define PS_MSGID_BLOCKVOTE_AGG_COMMIT_VOTE_PUSH 12

/////////////////////////////////
// CNetNode

class CNetNode
{
public:
    CNetNode()
      : fSubscribe(false), nPeerVersion(0), nPrevBitmapReqTime(0) {}

    void Subscribe()
    {
        fSubscribe = true;
    }
    bool IsSubscribe() const
    {
        return fSubscribe;
    }
    void SetPeerVersion(const uint32 nPeerVersionIn)
    {
        nPeerVersion = nPeerVersionIn;
    }
    uint32 GetPeerVersion() const
    {
        return nPeerVersion;
    }

    void SetPrevBitmapReqTime()
    {
        nPrevBitmapReqTime = GetTimeMillis();
    }
    bool CheckPrevBitmapReqTime(const int64 nTimeLen)
    {
        if (fSubscribe && GetTimeMillis() - nPrevBitmapReqTime >= nTimeLen)
        {
            nPrevBitmapReqTime = GetTimeMillis();
            return true;
        }
        return false;
    }

private:
    bool fSubscribe;
    uint32 nPeerVersion;
    int64 nPrevBitmapReqTime;
};

/////////////////////////////////
// CConsKey

class CConsKey
{
public:
    CConsKey(const uint256& prikeyIn, const uint384& pubkeyIn)
      : prikey(prikeyIn), pubkey(pubkeyIn) {}

    bool Sign(const uint256& hash, bytes& btSig);
    bool Verify(const uint256& hash, const bytes& btSig);

public:
    const uint256 prikey;
    const uint384 pubkey;
};

class CNodePubkey
{
public:
    CNodePubkey(const uint32 nIndexIn, const uint384& pubkeyIn)
      : nIndex(nIndexIn), pubkey(pubkeyIn), eStatus(ES_INIT), tmStatusTime(time(NULL)) {}

    enum E_STATUS
    {
        ES_INIT,
        ES_WAITING,
        ES_COMPLETED,
    };

    void InitStatus()
    {
        eStatus = ES_INIT;
        tmStatusTime = time(NULL);
    }
    void SetStatus(const E_STATUS eStatusIn)
    {
        eStatus = eStatusIn;
        tmStatusTime = time(NULL);
    }
    E_STATUS GetStatus() const
    {
        return eStatus;
    }
    void CheckStatus()
    {
        if (eStatus == ES_WAITING && time(NULL) - tmStatusTime >= 100)
        {
            eStatus = ES_INIT;
            tmStatusTime = time(NULL);
        }
    }

public:
    const uint32 nIndex;
    const uint384 pubkey;

    E_STATUS eStatus;
    time_t tmStatusTime;
};

/////////////////////////////////
// CConsBlock

class CConsBlock
{
public:
    CConsBlock(const uint256& hashBlockIn, const uint32 nBlockEpochIn, const int64 nVoteBeginTimeIn, const uint64 nAddTimeIn)
      : hashBlock(hashBlockIn), nBlockEpoch(nBlockEpochIn), nVoteBeginTime(nVoteBeginTimeIn), nBeginTimeMillis(GetTimeMillis()), nAddTime(nAddTimeIn) {}

    bool SetCandidateNodeList(const vector<uint384>& vCandidateNodePubkeyIn);
    bool IsExistCandidateNodePubkey(const uint384& pubkeyNode) const;
    bool ExistPreVoteSign(const uint384& pubkeyNode);
    bool ExistCommitVoteSign(const uint384& pubkeyNode);
    bool AddPreVoteSign(const uint384& pubkeyNode, const bytes& btSig);
    bool AddCommitVoteSign(const uint384& pubkeyNode, const bytes& btSig);
    bool GetPreVoteBitmap(bytes& btBitmap);
    bool GetCommitVoteBitmap(bytes& btBitmap);
    void GetPreVoteSigByBitmap(const bytes& btGetBitmap, map<uint384, bytes>& mapSigOut);
    void GetCommitVoteSigByBitmap(const CBitmap& bmGetBitmap, map<uint384, bytes>& mapSigOut);
    bool GetPreVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut);
    bool GetCommitVoteAwaitBitmap(const bytes& btBitmapPeer, bytes& btBitmapOut);
    bool GetPubkeysByBitmap(const CBitmap& bmBitmap, vector<uint384>& vPubkeys);
    bool GetLocalPreVoteSign(const int64 nEpochDurationIn, CBitmap& bmPreVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs);
    bool GetLocalCommitVoteSign(const int64 nEpochDurationIn, CBitmap& bmCommitVoteBitmap, vector<uint384>& vPubkeys, vector<bytes>& vSigs);
    void SetAggCommitVoteSign(const bytes& btAggCommitVoteBitmapIn, const bytes& btAggCommitVoteSigIn);
    bool GetAggCommitVoteSign(bytes& btAggCommitVoteBitmapOut, bytes& btAggCommitVoteSigOut);
    bool IsHasAggCommitVoteSign();

public:
    const uint256 hashBlock;
    const uint32 nBlockEpoch;
    const int64 nVoteBeginTime;
    map<uint384, uint32> mapCandidateNodeIndex; // key: node pubkey, value: index

    vector<CNodePubkey> vPreVoteCandidateNodePubkey;
    vector<CNodePubkey> vCommitVoteCandidateNodePubkey;

    map<uint384, bytes> mapPreVoteSig; // key: node pubkey, value: node hash
    CBitmap bmBlockPreVoteBitmap;

    map<uint384, bytes> mapCommitVoteSig; // key: node pubkey, value: node hash
    CBitmap bmBlockCommitVoteBitmap;

    CBitmap bmAggPreVoteBitmap;
    bytes btAggPreVoteSig;
    bytes btAggCommitVoteBitmap;
    bytes btAggCommitVoteSig;

    int64 nBeginTimeMillis;
    uint64 nAddTime;
};

/////////////////////////////////
// CConsBlockVote

class CConsBlockVote
{
public:
    CConsBlockVote(const uint8 nTunnelIdIn, const int64 nEpochDurationIn, funcSendNetData sendNetDataIn, funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkeyIn, funcAddBlockLocalSignFlag addBlockLocalSignFlagIn, funcCommitVoteResult commitVoteResultIn)
      : nTunnelId(nTunnelIdIn), nEpochDuration(nEpochDurationIn), sendNetData(sendNetDataIn), getVoteBlockCandidatePubkey(getVoteBlockCandidatePubkeyIn), addBlockLocalSignFlag(addBlockLocalSignFlagIn), commitVoteResult(commitVoteResultIn), nPrevCheckPreVoteBitmapTime(0) {}

    bool AddConsKey(const uint256& prikey, const uint384& pubkey);
    bool AddCandidatePubkey(const uint256& hashBlock, const uint32 nBlockEpoch, const int64 nVoteBeginTimeIn, const vector<uint384>& vPubkey);
    bool GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig);

    bool AddNetNode(const uint64 nNetId);
    void RemoveNetNode(const uint64 nNetId);

    void OnTimer();
    void OnEventNetData(const uint64 nNetId, const bytes& btData);

    static bool VerifyCommitVoteAggSig(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig, const vector<uint384>& vCandidatePubkeys);

private:
    void OnNetMsgSubscribeReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgSubscribeRsp(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgPreVoteBitmapReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgPreVoteBitmapPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgPreVoteSignReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgPreVoteSignPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgCommitVoteBitmapReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgCommitVoteBitmapPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgCommitVoteSignReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgCommitVoteSignPush(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgAggCommitVoteReq(const uint64 nNetId, const uint8* pData, const size_t nDataLen);
    void OnNetMsgAggCommitVotePush(const uint64 nNetId, const uint8* pData, const size_t nDataLen);

    bool MakeSubscribeReq(bytes& btMsg, const uint32 nVersion);
    bool MakeSubscribeRsp(bytes& btMsg, const uint32 nVersion, const uint32 nResult);
    bool MakePreVoteBitmapReq(bytes& btMsg, const uint256& hashBlock);
    bool MakePreVoteBitmapPush(bytes& btMsg, const uint256& hashBlock, const bytes& btBitmap);
    bool MakePreVoteSignReq(bytes& btMsg, const uint256& hashBlock, const bytes& btGetBitmap);
    bool MakePreVoteSignPush(bytes& btMsg, const uint256& hashBlock, const map<uint384, bytes>& mapPreVoteSigIn);
    bool MakeCommitVoteBitmapReq(bytes& btMsg, const uint256& hashBlock);
    bool MakeCommitVoteBitmapPush(bytes& btMsg, const uint256& hashBlock, const bytes& btBitmap);
    bool MakeCommitVoteSignReq(bytes& btMsg, const uint256& hashBlock, const bytes& btGetBitmap);
    bool MakeCommitVoteSignPush(bytes& btMsg, const uint256& hashBlock, const bytes& btPreVoteBitmap, const bytes& btPreVoteAggSig, const map<uint384, bytes>& mapCommitVoteSig);
    bool MakeAggCommitVoteReq(bytes& btMsg, const uint256& hashBlock);
    bool MakeAggCommitVotePush(bytes& btMsg, const uint256& hashBlock, const bytes& btCommitVoteBitmap, const bytes& btCommitVoteAggSig);

    bool SendNetData(const uint64 nNetId, const bytes& btData);
    void NodeSubscribe(const uint64 nNetId, const uint32 nPeerVersion);
    void SendSubscribeReq(const uint64 nNetId);
    void SendSubscribeRsp(const uint64 nNetId, const bool fResult);
    void BroadcastData(const uint64 nExcludeNetId, const bytes& btMsg);
    CConsBlock* LoadVoteBlock(const uint256& hashBlock, const uint64 nNetId = 0);
    void RemoveVoteBlock(const uint256& hashBlock);
    static uint256 GetPreVoteSignData(const uint256& hashBlock);
    static uint256 GetCommitVoteSignData(const uint256& hashBlock);
    static bool GetBitPubkeysByBitmap(const vector<uint384>& vCandidatePubkeys, const bytes& btBitmap, vector<uint384>& vBitmapPubkeys);
    bool GetLocalKeySignData(const CConsBlock& consHeight, const uint256& hash, map<uint384, bytes>& mapSigList);
    bool AddLocalPreVoteSign(const uint256& hashBlock, CConsBlock& consHeight);
    void BroadcastPreVoteBitmapPush(const uint256& hashBlock, CConsBlock& consHeight);
    bool VerifyPreVoteSign(const uint256& hashBlock, const uint384& pubkeyNode, const bytes& btSig);
    bool VerifyCommitVoteSign(const uint256& hashBlock, const uint384& pubkeyNode, const bytes& btSig);
    bool VerifyPreVoteAggSign(const uint256& hashBlock, const CBitmap& bmPreVoteBitmap, const bytes& btPreVoteAggSig);
    bool VerifyCommitVoteAggSign(const uint256& hashBlock, const CBitmap& bmCommitVoteBitmap, const bytes& btCommitVoteAggSig);
    void GetMaxConsBlockHash(set<uint256>& setMaxBlockHash);
    void BroadcastPreVoteBitmap();
    void BroadcastCommitVoteBitmap();
    void BroadcastAggCommitVoteSign();
    void CheckPreVoteBitmapReq();
    void CheckPreVote(const uint256& hashBlock);
    void CheckCommitVote(const uint256& hashBlock);
    void CheckLocalVote();

private:
    const uint8 nTunnelId;
    const int64 nEpochDuration; //Unit: milliseconds
    funcSendNetData sendNetData;
    funcGetVoteBlockCandidatePubkey getVoteBlockCandidatePubkey;
    funcAddBlockLocalSignFlag addBlockLocalSignFlag;
    funcCommitVoteResult commitVoteResult;

    map<uint256, CConsKey> mapConsKey;     // key: prikey
    map<uint64, CNetNode> mapNetNode;      // key: node netid
    map<uint256, CConsBlock> mapConsBlock; // key: block hash
    map<uint64, set<uint256>> mapAddTime;

    int64 nPrevCheckPreVoteBitmapTime;
};

} // namespace consblockvote
} // namespace consensus

#endif

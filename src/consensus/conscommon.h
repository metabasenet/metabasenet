// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __CONSENSUS_CONSCOMMON_H
#define __CONSENSUS_CONSCOMMON_H

#include "bitmap/bitmap.h"
#include "block.h"
#include "crypto.h"
#include "type.h"
#include "uint256.h"

namespace consensus
{

using namespace std;
using namespace mtbase;

/////////////////////////////////
// Function

typedef boost::function<bool(const uint64 nNetId, const uint8 nTunnelId, const bytes& btData)> funcSendNetData;
typedef boost::function<bool(const uint256& hashBlock, uint32& nBlockHeight, int64& nBlockTime, vector<uint384>& vPubkey, bytes& btAggBitmap, bytes& btAggSig)> funcGetVoteBlockCandidatePubkey;
typedef boost::function<bool(const uint256& hashBlock)> funcAddBlockLocalSignFlag;
typedef boost::function<bool(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig)> funcCommitVoteResult;
typedef boost::function<bool(const uint64 nNetId, const uint8 nTunnelId, const uint256& hashBlock, const uint384& pubkeyNode,
                             const CBitmap& bmBitmap, const uint256& hash, const bytes& btSig)>
    funcVerifySig;

class CElectionVoteMaxRandom;
typedef boost::function<void(const uint256& hashBlock, const CElectionVoteMaxRandom& elecMaxRandom)> funcElectionResultCallback;

/////////////////////////////////
// CElectionVoteMaxRandom

class CElectionVoteMaxRandom
{
public:
    CElectionVoteMaxRandom() {}

    void SetNull()
    {
        hashRandom = 0;
        btHashBitmap.clear();
        btHashAggSig.clear();
        btVoteBitmap.clear();
        btVoteAggSig.clear();
    }
    uint32 GetHashSignCount() const
    {
        if (hashRandom == 0)
        {
            return 0;
        }
        CBitmap bm;
        bm.ImportBytes(btHashBitmap);
        return bm.GetValidBits();
    }
    void SetRandomData(const uint256& hashRandomIn, const bytes& btHashBitmapIn, const bytes& btHashAggSigIn,
                       const bytes& btVoteBitmapIn, const bytes& btVoteAggSigIn)
    {
        hashRandom = hashRandomIn;
        btHashBitmap = btHashBitmapIn;
        btHashAggSig = btHashAggSigIn;
        btVoteBitmap = btVoteBitmapIn;
        btVoteAggSig = btVoteAggSigIn;
    }

public:
    uint256 hashRandom;
    bytes btHashBitmap;
    bytes btHashAggSig;
    bytes btVoteBitmap;
    bytes btVoteAggSig;
};

/////////////////////////////////
// CInterNetData

class CInterNetData
{
public:
    CInterNetData(const uint64 nNetIdIn, const uint8 nTunnelIdIn, const bytes& btNetDataIn)
      : nNetId(nNetIdIn), nTunnelId(nTunnelIdIn), btNetData(btNetDataIn) {}

public:
    const uint64 nNetId;
    const uint8 nTunnelId;
    const bytes btNetData;
};

/////////////////////////////////
// CInterConsKey

class CInterConsKey
{
public:
    CInterConsKey(const uint256& prikeyIn, const uint384& pubkeyIn)
      : prikey(prikeyIn), pubkey(pubkeyIn) {}

public:
    const uint256 prikey;
    const uint384 pubkey;
};

/////////////////////////////////
// CInterNodePubkey

class CInterNodePubkey
{
public:
    CInterNodePubkey(const uint256& hashBlockIn, const uint32 nBlockEpochIn, const int64 nVoteBeginTimeIn, const vector<uint384>& vPubkeyIn)
      : hashBlock(hashBlockIn), nBlockEpoch(nBlockEpochIn), nVoteBeginTime(nVoteBeginTimeIn), vPubkey(vPubkeyIn) {}

public:
    const uint256 hashBlock;
    const uint32 nBlockEpoch;
    const int64 nVoteBeginTime;
    const vector<uint384> vPubkey;
};

/////////////////////////////////
// CConsMsgData

class CConsMsgData
{
public:
    enum
    {
        ENIO_NET_DATA = 1,
        ENIO_NODE_CONSKEY = 2,
        ENIO_NODE_PUBKEY = 3,
    };
    typedef const std::integral_constant<int, ENIO_NET_DATA> TYPE_NET_DATA;
    typedef const std::integral_constant<int, ENIO_NODE_CONSKEY> TYPE_NODE_CONSKEY;
    typedef const std::integral_constant<int, ENIO_NODE_PUBKEY> TYPE_NODE_PUBKEY;

    CConsMsgData() {}
    CConsMsgData(TYPE_NET_DATA&, const uint64 nNetIdIn, const uint8 nTunnelIdIn, const bytes& btDataIn)
      : varMsgData(CInterNetData(nNetIdIn, nTunnelIdIn, btDataIn)) {}
    CConsMsgData(TYPE_NODE_CONSKEY&, const uint256& prikeyIn, const uint384& pubkeyIn)
      : varMsgData(CInterConsKey(prikeyIn, pubkeyIn)) {}
    CConsMsgData(TYPE_NODE_PUBKEY&, const uint256& hashBlock, const uint32 nBlockEpoch, const int64 nVoteBeginTime, const vector<uint384>& vPubkeyIn)
      : varMsgData(CInterNodePubkey(hashBlock, nBlockEpoch, nVoteBeginTime, vPubkeyIn)) {}

    size_t GetIndex() const
    {
        return varMsgData.index();
    }
    const CInterNetData& GetNetData() const
    {
        return std::get<CInterNetData>(varMsgData);
    }
    const CInterConsKey& GetConsKey() const
    {
        return std::get<CInterConsKey>(varMsgData);
    }
    const CInterNodePubkey& GetNodePubkey() const
    {
        return std::get<CInterNodePubkey>(varMsgData);
    }

protected:
    std::variant<std::monostate, CInterNetData, CInterConsKey, CInterNodePubkey> varMsgData;
};

typedef shared_ptr<CConsMsgData> CONS_MSG_DATA;

} // namespace consensus

#endif

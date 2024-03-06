/// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_STRUCT_H
#define METABASENET_STRUCT_H

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <map>
#include <set>
#include <vector>

#include "block.h"
#include "proto.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{

// Status
class CBlockStatus
{
public:
    CBlockStatus()
      : nBlockTime(0), nBlockHeight(-1), nBlockSlot(0), nBlockNumber(0), nTotalTxCount(0), nRewardTxCount(0), nUserTxCount(0), nBlockType(0), nMintType(-1) {}

public:
    uint256 hashFork;
    uint256 hashPrevBlock;
    uint256 hashBlock;
    uint256 hashRefBlock;
    uint64 nBlockTime;
    int nBlockHeight;
    uint16 nBlockSlot;
    uint64 nBlockNumber;
    uint64 nTotalTxCount;
    uint64 nRewardTxCount;
    uint64 nUserTxCount;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    uint16 nBlockType;
    uint16 nMintType;
    uint256 hashStateRoot;
    CDestination destMint;
};

class CForkStatus
{
public:
    CForkStatus() {}
    CForkStatus(const uint256& hashOriginIn, const uint256& hashParentIn, int nOriginHeightIn)
      : hashOrigin(hashOriginIn),
        hashParent(hashParentIn),
        nOriginHeight(nOriginHeightIn),
        nLastBlockTime(0),
        nLastBlockHeight(-1),
        nLastBlockNumber(0),
        nMintType(-1)
    {
    }

    bool IsNull()
    {
        return nLastBlockTime == 0;
    }

public:
    uint256 hashOrigin;
    uint256 hashParent;
    int nOriginHeight;

    uint256 hashPrevBlock;
    uint256 hashLastBlock;
    int64 nLastBlockTime;
    int nLastBlockHeight;
    uint32 nLastBlockNumber;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    uint16 nMintType;
    std::multimap<int, uint256> mapSubline;
};

class CWalletBalance
{
public:
    uint8 nDestType;
    uint8 nTemplateType;
    uint64 nTxNonce;
    uint256 nAvailable;
    uint256 nLocked;
    uint256 nUnconfirmedIn;
    uint256 nUnconfirmedOut;

public:
    CWalletBalance()
    {
        SetNull();
    }

    void SetNull()
    {
        nDestType = 0;
        nTemplateType = 0;
        nTxNonce = 0;
        nAvailable = 0;
        nLocked = 0;
        nUnconfirmedIn = 0;
        nUnconfirmedOut = 0;
    }
};

// Notify

class CNetworkPeerUpdate
{
public:
    bool fActive;
    uint64 nPeerNonce;
    network::CAddress addrPeer;
};

class CDelegateRoutine
{
public:
    std::vector<std::pair<uint256, std::map<CDestination, size_t>>> vEnrolledWeight;

    std::vector<CTransaction> vEnrollTx;
    std::vector<std::pair<uint256, std::map<CDestination, std::vector<unsigned char>>>> vDistributeData;
    std::map<CDestination, std::vector<unsigned char>> mapPublishData;
    uint256 hashDistributeOfPublish;
    bool fPublishCompleted;

public:
    CDelegateRoutine()
      : fPublishCompleted(false) {}
};

class CDelegateEnrolled
{
public:
    CDelegateEnrolled() {}
    void Clear()
    {
        mapWeight.clear();
        mapEnrollData.clear();
        vecAmount.clear();
    }

public:
    std::map<CDestination, std::size_t> mapWeight;
    std::map<CDestination, std::vector<unsigned char>> mapEnrollData;
    std::vector<std::pair<CDestination, uint256>> vecAmount;
};

class CDelegateAgreement
{
public:
    CDelegateAgreement()
    {
        nAgreement = 0;
        nWeight = 0;
    }
    bool IsProofOfWork() const
    {
        return (vBallot.empty());
    }
    const CDestination GetBallot(int nIndex) const
    {
        if (vBallot.empty())
        {
            return CDestination();
        }
        return vBallot[nIndex % vBallot.size()];
    }
    bool operator==(const CDelegateAgreement& other)
    {
        return (nAgreement == other.nAgreement && nWeight == other.nWeight);
    }
    void Clear()
    {
        nAgreement = 0;
        nWeight = 0;
        vBallot.clear();
    }

public:
    uint256 nAgreement;
    std::size_t nWeight;
    std::vector<CDestination> vBallot;
};

class CAgreementBlock
{
public:
    CAgreementBlock()
      : nPrevTime(0), nPrevHeight(0), nPrevNumber(0), nPrevMintType(0), nWaitTime(0), fCompleted(false), ret(false) {}

    uint256 hashPrev;
    int64 nPrevTime;
    int nPrevHeight;
    uint32 nPrevNumber;
    uint16 nPrevMintType;
    CDelegateAgreement agreement;
    int64 nWaitTime;
    bool fCompleted;
    bool ret;
};

class CCacheBlsPubkey
{
public:
    CCacheBlsPubkey()
      : nCreateIndex(0) {}

    bool AddBlsPubkey(const uint256& hashBlock, const std::vector<uint384>& vPubkey);
    bool GetBlsPubkey(const uint256& hashBlock, std::vector<uint384>& vPubkey) const;

protected:
    enum
    {
        MAX_PUBKEY_CACHE_COUNT = 10000,
        MAX_BLOCK_CACHE_COUNT = 100000,
    };

    uint64 nCreateIndex;
    std::map<uint256, std::vector<uint384>> mapPubkey;
    std::map<uint64, uint256> mapKeyIndex;
    std::map<uint256, uint256> mapBlock;
};

/* Protocol & Event */
class CNil
{
    friend class mtbase::CStream;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
    }
};

class CBlockMakerUpdate
{
public:
    uint256 hashParent;
    int nOriginHeight;

    uint256 hashPrevBlock;
    uint256 hashBlock;
    uint64 nBlockTime;
    int nBlockHeight;
    uint64 nBlockNumber;
    // uint256 nAgreement;
    // std::size_t nWeight;
    uint16 nMintType;
};

/* Net Channel */
class CPeerKnownTx
{
public:
    CPeerKnownTx() {}
    CPeerKnownTx(const uint256& txidIn)
      : txid(txidIn), nTime(mtbase::GetTime()) {}

public:
    uint256 txid;
    int64 nTime;
};

typedef boost::multi_index_container<
    CPeerKnownTx,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<CPeerKnownTx, uint256, &CPeerKnownTx::txid>>,
        boost::multi_index::ordered_non_unique<boost::multi_index::member<CPeerKnownTx, int64, &CPeerKnownTx::nTime>>>>
    CPeerKnownTxSet;

typedef CPeerKnownTxSet::nth_index<0>::type CPeerKnownTxSetById;
typedef CPeerKnownTxSet::nth_index<1>::type CPeerKnownTxSetByTime;

typedef boost::multi_index_container<
    uint256,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<boost::multi_index::identity<uint256>>>>
    CUInt256List;
typedef CUInt256List::nth_index<1>::type CUInt256ByValue;

/* CStatItemBlockMaker & CStatItemP2pSyn */
class CStatItemBlockMaker
{
public:
    CStatItemBlockMaker()
      : nTimeValue(0), nPOWBlockCount(0), nDPOSBlockCount(0), nBlockTPS(0), nTxTPS(0) {}

    uint32 nTimeValue;

    uint64 nPOWBlockCount;
    uint64 nDPOSBlockCount;
    uint64 nBlockTPS;
    uint64 nTxTPS;
};

class CStatItemP2pSyn
{
public:
    CStatItemP2pSyn()
      : nRecvBlockCount(0), nRecvTxTPS(0), nSendBlockCount(0), nSendTxTPS(0), nSynRecvTxTPS(0), nSynSendTxTPS(0) {}

    uint32 nTimeValue;

    uint64 nRecvBlockCount;
    uint64 nRecvTxTPS;
    uint64 nSendBlockCount;
    uint64 nSendTxTPS;

    uint64 nSynRecvTxTPS;
    uint64 nSynSendTxTPS;
};

} // namespace metabasenet

#endif // METABASENET_STRUCT_H

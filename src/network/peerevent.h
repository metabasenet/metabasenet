// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NETWORK_PEEREVENT_H
#define NETWORK_PEEREVENT_H

#include "block.h"
#include "mtbase.h"
#include "proto.h"
#include "transaction.h"

using namespace mtbase;

namespace metabasenet
{
namespace network
{

enum
{
    EVENT_PEER_BASE = mtbase::EVENT_USER_BASE,

    //PEER
    EVENT_PEER_ACTIVE,
    EVENT_PEER_DEACTIVE,
    EVENT_PEER_SUBSCRIBE,
    EVENT_PEER_UNSUBSCRIBE,
    EVENT_PEER_INV,
    EVENT_PEER_GETDATA,
    EVENT_PEER_GETBLOCKS,
    EVENT_PEER_TX,
    EVENT_PEER_BLOCK,
    EVENT_PEER_GETFAIL,
    EVENT_PEER_MSGRSP,

    EVENT_PEER_BLOCK_SUBSCRIBE,
    EVENT_PEER_BLOCK_UNSUBSCRIBE,
    EVENT_PEER_BLOCK_BKS,

    EVENT_PEER_CERTTX_SUBSCRIBE,
    EVENT_PEER_CERTTX_UNSUBSCRIBE,
    EVENT_PEER_CERTTX_TXS,

    EVENT_PEER_USERTX_SUBSCRIBE,
    EVENT_PEER_USERTX_UNSUBSCRIBE,
    EVENT_PEER_USERTX_TXS,

    EVENT_LOCAL_BLOCK_SUBSCRIBE_FORK,
    EVENT_LOCAL_BLOCK_BROADCAST_BKS,
    EVENT_LOCAL_CERTTX_BROADCAST_TXS,
    EVENT_LOCAL_USERTX_SUBSCRIBE_FORK,
    EVENT_LOCAL_USERTX_BROADCAST_TXS,

    EVENT_PEER_BULLETIN,
    EVENT_PEER_GETDELEGATED,
    EVENT_PEER_DISTRIBUTE,
    EVENT_PEER_PUBLISH,

    EVENT_PEER_MAX,
};

template <int type, typename L, typename D>
class CEventPeerData : public mtbase::CEvent
{
    friend class mtbase::CStream;

public:
    CEventPeerData(uint64 nNonceIn, const uint256& hashForkIn)
      : CEvent(nNonceIn, type), hashFork(hashForkIn) {}
    virtual ~CEventPeerData() {}
    virtual bool Handle(mtbase::CEventListener& listener)
    {
        try
        {
            return (dynamic_cast<L&>(listener)).HandleEvent(*this);
        }
        catch (std::bad_cast&)
        {
            return listener.HandleEvent(*this);
        }
        catch (std::exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashFork, opt);
        s.Serialize(data, opt);
    }

public:
    uint256 hashFork;
    D data;
};

template <int type, typename L, typename D>
class CEventPeerDelegated : public mtbase::CEvent
{
    friend class mtbase::CStream;

public:
    CEventPeerDelegated(uint64 nNonceIn, const uint256& hashAnchorIn)
      : CEvent(nNonceIn, type), hashAnchor(hashAnchorIn) {}
    virtual ~CEventPeerDelegated() {}
    virtual bool Handle(mtbase::CEventListener& listener)
    {
        try
        {
            return (dynamic_cast<L&>(listener)).HandleEvent(*this);
        }
        catch (std::bad_cast&)
        {
            return listener.HandleEvent(*this);
        }
        catch (std::exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashAnchor, opt);
        s.Serialize(data, opt);
    }

public:
    uint256 hashAnchor;
    D data;
};

class CEventPeerDelegatedBulletin
{
    friend class mtbase::CStream;

public:
    class CDelegatedBitmap
    {
    public:
        CDelegatedBitmap(const uint256& hashAnchorIn = uint64(0), uint64 bitmapIn = 0)
          : hashAnchor(hashAnchorIn), bitmap(bitmapIn)
        {
        }
        template <typename O>
        void Serialize(mtbase::CStream& s, O& opt)
        {
            s.Serialize(hashAnchor, opt);
            s.Serialize(bitmap, opt);
        }

    public:
        uint256 hashAnchor;
        uint64 bitmap;
    };

public:
    void AddBitmap(const uint256& hash, uint64 bitmap)
    {
        vBitmap.push_back(CDelegatedBitmap(hash, bitmap));
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(bmDistribute, opt);
        s.Serialize(bmPublish, opt);
        s.Serialize(vBitmap, opt);
    }

public:
    uint64 bmDistribute;
    uint64 bmPublish;
    std::vector<CDelegatedBitmap> vBitmap;
};

class CEventPeerDelegatedGetData
{
    friend class mtbase::CStream;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nInvType, opt);
        s.Serialize(destDelegate, opt);
    }

public:
    uint32 nInvType;
    CDestination destDelegate;
};

class CEventPeerDelegatedData
{
    friend class mtbase::CStream;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(destDelegate, opt);
        s.Serialize(vchData, opt);
    }

public:
    CDestination destDelegate;
    std::vector<unsigned char> vchData;
};

class CEventPeerTxData
{
    friend class mtbase::CStream;

public:
    CEventPeerTxData() {}
    CEventPeerTxData(const uint256& h, const bytes& t)
      : hash(h), txdata(t) {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hash, opt);
        s.Serialize(txdata, opt);
    }

public:
    uint256 hash;
    bytes txdata;
};

class CEventPeerBlockData
{
    friend class mtbase::CStream;

public:
    CEventPeerBlockData() {}
    CEventPeerBlockData(const uint256& h, const bytes& b)
      : hash(h), block(b) {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hash, opt);
        s.Serialize(block, opt);
    }

public:
    uint256 hash;
    bytes block;
};

class CPeerBlockData
{
    friend class mtbase::CStream;

public:
    CPeerBlockData() {}
    CPeerBlockData(const uint256& h, const uint256& p, const bytes& b)
      : hashBlock(h), hashPrev(p), btBlockData(b) {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(hashPrev, opt);
        s.Serialize(btBlockData, opt);
    }

public:
    uint256 hashBlock;
    uint256 hashPrev;
    bytes btBlockData;
};

class CBroadBlockData
{
    friend class mtbase::CStream;

public:
    CBroadBlockData() {}
    CBroadBlockData(const uint256& h, const uint256& p, const CBlock& b)
      : hashBlock(h), hashPrev(p), block(b) {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(hashPrev, opt);
        s.Serialize(block, opt);
    }

public:
    uint256 hashBlock;
    uint256 hashPrev;
    CBlock block;
};

class CBbPeerEventListener;

#define TYPE_PEEREVENT(type, body) \
    mtbase::CEventCategory<type, CBbPeerEventListener, body, bool>

#define TYPE_PEERDATAEVENT(type, body) \
    CEventPeerData<type, CBbPeerEventListener, body>

#define TYPE_PEERDELEGATEDEVENT(type, body) \
    CEventPeerDelegated<type, CBbPeerEventListener, body>

typedef TYPE_PEEREVENT(EVENT_PEER_ACTIVE, CAddress) CEventPeerActive;
typedef TYPE_PEEREVENT(EVENT_PEER_DEACTIVE, CAddress) CEventPeerDeactive;

typedef TYPE_PEERDATAEVENT(EVENT_PEER_SUBSCRIBE, std::vector<uint256>) CEventPeerSubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_UNSUBSCRIBE, std::vector<uint256>) CEventPeerUnsubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_INV, std::vector<CInv>) CEventPeerInv;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_GETDATA, std::vector<CInv>) CEventPeerGetData;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_GETBLOCKS, CBlockLocator) CEventPeerGetBlocks;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_TX, CEventPeerTxData) CEventPeerTx;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_BLOCK, CEventPeerBlockData) CEventPeerBlock;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_GETFAIL, std::vector<CInv>) CEventPeerGetFail;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_MSGRSP, CMsgRsp) CEventPeerMsgRsp;

typedef TYPE_PEERDATAEVENT(EVENT_PEER_BLOCK_SUBSCRIBE, std::vector<uint256>) CEventPeerBlockSubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_BLOCK_UNSUBSCRIBE, std::vector<uint256>) CEventPeerBlockUnsubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_BLOCK_BKS, CPeerBlockData) CEventPeerBlockBks;

typedef TYPE_PEERDATAEVENT(EVENT_PEER_CERTTX_SUBSCRIBE, std::vector<uint256>) CEventPeerCerttxSubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_CERTTX_UNSUBSCRIBE, std::vector<uint256>) CEventPeerCerttxUnsubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_CERTTX_TXS, bytes) CEventPeerCerttxTxs;

typedef TYPE_PEERDATAEVENT(EVENT_PEER_USERTX_SUBSCRIBE, std::vector<uint256>) CEventPeerUsertxSubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_USERTX_UNSUBSCRIBE, std::vector<uint256>) CEventPeerUsertxUnsubscribe;
typedef TYPE_PEERDATAEVENT(EVENT_PEER_USERTX_TXS, bytes) CEventPeerUsertxTxs;

typedef TYPE_PEERDATAEVENT(EVENT_LOCAL_BLOCK_SUBSCRIBE_FORK, std::vector<uint256>) CEventLocalBlockSubscribeFork;
typedef TYPE_PEERDATAEVENT(EVENT_LOCAL_BLOCK_BROADCAST_BKS, CBroadBlockData) CEventLocalBlockBroadcastBks;
typedef TYPE_PEERDATAEVENT(EVENT_LOCAL_CERTTX_BROADCAST_TXS, std::vector<CTransaction>) CEventLocalCerttxBroadcastTxs;
typedef TYPE_PEERDATAEVENT(EVENT_LOCAL_USERTX_SUBSCRIBE_FORK, std::vector<uint256>) CEventLocalUsertxSubscribeFork;
typedef TYPE_PEERDATAEVENT(EVENT_LOCAL_USERTX_BROADCAST_TXS, std::vector<CTransaction>) CEventLocalUsertxBroadcastTxs;

typedef TYPE_PEERDELEGATEDEVENT(EVENT_PEER_BULLETIN, CEventPeerDelegatedBulletin) CEventPeerBulletin;
typedef TYPE_PEERDELEGATEDEVENT(EVENT_PEER_GETDELEGATED, CEventPeerDelegatedGetData) CEventPeerGetDelegated;
typedef TYPE_PEERDELEGATEDEVENT(EVENT_PEER_DISTRIBUTE, CEventPeerDelegatedData) CEventPeerDistribute;
typedef TYPE_PEERDELEGATEDEVENT(EVENT_PEER_PUBLISH, CEventPeerDelegatedData) CEventPeerPublish;

class CBbPeerEventListener : virtual public mtbase::CEventListener
{
public:
    virtual ~CBbPeerEventListener() {}

    DECLARE_EVENTHANDLER(CEventPeerActive);
    DECLARE_EVENTHANDLER(CEventPeerDeactive);
    DECLARE_EVENTHANDLER(CEventPeerSubscribe);
    DECLARE_EVENTHANDLER(CEventPeerUnsubscribe);
    DECLARE_EVENTHANDLER(CEventPeerInv);
    DECLARE_EVENTHANDLER(CEventPeerGetData);
    DECLARE_EVENTHANDLER(CEventPeerGetBlocks);
    DECLARE_EVENTHANDLER(CEventPeerTx);
    DECLARE_EVENTHANDLER(CEventPeerBlock);
    DECLARE_EVENTHANDLER(CEventPeerGetFail);
    DECLARE_EVENTHANDLER(CEventPeerMsgRsp);

    DECLARE_EVENTHANDLER(CEventPeerBlockSubscribe);
    DECLARE_EVENTHANDLER(CEventPeerBlockUnsubscribe);
    DECLARE_EVENTHANDLER(CEventPeerBlockBks);

    DECLARE_EVENTHANDLER(CEventPeerCerttxSubscribe);
    DECLARE_EVENTHANDLER(CEventPeerCerttxUnsubscribe);
    DECLARE_EVENTHANDLER(CEventPeerCerttxTxs);

    DECLARE_EVENTHANDLER(CEventPeerUsertxSubscribe);
    DECLARE_EVENTHANDLER(CEventPeerUsertxUnsubscribe);
    DECLARE_EVENTHANDLER(CEventPeerUsertxTxs);

    DECLARE_EVENTHANDLER(CEventLocalBlockSubscribeFork);
    DECLARE_EVENTHANDLER(CEventLocalBlockBroadcastBks);
    DECLARE_EVENTHANDLER(CEventLocalCerttxBroadcastTxs);
    DECLARE_EVENTHANDLER(CEventLocalUsertxSubscribeFork);
    DECLARE_EVENTHANDLER(CEventLocalUsertxBroadcastTxs);

    DECLARE_EVENTHANDLER(CEventPeerBulletin);
    DECLARE_EVENTHANDLER(CEventPeerGetDelegated);
    DECLARE_EVENTHANDLER(CEventPeerDistribute);
    DECLARE_EVENTHANDLER(CEventPeerPublish);
};

} // namespace network
} // namespace metabasenet

#endif // NETWORK_PEEREVENT_H

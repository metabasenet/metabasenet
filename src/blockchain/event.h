// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_EVENT_H
#define METABASENET_EVENT_H

#include <map>
#include <set>
#include <vector>

#include "block.h"
#include "mtbase.h"
#include "peerevent.h"
#include "struct.h"
#include "transaction.h"

namespace metabasenet
{

///////////////////////////////////////////
// CBlockMakerEventListener

enum
{
    EVENT_BLOCKMAKER_BASE = network::EVENT_PEER_MAX,

    EVENT_BLOCKMAKER_UPDATE,
    EVENT_BLOCKMAKER_ENROLL,
    EVENT_BLOCKMAKER_DISTRIBUTE,
    EVENT_BLOCKMAKER_PUBLISH,
    EVENT_BLOCKMAKER_AGREE,

    EVENT_BLOCKMAKER_MAX
};

class CBlockMakerEventListener;
#define TYPE_BLOCKMAKEREVENT(type, body) \
    mtbase::CEventCategory<type, CBlockMakerEventListener, body, CNil>

typedef TYPE_BLOCKMAKEREVENT(EVENT_BLOCKMAKER_UPDATE, CBlockMakerUpdate) CEventBlockMakerUpdate;
typedef TYPE_BLOCKMAKEREVENT(EVENT_BLOCKMAKER_AGREE, CDelegateAgreement) CEventBlockMakerAgree;

class CBlockMakerEventListener : virtual public mtbase::CEventListener
{
public:
    virtual ~CBlockMakerEventListener() {}

    DECLARE_EVENTHANDLER(CEventBlockMakerUpdate);
    DECLARE_EVENTHANDLER(CEventBlockMakerAgree);
};

///////////////////////////////////////////
// CWsServiceEventListener

enum
{
    EVENT_WSSERVICE_BASE = EVENT_BLOCKMAKER_MAX,

    EVENT_WSSERVICE_PUSH_NEW_BLOCK,
    EVENT_WSSERVICE_PUSH_LOGS,
    EVENT_WSSERVICE_PUSH_NEW_PENDING_TX,
    EVENT_WSSERVICE_PUSH_SYNCING,

    EVENT_WSSERVICE_MAX
};

class CWssPushNewBlock
{
public:
    CWssPushNewBlock() {}

public:
    uint256 hashFork;
    uint256 hashBlock;
    CBlock block;
};

class CWssPushLogs
{
public:
    CWssPushLogs() {}

public:
    uint256 hashFork;
    uint256 hashBlock;
    CTransactionReceipt receipt;
};

class CWssPushNewPendingTx
{
public:
    CWssPushNewPendingTx() {}

public:
    uint256 hashFork;
    uint256 txid;
};

class CWssPushSyncing
{
public:
    CWssPushSyncing() {}

public:
    uint256 hashFork;
    bool fSyncing;
    uint64 nStartingBlockNumber;
    uint64 nCurrentBlockNumber;
    uint64 nHighestBlockNumber;
    uint64 nPulledStates;
    uint64 nKnownStates;
};

class CWsServiceEventListener;
#define TYPE_WSSERVICEEVENT(type, body) \
    mtbase::CEventCategory<type, CWsServiceEventListener, body, CNil>

typedef TYPE_WSSERVICEEVENT(EVENT_WSSERVICE_PUSH_NEW_BLOCK, CWssPushNewBlock) CEventWsServicePushNewBlock;
typedef TYPE_WSSERVICEEVENT(EVENT_WSSERVICE_PUSH_LOGS, CWssPushLogs) CEventWsServicePushLogs;
typedef TYPE_WSSERVICEEVENT(EVENT_WSSERVICE_PUSH_NEW_PENDING_TX, CWssPushNewPendingTx) CEventWsServicePushNewPendingTx;
typedef TYPE_WSSERVICEEVENT(EVENT_WSSERVICE_PUSH_SYNCING, CWssPushSyncing) CEventWsServicePushSyncing;

class CWsServiceEventListener : virtual public mtbase::CEventListener
{
public:
    virtual ~CWsServiceEventListener() {}

    DECLARE_EVENTHANDLER(CEventWsServicePushNewBlock);
    DECLARE_EVENTHANDLER(CEventWsServicePushLogs);
    DECLARE_EVENTHANDLER(CEventWsServicePushNewPendingTx);
    DECLARE_EVENTHANDLER(CEventWsServicePushSyncing);
};

} // namespace metabasenet

#endif // METABASENET_EVENT_H

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

enum
{
    EVENT_BASE = network::EVENT_PEER_MAX,
    EVENT_BLOCKMAKER_UPDATE,
    EVENT_BLOCKMAKER_ENROLL,
    EVENT_BLOCKMAKER_DISTRIBUTE,
    EVENT_BLOCKMAKER_PUBLISH,
    EVENT_BLOCKMAKER_AGREE
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

} // namespace metabasenet

#endif //METABASENET_EVENT_H

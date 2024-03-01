// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "event.h"

using namespace std;

namespace mtbase
{

///////////////////////////////
// CEventListener

bool CEventListener::DispatchEvent(CEvent* pEvent)
{
    return pEvent->Handle(*this);
}

} // namespace mtbase

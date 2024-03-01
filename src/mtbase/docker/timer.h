// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_DOCKER_TIMER_H
#define MTBASE_DOCKER_TIMER_H

#include <boost/function.hpp>
#include <boost/thread/thread_time.hpp>
#include <string>

#include "type.h"

namespace mtbase
{

typedef boost::function<void(uint64)> TimerCallback;

class CTimer
{
public:
    CTimer() {}
    CTimer(const std::string& keyIn, uint32 nTimerIdIn, boost::system_time& tExpiryAtIn,
           TimerCallback fnCallbackIn)
      : key(keyIn), nTimerId(nTimerIdIn), tExpiryAt(tExpiryAtIn), fnCallback(fnCallbackIn)
    {
    }

public:
    const std::string key;
    uint32 nTimerId;
    boost::system_time const tExpiryAt;
    TimerCallback fnCallback;
};

} // namespace mtbase

#endif //MTBASE_DOCKER_TIMER_H

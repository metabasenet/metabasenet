// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_EVENT_EVENTPROC_H
#define MTBASE_EVENT_EVENTPROC_H

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>

#include "base/base.h"
#include "event/event.h"

namespace mtbase
{

class CEventQueue
{
public:
    CEventQueue()
      : fAbort(false) {}
    ~CEventQueue()
    {
        Reset();
    }
    void AddNew(CEvent* p)
    {
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            que.push(p);
        }
        cond.notify_one();
    }
    CEvent* Fetch()
    {
        CEvent* p = nullptr;
        boost::unique_lock<boost::mutex> lock(mutex);
        while (!fAbort && que.empty())
        {
            cond.wait(lock);
        }
        if (!fAbort && !que.empty())
        {
            p = que.front();
            que.pop();
        }
        return p;
    }
    void Reset()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        while (!que.empty())
        {
            que.front()->Free();
            que.pop();
        }
        fAbort = false;
    }
    void Interrupt()
    {
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            while (!que.empty())
            {
                que.front()->Free();
                que.pop();
            }
            fAbort = true;
        }
        cond.notify_all();
    }

protected:
    boost::condition_variable cond;
    boost::mutex mutex;
    std::queue<CEvent*> que;
    bool fAbort;
};

class CEventProc : public IBase
{
public:
    CEventProc(const std::string& ownKeyIn, const uint32 nThreadCount = 1);
    void PostEvent(CEvent* pEvent);

    void AddEventThread(const uint32 nCount);

protected:
    bool HandleInvoke() override;
    void HandleHalt() override;
    void EventThreadFunc();

protected:
    std::string strThrName;
    CEventQueue queEvent;
    std::vector<CThread> vEventThread;
};

} // namespace mtbase

#endif //MTBASE_EVENT_EVENTPROC_H

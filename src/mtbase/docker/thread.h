// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_DOCKER_THREAD_H
#define MTBASE_DOCKER_THREAD_H

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <string>

namespace mtbase
{

class CDock;

class CThread
{
    friend class CDocker;

public:
    typedef boost::function<void()> ThreadFunc;
    CThread(const std::string& strNameIn, ThreadFunc fnCallbackIn)
      : strThreadName(strNameIn), pThread(nullptr), fnCallback(fnCallbackIn), fRunning(false)
    {
    }

    virtual ~CThread()
    {
        delete pThread;
    }

    bool IsRunning()
    {
        return fRunning;
    }

    void Interrupt()
    {
        if (pThread)
        {
            pThread->interrupt();
        }
    }

    void Exit()
    {
        if (pThread)
        {
            pThread->join();
            delete pThread;
            pThread = nullptr;
        }
    }

protected:
    const std::string strThreadName;
    boost::thread* pThread;
    ThreadFunc fnCallback;
    bool fRunning;
};

} // namespace mtbase

#endif //MTBASE_DOCKER_THREAD_H

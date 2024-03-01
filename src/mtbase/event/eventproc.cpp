// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "eventproc.h"

#include <boost/bind.hpp>

using namespace std;

namespace mtbase
{

///////////////////////////////
// CEventProc

CEventProc::CEventProc(const string& ownKeyIn, const uint32 nThreadCount)
  : IBase(ownKeyIn), strThrName(ownKeyIn)
{
    for (uint32 i = 0; i < nThreadCount; i++)
    {
        vEventThread.push_back(CThread(ownKeyIn + "-eventq-" + std::to_string(i), boost::bind(&CEventProc::EventThreadFunc, this)));
    }
}

void CEventProc::AddEventThread(const uint32 nCount)
{
    for (uint32 i = 0; i < nCount; i++)
    {
        vEventThread.push_back(CThread(strThrName + "-eventq-" + std::to_string(vEventThread.size()), boost::bind(&CEventProc::EventThreadFunc, this)));
    }
}

bool CEventProc::HandleInvoke()
{
    queEvent.Reset();

    for (auto& thr : vEventThread)
    {
        if (!ThreadStart(thr))
        {
            return false;
        }
    }
    return true;
}

void CEventProc::HandleHalt()
{
    queEvent.Interrupt();

    for (auto& thr : vEventThread)
    {
        ThreadExit(thr);
    }
}

void CEventProc::PostEvent(CEvent* pEvent)
{
    queEvent.AddNew(pEvent);
}

void CEventProc::EventThreadFunc()
{
    CEvent* pEvent = nullptr;
    while ((pEvent = queEvent.Fetch()) != nullptr)
    {
        if (!pEvent->Handle(*this))
        {
            queEvent.Reset();
        }
        pEvent->Free();
    }
}

} // namespace mtbase

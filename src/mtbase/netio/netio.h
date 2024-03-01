// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_NETIO_NETIO_H
#define MTBASE_NETIO_NETIO_H

#include "event/eventproc.h"
namespace mtbase
{

class IIOProc : public IBase
{
public:
    IIOProc(const std::string& ownKeyIn)
      : IBase(ownKeyIn) {}
    virtual bool DispatchEvent(CEvent* pEvent) = 0;
};

class IIOModule : public CEventProc
{
public:
    IIOModule(const std::string& ownKeyIn, const uint32 nThreadCount = 1)
      : CEventProc(ownKeyIn, nThreadCount) {}
};

} // namespace mtbase

#endif //MTBASE_NETIO_NETIO_H

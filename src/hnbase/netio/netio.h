// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XENGINE_NETIO_NETIO_H
#define XENGINE_NETIO_NETIO_H

#include "event/eventproc.h"
namespace hnbase
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
    IIOModule(const std::string& ownKeyIn)
      : CEventProc(ownKeyIn) {}
};

} // namespace hnbase

#endif //XENGINE_NETIO_NETIO_H

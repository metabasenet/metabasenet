// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HCODE_NETIO_NETIO_H
#define HCODE_NETIO_NETIO_H

#include "event/eventproc.h"
namespace hcode
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

} // namespace hcode

#endif //HCODE_NETIO_NETIO_H

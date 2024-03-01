// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "templateid.h"

namespace metabasenet
{

//////////////////////////////
// CTemplateId

CTemplateId::CTemplateId() {}

CTemplateId::CTemplateId(const uint256& data)
  : uint256(data)
{
}

CTemplateId& CTemplateId::operator=(uint64 b)
{
    *((uint256*)this) = b;
    return *this;
}

} // namespace metabasenet

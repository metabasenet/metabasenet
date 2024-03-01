// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_TEMPLATEID_H
#define COMMON_TEMPLATE_TEMPLATEID_H

#include "uint256.h"

namespace metabasenet
{

/**
 * hash of template data + template type
 */
class CTemplateId : public uint256
{
public:
    CTemplateId();
    CTemplateId(const uint256& data);

    CTemplateId& operator=(uint64 b);
};

} // namespace metabasenet

#endif //COMMON_TEMPLATE_TEMPLATEID_H

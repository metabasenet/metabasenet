// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_NETWORK_CONFIG_H
#define METABASENET_MODE_NETWORK_CONFIG_H

#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace metabasenet
{
class CNetworkConfig : virtual public CBasicConfig, virtual public CNetworkConfigOption
{
public:
    CNetworkConfig();
    virtual ~CNetworkConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    unsigned short nPort;
    unsigned int nMaxInBounds;
    unsigned int nMaxOutBounds;
};

} // namespace metabasenet

#endif // METABASENET_MODE_NETWORK_CONFIG_H

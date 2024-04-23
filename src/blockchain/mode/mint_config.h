// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_MINT_CONFIG_H
#define METABASENET_MODE_MINT_CONFIG_H

#include <string>

#include "destination.h"
#include "mode/basic_config.h"
#include "uint256.h"

namespace metabasenet
{

enum
{
    NODE_TYPE_COMMON,   // by default
    NODE_TYPE_SUPER,
    NODE_TYPE_FORK
};

class CMintConfig : virtual public CBasicConfig, virtual public CMintConfigOption
{
public:
    CMintConfig();
    virtual ~CMintConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    int nPeerType;
    std::map<uint256, std::pair<CDestination, uint32>> mapMint;
};

} // namespace metabasenet

#endif // METABASENET_MODE_MINT_CONFIG_H

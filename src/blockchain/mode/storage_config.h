// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_STORAGE_CONFIG_H
#define METABASENET_MODE_STORAGE_CONFIG_H

#include <string>

#include "mode/basic_config.h"

namespace metabasenet
{
class CStorageConfig : virtual public CBasicConfig, virtual public CStorageConfigOption
{
public:
    CStorageConfig();
    virtual ~CStorageConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;
};

} // namespace metabasenet

#endif // METABASENET_MODE_STORAGE_CONFIG_H

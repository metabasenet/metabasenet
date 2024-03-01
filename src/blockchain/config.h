// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_CONFIG_H
#define METABASENET_CONFIG_H

#include <string>
#include <type_traits>
#include <vector>

#include "mtbase.h"
#include "mode/mode.h"

namespace metabasenet
{
class CConfig
{
public:
    CConfig();
    ~CConfig();
    bool Load(int argc, char* argv[], const boost::filesystem::path& pathDefault,
              const std::string& strConfile);
    bool PostLoad();
    void ListConfig() const;
    std::string Help() const;

    inline CBasicConfig* GetConfig()
    {
        return pImpl;
    }
    inline EModeType GetModeType()
    {
        return emMode;
    }

protected:
    EModeType emMode;
    std::string subCmd;
    CBasicConfig* pImpl;
};

} // namespace metabasenet

#endif // METABASENET_CONFIG_H

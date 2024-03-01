// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mode/storage_config.h"

#include "mode/config_macro.h"

namespace metabasenet
{
namespace po = boost::program_options;

CStorageConfig::CStorageConfig()
{
    po::options_description desc("Storage");

    CStorageConfigOption::AddOptionsImpl(desc);

    AddOptions(desc);
}

CStorageConfig::~CStorageConfig() {}

bool CStorageConfig::PostLoad()
{
    if (fHelp)
    {
        return true;
    }

    if (boost::filesystem::path(strRecoveryDir) == pathRoot / "block")
    {
        printf("recoverydir must be not equal datadir/block!\n");
        return false;
    }

    return true;
}

std::string CStorageConfig::ListConfig() const
{
    std::ostringstream oss;
    oss << CStorageConfigOption::ListConfigImpl();
    return oss.str();
}

std::string CStorageConfig::Help() const
{
    return CStorageConfigOption::HelpImpl();
}

} // namespace metabasenet

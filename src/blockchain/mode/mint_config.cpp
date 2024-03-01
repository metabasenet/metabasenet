// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mode/mint_config.h"

#include "mode/config_macro.h"

namespace metabasenet
{
namespace po = boost::program_options;

CMintConfig::CMintConfig()
{
    po::options_description desc("MetabaseNetMint");

    CMintConfigOption::AddOptionsImpl(desc);

    AddOptions(desc);
}

CMintConfig::~CMintConfig() {}

bool CMintConfig::PostLoad()
{
    if (fHelp)
    {
        return true;
    }

    if (strPeerType == "super")
    {
        nPeerType = NODE_TYPE_SUPER;
    }
    else if (strPeerType == "fork")
    {
        nPeerType = NODE_TYPE_FORK;
    }
    else
    {
        nPeerType = NODE_TYPE_COMMON;
    }

    for (const string& strMint : vMint)
    {
        auto pos1 = strMint.find(":");
        if (pos1 == string::npos)
        {
            continue;
        }
        auto pos2 = strMint.find(":", pos1 + 1);
        if (pos2 == string::npos)
        {
            continue;
        }
        string strMintKey = strMint.substr(0, pos1);
        string strOwnerAddress = strMint.substr(pos1 + 1, pos2 - (pos1 + 1));
        string strRewardratio = strMint.substr(pos2 + 1);

        if (strMintKey.empty() || strOwnerAddress.empty() || strRewardratio.empty())
        {
            continue;
        }

        uint256 keyMint;
        CDestination destOwner;
        uint32 nRewardRatio;

        keyMint.SetHex(strMintKey);
        destOwner.ParseString(strOwnerAddress);
        nRewardRatio = (uint32)std::stol(strRewardratio);

        if (keyMint.IsNull() || destOwner.IsNull() || nRewardRatio > 10000)
        {
            continue;
        }

        if (mapMint.find(keyMint) != mapMint.end())
        {
            continue;
        }
        mapMint.insert(std::make_pair(keyMint, std::make_pair(destOwner, nRewardRatio)));
    }

    return true;
}

std::string CMintConfig::ListConfig() const
{
    std::ostringstream oss;
    for (auto& kv : mapMint)
    {
        oss << "mint key: " << kv.first.ToString() << ", owner address: " << kv.second.first.ToString() << ", reward ratio: " << kv.second.second << "\n";
    }
    return oss.str();
}

std::string CMintConfig::Help() const
{
    return CMintConfigOption::HelpImpl();
}

} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
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

    ExtractMintParamPair(strAddressMpvss, strKeyMpvss, destMpvss, keyMpvss);
    ExtractMintParamPair(strAddressCryptonight, strkeyCryptonight, destCryptonight, keyCryptonight);

    nDelegateRewardRatio = nRewardRatio;

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

    return true;
}

std::string CMintConfig::ListConfig() const
{
    std::ostringstream oss;
    oss << CMintConfigOption::ListConfigImpl();
    oss << "destMpvss: " << destMpvss.ToString() << "\n";
    oss << "keyMpvss: " << keyMpvss.GetHex() << "\n";
    oss << "destCryptonight: " << destCryptonight.ToString() << "\n";
    oss << "keyCryptonight: " << keyCryptonight.GetHex() << "\n";
    return oss.str();
}

std::string CMintConfig::Help() const
{
    return CMintConfigOption::HelpImpl();
}

void CMintConfig::ExtractMintParamPair(const std::string& strAddress,
                                       const std::string& strKey,
                                       CDestination& dest, uint256& privkey)
{
    CDestination address;
    if (address.ParseString(strAddress) && !address.IsNull() && strKey.size() == 64)
    {
        dest = address;
        privkey.SetHex(strKey);
    }
}

} // namespace metabasenet

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mode/rpc_config.h"

#include <boost/algorithm/algorithm.hpp>

#include "mode/config_macro.h"

namespace metabasenet
{
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using tcp = boost::asio::ip::tcp;

/////////////////////////////////////////////////////////////////////
// CRPCBasicConfig

CRPCBasicConfig::CRPCBasicConfig()
{
    po::options_description desc("RPCBasic");

    CRPCBasicConfigOption::AddOptionsImpl(desc);

    AddOptions(desc);
}

CRPCBasicConfig::~CRPCBasicConfig() {}

bool CRPCBasicConfig::PostLoad()
{
    if (fHelp)
    {
        return true;
    }

    if (nRPCPortInt <= 0 || nRPCPortInt > 0xFFFF)
    {
        nRPCPort = (fTestNet ? DEFAULT_TESTNET_RPCPORT : DEFAULT_RPCPORT);
    }
    else
    {
        nRPCPort = (unsigned short)nRPCPortInt;
    }

    if (nWsPortInt <= 0 || nWsPortInt > 0xFFFF)
    {
        nWsPort = (fTestNet ? DEFAULT_TESTNET_WSPORT : DEFAULT_WSPORT);
    }
    else
    {
        nWsPort = (unsigned short)nWsPortInt;
    }

    if (!strRPCCAFile.empty())
    {
        if (!fs::path(strRPCCAFile).is_complete())
        {
            strRPCCAFile = (pathRoot / strRPCCAFile).string();
        }
    }

    if (!strRPCCertFile.empty())
    {
        if (!fs::path(strRPCCertFile).is_complete())
        {
            strRPCCertFile = (pathRoot / strRPCCertFile).string();
        }
    }

    if (!strRPCPKFile.empty())
    {
        if (!fs::path(strRPCPKFile).is_complete())
        {
            strRPCPKFile = (pathRoot / strRPCPKFile).string();
        }
    }

    return true;
}

std::string CRPCBasicConfig::ListConfig() const
{
    std::ostringstream oss;
    oss << CRPCBasicConfigOption::ListConfigImpl();
    oss << "RPCPort: " << nRPCPort << "\n";
    oss << "WsPort: " << nWsPort << "\n";
    return oss.str();
}

std::string CRPCBasicConfig::Help() const
{
    return CRPCBasicConfigOption::HelpImpl();
}

/////////////////////////////////////////////////////////////////////
// CRPCServerConfig

CRPCServerConfig::CRPCServerConfig()
{
    po::options_description desc("RPCServer");

    CRPCServerConfigOption::AddOptionsImpl(desc);

    AddOptions(desc);
}

CRPCServerConfig::~CRPCServerConfig() {}

bool CRPCServerConfig::PostLoad()
{
    if (fHelp)
    {
        return true;
    }

    CRPCBasicConfig::PostLoad();

    if (fRPCListen || fRPCListen4 || (!fRPCListen4 && !fRPCListen6))
    {
        epRPC = tcp::endpoint(!vRPCAllowIP.empty()
                                  ? boost::asio::ip::address_v4::any()
                                  : boost::asio::ip::address_v4::loopback(),
                              nRPCPort);
    }

    if (fRPCListen || fRPCListen6)
    {
        epRPC = tcp::endpoint(!vRPCAllowIP.empty()
                                  ? boost::asio::ip::address_v6::any()
                                  : boost::asio::ip::address_v6::loopback(),
                              nRPCPort);
    }

    for (const string& strChainIdRpcPort : vChainIdRpcPort)
    {
        string strChainId;
        string strRpcPort;
        string strWsPort;

        try
        {
            std::regex pattern(R"((\d+):(\d+):(\d+))");
            std::smatch match;
            if (std::regex_match(strChainIdRpcPort, match, pattern))
            {
                strChainId = match[1].str();
                strRpcPort = match[2].str();
                strWsPort = match[3].str();
            }
            else
            {
                std::regex pattern(R"((\d+):(\d+))");
                std::smatch match;
                if (std::regex_match(strChainIdRpcPort, match, pattern))
                {
                    strChainId = match[1].str();
                    strRpcPort = match[2].str();
                }
                else
                {
                    continue;
                }
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            continue;
        }

        CChainId nCfgChainId = 0;
        uint16 nCfgRpcPort = 0;
        uint16 nCfgWsPort = 0;

        if (!strChainId.empty())
        {
            nCfgChainId = (CChainId)std::stol(strChainId);
        }
        if (!strRpcPort.empty())
        {
            nCfgRpcPort = (uint16)std::stol(strRpcPort);
        }
        if (!strWsPort.empty())
        {
            nCfgWsPort = (uint16)std::stol(strWsPort);
        }

        if (nCfgChainId != 0 && nCfgChainId != nChainId && nCfgRpcPort != 0)
        {
            bool fExist = false;
            for (const auto& kv : mapChainIdRpcPort)
            {
                const CChainId nEsChainId = kv.first;
                const uint16 nEsRpcPort = kv.second.first;
                const uint16 nEsWsPort = kv.second.second;
                if (nEsChainId == nCfgChainId || nEsRpcPort == nCfgRpcPort || nEsWsPort == nCfgRpcPort)
                {
                    fExist = true;
                    break;
                }
                if (nCfgWsPort != 0 && (nEsRpcPort == nCfgWsPort || nEsWsPort == nCfgWsPort))
                {
                    fExist = true;
                    break;
                }
            }
            if (!fExist)
            {
                mapChainIdRpcPort.insert(std::make_pair(nCfgChainId, std::make_pair(nCfgRpcPort, nCfgWsPort)));
            }
        }
    }

    return true;
}

std::string CRPCServerConfig::ListConfig() const
{
    std::ostringstream oss;
    oss << CRPCServerConfigOption::ListConfigImpl();
    oss << "epRPC: " << epRPC << "\n";
    for (const auto& kv : mapChainIdRpcPort)
    {
        oss << "chainid: " << kv.first << ", rpcport: " << kv.second.first << ", wsport: " << kv.second.second << "\n";
    }
    return CRPCBasicConfig::ListConfig() + oss.str();
}

std::string CRPCServerConfig::Help() const
{
    return CRPCBasicConfig::Help() + CRPCServerConfigOption::HelpImpl();
}

uint16 CRPCServerConfig::GetRpcPort(const CChainId nChainIdIn) const
{
    auto it = mapChainIdRpcPort.find(nChainIdIn);
    if (it != mapChainIdRpcPort.end())
    {
        return it->second.first;
    }
    return 0;
}

uint16 CRPCServerConfig::GetWsPort(const CChainId nChainIdIn) const
{
    auto it = mapChainIdRpcPort.find(nChainIdIn);
    if (it != mapChainIdRpcPort.end())
    {
        return it->second.second;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////
// CRPCClientConfig

CRPCClientConfig::CRPCClientConfig()
{
    po::options_description desc("RPCClient");

    CRPCClientConfigOption::AddOptionsImpl(desc);

    AddOptions(desc);
}

CRPCClientConfig::~CRPCClientConfig() {}

bool CRPCClientConfig::PostLoad()
{
    if (fHelp)
    {
        return true;
    }

    CRPCBasicConfig::PostLoad();

    try
    {
        boost::asio::ip::address addr(boost::asio::ip::address::from_string(strRPCConnect));
        if (fRPCListen4 || (!fRPCListen4 && !fRPCListen6))
        {
            if (addr.is_loopback())
            {
                strRPCConnect = boost::asio::ip::address_v4::loopback().to_string();
            }
        }

        if (fRPCListen6)
        {
            if (addr.is_loopback())
            {
                strRPCConnect = boost::asio::ip::address_v6::loopback().to_string();
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    if (nRPCConnectTimeout == 0)
    {
        nRPCConnectTimeout = 1;
    }

    return true;
}

std::string CRPCClientConfig::ListConfig() const
{
    std::ostringstream oss;
    oss << CRPCClientConfigOption::ListConfigImpl();
    return CRPCBasicConfig::ListConfig() + oss.str();
}

std::string CRPCClientConfig::Help() const
{
    return CRPCBasicConfig::Help() + CRPCClientConfigOption::HelpImpl();
}

} // namespace metabasenet

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
        auto pos = strChainIdRpcPort.find(":");
        if (pos != string::npos)
        {
            string strChainId = strChainIdRpcPort.substr(0, pos);
            string strRpcPort = strChainIdRpcPort.substr(pos + 1);
            uint32 nTempChainId = (uint32)std::stol(strChainId);
            uint16 nTempRpcPort = (uint16)std::stol(strRpcPort);
            if (nTempChainId != 0 && nTempRpcPort != 0)
            {
                if (nTempChainId == nChainId)
                {
                    continue;
                }
                bool fExist = false;
                for (auto& vd : vecChainIdRpcPort)
                {
                    if (nTempChainId == vd.first || nTempRpcPort == vd.second)
                    {
                        fExist = true;
                        break;
                    }
                }
                if (!fExist)
                {
                    vecChainIdRpcPort.push_back(std::make_pair(nTempChainId, nTempRpcPort));
                }
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
    for (auto& vd : vecChainIdRpcPort)
    {
        oss << "chainid: " << vd.first << ", rpcport: " << vd.second << "\n";
    }
    return CRPCBasicConfig::ListConfig() + oss.str();
}

std::string CRPCServerConfig::Help() const
{
    return CRPCBasicConfig::Help() + CRPCServerConfigOption::HelpImpl();
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

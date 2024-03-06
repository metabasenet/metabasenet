// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_RPC_CONFIG_H
#define METABASENET_MODE_RPC_CONFIG_H

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace metabasenet
{

class CRPCBasicConfig : virtual public CBasicConfig, virtual public CRPCBasicConfigOption
{
public:
    CRPCBasicConfig();
    virtual ~CRPCBasicConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    unsigned int nRPCPort;
    unsigned int nWsPort;
};

class CRPCServerConfig : virtual public CRPCBasicConfig, virtual public CRPCServerConfigOption
{
public:
    CRPCServerConfig();
    virtual ~CRPCServerConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

    uint16 GetRpcPort(const CChainId nChainIdIn) const;
    uint16 GetWsPort(const CChainId nChainIdIn) const;

public:
    boost::asio::ip::tcp::endpoint epRPC;
    std::map<CChainId, std::pair<uint16, uint16>> mapChainIdRpcPort; // key: chainid, value1: rpc port, value2: ws port
};

class CRPCClientConfig : virtual public CRPCBasicConfig, virtual public CRPCClientConfigOption
{
public:
    CRPCClientConfig();
    virtual ~CRPCClientConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;
};

} // namespace metabasenet

#endif // METABASENET_MODE_RPC_CONFIG_H

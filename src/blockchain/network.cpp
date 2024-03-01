// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "network.h"

#include <boost/any.hpp>
#include <boost/bind.hpp>

#include "param.h"
#include "version.h"

using namespace std;
using namespace mtbase;
using boost::asio::ip::tcp;

namespace metabasenet
{

//////////////////////////////
// CNetwork

CNetwork::CNetwork()
  : pCoreProtocol(nullptr)
{
}

CNetwork::~CNetwork()
{
}

bool CNetwork::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    Configure(NETWORK_NETID /*NetworkConfig()->nMagicNum*/, PROTO_VERSION, network::NODE_NETWORK | network::NODE_DELEGATED,
              FormatSubVersion(), !NetworkConfig()->vConnectTo.empty(), pCoreProtocol->GetGenesisBlockHash());

    CPeerNetConfig config;
    if (NetworkConfig()->fListen || NetworkConfig()->fListen4)
    {
        if (NetworkConfig()->strListenAddressv4.empty())
        {
            config.vecService.push_back(CPeerService(tcp::endpoint(tcp::v4(), NetworkConfig()->nPort),
                                                     NetworkConfig()->nMaxInBounds));
        }
        else
        {
            boost::system::error_code ec;
            boost::asio::ip::address addr(boost::asio::ip::address_v4::from_string(NetworkConfig()->strListenAddressv4, ec));
            if (ec)
            {
                Error("strListenAddressv4 param error, addr: %s, err: %s", NetworkConfig()->strListenAddressv4.c_str(), ec.message().c_str());
                return false;
            }
            config.vecService.push_back(CPeerService(tcp::endpoint(addr, NetworkConfig()->nPort), NetworkConfig()->nMaxInBounds));
            config.strSocketBindLocalIpV4 = NetworkConfig()->strListenAddressv4;
        }
    }
    if (NetworkConfig()->fListen || NetworkConfig()->fListen6)
    {
        if (NetworkConfig()->strListenAddressv6.empty())
        {
            config.vecService.push_back(CPeerService(tcp::endpoint(tcp::v6(), NetworkConfig()->nPort), NetworkConfig()->nMaxInBounds));
        }
        else
        {
            boost::system::error_code ec;
            boost::asio::ip::address addr(boost::asio::ip::address_v6::from_string(NetworkConfig()->strListenAddressv6, ec));
            if (ec)
            {
                Error("strListenAddressv6 param error, addr: %s, err: %s", NetworkConfig()->strListenAddressv6.c_str(), ec.message().c_str());
                return false;
            }
            config.vecService.push_back(CPeerService(tcp::endpoint(addr, NetworkConfig()->nPort), NetworkConfig()->nMaxInBounds));
            config.strSocketBindLocalIpV6 = NetworkConfig()->strListenAddressv6;
        }
    }
    config.nMaxOutBounds = NetworkConfig()->nMaxOutBounds;
    config.nPortDefault = (NetworkConfig()->fTestNet ? DEFAULT_TESTNET_P2PPORT : DEFAULT_P2PPORT);
    for (const string& conn : NetworkConfig()->vConnectTo)
    {
        config.vecNode.push_back(CNetHost(conn, config.nPortDefault, "connect",
                                          boost::any(uint64(network::NODE_NETWORK))));
    }
    if (config.vecNode.empty())
    {
        for (const string& seed : NetworkConfig()->vDNSeed)
        {
            config.vecNode.push_back(CNetHost(seed, DEFAULT_DNSEED_PORT, "dnseed",
                                              boost::any(uint64(network::NODE_NETWORK))));
        }
        for (const string& node : NetworkConfig()->vNode)
        {
            config.vecNode.push_back(CNetHost(node, config.nPortDefault, node,
                                              boost::any(uint64(network::NODE_NETWORK))));
        }
    }

    if ((NetworkConfig()->fListen || NetworkConfig()->fListen4 || NetworkConfig()->fListen6) && !NetworkConfig()->strGateWay.empty())
    {
        config.gateWayAddr.Set(NetworkConfig()->strGateWay, config.nPortDefault, NetworkConfig()->strGateWay,
                               boost::any(uint64(network::NODE_NETWORK)));
    }

    ConfigNetwork(config);

    return network::CBbPeerNet::HandleInitialize();
}

void CNetwork::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    network::CBbPeerNet::HandleDeinitialize();
}

bool CNetwork::CheckPeerVersion(uint32 nVersionIn, uint64 nServiceIn, const string& subVersionIn)
{
    (void)subVersionIn;
    if (nVersionIn < MIN_PROTO_VERSION || (nServiceIn & network::NODE_NETWORK) == 0)
    {
        return false;
    }
    return true;
}

} // namespace metabasenet

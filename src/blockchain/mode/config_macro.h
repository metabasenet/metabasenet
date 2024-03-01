// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_CONFIG_MACRO_H
#define METABASENET_MODE_CONFIG_MACRO_H

#include <boost/program_options.hpp>

// basic config
// #define MAINNET_MAGICNUM 0xea57b9c2
// #define TESTNET_MAGICNUM 0xa9d6c218

// rpc config
#define DEFAULT_RPCPORT 8812
#define DEFAULT_TESTNET_RPCPORT 8814
#define DEFAULT_RPC_MAX_CONNECTIONS 30
#define DEFAULT_RPC_CONNECT_TIMEOUT 600 //120

// network config
#define DEFAULT_P2PPORT 8811
#define DEFAULT_TESTNET_P2PPORT 8813
#define DEFAULT_DNSEED_PORT 8816
#define DEFAULT_MAX_INBOUNDS 155
#define DEFAULT_MAX_OUTBOUNDS 30
#define DEFAULT_CONNECT_TIMEOUT 5

// storage config
#define DEFAULT_DB_CONNECTION 8

// add options
template <typename T>
inline void AddOpt(boost::program_options::options_description& desc,
                   const std::string& name, T& var)
{
    desc.add_options()(name.c_str(), boost::program_options::value<T>(&var));
}
template <typename T>
inline void AddOpt(boost::program_options::options_description& desc,
                   const std::string& name, T& var, const T& def)
{
    desc.add_options()(name.c_str(), boost::program_options::value<T>(&var)->default_value(def));
}
template <typename T>
inline void AddOpt(boost::program_options::options_description& desc,
                   const std::string& name)
{
    desc.add_options()(name.c_str(), boost::program_options::value<T>());
}
template <typename T>
inline void AddOpt(boost::program_options::options_description& desc,
                   const std::string& name, const T& def)
{
    desc.add_options()(name.c_str(), boost::program_options::value<T>()->default_value(def));
}

#endif // METABASENET_MODE_CONFIG_MACRO_H

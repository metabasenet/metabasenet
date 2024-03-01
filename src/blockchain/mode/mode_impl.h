// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_MODE_MODE_IMPL_H
#define METABASENET_MODE_MODE_IMPL_H

#include <numeric>
#include <set>
#include <type_traits>

#include "mode/basic_config.h"
#include "mode/config_type.h"

namespace metabasenet
{
namespace mode_impl
{
/**
 * Combination of inheriting all need config class.
 */
template <typename... U>
class CCombinConfig : virtual public std::enable_if<std::is_base_of<CBasicConfig, U>::value, U>::type...
{
public:
    CCombinConfig() {}
    virtual ~CCombinConfig() {}

    virtual bool PostLoad()
    {
        std::vector<bool> v = { mtbase::CConfig::PostLoad(),
                                CBasicConfig::PostLoad(), U::PostLoad()... };
        return std::find(v.begin(), v.end(), false) == v.end();
    }

    virtual std::string ListConfig() const
    {
        std::vector<std::string> v = { mtbase::CConfig::ListConfig(),
                                       CBasicConfig::ListConfig(),
                                       U::ListConfig()... };
        return std::accumulate(v.begin(), v.end(), std::string());
    }

    virtual std::string Help() const
    {
        std::vector<std::string> v = { mtbase::CConfig::Help(),
                                       CBasicConfig::Help(),
                                       U::Help()... };
        return std::accumulate(v.begin(), v.end(), std::string());
    }
};

template <>
class CCombinConfig<> : virtual public CCombinConfig<CBasicConfig>
{
};

/**
 * check if type exists in t...
 */
template <EConfigType... t>
class CCheckType
{
public:
    bool Exist(EConfigType type)
    {
        std::set<EConfigType> s = { t... };
        return s.find(type) != s.end();
    }
};

} // namespace mode_impl

} // namespace metabasenet

#endif // METABASENET_MODE_MODE_IMPL_H

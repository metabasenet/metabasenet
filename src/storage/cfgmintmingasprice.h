// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_CFGMINTMINGASPRICE_H
#define STORAGE_CFGMINTMINGASPRICE_H

#include <boost/filesystem.hpp>

#include "destination.h"
#include "mtbase.h"

namespace metabasenet
{
namespace storage
{

class CCfgMintMinGasPriceDB
{
public:
    CCfgMintMinGasPriceDB();
    ~CCfgMintMinGasPriceDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool Remove();

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice);
    bool GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice);

protected:
    bool Load();
    bool Save();

protected:
    mtbase::CRWAccess rwAccess;
    boost::filesystem::path pathFile;
    std::map<uint256, uint256> mapMintMinGasPrice;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_CFGMINTMINGASPRICE_H

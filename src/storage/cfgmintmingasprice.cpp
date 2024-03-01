// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cfgmintmingasprice.h"

using namespace std;
using namespace boost::filesystem;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CCfgMintMinGasPriceDB

CCfgMintMinGasPriceDB::CCfgMintMinGasPriceDB()
{
}

CCfgMintMinGasPriceDB::~CCfgMintMinGasPriceDB()
{
}

bool CCfgMintMinGasPriceDB::Initialize(const path& pathData)
{
    path pathSave = pathData / "fdb";
    if (!exists(pathSave))
    {
        create_directories(pathSave);
    }
    if (!is_directory(pathSave))
    {
        return false;
    }

    pathFile = pathSave / "mintmingasprice.dat";
    if (exists(pathFile) && !is_regular_file(pathFile))
    {
        return false;
    }
    return Load();
}

void CCfgMintMinGasPriceDB::Deinitialize()
{
}

bool CCfgMintMinGasPriceDB::Remove()
{
    CWriteLock wlock(rwAccess);
    mapMintMinGasPrice.clear();
    if (is_regular_file(pathFile))
    {
        return remove(pathFile);
    }
    return true;
}

bool CCfgMintMinGasPriceDB::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    CWriteLock wlock(rwAccess);
    mapMintMinGasPrice[hashFork] = nMinGasPrice;
    Save();
    return true;
}

bool CCfgMintMinGasPriceDB::GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice)
{
    CReadLock rlock(rwAccess);
    auto it = mapMintMinGasPrice.find(hashFork);
    if (it == mapMintMinGasPrice.end())
    {
        return false;
    }
    nMinGasPrice = it->second;
    return true;
}

//------------------------------------------
bool CCfgMintMinGasPriceDB::Load()
{
    mapMintMinGasPrice.clear();
    if (!is_regular_file(pathFile))
    {
        return true;
    }
    try
    {
        CFileStream fs(pathFile.string().c_str());
        fs >> mapMintMinGasPrice;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CCfgMintMinGasPriceDB::Save()
{
    FILE* fp = fopen(pathFile.string().c_str(), "w");
    if (fp == nullptr)
    {
        return false;
    }
    fclose(fp);

    if (!is_regular_file(pathFile))
    {
        return false;
    }

    try
    {
        CFileStream fs(pathFile.string().c_str());
        fs << mapMintMinGasPrice;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

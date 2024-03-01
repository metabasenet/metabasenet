// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txpooldata.h"

using namespace std;
using namespace boost::filesystem;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CTxPoolData

CTxPoolData::CTxPoolData()
{
}

CTxPoolData::~CTxPoolData()
{
}

bool CTxPoolData::Initialize(const path& pathData)
{
    path pathTxPool = pathData / "fdb";

    if (!exists(pathTxPool))
    {
        create_directories(pathTxPool);
    }

    if (!is_directory(pathTxPool))
    {
        return false;
    }

    pathTxPoolFile = pathTxPool / "txpool.dat";

    if (exists(pathTxPoolFile) && !is_regular_file(pathTxPoolFile))
    {
        return false;
    }

    return true;
}

bool CTxPoolData::Remove()
{
    if (is_regular_file(pathTxPoolFile))
    {
        return remove(pathTxPoolFile);
    }
    return true;
}

bool CTxPoolData::Save(const std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx)
{
    FILE* fp = fopen(pathTxPoolFile.string().c_str(), "w");
    if (fp == nullptr)
    {
        return false;
    }
    fclose(fp);

    if (!is_regular_file(pathTxPoolFile))
    {
        return false;
    }

    try
    {
        CFileStream fs(pathTxPoolFile.string().c_str());
        fs << mapTx;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    return true;
}

bool CTxPoolData::Load(std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx)
{
    mapTx.clear();

    if (!is_regular_file(pathTxPoolFile))
    {
        return true;
    }

    try
    {
        CFileStream fs(pathTxPoolFile.string().c_str());
        fs >> mapTx;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    remove(pathTxPoolFile);

    return true;
}

bool CTxPoolData::LoadCheck(std::map<uint256, std::vector<std::pair<uint256, CTransaction>>>& mapTx)
{
    mapTx.clear();

    if (!is_regular_file(pathTxPoolFile))
    {
        return true;
    }

    try
    {
        CFileStream fs(pathTxPoolFile.string().c_str());
        fs >> mapTx;
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

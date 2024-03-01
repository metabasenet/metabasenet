// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressblacklistdb.h"

using namespace std;
using namespace boost::filesystem;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CAddressBlacklistDB

CAddressBlacklistDB::CAddressBlacklistDB()
{
}

CAddressBlacklistDB::~CAddressBlacklistDB()
{
}

bool CAddressBlacklistDB::Initialize(const path& pathData)
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

    pathFile = pathSave / "addressblacklist.dat";
    if (exists(pathFile) && !is_regular_file(pathFile))
    {
        return false;
    }
    return Load();
}

void CAddressBlacklistDB::Deinitialize()
{
}

bool CAddressBlacklistDB::Remove()
{
    CWriteLock wlock(rwAccess);
    setBlacklistAddress.clear();
    if (is_regular_file(pathFile))
    {
        return remove(pathFile);
    }
    return true;
}

bool CAddressBlacklistDB::AddAddress(const CDestination& dest)
{
    CWriteLock wlock(rwAccess);
    if (setBlacklistAddress.find(dest) == setBlacklistAddress.end())
    {
        setBlacklistAddress.insert(dest);
        Save();
    }
    return true;
}

void CAddressBlacklistDB::RemoveAddress(const CDestination& dest)
{
    CWriteLock wlock(rwAccess);
    if (setBlacklistAddress.find(dest) != setBlacklistAddress.end())
    {
        setBlacklistAddress.erase(dest);
        Save();
    }
}

bool CAddressBlacklistDB::IsExist(const CDestination& dest)
{
    CReadLock rlock(rwAccess);
    return (setBlacklistAddress.find(dest) != setBlacklistAddress.end());
}

void CAddressBlacklistDB::ListAddress(set<CDestination>& setAddressOut)
{
    CReadLock rlock(rwAccess);
    setAddressOut = setBlacklistAddress;
}

//------------------------------------------
bool CAddressBlacklistDB::Load()
{
    setBlacklistAddress.clear();
    if (!is_regular_file(pathFile))
    {
        return true;
    }
    try
    {
        CFileStream fs(pathFile.string().c_str());
        fs >> setBlacklistAddress;
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CAddressBlacklistDB::Save()
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
        fs << setBlacklistAddress;
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

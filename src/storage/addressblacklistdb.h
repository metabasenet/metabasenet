// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSBLACKLISTDB_H
#define STORAGE_ADDRESSBLACKLISTDB_H

#include <boost/filesystem.hpp>

#include "destination.h"
#include "mtbase.h"

namespace metabasenet
{
namespace storage
{

class CAddressBlacklistDB
{
public:
    CAddressBlacklistDB();
    ~CAddressBlacklistDB();

    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool Remove();
    bool AddAddress(const CDestination& dest);
    void RemoveAddress(const CDestination& dest);
    bool IsExist(const CDestination& dest);
    void ListAddress(std::set<CDestination>& setAddressOut);

protected:
    bool Load();
    bool Save();

protected:
    mtbase::CRWAccess rwAccess;
    boost::filesystem::path pathFile;
    std::set<CDestination> setBlacklistAddress;
};

} // namespace storage
} // namespace metabasenet

#endif // STORAGE_ADDRESSBLACKLISTDB_H

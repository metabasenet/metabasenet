// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DELEGATEVOTESAVE_H
#define STORAGE_DELEGATEVOTESAVE_H

#include <boost/filesystem.hpp>

#include "delegate.h"
#include "mtbase.h"

namespace metabasenet
{
namespace storage
{

class CDelegateVoteSave
{
public:
    CDelegateVoteSave();
    ~CDelegateVoteSave();
    bool Initialize(const boost::filesystem::path& pathData);
    bool Remove();
    bool Save(const delegate::CDelegate& delegate);
    bool Load(delegate::CDelegate& delegate);

protected:
    boost::filesystem::path pathDelegateVoteFile;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_DELEGATEVOTESAVE_H

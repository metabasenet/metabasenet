// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_WALLETDB_H
#define STORAGE_WALLETDB_H

#include <boost/function.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <hnbase.h>

#include "destination.h"
#include "key.h"

namespace metabasenet
{
namespace storage
{

class CWalletDBAddrWalker
{
public:
    virtual bool WalkPubkey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher) = 0;
    virtual bool WalkTemplate(const CTemplateId& tid, const std::vector<unsigned char>& vchData) = 0;
};

class CWalletAddrDB : public hnbase::CKVDB
{
public:
    CWalletAddrDB() {}
    bool Initialize(const boost::filesystem::path& pathWallet);
    void Deinitialize();
    bool UpdateKey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher);
    bool RemoveKey(const crypto::CPubKey& pubkey);
    bool UpdateTemplate(const CTemplateId& tid, const std::vector<unsigned char>& vchData);
    bool EraseAddress(const CDestination& dest);
    bool WalkThroughAddress(CWalletDBAddrWalker& walker);

protected:
    bool AddressDBWalker(hnbase::CBufStream& ssKey, hnbase::CBufStream& ssValue, CWalletDBAddrWalker& walker);
};

class CWalletDB
{
public:
    CWalletDB();
    ~CWalletDB();
    bool Initialize(const boost::filesystem::path& pathWallet);
    void Deinitialize();
    bool UpdateKey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher);
    bool RemoveKey(const crypto::CPubKey& pubkey);
    bool UpdateTemplate(const CTemplateId& tid, const std::vector<unsigned char>& vchData);
    bool RemoveTemplate(const CTemplateId& tid);
    bool WalkThroughAddress(CWalletDBAddrWalker& walker);

protected:
    CWalletAddrDB dbAddr;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_WALLETDB_H

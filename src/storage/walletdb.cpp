// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletdb.h"

#include <algorithm>
#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CWalletAddrDB

bool CWalletAddrDB::Initialize(const boost::filesystem::path& pathWallet)
{
    CLevelDBArguments args;
    args.path = (pathWallet / "addr").string();
    args.syncwrite = true;
    args.files = 8;
    args.cache = 1 << 20;

    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!Open(engine))
    {
        delete engine;
        return false;
    }

    return true;
}

void CWalletAddrDB::Deinitialize()
{
    Close();
}

bool CWalletAddrDB::UpdateKey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher)
{
    vector<unsigned char> vch;
    vch.resize(4 + 48 + 8);
    memcpy(&vch[0], &version, 4);
    memcpy(&vch[4], cipher.encrypted, 48);
    memcpy(&vch[52], &cipher.nonce, 8);

    return Write(CDestination(pubkey), vch);
}

bool CWalletAddrDB::RemoveKey(const crypto::CPubKey& pubkey)
{
    return Erase(CDestination(pubkey));
}

bool CWalletAddrDB::UpdateTemplate(const CTemplateId& tid, const vector<unsigned char>& vchData)
{
    return Write(CDestination(tid), vchData);
}

bool CWalletAddrDB::EraseAddress(const CDestination& dest)
{
    return Erase(dest);
}

bool CWalletAddrDB::WalkThroughAddress(CWalletDBAddrWalker& walker)
{
    return WalkThrough(boost::bind(&CWalletAddrDB::AddressDBWalker, this, _1, _2, boost::ref(walker)));
}

bool CWalletAddrDB::AddressDBWalker(CBufStream& ssKey, CBufStream& ssValue, CWalletDBAddrWalker& walker)
{
    CDestination dest;
    vector<unsigned char> vch;
    ssKey >> dest;
    ssValue >> vch;

    if (dest.IsTemplate())
    {
        return walker.WalkTemplate(dest.GetTemplateId(), vch);
    }

    crypto::CPubKey pubkey;
    if (dest.GetPubKey(pubkey) && vch.size() == 4 + 48 + 8)
    {
        int version;
        crypto::CCryptoCipher cipher;

        memcpy(&version, &vch[0], 4);
        memcpy(cipher.encrypted, &vch[4], 48);
        memcpy(&cipher.nonce, &vch[52], 8);

        return walker.WalkPubkey(pubkey, version, cipher);
    }

    return false;
}

//////////////////////////////
// CWalletDB

CWalletDB::CWalletDB()
{
}

CWalletDB::~CWalletDB()
{
    Deinitialize();
}

bool CWalletDB::Initialize(const boost::filesystem::path& pathWallet)
{
    if (!boost::filesystem::exists(pathWallet))
    {
        boost::filesystem::create_directories(pathWallet);
    }

    if (!boost::filesystem::is_directory(pathWallet))
    {
        return false;
    }

    if (!dbAddr.Initialize(pathWallet))
    {
        return false;
    }

    return true;
}

void CWalletDB::Deinitialize()
{
    dbAddr.Deinitialize();
}

bool CWalletDB::UpdateKey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher)
{
    return dbAddr.UpdateKey(pubkey, version, cipher);
}

bool CWalletDB::RemoveKey(const crypto::CPubKey& pubkey)
{
    return dbAddr.RemoveKey(pubkey);
}

bool CWalletDB::UpdateTemplate(const CTemplateId& tid, const vector<unsigned char>& vchData)
{
    return dbAddr.UpdateTemplate(tid, vchData);
}

bool CWalletDB::RemoveTemplate(const CTemplateId& tid)
{
    return dbAddr.EraseAddress(CDestination(tid));
}

bool CWalletDB::WalkThroughAddress(CWalletDBAddrWalker& walker)
{
    return dbAddr.WalkThroughAddress(walker);
}

} // namespace storage
} // namespace metabasenet

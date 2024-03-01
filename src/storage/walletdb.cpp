// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletdb.h"

#include <algorithm>
#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CWalletAddrDB

CWalletDB::~CWalletDB()
{
    Deinitialize();
}

bool CWalletDB::Initialize(const boost::filesystem::path& pathWallet)
{
    CLevelDBArguments args;
    args.path = pathWallet.string();
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

void CWalletDB::Deinitialize()
{
    Close();
}

bool CWalletDB::UpdateKey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher)
{
    uint8 nDestType = CDestination::PREFIX_PUBKEY;
    bytes btEncrypted;
    btEncrypted.assign(&(cipher.encrypted[0]), &(cipher.encrypted[0]) + 48);

    CBufStream ss;
    ss << nDestType << version << btEncrypted << cipher.nonce << pubkey;
    bytes btData;
    ss.GetData(btData);

    return Write(CDestination(pubkey), btData);
}

bool CWalletDB::RemoveKey(const CDestination& dest)
{
    return Erase(dest);
}

bool CWalletDB::UpdateTemplate(const CDestination& dest, const vector<unsigned char>& vchData)
{
    uint8 nDestType = CDestination::PREFIX_TEMPLATE;

    CBufStream ss;
    ss << nDestType << vchData;
    bytes btData;
    ss.GetData(btData);

    return Write(dest, btData);
}

bool CWalletDB::RemoveTemplate(const CDestination& dest)
{
    return Erase(dest);
}

bool CWalletDB::WalkThroughAddress(CWalletDBAddrWalker& walker)
{
    return WalkThrough(boost::bind(&CWalletDB::AddressDBWalker, this, _1, _2, boost::ref(walker)));
}

bool CWalletDB::AddressDBWalker(CBufStream& ssKey, CBufStream& ssValue, CWalletDBAddrWalker& walker)
{
    try
    {
        CDestination dest;
        bytes vch;
        ssKey >> dest;
        ssValue >> vch;

        CBufStream ss(vch);
        uint8 nDestType;
        ss >> nDestType;

        if (nDestType == CDestination::PREFIX_TEMPLATE)
        {
            bytes btTemplateData;
            ss >> btTemplateData;
            return walker.WalkTemplate(dest, btTemplateData);
        }

        int version;
        bytes btEncrypted;
        crypto::CCryptoCipher cipher;
        crypto::CPubKey pubkey;

        ss >> version >> btEncrypted >> cipher.nonce >> pubkey;
        if (btEncrypted.size() != 48)
        {
            return false;
        }
        memcpy(&(cipher.encrypted[0]), btEncrypted.data(), 48);
        return walker.WalkPubkey(pubkey, version, cipher);
    }
    catch (exception& e)
    {
        return false;
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

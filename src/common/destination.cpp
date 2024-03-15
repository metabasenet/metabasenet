// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "destination.h"

#include "base32.h"
//#include "block.h"
#include "key.h"
#include "template/mint.h"
#include "template/template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

//////////////////////////////
// CDestination

CDestination::CDestination()
{
    SetNull();
}

CDestination::CDestination(const uint160& data)
  : uint160(data)
{
}

CDestination::CDestination(const uint256& hash)
{
    SetHash(hash);
}

CDestination::CDestination(const uint8* pData, std::size_t nSize)
  : uint160(pData, nSize)
{
}

CDestination::CDestination(const std::string& str)
{
    ParseString(str);
}

CDestination::CDestination(const CPubKey& pubkey)
{
    SetPubKey(pubkey);
}

CDestination::CDestination(const CTemplateId& tid)
{
    SetTemplateId(tid);
}

CDestination::CDestination(const CContractId& cid)
{
    SetContractId(cid);
}

void CDestination::SetHash(const uint256& hashData)
{
    memcpy(begin(), hashData.begin() + 12, size());
}

uint256 CDestination::ToHash() const
{
    uint256 n;
    memcpy(n.begin() + 12, begin(), size());
    return n;
}

CDestination& CDestination::SetPubKey(const CPubKey& pubkey)
{
    *this = CryptoGetPubkeyAddress(pubkey);
    return *this;
}

CDestination& CDestination::SetTemplateId(const CTemplateId& tid)
{
    SetHash(tid);
    return *this;
}

// contract id
CDestination& CDestination::SetContractId(const CContractId& wid)
{
    SetHash(wid);
    return *this;
}

// CDestination& CDestination::SetContractId(const CDestination& from, const uint64 nTxNonce)
// {
//     mtbase::CBufStream ss;
//     ss << from << nTxNonce;
//     uint256 hash = CryptoHash(ss.GetData(), ss.GetSize());
//     SetHash(hash);
//     return *this;
// }

bool CDestination::ParseString(const string& str)
{
    SetHex(str);
    return true;
}

string CDestination::ToString() const
{
    return GetHex();
}

} // namespace metabasenet

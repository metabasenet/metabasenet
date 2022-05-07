// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "destination.h"

#include "base32.h"
#include "block.h"
#include "key.h"
#include "template/mint.h"
#include "template/template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hcode;
using namespace metabasenet::crypto;

//////////////////////////////
// CDestination

CDestination::CDestination()
{
    SetNull();
}

CDestination::CDestination(const std::string& strAddress)
{
    ParseString(strAddress);
}

CDestination::CDestination(const uint8 nPrefix, const uint256& hashData)
  : prefix(nPrefix), data(hashData)
{
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

void CDestination::SetNull()
{
    prefix = PREFIX_NULL;
    data = 0;
}

bool CDestination::IsNull() const
{
    return (prefix == PREFIX_NULL);
}

bool CDestination::IsPubKey() const
{
    return (prefix == PREFIX_PUBKEY);
}

bool CDestination::SetPubKey(const std::string& str)
{
    if (ParseStringHex(str))
    {
        if (IsNull() || !IsPubKey())
        {
            return false;
        }
    }
    else
    {
        if (data.SetHex(str) != str.size())
        {
            return false;
        }
        prefix = PREFIX_PUBKEY;
    }

    return true;
}

CDestination& CDestination::SetPubKey(const CPubKey& pubkey)
{
    prefix = PREFIX_PUBKEY;
    data = pubkey;
    return *this;
}

bool CDestination::GetPubKey(CPubKey& pubkey) const
{
    if (prefix == PREFIX_PUBKEY)
    {
        pubkey = CPubKey(data);
        return true;
    }
    return false;
}

const CPubKey CDestination::GetPubKey() const
{
    return (prefix == PREFIX_PUBKEY) ? CPubKey(data) : CPubKey(uint64(0));
}

bool CDestination::IsTemplate() const
{
    return (prefix == PREFIX_TEMPLATE);
}

bool CDestination::SetTemplateId(const std::string& str)
{
    if (ParseStringHex(str))
    {
        if (IsNull() || !IsTemplate())
        {
            return false;
        }
    }
    else
    {
        if (data.SetHex(str) != str.size())
        {
            return false;
        }
        prefix = PREFIX_TEMPLATE;
    }

    return true;
}

CDestination& CDestination::SetTemplateId(const CTemplateId& tid)
{
    prefix = PREFIX_TEMPLATE;
    data = tid;
    return *this;
}

bool CDestination::GetTemplateId(CTemplateId& tid) const
{
    if (prefix == PREFIX_TEMPLATE)
    {
        tid = CTemplateId(data);
        return true;
    }
    return false;
}

const CTemplateId CDestination::GetTemplateId() const
{
    return (prefix == PREFIX_TEMPLATE) ? CTemplateId(data) : CTemplateId(uint64(0));
}

// wasm id
bool CDestination::IsContract() const
{
    return (prefix == PREFIX_CONTRACT);
}

bool CDestination::IsNullContract() const
{
    return (prefix == PREFIX_CONTRACT && data == 0);
}

CDestination& CDestination::SetContractId(const uint256& wid)
{
    prefix = PREFIX_CONTRACT;
    data = wid;
    return *this;
}

CDestination& CDestination::SetContractId(const uint256& from, const uint64 nTxNonce)
{
    prefix = PREFIX_CONTRACT;

    hcode::CBufStream ss;
    ss << from << nTxNonce;
    data = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());
    return *this;
}

bool CDestination::GetContractId(uint256& wid) const
{
    if (prefix == PREFIX_CONTRACT)
    {
        wid = data;
        return true;
    }
    return false;
}

bool CDestination::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo, const bytes& btTemplateData,
                                     const std::vector<uint8>& vchSig, const int32 nForkHeight) const
{
    if (IsPubKey())
    {
        return GetPubKey().Verify(hash, vchSig);
    }
    if (IsTemplate())
    {
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(GetTemplateId().GetType(), btTemplateData);
        if (!ptr)
        {
            return false;
        }
        if (ptr->GetTemplateId() != GetTemplateId())
        {
            return false;
        }
        return ptr->VerifyTxSignature(hash, nTxType, destTo, vchSig, nForkHeight);
    }
    return false;
}

bool CDestination::VerifyBlockSignature(const uint256& hash, const bytes& btTemplateData, const bytes& vchSig) const
{
    if (IsPubKey())
    {
        return GetPubKey().Verify(hash, vchSig);
    }
    if (IsTemplate())
    {
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(GetTemplateId().GetType(), btTemplateData);
        if (!ptr)
        {
            return false;
        }
        if (ptr->GetTemplateId() != GetTemplateId())
        {
            return false;
        }
        CTemplateMintPtr ptrMint = boost::dynamic_pointer_cast<CTemplateMint>(ptr);
        return ptrMint->VerifyBlockSignature(hash, vchSig);
    }
    return false;
}

bool CDestination::ParseString(const std::string& strAddress)
{
    if (strAddress.size() != ADDRESS_LEN || strAddress[0] < '0' || strAddress[0] >= '0' + PREFIX_MAX)
    {
        return false;
    }
    if (!metabasenet::crypto::Base32Decode(strAddress.substr(1), data.begin()))
    {
        return false;
    }
    prefix = strAddress[0] - '0';
    return true;
}

string CDestination::ToString() const
{
    string strDataBase32;
    metabasenet::crypto::Base32Encode(data.begin(), strDataBase32);
    return (string(1, '0' + prefix) + strDataBase32);
}

bool CDestination::ParseStringHex(const string& str)
{
    if (str.size() != DESTINATION_SIZE * 2 - 1 || str[0] < '0' || str[0] >= '0' + PREFIX_MAX)
    {
        return false;
    }
    prefix = str[0] - '0';
    return ParseHexString(&str[1], data.begin(), uint256::size()) == uint256::size();
}

string CDestination::ToStringHex() const
{
    return (string(1, '0' + prefix) + ToHexString(data.begin(), data.size()));
}

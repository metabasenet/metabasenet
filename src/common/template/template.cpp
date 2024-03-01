// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "template.h"

#include "json/json_spirit_reader_template.h"
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include "activatecode.h"
#include "delegate.h"
#include "fork.h"
#include "pledge.h"
#include "proof.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "templateid.h"
#include "transaction.h"
#include "vote.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;
using namespace metabasenet::rpc;

namespace metabasenet
{

//////////////////////////////
// CTypeInfo

struct CTypeInfo
{
    uint8 nType;
    CTemplate* ptr;
    std::string strName;
};

using CTypeInfoSet = boost::multi_index_container<
    CTypeInfo,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<CTypeInfo, uint8, &CTypeInfo::nType>>,
        boost::multi_index::ordered_unique<boost::multi_index::member<CTypeInfo, std::string, &CTypeInfo::strName>>>>;

static const CTypeInfoSet setTypeInfo = {
    { TEMPLATE_FORK, new CTemplateFork, "fork" },
    { TEMPLATE_PROOF, new CTemplateProof, "mint" },
    { TEMPLATE_DELEGATE, new CTemplateDelegate, "delegate" },
    { TEMPLATE_VOTE, new CTemplateVote, "vote" },
    { TEMPLATE_PLEDGE, new CTemplatePledge, "pledge" },
    { TEMPLATE_ACTIVATECODE, new CTemplateActivateCode, "activatecode" },
};

static const CTypeInfo* GetTypeInfoByType(uint8 nTypeIn)
{
    const auto& idxType = setTypeInfo.get<0>();
    auto it = idxType.find(nTypeIn);
    return (it == idxType.end()) ? nullptr : &(*it);
}

const CTypeInfo* GetTypeInfoByName(std::string strNameIn)
{
    const auto& idxName = setTypeInfo.get<1>();
    auto it = idxName.find(strNameIn);
    return (it == idxName.end()) ? nullptr : &(*it);
}

//////////////////////////////
// CTemplate

const CTemplatePtr CTemplate::CreateTemplatePtr(CTemplate* ptr)
{
    if (ptr)
    {
        if (!ptr->ValidateParam())
        {
            delete ptr;
            return nullptr;
        }
        ptr->BuildTemplateData();
        ptr->BuildTemplateId();
    }
    return CTemplatePtr(ptr);
}

// const CTemplatePtr CTemplate::CreateTemplatePtr(const CTemplateId& nIdIn, const vector<uint8>& vchDataIn)
// {
//     return CreateTemplatePtr(nIdIn.GetType(), vchDataIn);
// }

const CTemplatePtr CTemplate::CreateTemplatePtr(uint8 nTypeIn, const vector<uint8>& vchDataIn)
{
    const CTypeInfo* pTypeInfo = GetTypeInfoByType(nTypeIn);
    if (!pTypeInfo)
    {
        return nullptr;
    }

    CTemplate* ptr = pTypeInfo->ptr->clone();
    if (ptr)
    {
        if (!ptr->SetTemplateData(vchDataIn) || !ptr->ValidateParam())
        {
            delete ptr;
            return nullptr;
        }
        ptr->BuildTemplateData();
        ptr->BuildTemplateId();
    }
    return CTemplatePtr(ptr);
}

const CTemplatePtr CTemplate::CreateTemplatePtr(const CTemplateRequest& obj)
{
    const CTypeInfo* pTypeInfo = GetTypeInfoByName(obj.strType);
    if (!pTypeInfo)
    {
        return nullptr;
    }

    CTemplate* ptr = pTypeInfo->ptr->clone();
    if (ptr)
    {
        if (!ptr->SetTemplateData(obj) || !ptr->ValidateParam())
        {
            delete ptr;
            return nullptr;
        }
        ptr->BuildTemplateData();
        ptr->BuildTemplateId();
    }
    return CTemplatePtr(ptr);
}

const CTemplatePtr CTemplate::Import(const vector<uint8>& vchTemplateIn)
{
    if (vchTemplateIn.size() < 1)
    {
        return nullptr;
    }
    uint8 nTemplateTypeIn = vchTemplateIn[0];
    vector<uint8> vchDataIn(vchTemplateIn.begin() + 1, vchTemplateIn.end());

    return CreateTemplatePtr(nTemplateTypeIn, vchDataIn);
}

bool CTemplate::IsValidType(const uint8 nTypeIn)
{
    return (nTypeIn > TEMPLATE_MIN && nTypeIn < TEMPLATE_MAX);
}

string CTemplate::GetTypeName(uint8 nTypeIn)
{
    const CTypeInfo* pTypeInfo = GetTypeInfoByType(nTypeIn);
    return pTypeInfo ? pTypeInfo->strName : "";
}

bool CTemplate::IsTxSpendable(const CDestination& dest)
{
    // if (dest.IsPubKey())
    // {
    //     return true;
    // }
    // else if (dest.IsTemplate())
    // {
    //     uint16 nType = dest.GetTemplateType();
    //     const CTypeInfo* pTypeInfo = GetTypeInfoByType(nType);
    //     if (pTypeInfo)
    //     {
    //         return (dynamic_cast<CSpendableTemplate*>(pTypeInfo->ptr) != nullptr);
    //     }
    // }
    // return false;
    return true;
}

const uint8& CTemplate::GetTemplateType() const
{
    return nType;
}

const CTemplateId& CTemplate::GetTemplateId() const
{
    return nId;
}

std::string CTemplate::GetName() const
{
    return GetTypeName(nType);
}

vector<uint8> CTemplate::Export() const
{
    vector<uint8> vchTemplate;
    vchTemplate.reserve(1 + vchData.size());
    vchTemplate.push_back(nType);
    vchTemplate.insert(vchTemplate.end(), vchData.begin(), vchData.end());
    return vchTemplate;
}

CTemplate::CTemplate(const uint8 nTypeIn)
  : nType(nTypeIn)
{
}

bool CTemplate::VerifyTemplateData(const vector<uint8>& vchSig) const
{
    return (vchSig.size() >= vchData.size() && memcmp(&vchData[0], &vchSig[0], vchData.size()) == 0);
}

void CTemplate::BuildTemplateId()
{
    bytes btData = Export();
    nId = CTemplateId(crypto::CryptoKeccakHash(&btData[0], btData.size()));
}

} // namespace metabasenet

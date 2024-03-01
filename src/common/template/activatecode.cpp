// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activatecode.h"

//#include "block.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

//////////////////////////////
// CTemplateActivateCode

CTemplateActivateCode::CTemplateActivateCode(const CDestination& destGrantIn)
  : CTemplate(TEMPLATE_ACTIVATECODE), destGrant(destGrantIn)
{
}

CTemplateActivateCode* CTemplateActivateCode::clone() const
{
    return new CTemplateActivateCode(*this);
}

void CTemplateActivateCode::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.activatecode.strGrant = destGrant.ToString();
}

bool CTemplateActivateCode::ValidateParam() const
{
    if (destGrant.IsNull())
    {
        return false;
    }
    return true;
}

bool CTemplateActivateCode::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destGrant;
    // }
    // catch (const std::exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }
    if (vchDataIn.size() != destGrant.size())
    {
        return false;
    }
    destGrant.SetBytes(vchDataIn.data(), destGrant.size());
    return true;
}

bool CTemplateActivateCode::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_ACTIVATECODE))
    {
        return false;
    }

    destGrant.ParseString(obj.activatecode.strGrant);
    if (destGrant.IsNull())
    {
        return false;
    }
    return true;
}

void CTemplateActivateCode::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destGrant.size());
    vchData.insert(vchData.end(), (char*)(destGrant.begin()), (char*)(destGrant.end()));

    // vchData.clear();
    // CBufStream os;
    // os << destGrant;
    // os.GetData(vchData);
}

bool CTemplateActivateCode::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destGrant;
    return true;
}

} // namespace metabasenet

// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activatecode.h"

#include "block.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hcode;
using namespace metabasenet::crypto;

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

void CTemplateActivateCode::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.activatecode.strGrant = destGrant.ToString();
}

bool CTemplateActivateCode::ValidateParam() const
{
    if (!IsTxSpendable(destGrant))
    {
        return false;
    }
    return true;
}

bool CTemplateActivateCode::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> destGrant;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateActivateCode::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_ACTIVATECODE))
    {
        return false;
    }

    if (!destGrant.ParseString(obj.activatecode.strGrant))
    {
        return false;
    }
    if (!destGrant.IsPubKey())
    {
        return false;
    }
    return true;
}

void CTemplateActivateCode::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << destGrant;
    os.GetData(vchData);
}

bool CTemplateActivateCode::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    destSign = destGrant;
    return true;
}

bool CTemplateActivateCode::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                              const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    return destGrant.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
}

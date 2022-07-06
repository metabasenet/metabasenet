// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "redeem.h"

#include "block.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hcode;
using namespace metabasenet::crypto;

//////////////////////////////
// CTemplateRedeem

CTemplateRedeem::CTemplateRedeem(const CDestination& destOwnerIn)
  : CTemplate(TEMPLATE_REDEEM), destOwner(destOwnerIn)
{
}

CTemplateRedeem* CTemplateRedeem::clone() const
{
    return new CTemplateRedeem(*this);
}

void CTemplateRedeem::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.redeem.strOwner = destOwner.ToString();
}

bool CTemplateRedeem::ValidateParam() const
{
    if (!IsTxSpendable(destOwner))
    {
        return false;
    }
    return true;
}

bool CTemplateRedeem::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> destOwner;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateRedeem::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_REDEEM))
    {
        return false;
    }

    if (!destOwner.ParseString(obj.redeem.strOwner))
    {
        return false;
    }
    if (!destOwner.IsPubKey())
    {
        return false;
    }

    return true;
}

void CTemplateRedeem::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << destOwner;
    os.GetData(vchData);
}

bool CTemplateRedeem::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    destSign = destOwner;
    return true;
}

bool CTemplateRedeem::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                        const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    return destOwner.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
}

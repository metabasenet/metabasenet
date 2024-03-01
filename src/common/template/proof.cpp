// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proof.h"

#include "key.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

//////////////////////////////
// CTemplateProof

CTemplateProof::CTemplateProof(const CDestination& destMintIn, const CDestination& destSpendIn)
  : CTemplate(TEMPLATE_PROOF), destMint(destMintIn), destSpend(destSpendIn)
{
}

CTemplateProof* CTemplateProof::clone() const
{
    return new CTemplateProof(*this);
}

void CTemplateProof::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.mint.strMint = destMint.ToString();
    obj.mint.strSpent = destSpend.ToString();
}

bool CTemplateProof::ValidateParam() const
{
    if (destMint.IsNull() /*|| !destMint.IsPubKey()*/)
    {
        return false;
    }
    if (destSpend.IsNull() || !IsTxSpendable(destSpend))
    {
        return false;
    }
    return true;
}

bool CTemplateProof::SetTemplateData(const vector<uint8>& vchDataIn)
{
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destMint >> destSpend;
    // }
    // catch (exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }
    if (vchDataIn.size() != destMint.size() + destSpend.size())
    {
        return false;
    }
    destMint.SetBytes(vchDataIn.data(), destMint.size());
    destSpend.SetBytes(vchDataIn.data() + destMint.size(), destSpend.size());
    return true;
}

bool CTemplateProof::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_PROOF))
    {
        return false;
    }

    destMint.ParseString(obj.mint.strMint);
    if (destMint.IsNull())
    {
        return false;
    }

    destSpend.ParseString(obj.mint.strSpent);
    if (destSpend.IsNull())
    {
        return false;
    }
    return true;
}

void CTemplateProof::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destMint.size() + destSpend.size());
    vchData.insert(vchData.end(), (char*)(destMint.begin()), (char*)(destMint.end()));
    vchData.insert(vchData.end(), (char*)(destSpend.begin()), (char*)(destSpend.end()));

    // vchData.clear();
    // CBufStream os;
    // os << destMint << destSpend;
    // os.GetData(vchData);
}

bool CTemplateProof::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destSpend;
    return true;
}

bool CTemplateProof::GetBlockSignDestination(CDestination& destSign) const
{
    destSign = destMint;
    return true;
}

} // namespace metabasenet

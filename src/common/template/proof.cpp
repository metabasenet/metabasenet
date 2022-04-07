// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proof.h"

#include "key.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace hnbase;
using namespace metabasenet::crypto;

//////////////////////////////
// CTemplateProof

CTemplateProof::CTemplateProof(const metabasenet::crypto::CPubKey keyMintIn, const CDestination& destSpendIn)
  : CTemplate(TEMPLATE_PROOF), keyMint(keyMintIn), destSpend(destSpendIn)
{
}

CTemplateProof* CTemplateProof::clone() const
{
    return new CTemplateProof(*this);
}

void CTemplateProof::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.mint.strMint = keyMint.ToString();
    obj.mint.strSpent = destSpend.ToString();
}

bool CTemplateProof::ValidateParam() const
{
    if (!keyMint)
    {
        return false;
    }
    if (!IsTxSpendable(destSpend))
    {
        return false;
    }
    return true;
}

bool CTemplateProof::SetTemplateData(const vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> keyMint >> destSpend;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateProof::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_PROOF))
    {
        return false;
    }

    CDestination destTemp;
    if (!destTemp.SetPubKey(obj.mint.strMint))
    {
        return false;
    }
    keyMint = destTemp.GetPubKey();

    if (!destSpend.ParseString(obj.mint.strSpent))
    {
        return false;
    }
    if (!destSpend.IsPubKey())
    {
        return false;
    }
    return true;
}

void CTemplateProof::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << keyMint << destSpend;
    os.GetData(vchData);
}

bool CTemplateProof::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    destSign = destSpend;
    return true;
}

bool CTemplateProof::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                       const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    return destSpend.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
}

bool CTemplateProof::VerifyBlockSignature(const uint256& hash, const vector<uint8>& vchSig) const
{
    return keyMint.Verify(hash, vchSig);
}

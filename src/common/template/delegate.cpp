// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegate.h"

#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hcode;
using namespace metabasenet::crypto;

//////////////////////////////
// CTemplateDelegate

CTemplateDelegate::CTemplateDelegate(const metabasenet::crypto::CPubKey& keyDelegateIn, const CDestination& destOwnerIn, const uint32 nRewardRatioIn)
  : CTemplate(TEMPLATE_DELEGATE), keyDelegate(keyDelegateIn), destOwner(destOwnerIn), nRewardRatio(nRewardRatioIn)
{
}

CTemplateDelegate* CTemplateDelegate::clone() const
{
    return new CTemplateDelegate(*this);
}

void CTemplateDelegate::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.delegate.strDelegate = keyDelegate.ToString();
    obj.delegate.strOwner = destOwner.ToString();
    obj.delegate.nRewardratio = nRewardRatio;
}

bool CTemplateDelegate::ValidateParam() const
{
    if (!keyDelegate)
    {
        return false;
    }
    if (!IsTxSpendable(destOwner))
    {
        return false;
    }
    if (nRewardRatio > MINT_REWARD_PER)
    {
        return false;
    }
    return true;
}

bool CTemplateDelegate::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> keyDelegate >> destOwner >> nRewardRatio;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateDelegate::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_DELEGATE))
    {
        return false;
    }

    CDestination destTemp;
    if (!destTemp.SetPubKey(obj.delegate.strDelegate))
    {
        return false;
    }
    keyDelegate = destTemp.GetPubKey();

    if (!destOwner.ParseString(obj.delegate.strOwner))
    {
        return false;
    }
    if (!destOwner.IsPubKey())
    {
        return false;
    }

    nRewardRatio = obj.delegate.nRewardratio;
    return true;
}

void CTemplateDelegate::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << keyDelegate << destOwner << nRewardRatio;
    os.GetData(vchData);
}

bool CTemplateDelegate::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    if (tx.nType == CTransaction::TX_CERT)
    {
        destSign = CDestination(keyDelegate);
    }
    else
    {
        destSign = destOwner;
    }
    return true;
}

bool CTemplateDelegate::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    if (nTxType == CTransaction::TX_CERT)
    {
        return CDestination(keyDelegate).VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
    }
    else
    {
        return destOwner.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
    }
}

bool CTemplateDelegate::VerifyBlockSignature(const uint256& hash, const vector<uint8>& vchSig) const
{
    return keyDelegate.Verify(hash, vchSig);
}

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegate.h"

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
// CTemplateDelegate

CTemplateDelegate::CTemplateDelegate(const CDestination& destMintIn, const CDestination& destOwnerIn, const uint16 nRewardRatioIn)
  : CTemplate(TEMPLATE_DELEGATE), destMint(destMintIn), destOwner(destOwnerIn), nRewardRatio(nRewardRatioIn)
{
}

CTemplateDelegate* CTemplateDelegate::clone() const
{
    return new CTemplateDelegate(*this);
}

void CTemplateDelegate::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.delegate.strMint = destMint.ToString();
    obj.delegate.strOwner = destOwner.ToString();
    obj.delegate.nRewardratio = nRewardRatio;
}

bool CTemplateDelegate::ValidateParam() const
{
    if (destMint.IsNull())
    {
        return false;
    }
    if (destOwner.IsNull() || !IsTxSpendable(destOwner))
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
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destMint >> destOwner >> nRewardRatio;
    // }
    // catch (const std::exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }
    if (vchDataIn.size() != (destMint.size() + destOwner.size() + 2))
    {
        return false;
    }
    const uint8* p = vchDataIn.data();
    destMint.SetBytes(p, destMint.size());
    p += destMint.size();
    destOwner.SetBytes(p, destOwner.size());
    p += destOwner.size();
    nRewardRatio = ((*p << 8) | *(p + 1));
    return true;
}

bool CTemplateDelegate::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_DELEGATE))
    {
        return false;
    }

    destMint.ParseString(obj.delegate.strMint);
    if (destMint.IsNull())
    {
        return false;
    }

    destOwner.ParseString(obj.delegate.strOwner);
    if (destOwner.IsNull())
    {
        return false;
    }

    nRewardRatio = obj.delegate.nRewardratio;
    return true;
}

void CTemplateDelegate::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destMint.size() + destOwner.size() + 2);
    vchData.insert(vchData.end(), (char*)(destMint.begin()), (char*)(destMint.end()));
    vchData.insert(vchData.end(), (char*)(destOwner.begin()), (char*)(destOwner.end()));
    vchData.push_back((uint8)(nRewardRatio >> 8));
    vchData.push_back((uint8)(nRewardRatio & 0xff));

    // vchData.clear();
    // CBufStream os;
    // os << destMint << destOwner << nRewardRatio;
    // os.GetData(vchData);
}

bool CTemplateDelegate::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    if (tx.GetTxType() == CTransaction::TX_CERT)
    {
        destSign = destMint;
    }
    else
    {
        destSign = destOwner;
    }
    return true;
}

bool CTemplateDelegate::GetBlockSignDestination(CDestination& destSign) const
{
    destSign = destMint;
    return true;
}

} // namespace metabasenet

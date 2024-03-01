// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "vote.h"

#include "block.h"
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
// CTemplateVote

CTemplateVote::CTemplateVote(const CDestination& destDelegateIn, const CDestination& destOwnerIn, const uint8 nRewardModeIn)
  : CTemplate(TEMPLATE_VOTE), destDelegate(destDelegateIn), destOwner(destOwnerIn), nRewardMode(nRewardModeIn)
{
}

CTemplateVote* CTemplateVote::clone() const
{
    return new CTemplateVote(*this);
}

void CTemplateVote::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.vote.strDelegate = destDelegate.ToString();
    obj.vote.strOwner = destOwner.ToString();
    obj.vote.nRewardmode = nRewardMode;
}

bool CTemplateVote::ValidateParam() const
{
    if (destDelegate.IsNull())
    {
        return false;
    }
    if (destOwner.IsNull() || !IsTxSpendable(destOwner))
    {
        return false;
    }
    if (nRewardMode != CVoteContext::REWARD_MODE_VOTE && nRewardMode != CVoteContext::REWARD_MODE_OWNER)
    {
        return false;
    }
    return true;
}

bool CTemplateVote::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    // CBufStream is(vchDataIn);
    // try
    // {
    //     is >> destDelegate >> destOwner >> nRewardMode;
    // }
    // catch (const std::exception& e)
    // {
    //     StdError(__PRETTY_FUNCTION__, e.what());
    //     return false;
    // }
    if (vchDataIn.size() != (destDelegate.size() + destOwner.size() + 1))
    {
        return false;
    }
    const uint8* p = vchDataIn.data();
    destDelegate.SetBytes(p, destDelegate.size());
    p += destDelegate.size();
    destOwner.SetBytes(p, destOwner.size());
    p += destOwner.size();
    nRewardMode = *p;
    return true;
}

bool CTemplateVote::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_VOTE))
    {
        return false;
    }

    destDelegate.ParseString(obj.vote.strDelegate);
    if (destDelegate.IsNull())
    {
        return false;
    }

    destOwner.ParseString(obj.vote.strOwner);
    if (destOwner.IsNull())
    {
        return false;
    }

    nRewardMode = obj.vote.nRewardmode;
    if (nRewardMode != CVoteContext::REWARD_MODE_VOTE && nRewardMode != CVoteContext::REWARD_MODE_OWNER)
    {
        return false;
    }
    return true;
}

void CTemplateVote::BuildTemplateData()
{
    vchData.clear();
    vchData.reserve(destDelegate.size() + destOwner.size() + 1);
    vchData.insert(vchData.end(), (char*)(destDelegate.begin()), (char*)(destDelegate.end()));
    vchData.insert(vchData.end(), (char*)(destOwner.begin()), (char*)(destOwner.end()));
    vchData.push_back(nRewardMode);

    // vchData.clear();
    // CBufStream os;
    // os << destDelegate << destOwner << nRewardMode;
    // os.GetData(vchData);
}

bool CTemplateVote::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destOwner;
    return true;
}

} // namespace metabasenet

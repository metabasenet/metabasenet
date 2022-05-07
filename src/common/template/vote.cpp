// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "vote.h"

#include "block.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace hcode;
using namespace metabasenet::crypto;

//////////////////////////////
// CTemplateVote

CTemplateVote::CTemplateVote(const CDestination& destDelegateIn, const CDestination& destOwnerIn, const int nRewardModeIn)
  : CTemplate(TEMPLATE_VOTE), destDelegate(destDelegateIn), destOwner(destOwnerIn), nRewardMode(nRewardModeIn)
{
}

CTemplateVote* CTemplateVote::clone() const
{
    return new CTemplateVote(*this);
}

void CTemplateVote::GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const
{
    obj.vote.strDelegate = destDelegate.ToString();
    obj.vote.strOwner = destOwner.ToString();
    obj.vote.nRewardmode = nRewardMode;
}

bool CTemplateVote::ValidateParam() const
{
    CTemplateId tid;
    if (!(destDelegate.GetTemplateId(tid) && tid.GetType() == TEMPLATE_DELEGATE))
    {
        return false;
    }
    if (!IsTxSpendable(destOwner))
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
    CBufStream is(vchDataIn);
    try
    {
        is >> destDelegate >> destOwner >> nRewardMode;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateVote::SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_VOTE))
    {
        return false;
    }

    if (!destDelegate.ParseString(obj.vote.strDelegate))
    {
        return false;
    }
    if (!(destDelegate.IsTemplate() && destDelegate.GetTemplateId().GetType() == TEMPLATE_DELEGATE))
    {
        return false;
    }

    if (!destOwner.ParseString(obj.vote.strOwner))
    {
        return false;
    }
    if (!destOwner.IsPubKey())
    {
        return false;
    }

    nRewardMode = obj.vote.nRewardmode;
    return true;
}

void CTemplateVote::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << destDelegate << destOwner << nRewardMode;
    os.GetData(vchData);
}

bool CTemplateVote::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const
{
    destSign = destOwner;
    return true;
}

bool CTemplateVote::VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                      const vector<uint8>& vchSig, const int32 nForkHeight) const
{
    return destOwner.VerifyTxSignature(hash, nTxType, destTo, bytes(), vchSig, nForkHeight);
}

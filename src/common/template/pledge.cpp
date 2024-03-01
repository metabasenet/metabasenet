// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pledge.h"

//#include "block.h"
#include "param.h"
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
// CTemplatePledge

CTemplatePledge::CTemplatePledge(const CDestination& destDelegateIn, const CDestination& destOwnerIn, const uint32 nPledgeTypeIn, const uint32 nCyclesIn, const uint32 nNonceIn)
  : CTemplate(TEMPLATE_PLEDGE), destDelegate(destDelegateIn), destOwner(destOwnerIn), nPledgeType(nPledgeTypeIn), nCycles(nCyclesIn), nNonce(nNonceIn)
{
}

CTemplatePledge* CTemplatePledge::clone() const
{
    return new CTemplatePledge(*this);
}

void CTemplatePledge::GetTemplateData(rpc::CTemplateResponse& obj) const
{
    obj.pledge.strDelegate = destDelegate.ToString();
    obj.pledge.strOwner = destOwner.ToString();
    obj.pledge.nPledgetype = nPledgeType;
    obj.pledge.nCycles = nCycles;
    obj.pledge.nNonce = nNonce;
}

bool CTemplatePledge::GetPledgeFinalHeight(const uint32 nHeight, uint32& nFinalHeight)
{
    // rule type, days, reward rate(base: 10000)
    const std::map<uint16, std::pair<uint32, uint32>>* pRule = GetRule(nHeight);
    if (!pRule)
    {
        return false;
    }
    auto mt = pRule->find(nPledgeType);
    if (mt == pRule->end())
    {
        return false;
    }
    if (nCycles == 0)
    {
        nFinalHeight = 0; // 0: unlimit
    }
    else
    {
        nFinalHeight = nHeight + (mt->second.first * DAY_HEIGHT * nCycles);
    }
    return true;
}

uint32 CTemplatePledge::GetRewardRate(const uint32 nHeight)
{
    // rule type, days, reward rate(base: 10000)
    const std::map<uint16, std::pair<uint32, uint32>>* pRule = GetRule(nHeight);
    if (!pRule)
    {
        return 0;
    }
    auto mt = pRule->find(nPledgeType);
    if (mt == pRule->end())
    {
        return 0;
    }
    return mt->second.second;
}

uint32 CTemplatePledge::GetPledgeDays(const uint32 nHeight)
{
    // rule type, days, reward rate(base: 10000)
    const std::map<uint16, std::pair<uint32, uint32>>* pRule = GetRule(nHeight);
    if (!pRule)
    {
        return 0;
    }
    auto mt = pRule->find(nPledgeType);
    if (mt == pRule->end())
    {
        return 0;
    }
    return mt->second.first;
}

const std::map<uint16, std::pair<uint32, uint32>>* CTemplatePledge::GetRule(const uint32 nHeight)
{
    // rule type, days, reward rate(base: 10000)
    const std::map<uint16, std::pair<uint32, uint32>>* pRule = nullptr;
    auto& mapRule = PLEDGE_REWARD_RULE;
    for (auto it = mapRule.begin(); it != mapRule.end(); it++)
    {
        if (nHeight >= it->first)
        {
            pRule = &(it->second);
        }
        else
        {
            break;
        }
    }
    return pRule;
}

//-------------------------------------------------------------------
bool CTemplatePledge::ValidateParam() const
{
    if (destDelegate.IsNull())
    {
        return false;
    }
    if (destOwner.IsNull() || !IsTxSpendable(destOwner))
    {
        return false;
    }
    if (nPledgeType == 0)
    {
        return false;
    }
    if (nCycles > 10000)
    {
        return false;
    }
    return true;
}

bool CTemplatePledge::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CBufStream is(vchDataIn);
    try
    {
        is >> destDelegate >> destOwner >> nPledgeType >> nCycles >> nNonce;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplatePledge::SetTemplateData(const rpc::CTemplateRequest& obj)
{
    if (obj.strType != GetTypeName(TEMPLATE_PLEDGE))
    {
        return false;
    }

    destDelegate.ParseString(obj.pledge.strDelegate);
    if (destDelegate.IsNull())
    {
        return false;
    }

    destOwner.ParseString(obj.pledge.strOwner);
    if (destOwner.IsNull())
    {
        return false;
    }

    nPledgeType = obj.pledge.nPledgetype;
    nCycles = obj.pledge.nCycles;
    nNonce = obj.pledge.nNonce;
    return true;
}

void CTemplatePledge::BuildTemplateData()
{
    vchData.clear();
    CBufStream os;
    os << destDelegate << destOwner << nPledgeType << nCycles << nNonce;
    os.GetData(vchData);
}

bool CTemplatePledge::GetSignDestination(const CTransaction& tx, CDestination& destSign) const
{
    destSign = destOwner;
    return true;
}

} // namespace metabasenet

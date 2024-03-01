// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_PLEDGE_H
#define COMMON_TEMPLATE_PLEDGE_H

#include "destination.h"
#include "mint.h"

namespace metabasenet
{

class CTemplatePledge : virtual public CTemplate
{
public:
    CTemplatePledge(const CDestination& destDelegateIn = CDestination(), const CDestination& destOwnerIn = CDestination(), const uint32 nPledgeTypeIn = 0, const uint32 nCyclesIn = 0, const uint32 nNonceIn = 0);
    virtual CTemplatePledge* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

    bool GetPledgeFinalHeight(const uint32 nHeight, uint32& nFinalHeight);
    uint32 GetRewardRate(const uint32 nHeight);
    uint32 GetPledgeDays(const uint32 nHeight);

protected:
    const std::map<uint16, std::pair<uint32, uint32>>* GetRule(const uint32 nHeight);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;

public:
    CDestination destDelegate;
    CDestination destOwner;
    uint32 nPledgeType;
    uint32 nCycles; // 0: unlimit, other: cycles
    uint32 nNonce;
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_PLEDGE_H

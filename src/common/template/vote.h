// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_VOTE_H
#define COMMON_TEMPLATE_VOTE_H

#include "destination.h"
#include "mint.h"

namespace metabasenet
{

class CTemplateVote : virtual public CTemplate
{
public:
    CTemplateVote(const CDestination& destDelegateIn = CDestination(), const CDestination& destOwnerIn = CDestination(), const uint8 nRewardModeIn = 0);
    virtual CTemplateVote* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;

public:
    CDestination destDelegate;
    CDestination destOwner;
    uint8 nRewardMode; // ref: REWARD_MODE_VOTE (0) or REWARD_MODE_OWNER (1)
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_VOTE_H

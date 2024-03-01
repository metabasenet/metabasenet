// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_DELEGATE_H
#define COMMON_TEMPLATE_DELEGATE_H

#include "destination.h"
#include "mint.h"

namespace metabasenet
{

class CTemplateDelegate : virtual public CTemplateMint
{
public:
    CTemplateDelegate(const CDestination& destMintIn = CDestination(),
                      const CDestination& destOwnerIn = CDestination(), const uint16 nRewardRatioIn = 0);
    virtual CTemplateDelegate* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;
    virtual bool GetBlockSignDestination(CDestination& destSign) const;

public:
    CDestination destMint;
    CDestination destOwner;
    uint16 nRewardRatio; // base reference MINT_REWARD_PER, example: 500/10000=0.05
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_DELEGATE_H

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_PROOF_H
#define COMMON_TEMPLATE_PROOF_H

#include "destination.h"
#include "mint.h"

namespace metabasenet
{

class CTemplateProof : virtual public CTemplateMint
{
public:
    CTemplateProof(const CDestination& destMintIn = CDestination(), const CDestination& destSpendIn = CDestination());
    virtual CTemplateProof* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;
    virtual bool GetBlockSignDestination(CDestination& destSign) const;

protected:
    CDestination destMint;
    CDestination destSpend;
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_PROOF_H

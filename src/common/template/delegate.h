// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_DELEGATE_H
#define COMMON_TEMPLATE_DELEGATE_H

#include "destination.h"
#include "mint.h"

class CTemplateDelegate : virtual public CTemplateMint
{
public:
    CTemplateDelegate(const metabasenet::crypto::CPubKey& keyDelegateIn = metabasenet::crypto::CPubKey(),
                      const CDestination& destOwnerIn = CDestination(), const uint32 nRewardRatioIn = 0);
    virtual CTemplateDelegate* clone() const;
    virtual void GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const;
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight) const;
    virtual bool VerifyBlockSignature(const uint256& hash, const std::vector<uint8>& vchSig) const;

public:
    metabasenet::crypto::CPubKey keyDelegate;
    CDestination destOwner;
    uint32 nRewardRatio; // base reference MINT_REWARD_PER, example: 500/10000=0.05
};

#endif // COMMON_TEMPLATE_DELEGATE_H

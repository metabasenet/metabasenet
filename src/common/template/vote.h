// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_VOTE_H
#define COMMON_TEMPLATE_VOTE_H

#include "destination.h"
#include "mint.h"

class CTemplateVote : virtual public CTemplate
{
public:
    CTemplateVote(const CDestination& destDelegateIn = CDestination(),
                  const CDestination& destOwnerIn = CDestination(), const int nRewardModeIn = 0);
    virtual CTemplateVote* clone() const;
    virtual void GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const;
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight) const;

public:
    CDestination destDelegate;
    CDestination destOwner;
    int nRewardMode;
};

#endif // COMMON_TEMPLATE_VOTE_H

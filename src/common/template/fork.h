// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_FORK_H
#define COMMON_TEMPLATE_FORK_H

#include "template.h"

class CTemplateFork : virtual public CTemplate
{
public:
    CTemplateFork(const CDestination& destRedeemIn = CDestination(), const uint256& hashForkIn = uint256());
    virtual CTemplateFork* clone() const;
    virtual void GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const;

public:
    static uint256 LockedCoin(const int32 nHeight);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const;
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight) const;

public:
    CDestination destRedeem;
    uint256 hashFork;
};

#endif // COMMON_TEMPLATE_FORK_H

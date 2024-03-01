// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_FORK_H
#define COMMON_TEMPLATE_FORK_H

#include "template.h"

namespace metabasenet
{

class CTemplateFork : virtual public CTemplate
{
public:
    CTemplateFork(const CDestination& destRedeemIn = CDestination(), const uint256& hashForkIn = uint256());
    virtual CTemplateFork* clone() const;
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const;

public:
    static uint256 LockedCoin(const int32 nHeight);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj);
    virtual void BuildTemplateData();
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const;

public:
    CDestination destRedeem;
    uint256 hashFork;
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_FORK_H

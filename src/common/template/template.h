// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_TEMPLATE_H
#define COMMON_TEMPLATE_TEMPLATE_H

#include <boost/shared_ptr.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "destination.h"
#include "mtbase.h"
#include "param.h"
#include "templateid.h"
#include "util.h"

namespace metabasenet
{

#define PRICE_PRECISION (10000000000L)
#define MAX_STRING_ADDRESS_LEN 128
#define MAX_STRING_AMOUNT_LEN 64

class CSpendableTemplate
{
};

enum TemplateType
{
    TEMPLATE_MIN = 0,
    TEMPLATE_FORK = 1,
    TEMPLATE_PROOF = 2,
    TEMPLATE_DELEGATE = 3,
    TEMPLATE_VOTE = 4,
    TEMPLATE_PLEDGE = 5,
    TEMPLATE_ACTIVATECODE = 6,
    TEMPLATE_MAX
};

class CTemplate;
typedef boost::shared_ptr<CTemplate> CTemplatePtr;

class CTransaction;

namespace rpc
{
class CTemplateRequest;
class CTemplateResponse;
} // namespace rpc

/**
 * template basic class
 */
class CTemplate
{
public:
    // Construct by CTemplate pointer.
    static const CTemplatePtr CreateTemplatePtr(CTemplate* ptr);

    // Construct by template id and template data.
    //static const CTemplatePtr CreateTemplatePtr(const CTemplateId& nIdIn, const std::vector<uint8>& vchDataIn);

    // Construct by template type and template data.
    static const CTemplatePtr CreateTemplatePtr(uint8 nTypeIn, const std::vector<uint8>& vchDataIn);

    // Construct by json object.
    static const CTemplatePtr CreateTemplatePtr(const rpc::CTemplateRequest& obj);

    // Construct by exported template data.
    static const CTemplatePtr Import(const std::vector<uint8>& vchTemplateIn);

    // Return template type is between TEMPLATE_MIN and TEMPLATE_MAX or not.
    static bool IsValidType(const uint8 nTypeIn);

    // Return template type name.
    static std::string GetTypeName(uint8 nTypeIn);

    // Return dest is spendable or not.
    static bool IsTxSpendable(const CDestination& dest);

public:
    // Deconstructor
    virtual ~CTemplate(){};

    // Return template type.
    const uint8& GetTemplateType() const;

    // Return template id.
    const CTemplateId& GetTemplateId() const;

    // Return template name.
    std::string GetName() const;

    // Export template type and template data.
    std::vector<uint8> Export() const;

    // Convert params of template to json object
    virtual void GetTemplateData(rpc::CTemplateResponse& obj) const = 0;

    // Build transaction signature by concrete template.
    virtual bool GetSignDestination(const CTransaction& tx, CDestination& destSign) const = 0;

protected:
    // Constructor
    CTemplate() = default;
    CTemplate(const uint8 nTypeIn);

    // Verify vchSig is begin with template data or not.
    bool VerifyTemplateData(const std::vector<uint8>& vchSig) const;

    // Build template id by vchData.
    void BuildTemplateId();

protected:
    virtual CTemplate* clone() const = 0;

    // Validate concrete params of template.
    virtual bool ValidateParam() const = 0;

    // Convert vchData to concrete params of template.
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn) = 0;

    // Convert json object to concrete params of template.
    virtual bool SetTemplateData(const rpc::CTemplateRequest& obj) = 0;

    // Convert concrete params of template to vchData.
    virtual void BuildTemplateData() = 0;

protected:
    uint8 nType;
    CTemplateId nId;
    std::vector<uint8> vchData;
};

} // namespace metabasenet

#endif // COMMON_TEMPLATE_TEMPLATE_H

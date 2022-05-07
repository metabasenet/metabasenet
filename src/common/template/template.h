// Copyright (c) 2021-2022 The MetabaseNet developers
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
#include "hcode.h"
#include "param.h"
#include "templateid.h"
#include "util.h"

#define PRICE_PRECISION (10000000000L)
#define MAX_STRING_ADDRESS_LEN 128
#define MAX_STRING_AMOUNT_LEN 64

class CSpendableTemplate
{
};

enum TemplateType
{
    TEMPLATE_MIN = 0,
    TEMPLATE_FORK = 3,
    TEMPLATE_PROOF = 4,
    TEMPLATE_DELEGATE = 5,
    TEMPLATE_VOTE = 7,
    TEMPLATE_ACTIVATECODE = 8,
    TEMPLATE_MAX
};

class CTemplate;
typedef boost::shared_ptr<CTemplate> CTemplatePtr;

class CTransaction;
namespace metabasenet
{
namespace rpc
{
class CTemplateRequest;
class CTemplateResponse;
} // namespace rpc
} // namespace metabasenet

/**
 * template basic class
 */
class CTemplate
{
public:
    // Construct by CTemplate pointer.
    static const CTemplatePtr CreateTemplatePtr(CTemplate* ptr);

    // Construct by template destination and template data.
    static const CTemplatePtr CreateTemplatePtr(const CDestination& destIn, const std::vector<uint8>& vchDataIn);

    // Construct by template id and template data.
    static const CTemplatePtr CreateTemplatePtr(const CTemplateId& nIdIn, const std::vector<uint8>& vchDataIn);

    // Construct by template type and template data.
    static const CTemplatePtr CreateTemplatePtr(uint16 nTypeIn, const std::vector<uint8>& vchDataIn);

    // Construct by json object.
    static const CTemplatePtr CreateTemplatePtr(const metabasenet::rpc::CTemplateRequest& obj);

    // Construct by exported template data.
    static const CTemplatePtr Import(const std::vector<uint8>& vchTemplateIn);

    // Return template type is between TEMPLATE_MIN and TEMPLATE_MAX or not.
    static bool IsValidType(const uint16 nTypeIn);

    // Return template type name.
    static std::string GetTypeName(uint16 nTypeIn);

    // Return dest is spendable or not.
    static bool IsTxSpendable(const CDestination& dest);

public:
    // Deconstructor
    virtual ~CTemplate(){};

    // Return template type.
    const uint16& GetTemplateType() const;

    // Return template id.
    const CTemplateId& GetTemplateId() const;

    // Return template data.
    const std::vector<uint8>& GetTemplateData() const;

    // Return template name.
    std::string GetName() const;

    // Export template type and template data.
    std::vector<uint8> Export() const;

    // Convert params of template to json object
    virtual void GetTemplateData(metabasenet::rpc::CTemplateResponse& obj) const = 0;

    // Build transaction signature by concrete template.
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, CDestination& destSign) const = 0;

    // Verify transaction signature by concrete template.
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nHeight) const = 0;

protected:
    // Constructor
    CTemplate() = default;
    CTemplate(const uint16 nTypeIn);

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
    virtual bool SetTemplateData(const metabasenet::rpc::CTemplateRequest& obj) = 0;

    // Convert concrete params of template to vchData.
    virtual void BuildTemplateData() = 0;

protected:
    uint16 nType;
    CTemplateId nId;
    std::vector<uint8> vchData;
};

#endif // COMMON_TEMPLATE_TEMPLATE_H

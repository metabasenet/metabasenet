// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_DESTINATION_H
#define COMMON_DESTINATION_H

#include <vector>

#include "key.h"
#include "stream/stream.h"
#include "template/templateid.h"
#include "uint256.h"

using namespace metabasenet::crypto;

namespace metabasenet
{

class CTemplate;
class CTransaction;
class CBlock;

class CContractId : public uint256
{
public:
    CContractId() {}
    CContractId(const uint256& data)
      : uint256(data) {}

    CContractId& operator=(uint64 b)
    {
        *((uint256*)this) = b;
        return *this;
    }
};

class CDestination : public uint160
{
public:
    enum
    {
        PREFIX_NULL = 0x00,
        PREFIX_PUBKEY = 0x01,
        PREFIX_TEMPLATE = 0x02,
        PREFIX_CONTRACT = 0x03,
        PREFIX_MAX
    };

public:
    CDestination();
    CDestination(const uint160& data);
    CDestination(const uint256& hash);
    CDestination(const uint8* pData, std::size_t nSize);
    CDestination(const std::string& str);
    CDestination(const CPubKey& pubkey);
    CDestination(const CTemplateId& tid);
    CDestination(const CContractId& cid);
    virtual ~CDestination() = default;

    void SetHash(const uint256& hashData);
    uint256 ToHash() const;

    // public key
    CDestination& SetPubKey(const CPubKey& pubkey);

    // template id
    CDestination& SetTemplateId(const CTemplateId& tid);

    // contract id
    CDestination& SetContractId(const CContractId& wid);
    //CDestination& SetContractId(const CDestination& from, const uint64 nTxNonce);

    // format
    bool ParseString(const std::string& str);
    std::string ToString() const;
};

using CPubkeyAddressContext = bytes;

class CTemplateAddressContext
{
    friend class mtbase::CStream;

public:
    CTemplateAddressContext() {}
    CTemplateAddressContext(const std::string& strNameIn, const std::string& strDescribeIn, const uint8 nTemplateTypeIn, const bytes& btDataIn)
      : strName(strNameIn), strDescribe(strDescribeIn), nTemplateType(nTemplateTypeIn), btData(btDataIn) {}

public:
    std::string strName;
    std::string strDescribe;
    uint8 nTemplateType;
    bytes btData;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(strDescribe, opt);
        s.Serialize(nTemplateType, opt);
        s.Serialize(btData, opt);
    }
};

class CContractAddressContext
{
    friend class mtbase::CStream;

public:
    CContractAddressContext() {}
    CContractAddressContext(const std::string& strTypeIn, const CDestination& destCodeOwnerIn, const std::string& strNameIn, const std::string& strDescribeIn, const uint256& hashCreateTxidIn,
                            const uint256& hashSourceCodeIn, const uint256& hashContractCreateCodeIn, const uint256& hashContractRunCodeIn)
      : strType(strTypeIn), destCodeOwner(destCodeOwnerIn), strName(strNameIn), strDescribe(strDescribeIn), hashCreateTxid(hashCreateTxidIn),
        hashSourceCode(hashSourceCodeIn), hashContractCreateCode(hashContractCreateCodeIn), hashContractRunCode(hashContractRunCodeIn) {}

public:
    std::string strType;
    CDestination destCodeOwner;
    std::string strName;
    std::string strDescribe;
    uint256 hashCreateTxid;
    uint256 hashSourceCode;
    uint256 hashContractCreateCode;
    uint256 hashContractRunCode;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(strType, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(strName, opt);
        s.Serialize(strDescribe, opt);
        s.Serialize(hashCreateTxid, opt);
        s.Serialize(hashSourceCode, opt);
        s.Serialize(hashContractCreateCode, opt);
        s.Serialize(hashContractRunCode, opt);
    }
};

class CAddressContext
{
    friend class mtbase::CStream;

public:
    CAddressContext()
      : nType(0) {}
    CAddressContext(const uint8 nDestTypeIn, const uint8 nTemplateTypeIn, const bytes& btDataIn)
      : btData(btDataIn)
    {
        SetType(nDestTypeIn, nTemplateTypeIn);
    }
    CAddressContext(const CPubkeyAddressContext& ctxPubkey)
      : btData(ctxPubkey)
    {
        SetType(CDestination::PREFIX_PUBKEY, 0);
    }
    CAddressContext(const CTemplateAddressContext& ctxtTemplateIn)
    {
        mtbase::CBufStream ss;
        ss << ctxtTemplateIn;
        btData.assign(ss.GetData(), ss.GetData() + ss.GetSize());
        SetType(CDestination::PREFIX_TEMPLATE, ctxtTemplateIn.nTemplateType);
    }
    CAddressContext(const CContractAddressContext& ctxtContractIn)
    {
        mtbase::CBufStream ss;
        ss << ctxtContractIn;
        btData.assign(ss.GetData(), ss.GetData() + ss.GetSize());
        SetType(CDestination::PREFIX_CONTRACT, 0);
    }

    bool IsNull() const
    {
        return (nType == 0);
    }
    void SetType(const uint8 nDestType, const uint8 nTemplateType)
    {
        nType = (nDestType << 5) | (nTemplateType & 0x1F);
    }
    uint8 GetDestType() const
    {
        return (nType >> 5);
    }
    uint8 GetTemplateType() const
    {
        return (nType & 0x1F);
    }
    bool IsPubkey() const
    {
        return (GetDestType() == CDestination::PREFIX_PUBKEY);
    }
    bool IsTemplate() const
    {
        return (GetDestType() == CDestination::PREFIX_TEMPLATE);
    }
    bool IsContract() const
    {
        return (GetDestType() == CDestination::PREFIX_CONTRACT);
    }

    bool GetTemplateAddressContext(CTemplateAddressContext& ctxtTemplate) const
    {
        if (GetDestType() == CDestination::PREFIX_TEMPLATE)
        {
            try
            {
                mtbase::CBufStream ss(btData);
                ss >> ctxtTemplate;
            }
            catch (std::exception& e)
            {
                return false;
            }
            return true;
        }
        return false;
    }
    bool GetContractAddressContext(CContractAddressContext& ctxtContract) const
    {
        if (GetDestType() == CDestination::PREFIX_CONTRACT)
        {
            try
            {
                mtbase::CBufStream ss(btData);
                ss >> ctxtContract;
            }
            catch (std::exception& e)
            {
                return false;
            }
            return true;
        }
        return false;
    }

public:
    uint8 nType; // high 3-bits: address type, low 5-bits: template type
    bytes btData;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nType, opt);
        s.Serialize(btData, opt);
    }
};

class CFunctionAddressContext
{
    friend class mtbase::CStream;

public:
    CFunctionAddressContext()
      : fDisableModify(false) {}
    CFunctionAddressContext(const CDestination& destFunctionIn, const bool fDisableModifyIn)
      : destFunction(destFunctionIn), fDisableModify(fDisableModifyIn) {}

    const CDestination& GetFunctionAddress() const
    {
        return destFunction;
    }
    bool IsDisableModify() const
    {
        return fDisableModify;
    }

    void SetFunctionAddress(const CDestination& dest)
    {
        destFunction = dest;
    }
    void SetDisableModify(const bool f)
    {
        fDisableModify = f;
    }

protected:
    CDestination destFunction;
    bool fDisableModify;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(destFunction, opt);
        s.Serialize(fDisableModify, opt);
    }
};

} // namespace metabasenet

#endif // COMMON_DESTINATION_H

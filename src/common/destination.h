// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_DESTINATION_H
#define COMMON_DESTINATION_H

#include <vector>

#include "key.h"
#include "stream/stream.h"
#include "template/templateid.h"
#include "uint256.h"

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

class CDestination
{
    friend class hnbase::CStream;

public:
    uint8 prefix;
    uint256 data;

    enum
    {
        PREFIX_NULL = 0x00,
        PREFIX_PUBKEY = 0x01,
        PREFIX_TEMPLATE = 0x02,
        PREFIX_CONTRACT = 0x03,
        PREFIX_MAX
    };
    enum
    {
        DESTINATION_SIZE = 33
    };
    enum
    {
        ADDRESS_LEN = 57
    };

public:
    CDestination();
    CDestination(const std::string& strAddress);
    CDestination(const uint8 nPrefix, const uint256& hashData);
    CDestination(const metabasenet::crypto::CPubKey& pubkey);
    CDestination(const CTemplateId& tid);
    CDestination(const CContractId& cid);
    virtual ~CDestination() = default;

    // null
    void SetNull();
    bool IsNull() const;

    // public key
    bool IsPubKey() const;
    bool SetPubKey(const std::string& str);
    CDestination& SetPubKey(const metabasenet::crypto::CPubKey& pubkey);
    bool GetPubKey(metabasenet::crypto::CPubKey& pubkey) const;
    const metabasenet::crypto::CPubKey GetPubKey() const;

    // template id
    bool IsTemplate() const;
    bool SetTemplateId(const std::string& str);
    CDestination& SetTemplateId(const CTemplateId& tid);
    bool GetTemplateId(CTemplateId& tid) const;
    const CTemplateId GetTemplateId() const;

    // contract id
    bool IsContract() const;
    bool IsNullContract() const;
    CDestination& SetContractId(const uint256& wid);
    CDestination& SetContractId(const uint256& from, const uint64 nTxNonce);
    bool GetContractId(uint256& wid) const;

    bool VerifyTxSignature(const uint256& hash, const uint16 nTxType, const CDestination& destTo, const bytes& btTemplateData,
                           const std::vector<uint8>& vchSig, const int32 nForkHeight) const;
    bool VerifyBlockSignature(const uint256& hash, const bytes& btTemplateData, const bytes& vchSig) const;

    // format
    bool ParseString(const std::string& str);
    std::string ToString() const;

    bool ParseStringHex(const std::string& str);
    std::string ToStringHex() const;

protected:
    void Serialize(hnbase::CStream& s, hnbase::LoadType& opt)
    {
        s.Serialize(prefix, opt);
        s.Serialize(data, opt);
    }
    template <typename O>
    void Serialize(hnbase::CStream& s, O& opt) const
    {
        s.Serialize(prefix, opt);
        s.Serialize(data, opt);
    }
};

inline bool operator==(const CDestination& a, const CDestination& b)
{
    return (a.prefix == b.prefix && a.data == b.data);
}

inline bool operator!=(const CDestination& a, const CDestination& b)
{
    return !(a == b);
}
inline bool operator<(const CDestination& a, const CDestination& b)
{
    return (a.prefix < b.prefix || (a.prefix == b.prefix && a.data < b.data));
}

#endif // COMMON_DESTINATION_H

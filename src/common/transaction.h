// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TRANSACTION_H
#define COMMON_TRANSACTION_H

#include <set>
#include <stream/stream.h>
#include <vector>

#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "uint256.h"

namespace metabasenet
{

/////////////////////////////
// CTransaction

class CTemplateContext;
class CTxContractData;
class CAddressContext;

class CTransaction
{
    friend class mtbase::CStream;

protected:
    uint8 nType;

    CChainId nChainId;
    uint64 nTxNonce;
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGasLimit;
    std::map<uint8, bytes> mapTxData;
    bytes vchSig;

    bytes btEthTx;
    uint256 txidEthTx;

public:
    enum
    {
        TXM_COMMON = 0x00,
        TXM_ETH = 0x01
    };
    enum
    {
        TX_NULL = 0x00, ///< Null transaction.

        TX_GENESIS = 0x01,     // Genesis tx
        TX_TOKEN = 0x02,       // Normal Tx
        TX_CERT = 0x03,        // Enroll Tx
        TX_STAKE = 0x04,       // PoS mint Tx
        TX_WORK = 0x05,        // PoA mint Tx
        TX_VOTE_REWARD = 0x06, // Vote reward Tx
        TX_INTERNAL = 0x07,    // Internal Tx

        TX_ETH_CREATE_CONTRACT = 0x11, // Eth create contract tx
        TX_ETH_MESSAGE_CALL = 0x12,    // Eth message call tx
    };
    enum
    {
        DF_COMMON = 0x00,
        DF_CREATE_CODE = 0x01,

        DF_ETH_TX_DATA = 0x05,

        DF_TO_ADDRESS_DATA = 0x11,
        //DF_CONTRACTCODE = 0x12,
        DF_CONTRACTPARAM = 0x13,
        DF_FORKDATA = 0x14,
        DF_CERTTXDATA = 0x15,
        DF_ACTIVATECODE = 0x16,
        DF_BLSPUBKEY = 0x17,
    };

public:
    CTransaction()
    {
        SetNull();
    }
    virtual ~CTransaction() = default;

    virtual void SetNull();
    bool IsNull() const;
    bool IsMintTx() const;
    bool IsRewardTx() const;
    bool IsCertTx() const;
    bool IsEthTx() const;
    bool IsUserTx() const;
    bool IsInternalTx() const;
    std::string GetTypeString() const;
    uint256 GetTxFee() const;
    uint256 GetHash() const;
    uint256 GetSignatureHash() const;
    void AddTxData(const uint16 nTypeIn, const bytes& btDataIn);
    bool GetTxData(const uint16 nTypeIn, bytes& btDataOut) const;
    std::size_t GetTxDataCount() const;
    std::size_t GetTxDataSize() const;
    uint256 GetTxBaseGas() const;
    uint256 GetTxDataGas() const;
    uint256 GetRunGasLimit() const;
    bytes GetTxExtData() const;
    bool SetTxExtData(const bytes& btExtData);

    uint8 GetTxType() const;
    const CDestination& GetFromAddress() const;
    const CDestination& GetToAddress() const;
    CChainId GetChainId() const;
    uint64 GetNonce() const;
    const uint256& GetAmount() const;
    const uint256& GetGasPrice() const;
    const uint256& GetGasLimit() const;
    const bytes& GetSignData() const;

    void SetTxType(const uint8 n);
    void SetChainId(const CChainId nChainIdIn);
    void SetNonce(const uint64 n);
    void SetFromAddress(const CDestination& dest);
    void SetToAddress(const CDestination& dest);
    void SetAmount(const uint256& n);
    void SetGasPrice(const uint256& n);
    void SetGasLimit(const uint256& n);
    void SetSignData(const bytes& d);

    bool VerifyTxSignature(const CDestination& destSign) const;

    void SetToAddressData(const CAddressContext& ctxAddress);
    bool GetToAddressData(CAddressContext& ctxAddress) const;

    bool SetEthTx(const bytes& btEthTxDataIn, const bool fToPubkey);
    bool GetCreateCodeContext(uint8& nCodeType, CTemplateContext& ctxTemplate, CTxContractData& txcd) const;

    static uint8 GetTxMode(const uint8 nTxType);
    static std::string GetTypeStringStatic(uint16 nTxType);
    static uint256 GetTxDataGasStatic(const uint64 nTxDataSize);
    static uint256 GetTxBaseGasStatic(const uint64 nTxDataSize);

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return (a.nType == b.nType
                && a.nChainId == b.nChainId
                && a.nTxNonce == b.nTxNonce
                && a.destFrom == b.destFrom
                && a.destTo == b.destTo
                && a.nAmount == b.nAmount
                && a.nGasPrice == b.nGasPrice
                && a.nGasLimit == b.nGasLimit
                && a.mapTxData == b.mapTxData
                && a.vchSig == b.vchSig);
    }
    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return !(a == b);
    }

protected:
    bool SetEthTxPacket(const bytes& btTxPacket);
    uint256 GetEthSignatureHash() const;
    void PrSetToAddressData(const CAddressContext& ctxAddress);

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CTxIndex
{
    friend class mtbase::CStream;

public:
    uint64 nBlockNumber;
    uint16 nTxSeq;
    uint32 nFile;
    uint32 nOffset;

public:
    CTxIndex()
    {
        SetNull();
    }
    CTxIndex(const uint64 nBlockNumberIn, const uint16 nTxSeqIn, const uint32 nFileIn, const uint32 nOffsetIn)
    {
        nBlockNumber = nBlockNumberIn;
        nTxSeq = nTxSeqIn;
        nFile = nFileIn;
        nOffset = nOffsetIn;
    }
    void SetNull()
    {
        nBlockNumber = 0;
        nTxSeq = 0;
        nFile = 0;
        nOffset = 0;
    }
    bool IsNull() const
    {
        return (nFile == 0);
    };

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nBlockNumber, opt);
        s.Serialize(nTxSeq, opt);
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
    }
};

class CTxInfo
{
public:
    CTxInfo() {}
    CTxInfo(const uint256& txidIn, const uint256& hashForkIn, const int nTxTypeIn, const uint64 nTimeStampIn, const uint64 nTxNonceIn,
            const int nBlockHeightIn, const uint64 nTxSeqIn, const CDestination& destFromIn, const CDestination& destToIn,
            const uint256& nAoumtIn, const uint256& nGasPriceIn, const uint256& nGasIn, const uint64 nSizeIn)
      : txid(txidIn), hashFork(hashForkIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn), nTxNonce(nTxNonceIn),
        nBlockHeight(nBlockHeightIn), nTxSeq(nTxSeqIn), destFrom(destFromIn), destTo(destToIn),
        nAmount(nAoumtIn), nGasPrice(nGasPriceIn), nGas(nGasIn), nSize(nSizeIn) {}

    bool IsRewardTx() const
    {
        return (nTxType == CTransaction::TX_GENESIS || nTxType == CTransaction::TX_STAKE
                || nTxType == CTransaction::TX_WORK || nTxType == CTransaction::TX_VOTE_REWARD);
    }

public:
    uint256 txid;
    uint256 hashFork;
    int nTxType;
    uint64 nTimeStamp;
    uint64 nTxNonce;
    int nBlockHeight;
    uint64 nTxSeq;
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGas;
    uint64 nSize;
};

class CAddrTxIndex
{
    friend class mtbase::CStream;

public:
    CDestination dest;
    int64 nHeightSeq;
    uint256 txid;

public:
    CAddrTxIndex() {}
    CAddrTxIndex(const CDestination& destIn, const int64 nHeightSeqIn, const uint256& txidIn)
      : dest(destIn), nHeightSeq(nHeightSeqIn), txid(txidIn) {}
    CAddrTxIndex(const CDestination& destIn, const int nHeightIn, const int nBlockSeqIn, const int nTxSeqIn, const uint256& txidIn)
      : dest(destIn), nHeightSeq(((int64)nHeightIn << 32) | ((nBlockSeqIn << 24) | nTxSeqIn)), txid(txidIn) {}

    int GetHeight() const
    {
        return (int)(nHeightSeq >> 32);
    }
    int GetSeq() const
    {
        return (int)(nHeightSeq & 0xFFFFFFFFL);
    }

    friend bool operator==(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        return (a.dest == b.dest && a.nHeightSeq == b.nHeightSeq && a.txid == b.txid);
    }
    friend bool operator!=(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        return !(a == b);
    }
    friend bool operator<(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        if (a.dest < b.dest)
        {
            return true;
        }
        else if (a.dest == b.dest)
        {
            if (a.nHeightSeq < b.nHeightSeq)
            {
                return true;
            }
            else if (a.nHeightSeq == b.nHeightSeq && a.txid < b.txid)
            {
                return true;
            }
        }
        return false;
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(dest, opt);
        s.Serialize(nHeightSeq, opt);
        s.Serialize(txid, opt);
    }
};

class CDestTxInfo
{
    friend class mtbase::CStream;

public:
    uint8 nDirection;
    uint64 nBlockNumber;
    uint256 txid;
    uint16 nTxType;
    uint64 nTimeStamp;
    CDestination destPeer;
    uint256 nAmount;
    uint256 nTxFee; // When nDirection is DXI_DIRECTION_IN, nTxFee is equal to 0

    enum
    {
        DXI_DIRECTION_NULL = 0,
        DXI_DIRECTION_IN = 1,        // local is to, peer is from
        DXI_DIRECTION_OUT = 2,       // local is from, peer is to
        DXI_DIRECTION_INOUT = 3,     // from equal to
        DXI_DIRECTION_CIN = 4,       // contract transfer
        DXI_DIRECTION_COUT = 5,      // contract transfer
        DXI_DIRECTION_CINOUT = 6,    // contract transfer from equal to
        DXI_DIRECTION_CODEREWARD = 7 // code reward
    };

    uint64 nTxIndex;

public:
    CDestTxInfo()
    {
        SetNull();
    }
    CDestTxInfo(const uint8 nDirectionIn, const uint64 nBlockNumberIn, const uint256& txidIn, const int nTxTypeIn, const uint64 nTimeStampIn, const CDestination& destPeerIn, const uint256& nAmountIn, const uint256& nTxFeeIn)
      : nDirection(nDirectionIn), nBlockNumber(nBlockNumberIn), txid(txidIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn), destPeer(destPeerIn), nAmount(nAmountIn), nTxFee(nTxFeeIn)
    {
    }

    void SetNull()
    {
        nDirection = DXI_DIRECTION_NULL;
        nBlockNumber = 0;
        txid = 0;
        nTxType = 0;
        nTimeStamp = 0;
        destPeer.SetNull();
        nAmount = 0;
        nTxFee = 0;

        nTxIndex = 0;
    }
    bool IsNull() const
    {
        return (nDirection == DXI_DIRECTION_NULL);
    };

    friend bool operator==(const CDestTxInfo& a, const CDestTxInfo& b)
    {
        return (a.nDirection == b.nDirection && a.nBlockNumber == b.nBlockNumber && a.txid == b.txid && a.nTxType == b.nTxType && a.nTimeStamp == b.nTimeStamp && a.destPeer == b.destPeer && a.nAmount == b.nAmount && a.nTxFee == b.nTxFee);
    }
    friend bool operator!=(const CDestTxInfo& a, const CDestTxInfo& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nDirection, opt);
        s.Serialize(nBlockNumber, opt);
        s.Serialize(txid, opt);
        s.Serialize(nTxType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(destPeer, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nTxFee, opt);
    }
};

class CContractTransfer
{
    friend class mtbase::CStream;

public:
    CContractTransfer()
      : nType(0) {}
    CContractTransfer(const uint8 nTypeIn, const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmountIn)
      : nType(nTypeIn), destFrom(destFromIn), destTo(destToIn), nAmount(nAmountIn) {}

    std::string GetTypeName() const
    {
        switch (nType)
        {
        case CT_CONTRACT:
            return "contract";
        case CT_VOTE:
            return "vote";
        case CT_REDEEM:
            return "redeem";
        case CT_CROSSCHAIN_TRANSFER:
            return "crosschain-transfer";
        case CT_CROSSCHAIN_DEX:
            return "crosschain-dex";
        }
        return "non";
    }

public:
    enum
    {
        CT_CONTRACT = 1,
        CT_VOTE = 2,
        CT_REDEEM = 3,
        CT_CROSSCHAIN_TRANSFER = 4,
        CT_CROSSCHAIN_DEX = 5
    };

    uint8 nType;
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const
    {
        s << nType << destFrom << destTo << nAmount.ToValidBigEndianData();
    }
    void Serialize(mtbase::CStream& s, mtbase::LoadType&)
    {
        bytes btAmount;
        s >> nType >> destFrom >> destTo >> btAmount;
        nAmount.FromValidBigEndianData(btAmount);
    }
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        mtbase::CBufStream ss;
        ss << nType << destFrom << destTo << nAmount.ToValidBigEndianData();
        serSize = ss.GetSize();
    }
};

class CTransactionLogs
{
    friend class mtbase::CStream;

public:
    uint2048 GetLogsBloom() const;
    void GetBloomDataSet(std::set<bytes>& setBloomData) const;

public:
    CDestination address;
    bytes data;
    std::vector<uint256> topics;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(address, opt);
        s.Serialize(data, opt);
        s.Serialize(topics, opt);
    }
};
using TransactionLogsVec = std::vector<CTransactionLogs>;

class CMatchLogs : public CTransactionLogs
{
public:
    CMatchLogs() {}
    CMatchLogs(const uint32 nLogIndexIn, const CTransactionLogs& logs)
      : nLogIndex(nLogIndexIn), CTransactionLogs(logs) {}

public:
    bool fRemoved = false;
    uint32 nLogIndex = 0;
};
using MatchLogsVec = std::vector<CMatchLogs>;

class CReceiptLogs
{
public:
    CReceiptLogs() {}
    CReceiptLogs(const MatchLogsVec& vLogs)
      : matchLogs(vLogs) {}

public:
    uint32 nTxIndex = 0;
    uint256 txid;
    uint64 nBlockNumber = 0;
    uint256 hashBlock;
    MatchLogsVec matchLogs;
};
using ReceiptLogsVec = std::vector<CReceiptLogs>;

class CTransactionReceipt
{
    friend class mtbase::CStream;

public:
    CTransactionReceipt()
      : nReceiptType(0), nTxIndex(0), nBlockNumber(0), nContractStatus(0) {}

    bool IsCommonReceipt() const
    {
        return (nReceiptType == RECEIPT_TYPE_COMMON);
    }
    bool IsContractReceipt() const
    {
        return (nReceiptType == RECEIPT_TYPE_CONTRACT);
    }
    void CalcLogsBloom();
    void GetBloomDataSet(std::set<bytes>& setBloomData) const;

public:
    enum
    {
        RECEIPT_TYPE_COMMON = 0,
        RECEIPT_TYPE_CONTRACT = 1,
    };

    // base
    uint8 nReceiptType;
    uint8 nContractStatus;
    uint256 nTxGasUsed;
    std::vector<CContractTransfer> vTransfer;

    // extend
    uint32 nTxIndex;
    uint256 txid;
    uint64 nBlockNumber;
    CDestination from;
    CDestination to;
    CDestination destContract;
    uint256 hashContractCode;

    uint256 nContractGasLeft;
    bytes btContractResult;
    uint256 hashStatusRoot;
    uint256 nEffectiveGasPrice;

    std::vector<CTransactionLogs> vLogs;

public:
    uint2048 nLogsBloom;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const
    {
        s << nReceiptType << nContractStatus << nTxGasUsed.ToValidBigEndianData() << vTransfer;
        if (nReceiptType == RECEIPT_TYPE_CONTRACT)
        {
            s << mtbase::CVarInt((uint64)nTxIndex);
            s << txid;
            s << mtbase::CVarInt(nBlockNumber);
            s << from.ToValidBigEndianData();
            s << to.ToValidBigEndianData();
            s << destContract.ToValidBigEndianData();
            s << hashContractCode.ToValidBigEndianData();
            s << nContractGasLeft.ToValidBigEndianData();
            s << btContractResult;
            s << hashStatusRoot.ToValidBigEndianData();
            s << nEffectiveGasPrice.ToValidBigEndianData();
            s << vLogs;
        }
    }
    void Serialize(mtbase::CStream& s, mtbase::LoadType&)
    {
        bytes btTxGasUsed;

        s >> nReceiptType >> nContractStatus >> btTxGasUsed >> vTransfer;

        nTxGasUsed.FromValidBigEndianData(btTxGasUsed);

        if (nReceiptType == RECEIPT_TYPE_CONTRACT)
        {
            mtbase::CVarInt varTxIndex;
            mtbase::CVarInt varBlockNumber;
            bytes btFrom;
            bytes btTo;
            bytes btContractAddress;
            bytes btContractCodeHash;
            bytes btContractGasLeft;
            bytes btStatusRootHash;
            bytes btEffectiveGasPrice;

            s >> varTxIndex;
            s >> txid;
            s >> varBlockNumber;
            s >> btFrom;
            s >> btTo;
            s >> btContractAddress;
            s >> btContractCodeHash;
            s >> btContractGasLeft;
            s >> btContractResult;
            s >> btStatusRootHash;
            s >> btEffectiveGasPrice;
            s >> vLogs;

            nTxIndex = varTxIndex.nValue;
            nBlockNumber = varBlockNumber.nValue;
            from.FromValidBigEndianData(btFrom);
            to.FromValidBigEndianData(btTo);
            destContract.FromValidBigEndianData(btContractAddress);
            hashContractCode.FromValidBigEndianData(btContractCodeHash);
            nContractGasLeft.FromValidBigEndianData(btContractGasLeft);
            hashStatusRoot.FromValidBigEndianData(btStatusRootHash);
            nEffectiveGasPrice.FromValidBigEndianData(btEffectiveGasPrice);
        }
    }
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        mtbase::CBufStream ss;
        ss << nReceiptType << nContractStatus << nTxGasUsed.ToValidBigEndianData() << vTransfer;
        if (nReceiptType == RECEIPT_TYPE_CONTRACT)
        {
            ss << mtbase::CVarInt((uint64)nTxIndex);
            ss << txid;
            ss << mtbase::CVarInt(nBlockNumber);
            ss << from.ToValidBigEndianData();
            ss << to.ToValidBigEndianData();
            ss << destContract.ToValidBigEndianData();
            ss << hashContractCode.ToValidBigEndianData();
            ss << nContractGasLeft.ToValidBigEndianData();
            ss << btContractResult;
            ss << hashStatusRoot.ToValidBigEndianData();
            ss << nEffectiveGasPrice.ToValidBigEndianData();
            ss << vLogs;
        }
        serSize = ss.GetSize();
    }
};
using TransactionReceipts = std::vector<CTransactionReceipt>;

class CTransactionReceiptEx : public CTransactionReceipt
{
public:
    CTransactionReceiptEx() {}
    CTransactionReceiptEx(const CTransactionReceipt& tr)
      : CTransactionReceipt(tr) {}

public:
    uint256 hashBlock;
    uint256 nBlockGasUsed;
};
using TransactionReceiptExs = std::vector<CTransactionReceiptEx>;

static const uint8 CODE_TYPE_TEMPLATE = 0;
static const uint8 CODE_TYPE_CONTRACT = 1;

class CTxContractData
{
    friend class mtbase::CStream;

public:
    CTxContractData()
      : nMuxType(0), nCompressDescribe(0), nCompressCode(0), nCompressSource(0) {}
    CTxContractData(const bytes& btCodeIn, const bool fCompress)
      : nMuxType(CF_CREATE), nCompressDescribe(0), nCompressCode(fCompress ? 1 : 0), btCode(btCodeIn), nCompressSource(0)
    {
    }
    CTxContractData(const uint256& hashContractIn)
      : nMuxType(CF_SETUP), nCompressDescribe(0), nCompressCode(0), btCode(hashContractIn.begin(), hashContractIn.end()), nCompressSource(0)
    {
    }

    void SetNull()
    {
        nMuxType = 0;
        strName.clear();
        destCodeOwner.SetNull();
        nCompressDescribe = 0;
        btDescribe.clear();
        nCompressCode = 0;
        btCode.clear();
        nCompressSource = 0;
        btSource.clear();
    }

    bool IsCreate() const
    {
        return (nMuxType == CF_CREATE);
    }
    bool IsSetup() const
    {
        return (nMuxType == CF_SETUP);
    }
    bool IsUpcode() const
    {
        return (nMuxType == CF_UPCODE);
    }

    bool PacketCompress(const uint8 nMuxTypeIn, const std::string& strTypeIn, const std::string& strNameIn, const CDestination& destCodeOwnerIn,
                        const std::string& strDescribeIn, const bytes& btCodeIn, const bytes& btSourceIn);
    void SetSetupHash(const uint256& hashContractIn);

    uint256 GetSourceCodeHash() const;
    uint256 GetContractCreateCodeHash() const;

    std::string GetType() const
    {
        return strType;
    }
    std::string GetName() const
    {
        return strName;
    }
    CDestination GetCodeOwner() const
    {
        return destCodeOwner;
    }
    std::string GetDescribe() const;
    bytes GetCode() const;
    bytes GetSource() const;

    bool UncompressDescribe();
    bool UncompressCode();
    bool UncompressSource();
    bool UncompressAll();

public:
    enum
    {
        CF_MIN = 0,
        CF_CREATE = 1,
        CF_SETUP = 2,
        CF_UPCODE = 3,
        CF_MAX
    };

    uint8 nMuxType;
    std::string strType;
    std::string strName;
    CDestination destCodeOwner;
    uint8 nCompressDescribe;
    bytes btDescribe;
    uint8 nCompressCode;
    bytes btCode; // contract create code
    uint8 nCompressSource;
    bytes btSource; // source code

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nMuxType, opt);
        s.Serialize(strType, opt);
        s.Serialize(strName, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(nCompressDescribe, opt);
        s.Serialize(btDescribe, opt);
        s.Serialize(nCompressCode, opt);
        s.Serialize(btCode, opt);
        s.Serialize(nCompressSource, opt);
        s.Serialize(btSource, opt);
    }
};

class CContractDestCodeContext
{
    friend class mtbase::CStream;

public:
    CContractDestCodeContext() {}
    CContractDestCodeContext(const uint256& hashContractCreateCodeIn, const uint256& hashContractRunCodeIn, const CDestination& destCodeOwnerIn)
      : hashContractCreateCode(hashContractCreateCodeIn), hashContractRunCode(hashContractRunCodeIn), destCodeOwner(destCodeOwnerIn) {}

public:
    uint256 hashContractCreateCode;
    uint256 hashContractRunCode;
    CDestination destCodeOwner;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashContractCreateCode, opt);
        s.Serialize(hashContractRunCode, opt);
        s.Serialize(destCodeOwner, opt);
    }
};

class CContractSourceCodeContext
{
    friend class mtbase::CStream;

public:
    CContractSourceCodeContext() {}
    CContractSourceCodeContext(const std::string& strNameIn, const CDestination& destCodeOwnerIn, const uint32 nFileIn, const uint32 nOffsetIn)
      : strName(strNameIn), destCodeOwner(destCodeOwnerIn)
    {
        vTxPos.push_back(std::make_pair(nFileIn, nOffsetIn));
    }
    void Clear()
    {
        strName = "";
        destCodeOwner.SetNull();
        vTxPos.clear();
    }
    void AddPos(const std::string& strNameIn, const CDestination& destCodeOwnerIn, const uint32 nFileIn, const uint32 nOffsetIn)
    {
        strName = strNameIn;
        destCodeOwner = destCodeOwnerIn;
        vTxPos.push_back(std::make_pair(nFileIn, nOffsetIn));
    }
    bool GetLastPos(uint32& nFile, uint32& nOffset)
    {
        if (vTxPos.empty())
        {
            return false;
        }
        auto& vd = vTxPos[vTxPos.size() - 1];
        nFile = vd.first;
        nOffset = vd.second;
        return true;
    }
    std::string GetName() const
    {
        return strName;
    }

public:
    std::string strName;
    CDestination destCodeOwner;
    std::vector<std::pair<uint32, uint32>> vTxPos;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(vTxPos, opt);
    }
};

class CContractCreateCodeContext
{
    friend class mtbase::CStream;

public:
    CContractCreateCodeContext() {}
    CContractCreateCodeContext(const std::string& strTypeIn, const std::string& strNameIn, const std::string& strDescribeIn,
                               const CDestination& destCodeOwnerIn, const bytes& btCreateCodeIn, const uint256& txidCreateIn,
                               const uint256& hashSourceCodeIn, const uint256& hashRuntimeCodeIn)
      : strType(strTypeIn), strName(strNameIn), strDescribe(strDescribeIn), destCodeOwner(destCodeOwnerIn),
        btCreateCode(btCreateCodeIn), txidCreate(txidCreateIn), hashSourceCode(hashSourceCodeIn), hashRuntimeCode(hashRuntimeCodeIn) {}

    uint256 GetContractCreateCodeHash() const
    {
        if (btCreateCode.empty())
        {
            return 0;
        }
        return crypto::CryptoKeccakHash(btCreateCode.data(), btCreateCode.size());
    }

public:
    std::string strType;
    std::string strName;
    std::string strDescribe;
    CDestination destCodeOwner;
    bytes btCreateCode;
    uint256 txidCreate;
    uint256 hashSourceCode;
    uint256 hashRuntimeCode;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(strType, opt);
        s.Serialize(strName, opt);
        s.Serialize(strDescribe, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(btCreateCode, opt);
        s.Serialize(txidCreate, opt);
        s.Serialize(hashSourceCode, opt);
        s.Serialize(hashRuntimeCode, opt);
    }
};

class CContractRunCodeContext
{
    friend class mtbase::CStream;

public:
    CContractRunCodeContext() {}
    CContractRunCodeContext(const uint256& hashContractCreateCodeIn, const bytes& btContractRunCodeIn)
      : hashContractCreateCode(hashContractCreateCodeIn), btContractRunCode(btContractRunCodeIn) {}

    uint256 GetContractRunCodeHash() const
    {
        if (btContractRunCode.empty())
        {
            return 0;
        }
        return crypto::CryptoKeccakHash(btContractRunCode.data(), btContractRunCode.size());
    }

public:
    uint256 hashContractCreateCode;
    bytes btContractRunCode;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashContractCreateCode, opt);
        s.Serialize(btContractRunCode, opt);
    }
};

class CTemplateContext
{
    friend class mtbase::CStream;

public:
    CTemplateContext() {}
    CTemplateContext(const std::string& strNameIn, const bytes& btTemplateDataIn)
      : strName(strNameIn), btTemplateData(btTemplateDataIn) {}

    std::string GetName() const;
    bytes GetTemplateData() const;
    uint8 GetTemplateType() const;
    CTemplateId GetTemplateId() const;
    CDestination GetTemplateAddress() const;

public:
    std::string strName;
    bytes btTemplateData;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(btTemplateData, opt);
    }
};

class CContractCodeContext
{
public:
    CContractCodeContext() {}

public:
    uint256 hashCode;
    uint256 hashSource;
    std::string strType;
    std::string strName;
    CDestination destOwner;
    std::string strDescribe;
    uint256 hashCreateTxid;
    int nStatus;
};

class CLogsFilter
{
public:
    CLogsFilter() {}
    CLogsFilter(const uint256& hashFromBlockIn, const uint256& hashToBlockIn)
      : hashFromBlock(hashFromBlockIn), hashToBlock(hashToBlockIn) {}

    CLogsFilter addAddress(const CDestination& dest)
    {
        setAddress.insert(dest);
        return *this;
    }
    CLogsFilter addTopic(unsigned _index, uint256 const& _t)
    {
        if (_index < MAX_LOGS_FILTER_TOPIC_COUNT)
        {
            arrayTopics[_index].insert(_t);
        }
        return *this;
    }
    bool isTopicEmpty(unsigned _index) const
    {
        if (_index < MAX_LOGS_FILTER_TOPIC_COUNT)
        {
            return arrayTopics[_index].empty();
        }
        return true;
    }
    CLogsFilter withFromBlock(const uint256& from)
    {
        hashFromBlock = from;
        return *this;
    }
    CLogsFilter withToBlock(const uint256& to)
    {
        hashToBlock = to;
        return *this;
    }

    void matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const;

public:
    enum
    {
        MAX_LOGS_FILTER_TOPIC_COUNT = 8
    };

    uint256 hashFromBlock;
    uint256 hashToBlock;
    std::set<CDestination> setAddress;
    std::array<std::set<uint256>, MAX_LOGS_FILTER_TOPIC_COUNT> arrayTopics;
};

class CFilterId
{
public:
    CFilterId() {}

    static uint8 GetFilterType(const uint256& nFilterId)
    {
        return *(nFilterId.begin());
    }
    static bool isLogsFilter(const uint256& nFilterId)
    {
        return (GetFilterType(nFilterId) == FILTERID_HEADER_LOGS);
    }
    static bool isBlockFilter(const uint256& nFilterId)
    {
        return (GetFilterType(nFilterId) == FILTERID_HEADER_BLOCK);
    }
    static bool isTxFilter(const uint256& nFilterId)
    {
        return (GetFilterType(nFilterId) == FILTERID_HEADER_TX);
    }

    uint256 CreateFilterId(const uint8 nFilterType, const uint64 nId, const uint256& hash);
    uint256 CreateLogsFilterId(const uint256& hash)
    {
        return CreateFilterId(FILTERID_HEADER_LOGS, nLogsFilterIdCreate++, hash);
    }
    uint256 CreateBlockFilterId(const uint256& hash)
    {
        return CreateFilterId(FILTERID_HEADER_BLOCK, nBlockFilterIdCreate++, hash);
    }
    uint256 CreateTxFilterId(const uint256& hash)
    {
        return CreateFilterId(FILTERID_HEADER_TX, nTxFilterIdCreate++, hash);
    }

protected:
    uint64 nLogsFilterIdCreate = 1;
    uint64 nBlockFilterIdCreate = 1;
    uint64 nTxFilterIdCreate = 1;

public:
    enum
    {
        FILTERID_HEADER_LOGS = 1,
        FILTERID_HEADER_BLOCK = 2,
        FILTERID_HEADER_TX = 3,
    };
};

/////////////////////////////
// WS SUBSCRIBE TYPE

enum
{
    WSCS_SUBS_TYPE_NEW_BLOCK = 1,
    WSCS_SUBS_TYPE_LOGS = 2,
    WSCS_SUBS_TYPE_NEW_PENDING_TX = 3,
    WSCS_SUBS_TYPE_SYNCING = 4,
};

class CDexCoinPairContext
{
public:
    CDexCoinPairContext()
      : nChainIdMin(0), nChainIdMax(0) {}
    CDexCoinPairContext(const std::string& strCoinSymbolMinIn, const uint32 nChainIdMinIn,
                        const std::string& strCoinSymbolMaxIn, const uint32 nChainIdMaxIn)
      : strCoinSymbolMin(strCoinSymbolMinIn), nChainIdMin(nChainIdMinIn),
        strCoinSymbolMax(strCoinSymbolMaxIn), nChainIdMax(nChainIdMaxIn) {}

public:
    std::string strCoinSymbolMin;
    uint32 nChainIdMin;
    std::string strCoinSymbolMax;
    uint32 nChainIdMax;
};

class CDexOrderHeader
{
    friend class mtbase::CStream;

public:
    CDexOrderHeader(const CChainId nChainIdIn, const CDestination& destOrderIn, const std::string& strSymbolOwner, const std::string& strSymbolPeer, const uint64 nOrderNumberIn)
      : nChainId(nChainIdIn), destOrder(destOrderIn), nOrderNumber(nOrderNumberIn)
    {
        nOwnerCoinFlag = GetOwnerCoinFlagStatic(strSymbolOwner, strSymbolPeer);
        hashCoinPair = GetCoinPairHashStatic(strSymbolOwner, strSymbolPeer);
    }
    CDexOrderHeader(const CChainId nChainIdIn, const CDestination& destOrderIn, const uint256& hashCoinPairIn, const uint8 nOwnerCoinFlagIn, const uint64 nOrderNumberIn)
      : nChainId(nChainIdIn), destOrder(destOrderIn), hashCoinPair(hashCoinPairIn), nOwnerCoinFlag(nOwnerCoinFlagIn), nOrderNumber(nOrderNumberIn)
    {
    }

    CChainId GetChainId() const
    {
        return nChainId;
    }
    const CDestination& GetOrderAddress() const
    {
        return destOrder;
    }
    const uint256& GetCoinPairHash() const
    {
        return hashCoinPair;
    }
    uint8 GetOwnerCoinFlag() const
    {
        return nOwnerCoinFlag;
    }
    uint64 GetOrderNumber() const
    {
        return nOrderNumber;
    }
    uint256 GetDexOrderHash() const
    {
        return GetDexOrderHashStatic(nChainId, destOrder, hashCoinPair, nOwnerCoinFlag, nOrderNumber);
    }

    static inline uint256 GetCoinPairHashStatic(const std::string& strSymbol1, const std::string& strSymbol2)
    {
        mtbase::CBufStream ss;
        if (strSymbol1 < strSymbol2)
        {
            ss << strSymbol1 << strSymbol2;
        }
        else
        {
            ss << strSymbol2 << strSymbol1;
        }
        return crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());
    }
    static inline uint8 GetOwnerCoinFlagStatic(const std::string& strSymbolOwner, const std::string& strSymbolPeer)
    {
        return (strSymbolOwner < strSymbolPeer ? 0 : 1);
    }
    static inline uint256 GetOrderRandomHashStatic(const uint256& hashBlockIn, const uint256& hashDexOrderIn)
    {
        mtbase::CBufStream ss;
        ss << hashBlockIn << hashDexOrderIn;
        return crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());
    }
    static inline uint256 GetDexOrderHashStatic(const CChainId nChainIdIn, const CDestination& destOrderIn, const uint256& hashCoinPairIn, const uint8 nOwnerCoinFlagIn, const uint64 nOrderNumberIn)
    {
        mtbase::CBufStream ss;
        ss << nChainIdIn << destOrderIn << hashCoinPairIn << nOwnerCoinFlagIn << nOrderNumberIn;
        return crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());
    }
    static inline std::string GetSellCoinSymbolStatic(const std::string& strSymbol1, const std::string& strSymbol2)
    {
        return std::min(strSymbol1, strSymbol2);
    }
    static inline std::string GetBuyCoinSymbolStatic(const std::string& strSymbol1, const std::string& strSymbol2)
    {
        return std::max(strSymbol1, strSymbol2);
    }

protected:
    CChainId nChainId;
    CDestination destOrder;
    uint256 hashCoinPair;
    uint8 nOwnerCoinFlag; // 0: min, 1: max
    uint64 nOrderNumber;

public:
    friend inline bool operator==(const CDexOrderHeader& a, const CDexOrderHeader& b)
    {
        return (a.nChainId == b.nChainId && a.destOrder == b.destOrder && a.hashCoinPair == b.hashCoinPair && a.nOwnerCoinFlag == b.nOwnerCoinFlag && a.nOrderNumber == b.nOrderNumber);
    }
    friend inline bool operator!=(const CDexOrderHeader& a, const CDexOrderHeader& b)
    {
        return (!(a == b));
    }
    friend inline bool operator<(const CDexOrderHeader& a, const CDexOrderHeader& b)
    {
        if (a.nChainId < b.nChainId)
        {
            return true;
        }
        if (a.nChainId == b.nChainId)
        {
            if (a.destOrder < b.destOrder)
            {
                return true;
            }
            if (a.destOrder == b.destOrder)
            {
                if (a.hashCoinPair < b.hashCoinPair)
                {
                    return true;
                }
                if (a.hashCoinPair == b.hashCoinPair)
                {
                    if (a.nOwnerCoinFlag < b.nOwnerCoinFlag)
                    {
                        return true;
                    }
                    if (a.nOwnerCoinFlag == b.nOwnerCoinFlag && a.nOrderNumber < b.nOrderNumber)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nChainId, opt);
        s.Serialize(destOrder, opt);
        s.Serialize(hashCoinPair, opt);
        s.Serialize(nOwnerCoinFlag, opt);
        s.Serialize(nOrderNumber, opt);
    }
};

class CDexOrderBody
{
    friend class mtbase::CStream;

public:
    CDexOrderBody()
      : nCoinAtChainIdPeer(0) {}
    CDexOrderBody(const std::string& strCoinSymbolOwnerIn, const std::string& strCoinSymbolPeerIn, const CChainId nCoinAtChainIdPeerIn, const uint256& nOrderAmountIn, const uint256& nOrderPriceIn)
      : strCoinSymbolOwner(strCoinSymbolOwnerIn), strCoinSymbolPeer(strCoinSymbolPeerIn), nCoinAtChainIdPeer(nCoinAtChainIdPeerIn), nOrderAmount(nOrderAmountIn), nOrderPrice(nOrderPriceIn) {}

    void SetNull()
    {
        strCoinSymbolOwner.clear();
        strCoinSymbolPeer.clear();
        nCoinAtChainIdPeer = 0;
        nOrderAmount = 0;
        nOrderPrice = 0;
    }
    void Save(bytes& btData) const
    {
        mtbase::CBufStream ss;
        ss << *this;
        ss.GetData(btData);
    }
    bool Load(const bytes& btData)
    {
        mtbase::CBufStream ss(btData);
        try
        {
            ss >> *this;
        }
        catch (const std::exception& e)
        {
            return false;
        }
        return true;
    }
    uint256 GetCoinPairHash() const
    {
        return CDexOrderHeader::GetCoinPairHashStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    }
    uint8 GetOwnerCoinFlag() const
    {
        return CDexOrderHeader::GetOwnerCoinFlagStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    }
    std::string GetSellCoinSymbol() const
    {
        return CDexOrderHeader::GetSellCoinSymbolStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    }
    std::string GetBuyCoinSymbol() const
    {
        return CDexOrderHeader::GetBuyCoinSymbolStatic(strCoinSymbolOwner, strCoinSymbolPeer);
    }
    bool IsOwnerSell() const
    {
        return (CDexOrderHeader::GetSellCoinSymbolStatic(strCoinSymbolOwner, strCoinSymbolPeer) == strCoinSymbolOwner);
    }
    bool IsOwnerBuy() const
    {
        return (CDexOrderHeader::GetBuyCoinSymbolStatic(strCoinSymbolOwner, strCoinSymbolPeer) == strCoinSymbolOwner);
    }

public:
    std::string strCoinSymbolOwner;
    std::string strCoinSymbolPeer;
    CChainId nCoinAtChainIdPeer;
    uint256 nOrderAmount;
    uint256 nOrderPrice;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const
    {
        s << strCoinSymbolOwner << strCoinSymbolPeer << nCoinAtChainIdPeer << nOrderAmount.ToValidBigEndianData() << nOrderPrice.ToValidBigEndianData();
    }
    void Serialize(mtbase::CStream& s, mtbase::LoadType&)
    {
        bytes btOrderAmount;
        bytes btOrderPrice;
        s >> strCoinSymbolOwner >> strCoinSymbolPeer >> nCoinAtChainIdPeer >> btOrderAmount >> btOrderPrice;
        nOrderAmount.FromValidBigEndianData(btOrderAmount);
        nOrderPrice.FromValidBigEndianData(btOrderPrice);
    }
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        mtbase::CBufStream ss;
        ss << strCoinSymbolOwner << strCoinSymbolPeer << nCoinAtChainIdPeer << nOrderAmount.ToValidBigEndianData() << nOrderPrice.ToValidBigEndianData();
        serSize = ss.GetSize();
    }
};

class CDexOrderSave
{
    friend class mtbase::CStream;

public:
    CDexOrderSave()
      : nAtChainId(0) {}
    CDexOrderSave(const CDexOrderBody& dexOrderIn, const CChainId nAtChainIdIn, const uint256& hashAtBlockIn)
      : dexOrder(dexOrderIn), nAtChainId(nAtChainIdIn), hashAtBlock(hashAtBlockIn) {}

public:
    CDexOrderBody dexOrder;
    CChainId nAtChainId;
    uint256 hashAtBlock;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(dexOrder, opt);
        s.Serialize(nAtChainId, opt);
        s.Serialize(hashAtBlock, opt);
    }
};

class CCompDexOrderRecord
{
    friend class mtbase::CStream;

public:
    CCompDexOrderRecord(const CDestination& destPeerOrderIn, const uint256& nCompleteAmountIn, const uint256& nCompletePriceIn)
      : destPeerOrder(destPeerOrderIn), nCompleteAmount(nCompleteAmountIn), nCompletePrice(nCompletePriceIn) {}

    void SetNull()
    {
        destPeerOrder.SetNull();
        nCompleteAmount = 0;
        nCompletePrice = 0;
    }
    void Save(bytes& btData) const
    {
        mtbase::CBufStream ss;
        ss << *this;
        ss.GetData(btData);
    }
    bool Load(const bytes& btData)
    {
        mtbase::CBufStream ss(btData);
        try
        {
            ss >> *this;
        }
        catch (const std::exception& e)
        {
            return false;
        }
        return true;
    }

public:
    CDestination destPeerOrder;
    uint256 nCompleteAmount;
    uint256 nCompletePrice;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const
    {
        s << destPeerOrder << nCompleteAmount.ToValidBigEndianData() << nCompletePrice.ToValidBigEndianData();
    }
    void Serialize(mtbase::CStream& s, mtbase::LoadType&)
    {
        bytes btCompleteAmount;
        bytes btCompletePrice;
        s >> destPeerOrder >> btCompleteAmount >> btCompletePrice;
        nCompleteAmount.FromValidBigEndianData(btCompleteAmount);
        nCompletePrice.FromValidBigEndianData(btCompletePrice);
    }
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        mtbase::CBufStream ss;
        ss << destPeerOrder << nCompleteAmount.ToValidBigEndianData() << nCompletePrice.ToValidBigEndianData();
        serSize = ss.GetSize();
    }
};

class CDexOrderData
{
    friend class mtbase::CStream;

public:
    CDexOrderData()
      : nOrderNumber(0), nCompleteCount(0) {}
    CDexOrderData(const CDestination& destOrderIn, const uint64 nOrderNumberIn, const CDexOrderBody& dexOrderIn,
                  const uint256& hashOrderAtBlockIn, const uint256& nCompleteAmountIn, const uint64 nCompleteCountIn)
      : destOrder(destOrderIn), nOrderNumber(nOrderNumberIn), dexOrder(dexOrderIn), hashOrderAtBlock(hashOrderAtBlockIn),
        nCompleteAmount(nCompleteAmountIn), nCompleteCount(nCompleteCountIn) {}

public:
    CDestination destOrder;
    uint64 nOrderNumber;
    CDexOrderBody dexOrder;
    uint256 hashOrderAtBlock;
    uint256 nCompleteAmount;
    uint64 nCompleteCount;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(destOrder, opt);
        s.Serialize(nOrderNumber, opt);
        s.Serialize(dexOrder, opt);
        s.Serialize(hashOrderAtBlock, opt);
        s.Serialize(nCompleteAmount, opt);
        s.Serialize(nCompleteCount, opt);
    }
};
typedef std::shared_ptr<CDexOrderData> SHP_DEX_ORDER_DATA;
#define MAKE_SHARED_DEX_ORDER_DATA std::make_shared<CDexOrderData>

class CMatchOrderRecord
{
public:
    CMatchOrderRecord() {}

public:
    CDestination destSellOrder;
    uint64 nSellOrderNumber;
    uint256 nSellCompleteAmount;
    uint256 nSellCompletePrice;
    CChainId nSellOrderAtChainId;

    CDestination destBuyOrder;
    uint64 nBuyOrderNumber;
    uint256 nBuyCompleteAmount;
    uint256 nBuyCompletePrice;
    CChainId nBuyOrderAtChainId;

    uint256 nCompletePrice;
};

class CMatchOrderResult
{
public:
    CMatchOrderResult() {}

public:
    std::string strCoinSymbolSell;
    std::string strCoinSymbolBuy;
    uint256 nSellPriceAnchor;

    std::vector<CMatchOrderRecord> vMatchOrderRecord;
};

class CMsOrder
{
public:
    CMsOrder()
      : nOrderNumber(0), nAtHeight(0), nAtSlot(0) {}

public:
    uint256 nPrice;
    uint256 nOrderAmount;
    uint256 nDealAmount;
    CDestination destOrder;
    uint64 nOrderNumber;
    uint256 nOriOrderAmount;
    uint32 nAtHeight;
    uint16 nAtSlot;
};

class CRealtimeDexOrder
{
public:
    CRealtimeDexOrder()
      : nSellChainId(0), nBuyChainId(0), nMaxMatchHeight(0), nMaxMatchSlot(0) {}

public:
    std::string strCoinSymbolSell;
    std::string strCoinSymbolBuy;
    uint256 nSellPriceAnchor;
    uint256 nPrevCompletePrice;
    CChainId nSellChainId;
    CChainId nBuyChainId;
    uint32 nMaxMatchHeight;
    uint16 nMaxMatchSlot;

    std::vector<CMsOrder> vSell;
    std::vector<CMsOrder> vBuy;
};

} // namespace metabasenet

#endif //COMMON_TRANSACTION_H

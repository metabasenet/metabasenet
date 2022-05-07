// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TRANSACTION_H
#define COMMON_TRANSACTION_H

#include <set>
#include <stream/stream.h>

#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "uint256.h"

class CTransaction
{
    friend class hcode::CStream;

public:
    uint16 nVersion;
    uint16 nType;
    uint32 nTimeStamp;
    uint256 hashFork;
    uint64 nTxNonce;
    CDestination from;
    CDestination to;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGasLimit;
    std::map<uint16, bytes> mapTxData;
    bytes vchSig;
    enum
    {
        TX_TOKEN = 0x0000,         // normal Tx 0
        TX_DEFI_RELATION = 0x0001, // DeFi invite relation Tx

        TX_CERT = 0xff00,        // Enroll Tx 65280
        TX_GENESIS = 0x0100,     // 256
        TX_STAKE = 0x0200,       // DPoS mint Tx 512
        TX_WORK = 0x0300,        // PoW mint Tx 768
        TX_DEFI_REWARD = 0x0400, // DeFi reward Tx
    };
    enum
    {
        DF_COMMON = 0,
        DF_TXT = 1,
        DF_JSON = 2,
        DF_XML = 3,
        DF_PROTOBUF = 4,

        DF_TEMPLATEDATA = 0x0101,
        DF_CONTRACTCODE = 0x0102,
        DF_CONTRACTPARAM = 0x0103,
        DF_FORKDATA = 0x0104,
        DF_MINTCOIN = 0x0105,
        DF_MINTREWARD = 0x0106,
        DF_VOTEREWARD = 0x0107,
        DF_CERTTXDATA = 0x0108,
        DF_FORKNAME = 0x0109,
        DF_FORKNONCE = 0x010A,
        DF_ACTIVATECODE = 0x010B,
    };
    CTransaction()
    {
        SetNull();
    }
    virtual ~CTransaction() = default;
    virtual void SetNull()
    {
        nVersion = 1;
        nType = 0;
        nTimeStamp = 0;
        hashFork = 0;
        nTxNonce = 0;
        from.SetNull();
        to.SetNull();
        nAmount = 0;
        nGasPrice = 0;
        nGasLimit = 0;
        mapTxData.clear();
        vchSig.clear();
    }
    bool IsNull() const
    {
        return (to.IsNull());
    }
    bool IsMintTx() const
    {
        return (nType == TX_GENESIS || nType == TX_STAKE || nType == TX_WORK);
    }
    bool IsNoMintRewardTx() const
    {
        return (nType == TX_DEFI_REWARD);
    }
    bool IsRewardTx() const
    {
        return (IsMintTx() || IsNoMintRewardTx());
    }
    static bool IsUserTx(const uint16 nTxType)
    {
        return (nTxType == TX_TOKEN || nTxType == TX_DEFI_RELATION);
    }
    static std::string GetTypeStringStatic(const uint16 nTxType)
    {
        if (nTxType == TX_TOKEN)
            return std::string("token");
        else if (nTxType == TX_CERT)
            return std::string("certification");
        else if (nTxType == TX_GENESIS)
            return std::string("genesis");
        else if (nTxType == TX_STAKE)
            return std::string("stake");
        else if (nTxType == TX_WORK)
            return std::string("work");
        else if (nTxType == TX_DEFI_REWARD)
            return std::string("defi-reward");
        else if (nTxType == TX_DEFI_RELATION)
            return std::string("defi-relation");
        return std::string("undefined");
    }
    std::string GetTypeString() const
    {
        return GetTypeStringStatic(nType);
    }
    int64 GetTxTime() const
    {
        return ((int64)nTimeStamp);
    }
    uint256 GetTxFee() const
    {
        return nGasPrice * nGasLimit;
    }
    uint256 GetHash() const
    {
        hcode::CBufStream ss;
        ss << (*this);

        uint256 hash = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());

        return uint256(GetForkSn(hashFork), nTimeStamp, hash);
    }
    uint256 GetSignatureHash() const
    {
        hcode::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << hashFork << nTxNonce << from << to << nAmount << nGasPrice << nGasLimit << mapTxData;
        return metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }
    void AddTxData(const uint16 nTypeIn, const bytes& btDataIn)
    {
        mapTxData[nTypeIn] = btDataIn;
    }
    bool GetTxData(const uint16 nTypeIn, bytes& btDataOut) const
    {
        auto it = mapTxData.find(nTypeIn);
        if (it != mapTxData.end())
        {
            btDataOut = it->second;
            return true;
        }
        return false;
    }
    std::size_t GetTxDataCount() const
    {
        return mapTxData.size();
    }
    std::size_t GetTxDataSize() const
    {
        hcode::CBufStream ss;
        ss << mapTxData;
        return ss.GetSize();
    }
    uint256 GetTxDataGas() const
    {
        hcode::CBufStream ss;
        ss << mapTxData;
        std::size_t nTxDataSize = ss.GetSize();
        if (nTxDataSize <= 1)
        {
            return 0;
        }
        return (DATA_GAS_PER_BYTE * nTxDataSize);
    }
    uint256 GetRunGasLimit() const
    {
        uint256 nTxMinGas = TX_BASE_GAS + GetTxDataGas();
        if (nGasLimit < nTxMinGas)
        {
            return 0;
        }
        return (nGasLimit - nTxMinGas);
    }
    void GetTxData(bytes& btData) const
    {
        hcode::CBufStream ss;
        ss << mapTxData;
        ss.GetData(btData);
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return (a.nVersion == b.nVersion
                && a.nType == b.nType
                && a.nTimeStamp == b.nTimeStamp
                && a.hashFork == b.hashFork
                && a.nTxNonce == b.nTxNonce
                && a.from == b.from
                && a.to == b.to
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

    static uint16 GetForkSn(const uint256& hashFork)
    {
        uint16 n = 0;
        uint16* p = (uint16*)(hashFork.begin());
        for (int i = 0; i < hashFork.size() / 2; i++)
        {
            n ^= p[i];
        }
        return n;
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nVersion, opt);
        s.Serialize(nType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(hashFork, opt);
        s.Serialize(nTxNonce, opt);
        s.Serialize(from, opt);
        s.Serialize(to, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nGasPrice, opt);
        s.Serialize(nGasLimit, opt);
        s.Serialize(mapTxData, opt);
        s.Serialize(vchSig, opt);
    }
};

class CTxIndex
{
    friend class hcode::CStream;

public:
    uint32 nBlockHeight;
    uint64 nBlockNumber;
    uint16 nTxSeq;
    uint32 nFile;
    uint32 nOffset;

public:
    CTxIndex()
    {
        SetNull();
    }
    CTxIndex(const uint32 nBlockHeightIn, const uint64 nBlockNumberIn, const uint16 nTxSeqIn, const uint32 nFileIn, const uint32 nOffsetIn)
    {
        nBlockHeight = nBlockHeightIn;
        nBlockNumber = nBlockNumberIn;
        nTxSeq = nTxSeqIn;
        nFile = nFileIn;
        nOffset = nOffsetIn;
    }
    void SetNull()
    {
        nBlockHeight = 0;
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
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nBlockHeight, opt);
        s.Serialize(nBlockNumber, opt);
        s.Serialize(nTxSeq, opt);
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
    }
};

class CTxFilter
{
public:
    std::set<CDestination> setDest;

public:
    CTxFilter(const CDestination& destIn)
    {
        setDest.insert(destIn);
    }
    CTxFilter(const std::set<CDestination>& setDestIn)
      : setDest(setDestIn) {}
    virtual bool FoundTx(const uint256& hashFork, const CTransaction& tx) = 0;
};

class CTxId : public uint256
{
public:
    CTxId(const uint256& txid = uint256())
      : uint256(txid) {}
    uint16 GetForkSn() const
    {
        return (uint16)(Get32(7) >> 16);
    }
    int64 GetTxTime() const
    {
        return ((Get32(7) << 16) | (Get32(6) >> 16));
    }
};

class CTxInfo
{
public:
    CTxInfo() {}
    CTxInfo(const uint256& txidIn, const uint256& hashForkIn, const int nTxTypeIn, const uint32 nTimeStampIn, const uint64 nTxNonceIn,
            const int nBlockHeightIn, const uint64 nTxSeqIn, const CDestination& destFromIn, const CDestination& destToIn,
            const uint256& nAoumtIn, const uint256& nGasPriceIn, const uint256& nGasIn, const uint64 nSizeIn)
      : txid(txidIn), hashFork(hashForkIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn), nTxNonce(nTxNonceIn),
        nBlockHeight(nBlockHeightIn), nTxSeq(nTxSeqIn), destFrom(destFromIn), destTo(destToIn),
        nAmount(nAoumtIn), nGasPrice(nGasPriceIn), nGas(nGasIn), nSize(nSizeIn) {}

    bool IsRewardTx() const
    {
        return (nTxType == CTransaction::TX_GENESIS || nTxType == CTransaction::TX_STAKE
                || nTxType == CTransaction::TX_WORK || nTxType == CTransaction::TX_DEFI_REWARD);
    }

public:
    uint256 txid;
    uint256 hashFork;
    int nTxType;
    uint32 nTimeStamp;
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
    friend class hcode::CStream;

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
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(dest, opt);
        s.Serialize(nHeightSeq, opt);
        s.Serialize(txid, opt);
    }
};

class CDestTxInfo
{
    friend class hcode::CStream;

public:
    uint8 nDirection;
    uint64 nBlockNumber;
    uint256 txid;
    uint16 nTxType;
    uint32 nTimeStamp;
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
    CDestTxInfo(const uint8 nDirectionIn, const uint64 nBlockNumberIn, const uint256& txidIn, const int nTxTypeIn, const uint32 nTimeStampIn, const CDestination& destPeerIn, const uint256& nAmountIn, const uint256& nTxFeeIn)
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
    void Serialize(hcode::CStream& s, O& opt)
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
public:
    CContractTransfer() {}
    CContractTransfer(const CDestination& destFromIn, const CDestination& destToIn, const uint256& nAmountIn)
      : destFrom(destFromIn), destTo(destToIn), nAmount(nAmountIn) {}

public:
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
};

class CTransactionLogs
{
    friend class hcode::CStream;

public:
    bytes address;
    bytes data;
    std::vector<bytes> topics;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(address, opt);
        s.Serialize(data, opt);
        s.Serialize(topics, opt);
    }
};

class CTransactionReceipt
{
    friend class hcode::CStream;

public:
    int nTxIndex;
    uint64 nBlockNumber;
    CDestination from;
    CDestination to;
    uint256 nTxGasUsed;
    CDestination destContract;
    uint256 hashContractCode;

    int nContractStatus;
    uint256 nContractGasLeft;
    bytes btContractResult;

    CTransactionLogs logs;
    uint2048 nLogsBloom;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nTxIndex, opt);
        s.Serialize(nBlockNumber, opt);
        s.Serialize(from, opt);
        s.Serialize(to, opt);
        s.Serialize(nTxGasUsed, opt);
        s.Serialize(destContract, opt);
        s.Serialize(hashContractCode, opt);
        s.Serialize(nContractStatus, opt);
        s.Serialize(nContractGasLeft, opt);
        s.Serialize(btContractResult, opt);
        s.Serialize(logs, opt);
        s.Serialize(nLogsBloom, opt);
    }
};

class CTxContractData
{
    friend class hcode::CStream;

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
    uint256 GetWasmCreateCodeHash() const;

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
    bytes btCode; // wasm create code
    uint8 nCompressSource;
    bytes btSource; // source code

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
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
    friend class hcode::CStream;

public:
    CContractDestCodeContext() {}
    CContractDestCodeContext(const uint256& hashWasmCreateCodeIn, const uint256& hashWasmRunCodeIn)
      : hashWasmCreateCode(hashWasmCreateCodeIn), hashWasmRunCode(hashWasmRunCodeIn) {}

public:
    uint256 hashWasmCreateCode;
    uint256 hashWasmRunCode;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(hashWasmCreateCode, opt);
        s.Serialize(hashWasmRunCode, opt);
    }
};

class CContracSourceCodeContext
{
    friend class hcode::CStream;

public:
    CContracSourceCodeContext() {}
    CContracSourceCodeContext(const std::string& strNameIn, const CDestination& destCodeOwnerIn, const uint32 nFileIn, const uint32 nOffsetIn)
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
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(vTxPos, opt);
    }
};

class CWasmCreateCodeContext
{
    friend class hcode::CStream;

public:
    CWasmCreateCodeContext() {}
    CWasmCreateCodeContext(const std::string& strNameIn, const CDestination& destCodeOwnerIn, const uint32 nFileIn, const uint32 nOffsetIn, const int nStatusIn)
      : strName(strNameIn), destCodeOwner(destCodeOwnerIn), nFile(nFileIn), nOffset(nOffsetIn), nStatus(nStatusIn) {}

    std::string GetName() const
    {
        return strName;
    }

public:
    enum
    {
        ST_INITIALIZE = 0,
        ST_ACTIVATED = 1
    };

    std::string strName;
    CDestination destCodeOwner;
    uint32 nFile;
    uint32 nOffset;
    int nStatus;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
        s.Serialize(nStatus, opt);
    }
};

class CWasmRunCodeContext
{
    friend class hcode::CStream;

public:
    CWasmRunCodeContext() {}
    CWasmRunCodeContext(const uint256& hashWasmCreateCodeIn, const bytes& btWasmRunCodeIn)
      : hashWasmCreateCode(hashWasmCreateCodeIn), btWasmRunCode(btWasmRunCodeIn) {}

    uint256 GetWasmRunCodeHash() const
    {
        if (btWasmRunCode.empty())
        {
            return 0;
        }
        return metabasenet::crypto::CryptoHash(btWasmRunCode.data(), btWasmRunCode.size());
    }

public:
    uint256 hashWasmCreateCode;
    bytes btWasmRunCode;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(hashWasmCreateCode, opt);
        s.Serialize(btWasmRunCode, opt);
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

class CTemplateAddressContext
{
    friend class hcode::CStream;

public:
    CTemplateAddressContext() {}
    CTemplateAddressContext(const std::string& strNameIn, const std::string& strDescribeIn, const bytes& btDataIn)
      : strName(strNameIn), strDescribe(strDescribeIn), btData(btDataIn) {}

public:
    std::string strName;
    std::string strDescribe;
    bytes btData;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(strName, opt);
        s.Serialize(strDescribe, opt);
        s.Serialize(btData, opt);
    }
};

class CContractAddressContext
{
    friend class hcode::CStream;

public:
    CContractAddressContext() {}
    CContractAddressContext(const std::string& strTypeIn, const CDestination& destCodeOwnerIn, const std::string& strNameIn, const std::string& strDescribeIn, const uint256& hashCreateTxidIn,
                            const uint256& hashSourceCodeIn, const uint256& hashWasmCreateCodeIn, const uint256& hashWasmRunCodeIn)
      : strType(strTypeIn), destCodeOwner(destCodeOwnerIn), strName(strNameIn), strDescribe(strDescribeIn), hashCreateTxid(hashCreateTxidIn),
        hashSourceCode(hashSourceCodeIn), hashWasmCreateCode(hashWasmCreateCodeIn), hashWasmRunCode(hashWasmRunCodeIn) {}

public:
    std::string strType;
    CDestination destCodeOwner;
    std::string strName;
    std::string strDescribe;
    uint256 hashCreateTxid;
    uint256 hashSourceCode;
    uint256 hashWasmCreateCode;
    uint256 hashWasmRunCode;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(strType, opt);
        s.Serialize(destCodeOwner, opt);
        s.Serialize(strName, opt);
        s.Serialize(strDescribe, opt);
        s.Serialize(hashCreateTxid, opt);
        s.Serialize(hashSourceCode, opt);
        s.Serialize(hashWasmCreateCode, opt);
        s.Serialize(hashWasmRunCode, opt);
    }
};

class CAddressContext
{
    friend class hcode::CStream;

public:
    CAddressContext()
      : nType(CDestination::PREFIX_NULL), nBlockNumber(0) {}
    CAddressContext(const int64 nBlockNumberIn)
      : nType(CDestination::PREFIX_PUBKEY), nBlockNumber(nBlockNumberIn) {}
    CAddressContext(const uint8 nTypeIn, const bytes& btDataIn, const uint64 nBlockNumberIn)
      : nType(nTypeIn), btData(btDataIn), nBlockNumber(nBlockNumberIn) {}
    CAddressContext(const CTemplateAddressContext& ctxtTemplateIn, const uint64 nBlockNumberIn)
    {
        hcode::CBufStream ss;
        ss << ctxtTemplateIn;
        btData.assign(ss.GetData(), ss.GetData() + ss.GetSize());
        nType = CDestination::PREFIX_TEMPLATE;
        nBlockNumber = nBlockNumberIn;
    }
    CAddressContext(const CContractAddressContext& ctxtContractIn, const uint64 nBlockNumberIn)
    {
        hcode::CBufStream ss;
        ss << ctxtContractIn;
        btData.assign(ss.GetData(), ss.GetData() + ss.GetSize());
        nType = CDestination::PREFIX_CONTRACT;
        nBlockNumber = nBlockNumberIn;
    }

    bool IsNull() const
    {
        return (nType == CDestination::PREFIX_NULL);
    }
    bool IsPubkey() const
    {
        return (nType == CDestination::PREFIX_PUBKEY);
    }
    bool IsTemplate() const
    {
        return (nType == CDestination::PREFIX_TEMPLATE);
    }
    bool IsContract() const
    {
        return (nType == CDestination::PREFIX_CONTRACT);
    }

    bool GetTemplateAddressContext(CTemplateAddressContext& ctxtTemplate) const
    {
        if (nType == CDestination::PREFIX_TEMPLATE)
        {
            try
            {
                hcode::CBufStream ss(btData);
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
        if (nType == CDestination::PREFIX_CONTRACT)
        {
            try
            {
                hcode::CBufStream ss(btData);
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
    uint8 nType;
    uint64 nBlockNumber;
    bytes btData;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nType, opt);
        s.Serialize(nBlockNumber, opt);
        s.Serialize(btData, opt);
    }
};

#endif //COMMON_TRANSACTION_H

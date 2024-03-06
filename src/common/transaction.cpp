// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transaction.h"

#include "bloomfilter/bloomfilter.h"
#include "devcommon/util.h"
#include "mtbase.h"
#include "libethcore/TransactionBase.h"
#include "template/template.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;
using namespace dev;
using namespace dev::eth;

namespace metabasenet
{

//////////////////////////////
// CTransaction

void CTransaction::SetNull()
{
    nType = 0;
    nChainId = 0;
    nTxNonce = 0;
    destFrom.SetNull();
    destTo.SetNull();
    nAmount = 0;
    nGasPrice = 0;
    nGasLimit = 0;
    mapTxData.clear();
    vchSig.clear();

    btEthTx.clear();
    txidEthTx = 0;
}

bool CTransaction::IsNull() const
{
    return (nType == TX_NULL);
}

bool CTransaction::IsMintTx() const
{
    return (nType == TX_GENESIS || nType == TX_STAKE || nType == TX_WORK);
}

bool CTransaction::IsRewardTx() const
{
    return (IsMintTx() || nType == TX_VOTE_REWARD);
}

bool CTransaction::IsCertTx() const
{
    return (nType == TX_CERT);
}

bool CTransaction::IsEthTx() const
{
    return (nType == TX_ETH_CREATE_CONTRACT || nType == TX_ETH_MESSAGE_CALL);
}

bool CTransaction::IsUserTx() const
{
    return (nType == TX_TOKEN || IsEthTx());
}

bool CTransaction::IsInternalTx() const
{
    return (nType == TX_INTERNAL);
}

std::string CTransaction::GetTypeString() const
{
    return GetTypeStringStatic(nType);
}

uint256 CTransaction::GetTxFee() const
{
    return nGasPrice * nGasLimit;
}

uint256 CTransaction::GetHash() const
{
    if (IsEthTx())
    {
        return txidEthTx;
    }
    else
    {
        mtbase::CBufStream ss;
        ss << (*this);
        return CryptoHash(ss.GetData(), ss.GetSize());
    }
}

uint256 CTransaction::GetSignatureHash() const
{
    if (IsEthTx())
    {
        return GetEthSignatureHash();
    }
    else
    {
        mtbase::CBufStream ss;
        ss << nType << nChainId << nTxNonce << destFrom << destTo << nAmount << nGasPrice << nGasLimit << mapTxData;
        return CryptoHash(ss.GetData(), ss.GetSize());
    }
}

void CTransaction::AddTxData(const uint16 nTypeIn, const bytes& btDataIn)
{
    if (IsEthTx())
    {
        return;
    }
    mapTxData[nTypeIn] = btDataIn;
}

bool CTransaction::GetTxData(const uint16 nTypeIn, bytes& btDataOut) const
{
    auto it = mapTxData.find(nTypeIn);
    if (it != mapTxData.end())
    {
        btDataOut = it->second;
        return true;
    }
    return false;
}

std::size_t CTransaction::GetTxDataCount() const
{
    return mapTxData.size();
}

std::size_t CTransaction::GetTxDataSize() const
{
    return GetTxExtData().size();
}

uint256 CTransaction::GetTxBaseGas() const
{
    return (TX_BASE_GAS + GetTxDataGas());
}

uint256 CTransaction::GetTxDataGas() const
{
    return GetTxDataGasStatic(GetTxDataSize());
}

uint256 CTransaction::GetRunGasLimit() const
{
    uint256 nTxMinGas = GetTxBaseGas();
    if (nGasLimit < nTxMinGas)
    {
        return 0;
    }
    return (nGasLimit - nTxMinGas);
}

bytes CTransaction::GetTxExtData() const
{
    if (IsEthTx())
    {
        bytes btData;
        if (!GetTxData(DF_ETH_TX_DATA, btData))
        {
            return bytes();
        }
        return btData;
    }
    else
    {
        bytes btExtData;
        if (!mapTxData.empty())
        {
            mtbase::CBufStream ss;
            ss << mapTxData;
            ss.GetData(btExtData);
        }
        return btExtData;
    }
}

bool CTransaction::SetTxExtData(const bytes& btExtData)
{
    if (IsEthTx())
    {
        return false;
    }
    try
    {
        mtbase::CBufStream ss(btExtData);
        ss >> mapTxData;
    }
    catch (std::exception& e)
    {
        return false;
    }
    return true;
}

//---------------------------
uint8 CTransaction::GetTxType() const
{
    return nType;
}

const CDestination& CTransaction::GetFromAddress() const
{
    return destFrom;
}

const CDestination& CTransaction::GetToAddress() const
{
    return destTo;
}

CChainId CTransaction::GetChainId() const
{
    return nChainId;
}

uint64 CTransaction::GetNonce() const
{
    return nTxNonce;
}

const uint256& CTransaction::GetAmount() const
{
    return nAmount;
}

const uint256& CTransaction::GetGasPrice() const
{
    return nGasPrice;
}

const uint256& CTransaction::GetGasLimit() const
{
    return nGasLimit;
}

const bytes& CTransaction::GetSignData() const
{
    return vchSig;
}

//--------------------------------
void CTransaction::SetTxType(const uint8 n)
{
    if (IsEthTx())
    {
        return;
    }
    nType = n;
}

void CTransaction::SetChainId(const CChainId nChainIdIn)
{
    if (IsEthTx())
    {
        return;
    }
    nChainId = nChainIdIn;
}

void CTransaction::SetNonce(const uint64 n)
{
    if (IsEthTx())
    {
        return;
    }
    nTxNonce = n;
}

void CTransaction::SetFromAddress(const CDestination& dest)
{
    if (IsEthTx())
    {
        return;
    }
    destFrom = dest;
}

void CTransaction::SetToAddress(const CDestination& dest)
{
    if (IsEthTx())
    {
        return;
    }
    destTo = dest;
}

void CTransaction::SetAmount(const uint256& n)
{
    if (IsEthTx())
    {
        return;
    }
    nAmount = n;
}

void CTransaction::SetGasPrice(const uint256& n)
{
    if (IsEthTx())
    {
        return;
    }
    nGasPrice = n;
}

void CTransaction::SetGasLimit(const uint256& n)
{
    if (IsEthTx())
    {
        return;
    }
    nGasLimit = n;
}

void CTransaction::SetSignData(const bytes& d)
{
    if (IsEthTx())
    {
        return;
    }
    vchSig = d;
}

//------------------------------------
bool CTransaction::VerifyTxSignature(const CDestination& destSign) const
{
    if (IsEthTx())
    {
        return !destFrom.IsNull();
    }
    if (destSign.IsNull())
    {
        return false;
    }
    return CryptoVerify(static_cast<uint160>(destSign), GetSignatureHash(), vchSig);
}

void CTransaction::SetToAddressData(const CAddressContext& ctxAddress)
{
    if (IsEthTx())
    {
        return;
    }
    PrSetToAddressData(ctxAddress);
}

bool CTransaction::GetToAddressData(CAddressContext& ctxAddress) const
{
    try
    {
        bytes btAddressData;
        if (!GetTxData(DF_TO_ADDRESS_DATA, btAddressData))
        {
            return false;
        }
        CBufStream ss(btAddressData);
        ss >> ctxAddress;
    }
    catch (exception& e)
    {
        return false;
    }
    return true;
}

bool CTransaction::SetEthTx(const bytes& btEthTxDataIn, const bool fToPubkey)
{
    if (!SetEthTxPacket(btEthTxDataIn))
    {
        return false;
    }
    if (fToPubkey)
    {
        PrSetToAddressData(CAddressContext(CPubkeyAddressContext()));
    }
    return true;
}

bool CTransaction::GetCreateCodeContext(uint8& nCodeType, CTemplateContext& ctxTemplate, CTxContractData& ctxContract) const
{
    if (IsEthTx())
    {
        bytes btData;
        if (!GetTxData(DF_ETH_TX_DATA, btData))
        {
            return false;
        }
        nCodeType = CODE_TYPE_CONTRACT;
        ctxContract = CTxContractData(btData, false);
        ctxContract.destCodeOwner = GetFromAddress();
        return true;
    }
    bytes btCodeData;
    if (!GetTxData(CTransaction::DF_CREATE_CODE, btCodeData))
    {
        return false;
    }
    try
    {
        mtbase::CBufStream ss(btCodeData);
        ss >> nCodeType;
        if (nCodeType == CODE_TYPE_TEMPLATE)
        {
            ss >> ctxTemplate;
        }
        else if (nCodeType == CODE_TYPE_CONTRACT)
        {
            ss >> ctxContract;
            ctxContract.UncompressCode();
        }
        else
        {
            return false;
        }
    }
    catch (exception& e)
    {
        return false;
    }
    return true;
}

//-------------------------------------
// static
uint8 CTransaction::GetTxMode(const uint8 nTxType)
{
    return (nTxType >> 4);
}

std::string CTransaction::GetTypeStringStatic(uint16 nTxType)
{
    if (nTxType == TX_GENESIS)
        return std::string("genesis");
    else if (nTxType == TX_TOKEN)
        return std::string("token");
    else if (nTxType == TX_CERT)
        return std::string("certification");
    else if (nTxType == TX_STAKE)
        return std::string("stake");
    else if (nTxType == TX_WORK)
        return std::string("poa");
    else if (nTxType == TX_VOTE_REWARD)
        return std::string("vote-reward");
    else if (nTxType == TX_INTERNAL)
        return std::string("internal");

    else if (nTxType == TX_ETH_CREATE_CONTRACT)
        return std::string("eth-create-contract");
    else if (nTxType == TX_ETH_MESSAGE_CALL)
        return std::string("eth-message-call");

    return std::string("undefined");
}

uint256 CTransaction::GetTxDataGasStatic(const uint64 nTxDataSize)
{
    return DATA_GAS_PER_BYTE * nTxDataSize;
}

uint256 CTransaction::GetTxBaseGasStatic(const uint64 nTxDataSize)
{
    return (TX_BASE_GAS + GetTxDataGasStatic(nTxDataSize));
}

//-------------------------------------
bool CTransaction::SetEthTxPacket(const bytes& btTxPacket)
{
    try
    {
        /// Constructs a transaction from the given RLP.
        //TransactionBase(bytes const& _rlp, CheckTransaction _checkSig) : TransactionBase(&_rlp, _checkSig) {}

        TransactionBase ethtx(btTxPacket, CheckTransaction::Everything);

        nType = (ethtx.isCreation() ? TX_ETH_CREATE_CONTRACT : TX_ETH_MESSAGE_CALL);
        nChainId = ethtx.getChainId();
        nTxNonce = (uint64)(ethtx.nonce());
        destFrom = CDestination(address_to_hex(ethtx.from()));
        destTo = CDestination(address_to_hex(ethtx.to()));
        nAmount = uint256(reverse_u256_string(ethtx.value()));
        nGasPrice = uint256(reverse_u256_string(ethtx.gasPrice()));
        nGasLimit = uint256(reverse_u256_string(ethtx.gas()));
        vchSig = ethtx.getSignature();

        bytes btData = ethtx.data();
        if (!btData.empty())
        {
            mapTxData[DF_ETH_TX_DATA] = btData;
        }

        btEthTx = btTxPacket;
        txidEthTx = uint256(h256_to_hex(ethtx.sha3()));
    }
    catch (std::exception& e)
    {
        return false;
    }
    return true;
}

uint256 CTransaction::GetEthSignatureHash() const
{
    try
    {
        TransactionBase ethtx(btEthTx, CheckTransaction::Everything);
        return uint256(h256_to_hex(ethtx.sha3(WithoutSignature)));
    }
    catch (std::exception& e)
    {
        return uint256();
    }
    return uint256();
}

void CTransaction::PrSetToAddressData(const CAddressContext& ctxAddress)
{
    CBufStream ss;
    ss << ctxAddress;
    mapTxData[DF_TO_ADDRESS_DATA] = ss.GetBytes();
}

//-------------------------------------
void CTransaction::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    if (nType == TX_ETH_CREATE_CONTRACT || nType == TX_ETH_MESSAGE_CALL)
    {
        bool fLinkPubkeyAddress = false;
        CAddressContext ctxAddress;
        if (GetToAddressData(ctxAddress) && ctxAddress.IsPubkey())
        {
            fLinkPubkeyAddress = true;
        }
        s << nType << fLinkPubkeyAddress << btEthTx;
    }
    else
    {
        //s << nType << nChainId << nTxNonce << destFrom << destTo << nAmount << nGasPrice << nGasLimit << mapTxData << vchSig;

        bytes btFrom = destFrom.ToValidBigEndianData();
        bytes btTo = destTo.ToValidBigEndianData();
        bytes btAmount = nAmount.ToValidBigEndianData();
        bytes btGasPrice = nGasPrice.ToValidBigEndianData();
        bytes btGasLimit = nGasLimit.ToValidBigEndianData();

        s << nType << CVarInt((uint64)nChainId) << CVarInt(nTxNonce) << btFrom << btTo << btAmount << btGasPrice << btGasLimit << mapTxData << vchSig;
    }
}

void CTransaction::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    s >> nType;
    if (nType == TX_ETH_CREATE_CONTRACT || nType == TX_ETH_MESSAGE_CALL)
    {
        bool fLinkPubkeyAddress;
        bytes btEthTxData;
        s >> fLinkPubkeyAddress >> btEthTxData;
        if (!SetEthTxPacket(btEthTxData))
        {
            throw runtime_error("set error");
        }
        if (fLinkPubkeyAddress)
        {
            PrSetToAddressData(CAddressContext(CPubkeyAddressContext()));
        }
    }
    else
    {
        //s >> nChainId >> nTxNonce >> destFrom >> destTo >> nAmount >> nGasPrice >> nGasLimit >> mapTxData >> vchSig;

        CVarInt varChainId;
        CVarInt varTxNonce;
        bytes btFrom;
        bytes btTo;
        bytes btAmount;
        bytes btGasPrice;
        bytes btGasLimit;

        s >> varChainId >> varTxNonce >> btFrom >> btTo >> btAmount >> btGasPrice >> btGasLimit >> mapTxData >> vchSig;

        nChainId = (CChainId)(varChainId.nValue);
        nTxNonce = varTxNonce.nValue;
        destFrom.FromValidBigEndianData(btFrom);
        destTo.FromValidBigEndianData(btTo);
        nAmount.FromValidBigEndianData(btAmount);
        nGasPrice.FromValidBigEndianData(btGasPrice);
        nGasLimit.FromValidBigEndianData(btGasLimit);
    }
}

void CTransaction::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    mtbase::CBufStream ss;
    if (nType == TX_ETH_CREATE_CONTRACT || nType == TX_ETH_MESSAGE_CALL)
    {
        bool fLinkPubkeyAddress = true;
        ss << nType << fLinkPubkeyAddress << btEthTx;
    }
    else
    {
        //ss << nType << nChainId << nTxNonce << destFrom << destTo << nAmount << nGasPrice << nGasLimit << mapTxData << vchSig;

        bytes btFrom = destFrom.ToValidBigEndianData();
        bytes btTo = destTo.ToValidBigEndianData();
        bytes btAmount = nAmount.ToValidBigEndianData();
        bytes btGasPrice = nGasPrice.ToValidBigEndianData();
        bytes btGasLimit = nGasLimit.ToValidBigEndianData();

        ss << nType << CVarInt((uint64)nChainId) << CVarInt(nTxNonce) << btFrom << btTo << btAmount << btGasPrice << btGasLimit << mapTxData << vchSig;
    }
    serSize = ss.GetSize();
}

//////////////////////////////
// CTransactionLogs

uint2048 CTransactionLogs::GetLogsBloom() const
{
    CNhBloomFilter bf(2048);
    bf.Add(address.begin(), address.size());
    for (auto& t : topics)
    {
        bf.Add(t.begin(), t.size());
    }
    return uint2048(bf.GetData());
}

void CTransactionLogs::GetBloomDataSet(std::set<bytes>& setBloomData) const
{
    setBloomData.insert(address.GetBytes());
    for (auto& t : topics)
    {
        setBloomData.insert(t.GetBytes());
    }
}

//////////////////////////////
// CTransactionReceipt

void CTransactionReceipt::CalcLogsBloom()
{
    nLogsBloom = 0;
    for (auto& logs : vLogs)
    {
        nLogsBloom |= logs.GetLogsBloom();
    }
}

void CTransactionReceipt::GetBloomDataSet(std::set<bytes>& setBloomData) const
{
    for (auto& logs : vLogs)
    {
        logs.GetBloomDataSet(setBloomData);
    }
}

//////////////////////////////
// CTxContractData

bool CTxContractData::PacketCompress(const uint8 nMuxTypeIn, const std::string& strTypeIn, const std::string& strNameIn, const CDestination& destCodeOwnerIn,
                                     const std::string& strDescribeIn, const bytes& btCodeIn, const bytes& btSourceIn)
{
    if (nMuxTypeIn <= CF_MIN || nMuxTypeIn >= CF_MAX)
    {
        return false;
    }

    nMuxType = nMuxTypeIn;
    strType = strTypeIn;
    strName = strNameIn;
    destCodeOwner = destCodeOwnerIn;
    nCompressDescribe = 0;
    if (!strDescribeIn.empty())
    {
        if (strDescribeIn.size() < 128)
        {
            btDescribe.assign(strDescribeIn.begin(), strDescribeIn.end());
        }
        else
        {
            bytes btSrc(strDescribeIn.begin(), strDescribeIn.end());
            if (!BtCompress(btSrc, btDescribe))
            {
                return false;
            }
            nCompressDescribe = 1;
        }
    }
    nCompressCode = 0;
    if (!btCodeIn.empty())
    {
        if (nMuxTypeIn == CF_CREATE || nMuxTypeIn == CF_UPCODE)
        {
            if (!BtCompress(btCodeIn, btCode))
            {
                return false;
            }
            nCompressCode = 1;
        }
        else
        {
            btCode = btCodeIn;
        }
    }
    nCompressSource = 0;
    if (!btSourceIn.empty())
    {
        if (!BtCompress(btSourceIn, btSource))
        {
            return false;
        }
        nCompressSource = 1;
    }
    return true;
}

void CTxContractData::SetSetupHash(const uint256& hashContractIn)
{
    btCode.assign(hashContractIn.begin(), hashContractIn.end());
    nCompressCode = 0;
    nMuxType = CF_SETUP;
}

uint256 CTxContractData::GetSourceCodeHash() const
{
    if (btSource.empty())
    {
        return 0;
    }
    if (nCompressSource != 0)
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return 0;
        }
        return metabasenet::crypto::CryptoKeccakHash(btDst.data(), btDst.size());
    }
    return metabasenet::crypto::CryptoKeccakHash(btSource.data(), btSource.size());
}

uint256 CTxContractData::GetContractCreateCodeHash() const
{
    if (btCode.empty())
    {
        return 0;
    }
    if (nMuxType == CF_SETUP)
    {
        return uint256(btCode);
    }
    else
    {
        if (nCompressCode != 0)
        {
            bytes btDst;
            if (!BtUncompress(btCode, btDst))
            {
                return 0;
            }
            //return metabasenet::crypto::CryptoHash(btDst.data(), btDst.size());
            return metabasenet::crypto::CryptoKeccakHash(btDst.data(), btDst.size());
        }
        //return metabasenet::crypto::CryptoHash(btCode.data(), btCode.size());
        return metabasenet::crypto::CryptoKeccakHash(btCode.data(), btCode.size());
    }
}

std::string CTxContractData::GetDescribe() const
{
    if (nCompressDescribe != 0 && !btDescribe.empty())
    {
        bytes btDst;
        if (!BtUncompress(btDescribe, btDst))
        {
            return std::string();
        }
        return std::string(btDst.begin(), btDst.end());
    }
    return std::string(btDescribe.begin(), btDescribe.end());
}

bytes CTxContractData::GetCode() const
{
    if (nCompressCode != 0 && !btCode.empty())
    {
        bytes btDst;
        if (!BtUncompress(btCode, btDst))
        {
            return bytes();
        }
        return btDst;
    }
    return btCode;
}

bytes CTxContractData::GetSource() const
{
    if (nCompressSource != 0 && !btSource.empty())
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return bytes();
        }
        return btDst;
    }
    return btSource;
}

bool CTxContractData::UncompressDescribe()
{
    if (nCompressDescribe != 0 && !btDescribe.empty())
    {
        bytes btDst;
        if (!BtUncompress(btDescribe, btDst))
        {
            return false;
        }
        btDescribe.clear();
        btDescribe.assign(btDst.begin(), btDst.end());
        nCompressDescribe = 0;
    }
    return true;
}

bool CTxContractData::UncompressCode()
{
    if (nCompressCode != 0 && !btCode.empty())
    {
        bytes btDst;
        if (!BtUncompress(btCode, btDst))
        {
            return false;
        }
        btCode.clear();
        btCode.assign(btDst.begin(), btDst.end());
        nCompressCode = 0;
    }
    return true;
}

bool CTxContractData::UncompressSource()
{
    if (nCompressSource != 0 && !btSource.empty())
    {
        bytes btDst;
        if (!BtUncompress(btSource, btDst))
        {
            return false;
        }
        btSource.clear();
        btSource.assign(btDst.begin(), btDst.end());
        nCompressSource = 0;
    }
    return true;
}

bool CTxContractData::UncompressAll()
{
    if (!UncompressDescribe())
    {
        return false;
    }
    if (!UncompressCode())
    {
        return false;
    }
    if (!UncompressSource())
    {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////
// CTemplateContext

std::string CTemplateContext::GetName() const
{
    return strName;
}

bytes CTemplateContext::GetTemplateData() const
{
    return btTemplateData;
}

uint8 CTemplateContext::GetTemplateType() const
{
    CTemplatePtr ptr = CTemplate::Import(btTemplateData);
    if (!ptr)
    {
        return 0;
    }
    return ptr->GetTemplateType();
}

CTemplateId CTemplateContext::GetTemplateId() const
{
    CTemplatePtr ptr = CTemplate::Import(btTemplateData);
    if (!ptr)
    {
        return CTemplateId();
    }
    return ptr->GetTemplateId();
}

CDestination CTemplateContext::GetTemplateAddress() const
{
    return CDestination(GetTemplateId());
}

//////////////////////////////
// CLogsFilter

void CLogsFilter::matchesLogs(CTransactionReceipt const& _m, MatchLogsVec& vLogs) const
{
    for (unsigned i = 0; i < _m.vLogs.size(); ++i)
    {
        auto& logs = _m.vLogs[i];
        if (!setAddress.empty() && setAddress.count(logs.address) == 0)
        {
            continue;
        }
        bool m = true;
        for (unsigned i = 0; i < MAX_LOGS_FILTER_TOPIC_COUNT; ++i)
        {
            const std::set<uint256>& setTopics = arrayTopics[i];
            if (!setTopics.empty() && *setTopics.begin() != 0 && (logs.topics.size() <= i || !setTopics.count(logs.topics[i])))
            {
                m = false;
                break;
            }
        }
        if (m)
        {
            vLogs.push_back(CMatchLogs(i, logs));
        }
    }
}

//////////////////////////////
// CFilterId

uint256 CFilterId::CreateFilterId(const uint8 nFilterType, const uint64 nId, const uint256& hash)
{
    mtbase::CBufStream ss;
    ss << nFilterType << hash << nId;
    uint256 hashFilterId = CryptoHash(ss.GetData(), ss.GetSize());
    *(hashFilterId.begin()) = nFilterType;
    return hashFilterId;
}

} // namespace metabasenet

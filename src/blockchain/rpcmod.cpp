// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcmod.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/regex.hpp>
#include <regex>

//#include <algorithm>

#include "bloomfilter/bloomfilter.h"
#include "core.h"
#include "devcommon/util.h"
#include "param.h"
#include "rpc/auto_protocol.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/template.h"
#include "util.h"
#include "version.h"

using namespace std;
using namespace mtbase;
using namespace json_spirit;
using namespace metabasenet::rpc;
using namespace metabasenet;
namespace fs = boost::filesystem;

namespace metabasenet
{

#define UNLOCKKEY_RELEASE_DEFAULT_TIME 60

const char* GetGitVersion();

///////////////////////////////
// static function

static CBlockData BlockToJSON(const uint256& hashBlock, const CBlock& block, const CChainId nChainId, const uint256& hashFork, const uint32 nHeight, const uint256& nBlockReward)
{
    CBlockData data;
    data.strHash = hashBlock.GetHex();
    data.strPrev = block.hashPrev.GetHex();
    data.nChainid = nChainId;
    data.strFork = hashFork.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.GetTxType());
    data.nTime = block.GetBlockTime();
    data.nNumber = block.GetBlockNumber();
    data.nHeight = nHeight;
    data.nSlot = block.GetBlockSlot();
    data.strReward = CoinToTokenBigFloat(nBlockReward);
    data.strStateroot = block.hashStateRoot.GetHex();
    if (!block.hashReceiptsRoot.IsNull())
    {
        data.strReceiptsroot = block.hashReceiptsRoot.GetHex();
    }
    else
    {
        data.strReceiptsroot = "0x";
    }
    if (!block.btBloomData.empty())
    {
        data.strBloom = ToHexString(block.btBloomData);
    }
    else
    {
        data.strBloom = "0x";
    }
    data.strTxmint = block.txMint.GetHash().GetHex();
    for (const CTransaction& tx : block.vtx)
    {
        data.vecTx.push_back(tx.GetHash().GetHex());
    }
    return data;
}

CTransactionData TxToJSON(const uint256& txid, const CTransaction& tx, const uint256& hashFork, const uint256& blockHash, const int nDepth)
{
    CTransactionData ret;

    ret.strTxid = txid.GetHex();
    //ret.nVersion = tx.GetVersion();
    ret.strType = tx.GetTypeString();
    //ret.nTime = tx.GetTxTime();
    ret.nNonce = tx.GetNonce();
    ret.strFrom = tx.GetFromAddress().ToString();
    ret.strTo = tx.GetToAddress().ToString();
    ret.strAmount = CoinToTokenBigFloat(tx.GetAmount());

    ret.nGaslimit = tx.GetGasLimit().Get64();
    ret.strGasprice = CoinToTokenBigFloat(tx.GetGasPrice());
    ret.strTxfee = CoinToTokenBigFloat(tx.GetGasLimit() * tx.GetGasPrice());

    ret.strData = mtbase::ToHexString(tx.GetTxExtData());

    if (tx.GetSignData().empty())
    {
        ret.strSignhash = "";
    }
    else
    {
        ret.strSignhash = tx.GetSignatureHash().GetHex();
    }
    ret.strSig = mtbase::ToHexString(tx.GetSignData());
    ret.strFork = hashFork.GetHex();
    if (blockHash != 0)
    {
        ret.nHeight = CBlock::GetBlockHeightByHash(blockHash);
        ret.strBlockhash = blockHash.GetHex();
    }
    else
    {
        ret.nHeight = 0;
        ret.strBlockhash = std::string();
    }
    if (nDepth >= 0)
    {
        ret.nConfirmations = nDepth;
    }
    return ret;
}

static CWalletTxData TxInfoToJSON(const CDestination& destLocal, const CDestTxInfo& txInfo)
{
    CWalletTxData data;

    data.strTxid = txInfo.txid.GetHex();
    data.nTxindex = txInfo.nTxIndex;
    data.nBlocknumber = txInfo.nBlockNumber;
    data.strTxtype = CTransaction::GetTypeStringStatic(txInfo.nTxType);
    data.nTime = txInfo.nTimeStamp;
    data.strAmount = CoinToTokenBigFloat(txInfo.nAmount);
    data.strFee = CoinToTokenBigFloat(txInfo.nTxFee);

    if (txInfo.nDirection == CDestTxInfo::DXI_DIRECTION_CIN
        || txInfo.nDirection == CDestTxInfo::DXI_DIRECTION_COUT
        || txInfo.nDirection == CDestTxInfo::DXI_DIRECTION_CINOUT)
    {
        data.strTransfertype = "internal";
    }
    else if (txInfo.nDirection == CDestTxInfo::DXI_DIRECTION_CODEREWARD)
    {
        data.strTransfertype = "codereward";
    }
    else
    {
        data.strTransfertype = "common";
    }

    switch (txInfo.nDirection)
    {
    case CDestTxInfo::DXI_DIRECTION_IN:         // local is to, peer is from
    case CDestTxInfo::DXI_DIRECTION_CIN:        // contract transfer
    case CDestTxInfo::DXI_DIRECTION_CODEREWARD: // code reward
        data.strFrom = txInfo.destPeer.ToString();
        data.strTo = destLocal.ToString();
        break;
    case CDestTxInfo::DXI_DIRECTION_OUT:    // local is from, peer is to
    case CDestTxInfo::DXI_DIRECTION_COUT:   // contract transfer
    case CDestTxInfo::DXI_DIRECTION_INOUT:  // from equal to
    case CDestTxInfo::DXI_DIRECTION_CINOUT: // contract transfer from equal to
        data.strFrom = destLocal.ToString();
        data.strTo = txInfo.destPeer.ToString();
        break;
    default:
        data.strFrom = "";
        data.strTo = "";
    }
    return data;
}

// "type": "0x0",
// "chainId": "0x38c",
// "v": "0x73b",
// "r": "0x2aa1b0b294d0008aeb7841f31b036e4eb97bdd646e12d521167ef99654cea89c",
// "s": "0x7b3411ae658e9e0d38863e2131dde1b88fc394b33d9229d083bff4ed57eb556b"

static CEthTransaction EthTxToJSON(const uint256& txid, const CTransaction& tx, const uint256& blockHash, const uint64 nBlockNumber, const uint64 nTxIndex)
{
    CEthTransaction txData;

    txData.strHash = txid.GetHex();
    txData.strNonce = ToHexString(tx.GetNonce());
    txData.strBlockhash = blockHash.GetHex();
    txData.strBlocknumber = ToHexString(nBlockNumber);
    txData.strTransactionindex = ToHexString(nTxIndex);
    txData.strFrom = tx.GetFromAddress().ToString();
    txData.strTo = tx.GetToAddress().ToString();
    txData.strValue = tx.GetAmount().GetValueHex();
    txData.strGasprice = tx.GetGasPrice().GetValueHex();
    txData.strGas = tx.GetGasLimit().GetValueHex();
    if (tx.GetTxExtData().empty())
    {
        txData.strInput = "0x";
    }
    else
    {
        txData.strInput = ToHexString(tx.GetTxExtData());
    }
    txData.strType = "0x0";
    txData.strChainid = ToHexString(tx.GetChainId());
    txData.strV = "0x";
    txData.strR = "0x";
    txData.strS = "0x";

    if (tx.GetTxType() == CTransaction::TX_ETH_CREATE_CONTRACT)
    {
        txData.strType = "0x1";
    }
    else if (tx.GetTxType() == CTransaction::TX_ETH_MESSAGE_CALL)
    {
        txData.strType = "0x2";
    }

    uint256 r;
    uint256 s;
    uint8 v;
    if (crypto::GetEthSignRsv(tx.GetSignData(), r, s, v))
    {
        txData.strV = r.ToString();
        txData.strR = s.ToString();
        txData.strS = ToHexString((uint32)v);
    }

    return txData;
}

static CEthBlock EthBlockToJSON(const CBlock& block, const bool fTxDetail)
{
    CEthBlock data;

    uint64 nBlockSize = 0;
    try
    {
        CBufStream ss;
        ss << block;
        nBlockSize = ss.GetSize();
    }
    catch (const std::exception& e)
    {
        return data;
    }

    uint256 hashBlock = block.GetHash();

    data.strNumber = ToHexString(block.GetBlockNumber());
    data.strHash = hashBlock.GetHex();
    data.strParenthash = block.hashPrev.GetHex();
    //data.strNonce = ToHexString(block.GetBlockNumber()); // __optional__ CRPCString strNonce;
    std::string strNonceTemp = hashBlock.ToString();
    data.strNonce = std::string("0x") + strNonceTemp.substr(strNonceTemp.size() - 16);
    data.strSha3Uncles = "0x"; // __optional__ CRPCString strSha3Uncles;
    if (!block.btBloomData.empty())
    {
        data.strLogsbloom = ToHexString(block.btBloomData);
    }
    else
    {
        data.strLogsbloom = "0x";
    }
    data.strTransactionsroot = block.hashMerkleRoot.GetHex();
    data.strStateroot = block.hashStateRoot.GetHex();
    if (!block.hashReceiptsRoot.IsNull())
    {
        data.strReceiptsroot = block.hashReceiptsRoot.GetHex();
    }
    else
    {
        data.strReceiptsroot = "0x";
    }
    data.strMiner = block.txMint.GetToAddress().ToString();
    data.strMixhash = "0x";
    data.strDifficulty = "0x0";      // __optional__ CRPCString strDifficulty;
    data.strTotaldifficulty = "0x0"; // __optional__ CRPCString strTotaldifficulty;
    data.strExtradata = "0x";        // __optional__ CRPCString strExtradata;
    data.strSize = ToHexString(nBlockSize);
    data.strGaslimit = block.nGasLimit.GetValueHex();
    data.strGasused = block.nGasUsed.GetValueHex();
    data.strTimestamp = ToHexString(block.GetBlockTime());

    if (fTxDetail)
    {
        data.vecTransactiondetails.push_back(EthTxToJSON(block.txMint.GetHash(), block.txMint, hashBlock, block.GetBlockNumber(), 0));
        for (size_t i = 0; i < block.vtx.size(); ++i)
        {
            const CTransaction& tx = block.vtx[i];
            data.vecTransactiondetails.push_back(EthTxToJSON(tx.GetHash(), tx, hashBlock, block.GetBlockNumber(), i + 1));
        }
    }
    else
    {
        data.vecTransactions.push_back(block.txMint.GetHash().GetHex());
        for (const CTransaction& tx : block.vtx)
        {
            data.vecTransactions.push_back(tx.GetHash().GetHex());
        }
    }

    //data.vecUncles.push_back("0x");

    return data;
}

///////////////////////////////
// CRPCMod

CRPCMod::CRPCMod()
  : IIOModule("rpcmod")
{
    pHttpServer = nullptr;
    pCoreProtocol = nullptr;
    pService = nullptr;
    pDataStat = nullptr;
    pForkManager = nullptr;
    pBlockMaker = nullptr;

    std::map<std::string, RPCFunc> temp_map = boost::assign::map_list_of
        /* System */
        ("help", &CRPCMod::RPCHelp)
        //
        ("stop", &CRPCMod::RPCStop)
        //
        ("version", &CRPCMod::RPCVersion)
        /* Network */
        ("getpeercount", &CRPCMod::RPCGetPeerCount)
        //
        ("listpeer", &CRPCMod::RPCListPeer)
        //
        ("addnode", &CRPCMod::RPCAddNode)
        //
        ("removenode", &CRPCMod::RPCRemoveNode)
        //
        ("getforkport", &CRPCMod::RPCGetForkPort)
        /* Blockchain & TxPool */
        ("getforkcount", &CRPCMod::RPCGetForkCount)
        //
        ("listfork", &CRPCMod::RPCListFork)
        //
        ("getgenealogy", &CRPCMod::RPCGetForkGenealogy)
        //
        ("getblocklocation", &CRPCMod::RPCGetBlockLocation)
        //
        ("getblockcount", &CRPCMod::RPCGetBlockCount)
        //
        ("getblockhash", &CRPCMod::RPCGetBlockHash)
        //
        ("getblocknumberhash", &CRPCMod::RPCGetBlockNumberHash)
        //
        ("getblock", &CRPCMod::RPCGetBlock)
        //
        ("getblockdetail", &CRPCMod::RPCGetBlockDetail)
        //
        ("getblockdata", &CRPCMod::RPCGetBlockData)
        //
        ("gettxpool", &CRPCMod::RPCGetTxPool)
        //
        ("gettransaction", &CRPCMod::RPCGetTransaction)
        //
        ("sendtransaction", &CRPCMod::RPCSendTransaction)
        //
        ("getforkheight", &CRPCMod::RPCGetForkHeight)
        //
        ("getvotes", &CRPCMod::RPCGetVotes)
        //
        ("listdelegate", &CRPCMod::RPCListDelegate)
        //
        ("getdelegatevotes", &CRPCMod::RPCGetDelegateVotes)
        //
        ("getuservotes", &CRPCMod::RPCGetUserVotes)
        //
        ("gettimevault", &CRPCMod::RPCGetTimeVault)
        //
        ("getaddresscount", &CRPCMod::RPCGetAddressCount)
        /* Wallet */
        ("listkey", &CRPCMod::RPCListKey)
        //
        ("getnewkey", &CRPCMod::RPCGetNewKey)
        //
        ("encryptkey", &CRPCMod::RPCEncryptKey)
        //
        ("lockkey", &CRPCMod::RPCLockKey)
        //
        ("unlockkey", &CRPCMod::RPCUnlockKey)
        //
        ("removekey", &CRPCMod::RPCRemoveKey)
        //
        ("importprivkey", &CRPCMod::RPCImportPrivKey)
        //
        ("importpubkey", &CRPCMod::RPCImportPubKey)
        //
        ("importkey", &CRPCMod::RPCImportKey)
        //
        ("exportkey", &CRPCMod::RPCExportKey)
        //
        ("addnewtemplate", &CRPCMod::RPCAddNewTemplate)
        //
        ("importtemplate", &CRPCMod::RPCImportTemplate)
        //
        ("exporttemplate", &CRPCMod::RPCExportTemplate)
        //
        ("removetemplate", &CRPCMod::RPCRemoveTemplate)
        //
        ("validateaddress", &CRPCMod::RPCValidateAddress)
        //
        ("getbalance", &CRPCMod::RPCGetBalance)
        //
        ("listtransaction", &CRPCMod::RPCListTransaction)
        //
        ("sendfrom", &CRPCMod::RPCSendFrom)
        //
        ("createtransaction", &CRPCMod::RPCCreateTransaction)
        //
        ("signtransaction", &CRPCMod::RPCSignTransaction)
        //
        ("signmessage", &CRPCMod::RPCSignMessage)
        //
        ("listaddress", &CRPCMod::RPCListAddress)
        //
        ("exportwallet", &CRPCMod::RPCExportWallet)
        //
        ("importwallet", &CRPCMod::RPCImportWallet)
        //
        ("makeorigin", &CRPCMod::RPCMakeOrigin)
        //
        ("callcontract", &CRPCMod::RPCCallContract)
        //
        ("gettransactionreceipt", &CRPCMod::RPCGetTransactionReceipt)
        //
        ("getcontractmuxcode", &CRPCMod::RPCGetContractMuxCode)
        //
        ("gettemplatemuxcode", &CRPCMod::RPCGetTemplateMuxCode)
        //
        ("listcontractcode", &CRPCMod::RPCListContractCode)
        //
        ("listcontractaddress", &CRPCMod::RPCListContractAddress)
        //
        ("getdestcontract", &CRPCMod::RPCGetDestContract)
        //
        ("getcontractsource", &CRPCMod::RPCGetContractSource)
        //
        ("getcontractcode", &CRPCMod::RPCGetContractCode)
        //
        ("addblacklistaddress", &CRPCMod::RPCAddBlacklistAddress)
        //
        ("removeblacklistaddress", &CRPCMod::RPCRemoveBlacklistAddress)
        //
        ("listblacklistaddress", &CRPCMod::RPCListBlacklistAddress)
        //
        ("setfunctionaddress", &CRPCMod::RPCSetFunctionAddress)
        //
        ("listfunctionaddress", &CRPCMod::RPCListFunctionAddress)
        /* Util */
        ("verifymessage", &CRPCMod::RPCVerifyMessage)
        //
        ("makekeypair", &CRPCMod::RPCMakeKeyPair)
        //
        ("getpubkey", &CRPCMod::RPCGetPubKey)
        //
        ("getpubkeyaddress", &CRPCMod::RPCGetPubKeyAddress)
        //
        ("maketemplate", &CRPCMod::RPCMakeTemplate)
        //
        ("decodetransaction", &CRPCMod::RPCDecodeTransaction)
        //
        ("gettxfee", &CRPCMod::RPCGetTxFee)
        //
        ("makesha256", &CRPCMod::RPCMakeSha256)
        //
        ("reversehex", &CRPCMod::RPCReverseHex)
        //
        ("funcsign", &CRPCMod::RPCFuncSign)
        //
        ("makehash", &CRPCMod::RPCMakeHash)
        //
        ("setmintmingasprice", &CRPCMod::RPCSetMintMinGasPrice)
        //
        ("getmintmingasprice", &CRPCMod::RPCGetMintMinGasPrice)
        //
        ("listmintmingasprice", &CRPCMod::RPCListMintMinGasPrice)
        //
        ("checkatbloomfilter", &CRPCMod::RPCCheckAtBloomFilter)
        /* tool */
        ("querystat", &CRPCMod::RPCQueryStat)
        /* eth rpc */
        ("web3_clientVersion", &CRPCMod::RPCEthGetWebClientVersion)
        //
        ("web3_sha3", &CRPCMod::RPCEthGetSha3)
        //
        ("net_version", &CRPCMod::RPCEthGetNetVersion)
        //
        ("net_listening", &CRPCMod::RPCEthGetNetListening)
        //
        ("net_peerCount", &CRPCMod::RPCEthGetPeerCount)
        //
        ("eth_chainId", &CRPCMod::RPCEthGetChainId)
        //
        ("eth_protocolVersion", &CRPCMod::RPCEthGetProtocolVersion)
        //
        ("eth_syncing", &CRPCMod::RPCEthGetSyncing)
        //
        ("eth_coinbase", &CRPCMod::RPCEthGetCoinbase)
        //
        ("eth_mining", &CRPCMod::RPCEthGetMining)
        //
        ("eth_hashrate", &CRPCMod::RPCEthGetHashRate)
        //
        ("eth_gasPrice", &CRPCMod::RPCEthGetGasPrice)
        //
        ("eth_accounts", &CRPCMod::RPCEthGetAccounts)
        //
        ("eth_getBalance", &CRPCMod::RPCEthGetBalance)
        //
        ("eth_blockNumber", &CRPCMod::RPCEthGetBlockNumber)
        //
        ("eth_getStorageAt", &CRPCMod::RPCEthGetStorageAt)
        //
        ("eth_getStorageRoot", &CRPCMod::RPCEthGetStorageRoot)
        //
        ("eth_getTransactionCount", &CRPCMod::RPCEthGetTransactionCount)
        //
        ("eth_pendingTransactions", &CRPCMod::RPCEthGetPendingTransactions)
        //
        ("eth_getBlockTransactionCountByHash", &CRPCMod::RPCEthGetBlockTransactionCountByHash)
        //
        ("eth_getBlockTransactionCountByNumber", &CRPCMod::RPCEthGetBlockTransactionCountByNumber)
        //
        ("eth_getUncleCountByBlockHash", &CRPCMod::RPCEthGetUncleCountByBlockHash)
        //
        ("eth_getUncleCountByBlockNumber", &CRPCMod::RPCEthGetUncleCountByBlockNumber)
        //
        ("eth_getCode", &CRPCMod::RPCEthGetCode)
        //
        ("eth_sign", &CRPCMod::RPCEthSign)
        //
        ("eth_signTransaction", &CRPCMod::RPCEthSignTransaction)
        //
        ("eth_sendTransaction", &CRPCMod::RPCEthSendTransaction)
        //
        ("eth_sendRawTransaction", &CRPCMod::RPCEthSendRawTransaction)
        //
        ("eth_call", &CRPCMod::RPCEthCall)
        //
        ("eth_estimateGas", &CRPCMod::RPCEthEstimateGas)
        //
        ("eth_getBlockByHash", &CRPCMod::RPCEthGetBlockByHash)
        //
        ("eth_getBlockByNumber", &CRPCMod::RPCEthGetBlockByNumber)
        //
        ("eth_getTransactionByHash", &CRPCMod::RPCEthGetTransactionByHash)
        //
        ("eth_getTransactionByBlockHashAndIndex", &CRPCMod::RPCEthGetTransactionByBlockHashAndIndex)
        //
        ("eth_getTransactionByBlockNumberAndIndex", &CRPCMod::RPCEthGetTransactionByBlockNumberAndIndex)
        //
        ("eth_getTransactionReceipt", &CRPCMod::RPCEthGetTransactionReceipt)
        //
        ("eth_getUncleByBlockHashAndIndex", &CRPCMod::RPCEthGetUncleByBlockHashAndIndex)
        //
        ("eth_getUncleByBlockNumberAndIndex", &CRPCMod::RPCEthGetUncleByBlockNumberAndIndex)
        //
        ("eth_newFilter", &CRPCMod::RPCEthNewFilter)
        //
        ("eth_newBlockFilter", &CRPCMod::RPCEthNewBlockFilter)
        //
        ("eth_newPendingTransactionFilter", &CRPCMod::RPCEthNewPendingTransactionFilter)
        //
        ("eth_uninstallFilter", &CRPCMod::RPCEthUninstallFilter)
        //
        ("eth_getFilterChanges", &CRPCMod::RPCEthGetFilterChanges)
        //
        ("eth_getFilterLogs", &CRPCMod::RPCEthGetFilterLogs)
        //
        ("eth_getLogs", &CRPCMod::RPCEthGetLogs)
        //
        ;
    mapRPCFunc = temp_map;
    fWriteRPCLog = true;
}

CRPCMod::~CRPCMod()
{
}

bool CRPCMod::HandleInitialize()
{
    if (!GetObject("httpserver", pHttpServer))
    {
        Error("Failed to request httpserver");
        return false;
    }

    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("service", pService))
    {
        Error("Failed to request service");
        return false;
    }

    if (!GetObject("datastat", pDataStat))
    {
        Error("Failed to request datastat");
        return false;
    }
    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }
    if (!GetObject("blockmaker", pBlockMaker))
    {
        Error("Failed to request blockmaker");
        return false;
    }
    fWriteRPCLog = RPCServerConfig()->fRPCLogEnable;

    if (BasicConfig()->nModRpcThreads > 1)
    {
        AddEventThread(BasicConfig()->nModRpcThreads - 1);
    }
    return true;
}

void CRPCMod::HandleDeinitialize()
{
    pHttpServer = nullptr;
    pCoreProtocol = nullptr;
    pService = nullptr;
    pDataStat = nullptr;
    pForkManager = nullptr;
    pBlockMaker = nullptr;
}

bool CRPCMod::HandleEvent(CEventHttpReq& eventHttpReq)
{
    auto lmdMask = [](const string& data) -> string {
        //remove all sensible information such as private key
        // or passphrass from log content

        //log for debug mode
        boost::regex ptnSec(R"raw(("privkey"|"passphrase"|"oldpassphrase"|"signsecret"|"privkeyaddress")(\s*:\s*)(".*?"))raw", boost::regex::perl);
        return boost::regex_replace(data, ptnSec, string(R"raw($1$2"***")raw"));
    };

    uint64 nNonce = eventHttpReq.nNonce;

    string strResult;
    try
    {
        // check version
        string strVersion = eventHttpReq.data.mapHeader["url"].substr(1);
        if (!strVersion.empty())
        {
            if (!CheckVersion(strVersion))
            {
                throw CRPCException(RPC_VERSION_OUT_OF_DATE,
                                    string("Out of date version. Server version is v") + VERSION_STR
                                        + ", but client version is v" + strVersion);
            }
        }

        if (fWriteRPCLog)
        {
            Debug("request content [chainid=%d,port=%d,peer=%s:%d]: %s",
                  eventHttpReq.data.nReqChainId, eventHttpReq.data.nListenPort,
                  eventHttpReq.data.strPeerIp.c_str(), eventHttpReq.data.nPeerPort,
                  eventHttpReq.data.strContent.c_str());
        }

        uint256 hashReqFork;
        if (eventHttpReq.data.nReqChainId == 0)
        {
            hashReqFork = pCoreProtocol->GetGenesisBlockHash();
        }
        else
        {
            if (!pService->GetForkHashByChainId(eventHttpReq.data.nReqChainId, hashReqFork))
            {
                throw CRPCException(RPC_REQUEST_FUNC_OBSOLETE, string("chain id error, chain id is ") + to_string(eventHttpReq.data.nReqChainId));
            }
        }
        CReqContext ctxReq(hashReqFork, eventHttpReq.data.nReqChainId, eventHttpReq.data.nListenPort, eventHttpReq.data.strPeerIp, eventHttpReq.data.nPeerPort);

        const std::set<std::string> setNoParserMethod = { "eth_getBlockByHash", "eth_getBlockByNumber", "eth_call", "eth_estimateGas", "eth_newFilter", "eth_getLogs" };

        bool fArray = false;
        CRPCReqVec vecReq;
        {
            boost::unique_lock<boost::mutex> lock(mutexDec);
            vecReq = DeserializeCRPCReq(eventHttpReq.data.strContent, setNoParserMethod, fArray);
        }
        CRPCRespVec vecResp;
        for (auto& spReq : vecReq)
        {
            CRPCErrorPtr spError;
            CRPCResultPtr spResult;
            try
            {
                map<string, RPCFunc>::iterator it = mapRPCFunc.find(spReq->strMethod);
                if (it == mapRPCFunc.end())
                {
                    throw CRPCException(RPC_METHOD_NOT_FOUND, "Method not found");
                }

                // if (fWriteRPCLog)
                // {
                //     Debug("request : %s", lmdMask(spReq->Serialize()).c_str());
                // }

                ctxReq.strMethod = spReq->strMethod;

                spResult = (this->*(*it).second)(ctxReq, spReq->spParam);
            }
            catch (CRPCException& e)
            {
                spError = CRPCErrorPtr(new CRPCError(e));
            }
            catch (exception& e)
            {
                spError = CRPCErrorPtr(new CRPCError(RPC_MISC_ERROR, e.what()));
            }

            if (spError)
            {
                vecResp.push_back(MakeCRPCRespPtr(spReq->valID, spError));
            }
            else if (spResult)
            {
                vecResp.push_back(MakeCRPCRespPtr(spReq->valID, spResult));
            }
            else
            {
                // no result means no return
                vecResp.push_back(MakeCRPCRespPtr(spReq->valID, spResult));
            }
        }

        if (fArray)
        {
            strResult = SerializeCRPCResp(vecResp);
        }
        else if (vecResp.size() > 0)
        {
            strResult = vecResp[0]->Serialize();
        }
        else
        {
            // no result means no return
        }
    }
    catch (CRPCException& e)
    {
        auto spError = MakeCRPCErrorPtr(e);
        CRPCResp resp(e.valData, spError);
        strResult = resp.Serialize();
    }
    catch (exception& e)
    {
        cout << "error: " << e.what() << endl;
        auto spError = MakeCRPCErrorPtr(RPC_MISC_ERROR, e.what());
        CRPCResp resp(Value(), spError);
        strResult = resp.Serialize();
    }

    if (fWriteRPCLog)
    {
        Debug("response : %s ", lmdMask(strResult).c_str());
    }

    // no result means no return
    if (!strResult.empty())
    {
        JsonReply(nNonce, strResult);
    }

    return true;
}

bool CRPCMod::HandleEvent(CEventHttpBroken& eventHttpBroken)
{
    (void)eventHttpBroken;
    return true;
}

void CRPCMod::JsonReply(uint64 nNonce, const std::string& result)
{
    CEventHttpRsp eventHttpRsp(nNonce);
    eventHttpRsp.data.nStatusCode = 200;
    eventHttpRsp.data.mapHeader["content-type"] = "application/json";
    eventHttpRsp.data.mapHeader["connection"] = "Keep-Alive";
    eventHttpRsp.data.mapHeader["server"] = "metabasenet-rpc";
    eventHttpRsp.data.strContent = result + "\n";

    pHttpServer->DispatchEvent(&eventHttpRsp);
}

int CRPCMod::GetInt(const CRPCInt64& i, int valDefault)
{
    return i.IsValid() ? int(i) : valDefault;
}

unsigned int CRPCMod::GetUint(const CRPCUint64& i, unsigned int valDefault)
{
    return i.IsValid() ? uint64(i) : valDefault;
}

uint64 CRPCMod::GetUint64(const CRPCUint64& i, uint64 valDefault)
{
    return i.IsValid() ? uint64(i) : valDefault;
}

bool CRPCMod::GetForkHashOfDef(const CRPCString& hex, const uint256& hashDefFork, uint256& hashFork)
{
    if (!hex.empty())
    {
        if (hex.size() >= hashFork.size() * 2)
        {
            if (hashFork.SetHex(hex) != hex.size())
            {
                return false;
            }
        }
        else
        {
            CChainId nChainId;
            if (hex.size() >= 2 && ((string)hex).substr(0, 2) == string("0x"))
            {
                nChainId = ParseNumericHexString(hex);
            }
            else
            {
                nChainId = atoi(hex.c_str());
            }
            if (!pService->GetForkHashByChainId(nChainId, hashFork))
            {
                return false;
            }
        }
    }
    else
    {
        if (hashDefFork != 0)
        {
            hashFork = hashDefFork;
        }
        else
        {
            hashFork = pCoreProtocol->GetGenesisBlockHash();
        }
    }
    return true;
}

void CRPCMod::ListDestination(vector<tuple<CDestination, uint8, uint512>>& vDestination, const uint64 nPage, const uint64 nCount)
{
    set<crypto::CPubKey> setPubKey;
    size_t nPubkeyCount = pService->GetPubKeys(setPubKey, nCount * nPage, nCount);

    vDestination.clear();
    for (const crypto::CPubKey& pubkey : setPubKey)
    {
        uint8 nType = CDestination::PREFIX_PUBKEY;
        nType <<= 5;
        vDestination.push_back(make_tuple(CDestination(pubkey), nType, pubkey));
    }
    if (nCount > 0 && vDestination.size() >= nCount)
    {
        return;
    }

    uint64 nTidStartPos = 0;
    uint64 nTidCount = nCount;
    if (nCount > 0 && nPubkeyCount > 0)
    {
        if (nCount * nPage > nPubkeyCount)
        {
            nTidStartPos = nCount * nPage - nPubkeyCount;
        }
        nTidCount = nCount - vDestination.size();
    }

    set<pair<uint8, CTemplateId>> setTid;
    pService->GetTemplateIds(setTid, nTidStartPos, nTidCount);
    for (const auto& vd : setTid)
    {
        uint8 nType = CDestination::PREFIX_TEMPLATE;
        nType = (nType << 5) | vd.first;
        vDestination.push_back(make_tuple(CDestination(vd.second), nType, uint512()));
    }
}

bool CRPCMod::CheckVersion(string& strVersion)
{
    int nMajor, nMinor, nRevision;
    if (!ResolveVersion(strVersion, nMajor, nMinor, nRevision))
    {
        return false;
    }

    strVersion = FormatVersion(nMajor, nMinor, nRevision);
    if (nMajor != VERSION_MAJOR || nMinor != VERSION_MINOR)
    {
        return false;
    }

    return true;
}

string CRPCMod::GetWidthString(const string& strIn, int nWidth)
{
    string str = strIn;
    int nCurLen = str.size();
    if (nWidth > nCurLen)
    {
        str.append(nWidth - nCurLen, ' ');
    }
    return str;
}

std::string CRPCMod::GetWidthString(uint64 nCount, int nWidth)
{
    char tempbuf[12] = { 0 };
    sprintf(tempbuf, "%2.2d", (int)(nCount % 100));
    return GetWidthString(std::to_string(nCount / 100) + std::string(".") + tempbuf, nWidth);
}

uint256 CRPCMod::GetRefBlock(const uint256& hashFork, const string& strRefBlock, const bool fDefZero)
{
    uint256 hashBlock;
    if (!strRefBlock.empty())
    {
        if (strRefBlock == string("latest"))
        {
            int nLastHeight;
            if (!pService->GetForkLastBlock(hashFork, nLastHeight, hashBlock))
            {
                hashBlock = 0;
            }
        }
        else if (strRefBlock == string("earliest"))
        {
            hashBlock = hashFork;
        }
        else if (strRefBlock == string("pending"))
        {
            hashBlock = 0;
        }
        else if (strRefBlock.size() >= uint256::size() * 2)
        {
            hashBlock.SetHex(strRefBlock);
        }
        else if (isNumeric(strRefBlock))
        {
            pService->GetBlockNumberHash(hashFork, std::stoll(strRefBlock), hashBlock);
        }
        else if (isHexNumeric(strRefBlock))
        {
            pService->GetBlockNumberHash(hashFork, std::stoll(strRefBlock, nullptr, 16), hashBlock);
        }
    }
    else
    {
        if (!fDefZero)
        {
            int nLastHeight;
            if (!pService->GetForkLastBlock(hashFork, nLastHeight, hashBlock))
            {
                hashBlock = hashFork;
            }
        }
    }
    return hashBlock;
}

bool CRPCMod::VerifyClientOrder(const CReqContext& ctxReq)
{
    if (ctxReq.strPeerIp != string("127.0.0.1"))
    {
        StdLog("CRPCMod", "%s: Permission denied, peer: %s:%d", ctxReq.strMethod.c_str(), ctxReq.strPeerIp.c_str(), ctxReq.nPeerPort);
        return false;
    }
    return true;
}

CLogsFilter CRPCMod::GetLogFilterFromJson(const uint256& hashFork, const std::string& strJsonValue)
{
    CLogsFilter logFilter;

    json_spirit::Value valParam;
    if (!json_spirit::read_string(strJsonValue, valParam, RPC_MAX_DEPTH))
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }
    if (valParam.type() != json_spirit::array_type || valParam.get_array().size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must be an array and non empty.");
    }

    const json_spirit::Object& item = valParam.get_array()[0].get_obj();

    json_spirit::Value from_block = find_value(item, "fromBlock");
    if (!from_block.is_null())
    {
        if (from_block.type() != json_spirit::str_type)
        {
            throw CRPCException(RPC_INVALID_REQUEST, "fromBlock type error");
        }
        if (from_block.get_str() == std::string("earliest") || from_block.get_str() == std::string("pending"))
        {
            logFilter.withFromBlock(0);
        }
        else
        {
            logFilter.withFromBlock(GetRefBlock(hashFork, from_block.get_str()));
        }
    }
    else
    {
        logFilter.withFromBlock(0);
    }

    json_spirit::Value to_block = find_value(item, "toBlock");
    if (!to_block.is_null())
    {
        if (to_block.type() != json_spirit::str_type)
        {
            throw CRPCException(RPC_INVALID_REQUEST, "toBlock type error");
        }
        if (from_block.get_str() == std::string("latest") || from_block.get_str() == std::string("pending"))
        {
            logFilter.withToBlock(0);
        }
        else
        {
            logFilter.withToBlock(GetRefBlock(hashFork, to_block.get_str()));
        }
    }
    else
    {
        logFilter.withToBlock(0);
    }

    json_spirit::Value value_address = find_value(item, "address");
    if (!value_address.is_null())
    {
        if (value_address.type() == json_spirit::str_type)
        {
            CDestination dest(value_address.get_str());
            if (!dest.IsNull())
            {
                logFilter.addAddress(dest);
            }
        }
        else if (value_address.type() == json_spirit::array_type)
        {
            for (size_t i = 0; i < value_address.get_array().size(); ++i)
            {
                const json_spirit::Value item_address = value_address.get_array()[i];
                if (item_address.type() == json_spirit::str_type)
                {
                    CDestination dest(item_address.get_str());
                    if (!dest.IsNull())
                    {
                        logFilter.addAddress(dest);
                    }
                }
            }
        }
        else
        {
            throw CRPCException(RPC_INVALID_REQUEST, "address type error");
        }
    }

    json_spirit::Value value_topics = find_value(item, "topics");
    if (!value_topics.is_null())
    {
        if (value_topics.type() != json_spirit::array_type)
        {
            throw CRPCException(RPC_INVALID_REQUEST, "topics type error");
        }
        for (size_t i = 0; i < value_topics.get_array().size() && i < CLogsFilter::MAX_LOGS_FILTER_TOPIC_COUNT; ++i)
        {
            const json_spirit::Value item_topics = value_topics.get_array()[i];
            if (item_topics.type() == json_spirit::str_type)
            {
                if (!item_topics.get_str().empty())
                {
                    uint256 hash;
                    hash.SetHex(item_topics.get_str());
                    if (hash != 0)
                    {
                        logFilter.addTopic(i, hash);
                    }
                }
            }
            else if (item_topics.type() == json_spirit::array_type)
            {
                for (size_t j = 0; j < value_topics.get_array().size(); ++j)
                {
                    const json_spirit::Value next_topics = value_topics.get_array()[j];
                    if (next_topics.type() == json_spirit::str_type && !next_topics.get_str().empty())
                    {
                        uint256 hash;
                        hash.SetHex(next_topics.get_str());
                        if (hash != 0)
                        {
                            logFilter.addTopic(i, hash);
                        }
                    }
                }
            }
            else if (item_topics.type() == json_spirit::null_type)
            {
                logFilter.addTopic(i, uint256());
            }
            if (logFilter.isTopicEmpty(i))
            {
                logFilter.addTopic(i, uint256());
            }
        }
    }

    return logFilter;
}

/////////////////////////////////////////////////////////////////////////////////////
/* System */
CRPCResultPtr CRPCMod::RPCHelp(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CHelpParam>(param);
    string command = spParam->strCommand;
    return MakeCHelpResultPtr(RPCHelpInfo(EModeType::MODE_CONSOLE, command));
}

CRPCResultPtr CRPCMod::RPCStop(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    pService->Stop();
    return MakeCStopResultPtr("metabasenet server stopping");
}

CRPCResultPtr CRPCMod::RPCVersion(const CReqContext& ctxReq, CRPCParamPtr param)
{
    string strVersion = string("metabasenet server version is v") + VERSION_STR + string(", git commit id is ") + GetGitVersion();
    return MakeCVersionResultPtr(strVersion);
}

/* Network */
CRPCResultPtr CRPCMod::RPCGetPeerCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    return MakeCGetPeerCountResultPtr(pService->GetPeerCount());
}

CRPCResultPtr CRPCMod::RPCListPeer(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    vector<network::CBbPeerInfo> vPeerInfo;
    pService->GetPeers(vPeerInfo);

    auto spResult = MakeCListPeerResultPtr();
    for (const network::CBbPeerInfo& info : vPeerInfo)
    {
        CListPeerResult::CPeer peer;
        peer.strAddress = info.strAddress;
        if (info.nService == 0)
        {
            // Handshaking
            peer.strServices = "NON";
        }
        else
        {
            if (info.nService & network::NODE_NETWORK)
            {
                peer.strServices = "NODE_NETWORK";
            }
            if (info.nService & network::NODE_DELEGATED)
            {
                if (peer.strServices.empty())
                {
                    peer.strServices = "NODE_DELEGATED";
                }
                else
                {
                    peer.strServices = peer.strServices + ",NODE_DELEGATED";
                }
            }
            if (peer.strServices.empty())
            {
                peer.strServices = string("OTHER:") + to_string(info.nService);
            }
        }
        peer.strLastsend = GetTimeString(info.nLastSend);
        peer.strLastrecv = GetTimeString(info.nLastRecv);
        peer.strConntime = GetTimeString(info.nActive);
        peer.nPingtime = info.nPingPongTimeDelta;
        peer.strVersion = FormatVersion(info.nVersion);
        peer.strSubver = info.strSubVer;
        peer.fInbound = info.fInBound;
        peer.nHeight = info.nStartingHeight;
        peer.nBanscore = info.nScore;
        spResult->vecPeer.push_back(peer);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCAddNode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    auto spParam = CastParamPtr<CAddNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->AddNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to add node.");
    }

    return MakeCAddNodeResultPtr(string("Add node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCRemoveNode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    auto spParam = CastParamPtr<CRemoveNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->RemoveNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to remove node.");
    }

    return MakeCRemoveNodeResultPtr(string("Remove node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCGetForkPort(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    auto spParam = CastParamPtr<CGetForkPortParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strForkid, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid forkid");
    }

    uint16 nPort;
    if (!pService->GetForkRpcPort(hashFork, nPort))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid forkid");
    }
    return MakeCGetForkPortResultPtr(nPort);
}

CRPCResultPtr CRPCMod::RPCGetForkCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCGetForkCountResultPtr(pService->GetForkCount());
}

CRPCResultPtr CRPCMod::RPCListFork(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListForkParam>(param);
    vector<pair<uint256, CProfile>> vFork;
    pService->ListFork(vFork, spParam->fAll);
    auto spResult = MakeCListForkResultPtr();
    for (size_t i = 0; i < vFork.size(); i++)
    {
        const uint256& hashFork = vFork[i].first;
        CProfile& profile = vFork[i].second;

        CForkContext forkContext;
        if (!pService->GetForkContext(hashFork, forkContext))
        {
            StdError("CRPCMod", "RPCListFork: Get fork context fail, fork: %s", hashFork.GetHex().c_str());
            continue;
        }

        CBlockStatus lastStatus;
        pService->GetLastBlockStatus(hashFork, lastStatus);

        uint64 nAddressCount = 0;
        uint64 nNewAddressCount = 0;
        pService->GetAddressCount(hashFork, lastStatus.hashBlock, nAddressCount, nNewAddressCount);

        CListForkResult::CProfile displayProfile;
        displayProfile.strFork = hashFork.GetHex();
        displayProfile.strType = CProfile::GetTypeStr(profile.nType);
        displayProfile.nChainid = profile.nChainId;
        displayProfile.strName = profile.strName;
        displayProfile.strSymbol = profile.strSymbol;
        displayProfile.strAmount = CoinToTokenBigFloat(profile.nAmount);
        displayProfile.strReward = CoinToTokenBigFloat(profile.nMintReward);
        displayProfile.nHalvecycle = (uint64)(profile.nHalveCycle);
        displayProfile.strOwner = profile.destOwner.ToString();
        displayProfile.strCreatetxid = forkContext.txidEmbedded.GetHex();
        displayProfile.nCreateforkheight = forkContext.nJointHeight;
        displayProfile.strParentfork = forkContext.hashParent.GetHex();
        displayProfile.nForkheight = lastStatus.nBlockHeight;
        displayProfile.strLasttype = GetBlockTypeStr(lastStatus.nBlockType, lastStatus.nMintType);
        displayProfile.nLastnumber = lastStatus.nBlockNumber;
        displayProfile.nLastslot = lastStatus.nBlockSlot;
        displayProfile.strLastblock = lastStatus.hashBlock.ToString();
        displayProfile.nTotaltxcount = lastStatus.nTotalTxCount;
        displayProfile.nRewardtxcount = lastStatus.nRewardTxCount;
        displayProfile.nUsertxcount = lastStatus.nUserTxCount;
        displayProfile.nAddresscount = nAddressCount;
        displayProfile.strMoneysupply = CoinToTokenBigFloat(lastStatus.nMoneySupply);
        displayProfile.strMoneydestroy = CoinToTokenBigFloat(lastStatus.nMoneyDestroy);

        spResult->vecProfile.push_back(displayProfile);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetForkGenealogy(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetGenealogyParam>(param);

    //getgenealogy (-f="fork")
    uint256 fork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, fork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    vector<pair<uint256, int>> vAncestry;
    vector<pair<int, uint256>> vSubline;
    if (!pService->GetForkGenealogy(fork, vAncestry, vSubline))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    auto spResult = MakeCGetGenealogyResultPtr();
    for (int i = vAncestry.size(); i > 0; i--)
    {
        spResult->vecAncestry.push_back({ vAncestry[i - 1].first.GetHex(), vAncestry[i - 1].second });
    }
    for (std::size_t i = 0; i < vSubline.size(); i++)
    {
        spResult->vecSubline.push_back({ vSubline[i].second.GetHex(), vSubline[i].first });
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockLocation(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockLocationParam>(param);

    //getblocklocation <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CChainId nChainId;
    uint256 fork;
    int height;
    if (!pService->GetBlockLocation(hashBlock, nChainId, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    auto spResult = MakeCGetBlockLocationResultPtr();
    spResult->nChainid = nChainId;
    spResult->strFork = fork.GetHex();
    spResult->nHeight = height;
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockCountParam>(param);

    //getblockcount (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetBlockCountResultPtr(pService->GetBlockCount(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetBlockHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockHashParam>(param);

    //getblockhash <height> (-f="fork")
    int nHeight = spParam->nHeight;

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<uint256> vBlockHash;
    if (!pService->GetBlockHashList(hashFork, nHeight, vBlockHash))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block number out of range.");
    }

    auto spResult = MakeCGetBlockHashResultPtr();
    for (const uint256& hash : vBlockHash)
    {
        spResult->vecHash.push_back(hash.GetHex());
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockNumberHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockNumberHashParam>(param);

    uint64 nNumber = spParam->nNumber;

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock;
    if (!pService->GetBlockNumberHash(hashFork, nNumber, hashBlock))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block number out of range.");
    }

    return MakeCGetBlockNumberHashResultPtr(hashBlock.GetHex());
}

CRPCResultPtr CRPCMod::RPCGetBlock(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockParam>(param);

    //getblock <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlock block;
    CChainId nChainId;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, nChainId, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    return MakeCGetBlockResultPtr(BlockToJSON(hashBlock, block, nChainId, fork, height, block.GetBlockTotalReward()));
}

CRPCResultPtr CRPCMod::RPCGetBlockDetail(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CgetblockdetailParam>(param);

    //getblockdetail <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlockEx block;
    CChainId nChainId;
    uint256 hashFork;
    int height;
    if (!pService->GetBlockEx(hashBlock, block, nChainId, hashFork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    Cblockdatadetail data;

    data.strHash = hashBlock.GetHex();
    data.strPrev = block.hashPrev.GetHex();
    data.nChainid = nChainId;
    data.strFork = hashFork.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.GetTxType());
    data.nTime = block.GetBlockTime();
    data.nNumber = block.GetBlockNumber();
    data.nHeight = height;
    data.nSlot = block.GetBlockSlot();
    data.strReward = CoinToTokenBigFloat(block.GetBlockTotalReward());
    data.strStateroot = block.hashStateRoot.GetHex();
    if (!block.hashReceiptsRoot.IsNull())
    {
        data.strReceiptsroot = block.hashReceiptsRoot.GetHex();
    }
    else
    {
        data.strReceiptsroot = "0x";
    }
    if (!block.btBloomData.empty())
    {
        data.strBloom = ToHexString(block.btBloomData);
    }
    else
    {
        data.strBloom = "0x";
    }
    int nDepth = height < 0 ? 0 : pService->GetForkHeight(hashFork) - height;
    data.txmint = TxToJSON(block.txMint.GetHash(), block.txMint, hashFork, hashBlock, nDepth);
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        data.vecTx.push_back(TxToJSON(tx.GetHash(), tx, hashFork, hashBlock, nDepth));
    }
    return MakeCgetblockdetailResultPtr(data);
}

CRPCResultPtr CRPCMod::RPCGetBlockData(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockDataParam>(param);

    uint256 hashBlock;
    if (spParam->strBlockhash.IsValid())
    {
        hashBlock.SetHex(spParam->strBlockhash);
    }
    if (hashBlock == 0)
    {
        uint256 hashFork;
        if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
        }
        if (!pService->HaveFork(hashFork))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
        }

        if (spParam->nNumber.IsValid())
        {
            uint64 nBlockNumber = spParam->nNumber;
            if (!pService->GetBlockNumberHash(hashFork, nBlockNumber, hashBlock))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid number");
            }
        }
        if (hashBlock == 0)
        {
            if (spParam->nHeight.IsValid())
            {
                uint16 nSlot = 0;
                if (spParam->nSlot.IsValid())
                {
                    nSlot = spParam->nSlot;
                }
                uint32 nBlockHeight = spParam->nHeight;
                if (!pService->GetBlockHashByHeightSlot(hashFork, nBlockHeight, nSlot, hashBlock))
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "Invalid height or slot");
                }
            }
        }
    }
    if (hashBlock == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
    }

    CBlock block;
    CChainId nChainId;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, nChainId, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    return MakeCGetBlockDataResultPtr(BlockToJSON(hashBlock, block, nChainId, fork, height, block.GetBlockTotalReward()));
}

CRPCResultPtr CRPCMod::RPCGetTxPool(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTxPoolParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CDestination address;
    if (spParam->strAddress.IsValid())
    {
        address.ParseString(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
    }
    bool fDetail = spParam->fDetail.IsValid() ? bool(spParam->fDetail) : false;
    int64 nGetOffset = spParam->nGetoffset.IsValid() ? int64(spParam->nGetoffset) : 0;
    int64 nGetCount = spParam->nGetcount.IsValid() ? int64(spParam->nGetcount) : 20;

    auto spResult = MakeCGetTxPoolResultPtr();
    if (!fDetail)
    {
        vector<pair<uint256, size_t>> vTxPool;
        pService->GetTxPool(hashFork, vTxPool);

        size_t nTotalSize = 0;
        for (std::size_t i = 0; i < vTxPool.size(); i++)
        {
            nTotalSize += vTxPool[i].second;
        }
        spResult->nCount = vTxPool.size();
        spResult->nSize = nTotalSize;
    }
    else
    {
        vector<CTxInfo> vTxPool;
        pService->ListTxPool(hashFork, address, vTxPool, nGetOffset, nGetCount);

        for (const CTxInfo& txinfo : vTxPool)
        {
            uint256 nTxFee = txinfo.nGasPrice * txinfo.nGas;
            spResult->vecList.push_back({ txinfo.txid.GetHex(), CTransaction::GetTypeStringStatic(txinfo.nTxType), txinfo.nTxNonce,
                                          txinfo.destFrom.ToString(), txinfo.destTo.ToString(),
                                          CoinToTokenBigFloat(txinfo.nAmount), CoinToTokenBigFloat(txinfo.nGasPrice), txinfo.nGas.Get64(),
                                          CoinToTokenBigFloat(nTxFee), txinfo.nSize });
        }
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionParam>(param);
    uint256 txid;
    txid.SetHex(spParam->strTxid);
    if (txid == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txid");
    }

    uint256 hashRefFork;
    if (spParam->strFork.IsValid())
    {
        if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashRefFork))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
        }
    }
    else
    {
        hashRefFork = ctxReq.hashFork;
    }
    if (!pService->HaveFork(hashRefFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CTransaction tx;
    uint256 hashFork;
    uint256 hashBlock;
    uint64 nBlockNumber = 0;
    uint16 nTxIndex = 0;

    if (!pService->GetTransactionAndPosition(hashRefFork, txid, tx, hashFork, hashBlock, nBlockNumber, nTxIndex))
    {
        throw CRPCException(RPC_INVALID_REQUEST, "No information available about transaction");
    }

    auto spResult = MakeCGetTransactionResultPtr();

    if (spParam->fSerialized)
    {
        CBufStream ss;
        ss << tx;
        spResult->strSerialization = ToHexString((const unsigned char*)ss.GetData(), ss.GetSize());
        return spResult;
    }

    int nHeight = -1;
    if (hashBlock != 0)
    {
        nHeight = CBlock::GetBlockHeightByHash(hashBlock);
    }
    int nDepth = nHeight < 0 ? 0 : pService->GetForkHeight(hashFork) - nHeight;

    spResult->transaction = TxToJSON(txid, tx, hashFork, hashBlock, nDepth);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSendTransactionParam>(param);

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());

    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    uint256 hashFork;
    if (!pService->GetForkHashByChainId(rawTx.GetChainId(), hashFork))
    {
        throw CRPCException(RPC_TRANSACTION_ERROR, "Get fork hash failed");
    }

    Errno err = pService->SendTransaction(hashFork, rawTx);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ") + ErrorString(err));
    }

    return MakeCSendTransactionResultPtr(rawTx.GetHash().GetHex());
}

CRPCResultPtr CRPCMod::RPCGetForkHeight(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetForkHeightParam>(param);

    //getforkheight (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetForkHeightResultPtr(pService->GetForkHeight(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetVotes(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetVotesParam>(param);

    CDestination destDelegate;
    destDelegate.ParseString(spParam->strAddress);
    if (destDelegate.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint256 hashRefBlock = GetRefBlock(pCoreProtocol->GetGenesisBlockHash(), spParam->strBlock);
    if (hashRefBlock != 0)
    {
        CChainId nChainId;
        uint256 hashRefFork;
        int nHeight;
        if (!pService->GetBlockLocation(hashRefBlock, nChainId, hashRefFork, nHeight))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
        if (hashRefFork != pCoreProtocol->GetGenesisBlockHash())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
    }
    else
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(pCoreProtocol->GetGenesisBlockHash(), nLastHeight, hashRefBlock))
        {
            hashRefBlock = pCoreProtocol->GetGenesisBlockHash();
        }
    }

    uint256 nVotesToken;
    string strFailCause;
    if (!pService->GetVotes(hashRefBlock, destDelegate, nVotesToken, strFailCause))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, strFailCause);
    }

    return MakeCGetVotesResultPtr(CoinToTokenBigFloat(nVotesToken));
}

CRPCResultPtr CRPCMod::RPCListDelegate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListDelegateParam>(param);

    uint256 hashRefBlock = GetRefBlock(pCoreProtocol->GetGenesisBlockHash(), spParam->strBlock);
    if (hashRefBlock != 0)
    {
        CChainId nChainId;
        uint256 hashRefFork;
        int nHeight;
        if (!pService->GetBlockLocation(hashRefBlock, nChainId, hashRefFork, nHeight))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
        if (hashRefFork != pCoreProtocol->GetGenesisBlockHash())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
    }
    else
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(pCoreProtocol->GetGenesisBlockHash(), nLastHeight, hashRefBlock))
        {
            hashRefBlock = pCoreProtocol->GetGenesisBlockHash();
        }
    }

    std::multimap<uint256, CDestination> mapVotes;
    if (!pService->ListDelegate(hashRefBlock, 0, spParam->nCount, mapVotes))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Query fail");
    }

    auto spResult = MakeCListDelegateResultPtr();
    for (const auto& d : boost::adaptors::reverse(mapVotes))
    {
        CListDelegateResult::CDelegate delegateData;
        delegateData.strAddress = d.second.ToString();
        delegateData.strVotes = CoinToTokenBigFloat(d.first);
        spResult->vecDelegate.push_back(delegateData);
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetDelegateVotes(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetDelegateVotesParam>(param);

    CDestination destDelegate;
    destDelegate.ParseString(spParam->strAddress);
    if (destDelegate.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashRefBlock = GetRefBlock(pCoreProtocol->GetGenesisBlockHash(), spParam->strBlock);
    if (hashRefBlock != 0)
    {
        CChainId nChainId;
        uint256 hashRefFork;
        int nHeight;
        if (!pService->GetBlockLocation(hashRefBlock, nChainId, hashRefFork, nHeight))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
        if (hashRefFork != pCoreProtocol->GetGenesisBlockHash())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
    }
    else
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(pCoreProtocol->GetGenesisBlockHash(), nLastHeight, hashRefBlock))
        {
            hashRefBlock = pCoreProtocol->GetGenesisBlockHash();
        }
    }

    uint256 nVotesToken;
    if (!pService->GetDelegateVotes(hashRefBlock, destDelegate, nVotesToken))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Get votes fail");
    }

    return MakeCGetDelegateVotesResultPtr(CoinToTokenBigFloat(nVotesToken));
}

CRPCResultPtr CRPCMod::RPCGetUserVotes(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetUserVotesParam>(param);

    CDestination destVote;
    destVote.ParseString(spParam->strAddress);
    if (destVote.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashRefBlock = GetRefBlock(pCoreProtocol->GetGenesisBlockHash(), spParam->strBlock);
    if (hashRefBlock != 0)
    {
        CChainId nChainId;
        uint256 hashRefFork;
        int nHeight;
        if (!pService->GetBlockLocation(hashRefBlock, nChainId, hashRefFork, nHeight))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
        if (hashRefFork != pCoreProtocol->GetGenesisBlockHash())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
        }
    }
    else
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(pCoreProtocol->GetGenesisBlockHash(), nLastHeight, hashRefBlock))
        {
            hashRefBlock = pCoreProtocol->GetGenesisBlockHash();
        }
    }

    uint32 nTemplateType = 0;
    uint256 nVotes;
    uint32 nUnlockHeight = 0;
    if (!pService->GetUserVotes(hashRefBlock, destVote, nTemplateType, nVotes, nUnlockHeight))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Get votes fail");
    }

    auto spResult = MakeCGetUserVotesResultPtr();

    switch (nTemplateType)
    {
    case TEMPLATE_DELEGATE:
        spResult->strType = "delegate";
        break;
    case TEMPLATE_VOTE:
        spResult->strType = "vote";
        break;
    case TEMPLATE_PLEDGE:
        spResult->strType = "pledge";
        break;
    default:
        spResult->strType = "other";
    }
    spResult->strVotes = CoinToTokenBigFloat(nVotes);
    spResult->nUnlockheight = nUnlockHeight;

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetTimeVault(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTimeVaultParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashRefBlock = GetRefBlock(hashFork, spParam->strBlock);
    if (hashRefBlock == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
    }
    CBlockStatus status;
    if (!pService->GetBlockStatus(hashRefBlock, status))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block status error");
    }
    if (hashFork != status.hashFork)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block is not on chain");
    }

    CTimeVault tv;
    if (!pService->RetrieveTimeVault(hashFork, hashRefBlock, address, tv))
    {
        tv.SetNull();
    }
    uint32 nPrevSettlementTime = tv.nPrevSettlementTime;
    tv.SettlementTimeVault(status.nBlockTime);

    auto spResult = MakeCGetTimeVaultResultPtr();
    if (tv.fSurplus || tv.nTvAmount == 0)
    {
        spResult->strTimevault = CoinToTokenBigFloat(tv.nTvAmount);
    }
    else
    {
        spResult->strTimevault = (std::string("-") + CoinToTokenBigFloat(tv.nTvAmount));
    }
    spResult->strBalance = CoinToTokenBigFloat(tv.nBalanceAmount);
    spResult->nPrevsettlementtime = nPrevSettlementTime;

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetAddressCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetAddressCountParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashRefBlock = GetRefBlock(hashFork, spParam->strBlock);
    if (hashRefBlock == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
    }
    CBlockStatus status;
    if (!pService->GetBlockStatus(hashRefBlock, status))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block status error");
    }
    if (hashFork != status.hashFork)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block is not on chain");
    }

    uint64 nAddressCount = 0;
    uint64 nNewAddressCount = 0;
    if (!pService->GetAddressCount(hashFork, hashRefBlock, nAddressCount, nNewAddressCount))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Get address count fail");
    }

    return MakeCGetAddressCountResultPtr(hashRefBlock.ToString(), nAddressCount, nNewAddressCount);
}

/* Wallet */
CRPCResultPtr CRPCMod::RPCListKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListKeyParam>(param);

    uint64 nPage = 0;
    uint64 nCount = 30;
    if (spParam->nPage.IsValid())
    {
        nPage = spParam->nPage;
    }
    if (spParam->nCount.IsValid())
    {
        nCount = spParam->nCount;
    }

    set<crypto::CPubKey> setPubKey;
    pService->GetPubKeys(setPubKey, nCount * nPage, nCount);

    auto spResult = MakeCListKeyResultPtr();
    for (const crypto::CPubKey& pubkey : setPubKey)
    {
        int nVersion;
        bool fLocked, fPublic;
        int64 nAutoLockTime;
        if (pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
        {
            CListKeyResult::CPubkey p;
            p.strPubkey = pubkey.GetHex();
            p.strAddress = CDestination(pubkey).ToString();
            p.nVersion = nVersion;
            p.fPublic = fPublic;
            p.fLocked = fLocked;
            if (!fLocked && nAutoLockTime > 0)
            {
                p.nTimeout = (nAutoLockTime - GetTime());
            }
            spResult->vecPubkey.push_back(p);
        }
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetNewKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetNewKeyParam>(param);

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
    crypto::CPubKey pubkey;
    auto strErr = pService->MakeNewKey(strPassphrase, pubkey);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed add new key: ") + *strErr);
    }

    auto spResult = MakeCGetNewKeyResultPtr();
    spResult->strPubkey = pubkey.ToString();
    spResult->strAddress = CDestination(pubkey).ToString();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCEncryptKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CEncryptKeyParam>(param);

    //encryptkey <"pubkey"> <-new="passphrase"> <-old="oldpassphrase">
    CDestination address;
    if (!address.ParseString(spParam->strPubkey))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }
    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();

    if (spParam->strOldpassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Old passphrase must be nonempty");
    }
    crypto::CCryptoString strOldPassphrase = spParam->strOldpassphrase.c_str();

    if (!pService->HaveKey(address, crypto::CKey::PRIVATE_KEY))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (!pService->EncryptKey(address, strPassphrase, strOldPassphrase))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCEncryptKeyResultPtr(string("Encrypt key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCLockKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CLockKeyParam>(param);

    CDestination address;
    if (!address.ParseString(spParam->strPubkey))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(address, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't lock public key");
    }
    if (!fLocked && !pService->Lock(address))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to lock key");
    }
    return MakeCLockKeyResultPtr(string("Lock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCUnlockKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CUnlockKeyParam>(param);

    CDestination address;
    if (!address.ParseString(spParam->strPubkey))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
    int64 nTimeout = 0;
    if (spParam->nTimeout.IsValid())
    {
        nTimeout = spParam->nTimeout;
    }
    else if (!RPCServerConfig()->fDebug)
    {
        nTimeout = UNLOCKKEY_RELEASE_DEFAULT_TIME;
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(address, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't unlock public key");
    }
    if (!fLocked)
    {
        throw CRPCException(RPC_WALLET_ALREADY_UNLOCKED, "Key is already unlocked");
    }

    if (!pService->Unlock(address, strPassphrase, nTimeout))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCUnlockKeyResultPtr(string("Unlock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCRemoveKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveKeyParam>(param);

    CDestination address;
    if (!address.ParseString(spParam->strPubkey))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(address, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }

    if (!fPublic)
    {
        pService->Lock(address);
        crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
        if (!pService->Unlock(address, strPassphrase, UNLOCKKEY_RELEASE_DEFAULT_TIME))
        {
            throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "Can't remove key with incorrect passphrase");
        }
    }

    auto strErr = pService->RemoveKey(address);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_REMOVE_KEY_ERROR, *strErr);
    }

    return MakeCRemoveKeyResultPtr(string("Remove key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCImportPrivKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportPrivKeyParam>(param);

    //importprivkey <"privkey"> <"passphrase">
    uint256 nPriv;
    if (nPriv.SetHex(spParam->strPrivkey) != spParam->strPrivkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid private key");
    }

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();

    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid private key");
    }
    if (!pService->HaveKey(CDestination(key.GetPubKey()), crypto::CKey::PRIVATE_KEY))
    {
        if (!strPassphrase.empty())
        {
            key.Encrypt(strPassphrase);
        }
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    auto spResult = MakeCImportPrivKeyResultPtr();
    spResult->strPubkey = key.GetPubKey().ToString();
    spResult->strAddress = CDestination(key.GetPubKey()).ToString();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCImportPubKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportPubKeyParam>(param);

    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);
    if (!pubkey)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }
    CDestination address(pubkey);

    if (!pService->HaveKey(address))
    {
        crypto::CKey key;
        key.Load(pubkey, crypto::CKey::PUBLIC_KEY, crypto::CCryptoCipher());

        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    return MakeCImportPubKeyResultPtr(address.ToString());
}

CRPCResultPtr CRPCMod::RPCImportKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportKeyParam>(param);

    vector<unsigned char> vchKey = ParseHexString(spParam->strPubkey);
    crypto::CKey key;
    if (!key.Load(vchKey))
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Failed to verify serialized key");
    }
    if (key.GetVersion() == crypto::CKey::INIT)
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Can't import the key with empty passphrase");
    }
    CDestination dest(key.GetPubKey());
    if ((key.IsPrivKey() && !pService->HaveKey(dest, crypto::CKey::PRIVATE_KEY))
        || (key.IsPubKey() && !pService->HaveKey(dest)))
    {
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    return MakeCImportKeyResultPtr(key.GetPubKey().GetHex());
}

CRPCResultPtr CRPCMod::RPCExportKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportKeyParam>(param);

    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);
    if (!pubkey)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    CDestination dest(pubkey);
    if (!pService->HaveKey(dest))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    vector<unsigned char> vchKey;
    if (!pService->ExportKey(dest, vchKey))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
    }

    return MakeCExportKeyResultPtr(ToHexString(vchKey));
}

CRPCResultPtr CRPCMod::RPCAddNewTemplate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddNewTemplateParam>(param);

    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(spParam->data);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }

    CDestination dest(ptr->GetTemplateId());
    if (!pService->HaveTemplate(dest))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCAddNewTemplateResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCImportTemplate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportTemplateParam>(param);

    vector<unsigned char> vchTemplate = ParseHexString(spParam->strData);
    CTemplatePtr ptr = CTemplate::Import(vchTemplate);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }

    CDestination dest(ptr->GetTemplateId());
    if (!pService->HaveTemplate(dest))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCImportTemplateResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCExportTemplate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportTemplateParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    if (address.IsNull() /*|| !address.IsTemplate()*/)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    CTemplatePtr ptr = pService->GetTemplate(address);
    if (!ptr)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
    }

    return MakeCExportTemplateResultPtr(ToHexString(ptr->Export()));
}

CRPCResultPtr CRPCMod::RPCRemoveTemplate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveTemplateParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    if (address.IsNull() /*|| !address.IsTemplate()*/)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid template address");
    }

    if (!pService->RemoveTemplate(address))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Remove template address fail");
    }

    return MakeCRemoveTemplateResultPtr("Success");
}

CRPCResultPtr CRPCMod::RPCValidateAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CValidateAddressParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    bool isValid = !address.IsNull();

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    CAddressContext ctxAddress;
    if (!address.IsNull())
    {
        if (!pService->RetrieveAddressContext(hashFork, address, ctxAddress, hashBlock))
        {
            isValid = false;
        }
    }

    auto spResult = MakeCValidateAddressResultPtr();
    spResult->fIsvalid = isValid;
    if (isValid)
    {
        auto& addressData = spResult->addressdata;

        addressData.strAddress = address.ToString();
        if (ctxAddress.IsPubkey())
        {
            crypto::CPubKey pubkey;
            bool isMine = pService->GetPubkey(address, pubkey);
            addressData.fIsmine = isMine;
            addressData.strType = "pubkey";
            if (isMine)
            {
                addressData.strPubkey = pubkey.GetHex();
            }
        }
        else if (ctxAddress.IsTemplate())
        {
            CTemplatePtr ptr = pService->GetTemplate(address);
            if (ptr == nullptr)
            {
                bytes btTemplateData;
                if (pService->GetDestTemplateData(hashFork, hashBlock, address, btTemplateData))
                {
                    ptr = CTemplate::Import(btTemplateData);
                    if (ptr == nullptr)
                    {
                        StdLog("CRPCMod", "RPC Validate Address: Create template fail");
                    }
                }
                addressData.fIsmine = false;
            }
            else
            {
                addressData.fIsmine = true;
            }
            addressData.strType = "template";
            if (ptr)
            {
                addressData.strTemplate = CTemplate::GetTypeName(ptr->GetTemplateType());

                auto& templateData = addressData.templatedata;
                templateData.strHex = ToHexString(ptr->Export());
                templateData.strType = ptr->GetName();
                ptr->GetTemplateData(templateData);
            }
        }
        else if (ctxAddress.IsContract())
        {
            addressData.fIsmine = false;
            addressData.strType = "contract";
        }
        else
        {
            StdLog("CRPCMod", "RPC Validate Address: Invalid address, address: %s", address.ToString().c_str());
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBalance(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBalanceParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<tuple<CDestination, uint8, uint512>> vDes;
    if (spParam->strAddress.IsValid())
    {
        CDestination address;
        address.ParseString(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
        vDes.push_back(make_tuple(address, 0, uint512()));
    }
    else
    {
        uint64 nPage = 0;
        uint64 nCount = 30;
        if (spParam->nPage.IsValid())
        {
            nPage = spParam->nPage;
        }
        if (spParam->nCount.IsValid())
        {
            nCount = spParam->nCount;
        }
        ListDestination(vDes, nPage, nCount);
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock, true);

    auto spResult = MakeCGetBalanceResultPtr();
    for (const auto& vd : vDes)
    {
        const CDestination dest = std::get<0>(vd);
        CWalletBalance balance;
        if (!pService->GetBalance(hashFork, hashBlock, dest, balance))
        {
            balance.SetNull();
        }

        if (balance.nDestType == 0)
        {
            const uint8 nType = std::get<1>(vd);
            balance.nDestType = (nType >> 5);
            balance.nTemplateType = (nType & 0x1F);
        }

        string strDestType;
        switch (balance.nDestType)
        {
        case CDestination::PREFIX_PUBKEY:
            strDestType = "pubkey";
            break;
        case CDestination::PREFIX_TEMPLATE:
        {
            strDestType = "template-";
            strDestType += CTemplate::GetTypeName(balance.nTemplateType);
            break;
        }
        case CDestination::PREFIX_CONTRACT:
            strDestType = "contract";
            break;
        default:
            strDestType = "non";
            break;
        }

        CGetBalanceResult::CBalance b;
        b.strAddress = dest.ToString();
        b.strType = strDestType;
        b.nNonce = balance.nTxNonce;
        b.strAvail = CoinToTokenBigFloat(balance.nAvailable);
        b.strLocked = CoinToTokenBigFloat(balance.nLocked);
        b.strUnconfirmedin = CoinToTokenBigFloat(balance.nUnconfirmedIn);
        b.strUnconfirmedout = CoinToTokenBigFloat(balance.nUnconfirmedOut);
        spResult->vecBalance.push_back(b);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCListTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!BasicConfig()->fFullDb)
    {
        throw CRPCException(RPC_INVALID_REQUEST, "If you need this function, please set config 'fulldb=true' and restart");
    }

    auto spParam = CastParamPtr<CListTransactionParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CDestination dest;
    dest.ParseString(spParam->strAddress);
    if (dest.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    vector<CDestTxInfo> vTx;
    if (!pService->ListTransaction(hashFork, hashBlock, dest, spParam->nOffset, spParam->nCount, spParam->fReverse, vTx))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed list transactions");
    }

    auto spResult = MakeCListTransactionResultPtr();
    for (const CDestTxInfo& tx : vTx)
    {
        spResult->vecTransaction.push_back(TxInfoToJSON(dest, tx));
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendFrom(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSendFromParam>(param);

    CDestination from;
    from.ParseString(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to;
    if (spParam->strTo.IsValid() && !spParam->strTo.empty() && spParam->strTo == string("0x0"))
    {
        to.SetNull();
    }
    else
    {
        if (!spParam->strTo.IsValid())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
        if (!to.ParseString(spParam->strTo))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
        if (to.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint64 nNonce = 0;
    if (spParam->nNonce.IsValid())
    {
        nNonce = spParam->nNonce;
    }

    vector<unsigned char> vchData;
    if (spParam->strData.IsValid())
    {
        auto strDataTmp = spParam->strData;
        if (((std::string)strDataTmp).substr(0, 4) == "msg:")
        {
            auto hex = mtbase::ToHexString((const unsigned char*)strDataTmp.c_str(), strlen(strDataTmp.c_str()));
            vchData = ParseHexString(hex);
        }
        else
        {
            vchData = ParseHexString(strDataTmp);
        }
    }

    bytes btFormatData;
    if (spParam->strFdata.IsValid())
    {
        btFormatData = ParseHexString(spParam->strFdata);
    }

    uint256 nGasPrice;
    if (spParam->strGasprice.IsValid())
    {
        if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
        }
    }
    uint256 nGas = uint256(spParam->nGas);

    vector<uint8> vchToData;
    if (spParam->strTodata.IsValid())
    {
        vchToData = ParseHexString(spParam->strTodata);
    }

    bytes btContractCode;
    bytes btContractParam;
    if (to.IsNull())
    {
        if (spParam->strContractcode.IsValid())
        {
            if (spParam->strContractcode.size() == CDestination::size() * 2 + 2)
            {
                CDestination dest;
                dest.ParseString(spParam->strContractcode);
                if (!dest.IsNull())
                {
                    mtbase::CBufStream ss;
                    ss << dest;
                    ss.GetData(btContractCode);
                }
            }
            if (btContractCode.empty())
            {
                btContractCode = ParseHexString(spParam->strContractcode);
            }
        }
    }
    if (spParam->strContractparam.IsValid())
    {
        btContractParam = ParseHexString(spParam->strContractparam);
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransaction(hashFork, from, to, vchToData, nAmount, nNonce, nGasPrice,
                                              nGas, vchData, btFormatData, btContractCode, btContractParam, txNew);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr);
    }

    if (!pService->SignTransaction(hashFork, txNew))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }

    Errno err = pService->SendTransaction(hashFork, txNew);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ") + ErrorString(err));
    }

    uint256 txid = txNew.GetHash();

    StdDebug("CRPCMod", "Sendfrom success, txid: %s, to: %s", txid.GetHex().c_str(), to.ToString().c_str());

    return MakeCSendFromResultPtr(txid.GetHex());
}

CRPCResultPtr CRPCMod::RPCCreateTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCreateTransactionParam>(param);

    CDestination from;
    from.ParseString(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to;
    if (spParam->strTo.IsValid() && !spParam->strTo.empty() && spParam->strTo == string("0x0"))
    {
        to.SetNull();
    }
    else
    {
        if (!spParam->strTo.IsValid())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
        if (!to.ParseString(spParam->strTo))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
        if (to.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint64 nNonce = 0;
    if (spParam->nNonce.IsValid())
    {
        nNonce = spParam->nNonce;
    }

    vector<unsigned char> vchData;
    if (spParam->strData.IsValid())
    {
        auto strDataTmp = spParam->strData;
        if (((std::string)strDataTmp).substr(0, 4) == "msg:")
        {
            auto hex = mtbase::ToHexString((const unsigned char*)strDataTmp.c_str(), strlen(strDataTmp.c_str()));
            vchData = ParseHexString(hex);
        }
        else
        {
            vchData = ParseHexString(strDataTmp);
        }
    }

    bytes btFormatData;
    if (spParam->strFdata.IsValid())
    {
        btFormatData = ParseHexString(spParam->strFdata);
    }

    uint256 nGasPrice;
    if (spParam->strGasprice.IsValid())
    {
        if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
        }
    }
    uint256 nGas = uint256(spParam->nGas);

    vector<uint8> vchToData;
    if (spParam->strTodata.IsValid())
    {
        vchToData = ParseHexString(spParam->strTodata);
    }

    bytes btContractCode;
    bytes btContractParam;
    if (to.IsNull())
    {
        if (spParam->strContractcode.IsValid())
        {
            if (spParam->strContractcode.size() == CDestination::size() * 2 + 2)
            {
                CDestination dest;
                dest.ParseString(spParam->strContractcode);
                if (!dest.IsNull())
                {
                    mtbase::CBufStream ss;
                    ss << dest;
                    ss.GetData(btContractCode);
                }
            }
            if (btContractCode.empty())
            {
                btContractCode = ParseHexString(spParam->strContractcode);
            }
        }
    }
    if (spParam->strContractparam.IsValid())
    {
        btContractParam = ParseHexString(spParam->strContractparam);
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransaction(hashFork, from, to, vchToData, nAmount, nNonce, nGasPrice,
                                              nGas, vchData, btFormatData, btContractCode, btContractParam, txNew);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr);
    }

    CBufStream ss;
    ss << txNew;

    return MakeCCreateTransactionResultPtr(ToHexString((const unsigned char*)ss.GetData(), ss.GetSize()));
}

CRPCResultPtr CRPCMod::RPCSignTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSignTransactionParam>(param);

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    uint256 hashFork;
    if (!pService->GetForkHashByChainId(rawTx.GetChainId(), hashFork))
    {
        throw CRPCException(RPC_TRANSACTION_ERROR, "Get fork hash failed");
    }

    if (!pService->SignTransaction(hashFork, rawTx))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }

    CBufStream ssNew;
    ssNew << rawTx;

    return MakeCSignTransactionResultPtr(ToHexString((const unsigned char*)ssNew.GetData(), ssNew.GetSize()));
}

CRPCResultPtr CRPCMod::RPCSignMessage(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSignMessageParam>(param);

    crypto::CPubKey pubkey;
    crypto::CKey privkey;
    if (spParam->strPubkey.IsValid())
    {
        if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid pubkey error");
        }
    }
    else if (spParam->strPrivkey.IsValid())
    {
        uint256 nPriv;
        if (nPriv.SetHex(spParam->strPrivkey) != spParam->strPrivkey.size())
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
        }
        privkey.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end()));
    }
    else
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "no pubkey or privkey");
    }

    string strMessage = spParam->strMessage;

    if (!!pubkey)
    {
        int nVersion;
        bool fLocked, fPublic;
        int64 nAutoLockTime;
        if (!pService->GetKeyStatus(CDestination(pubkey), nVersion, fLocked, nAutoLockTime, fPublic))
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
        }
        if (fPublic)
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't sign message by public key");
        }
        if (fLocked)
        {
            throw CRPCException(RPC_WALLET_UNLOCK_NEEDED, "Key is locked");
        }
    }

    vector<unsigned char> vchSig;
    if (spParam->strAddr.IsValid())
    {
        CDestination addr;
        addr.ParseString(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() /*|| addr.IsPubKey()*/)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        if (!!pubkey)
        {
            if (!pService->SignSignature(CDestination(pubkey), addr.ToHash(), vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
        else
        {
            if (!privkey.Sign(addr.ToHash(), vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
    }
    else
    {
        uint256 hashStr;
        if (spParam->fHasprefix)
        {
            CBufStream ss;
            const string strMessageMagic = "MetabaseNet Signed Message:\n";
            ss << strMessageMagic;
            ss << strMessage;
            hashStr = crypto::CryptoHash(ss.GetData(), ss.GetSize());
        }
        else
        {
            hashStr = crypto::CryptoHash(strMessage.data(), strMessage.size());
        }

        if (!!pubkey)
        {
            if (!pService->SignSignature(CDestination(pubkey), hashStr, vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
        else
        {
            if (!privkey.Sign(hashStr, vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
    }
    return MakeCSignMessageResultPtr(ToHexString(vchSig));
}

CRPCResultPtr CRPCMod::RPCListAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListAddressParam>(param);

    uint64 nPage = 0;
    uint64 nCount = 30;
    if (spParam->nPage.IsValid())
    {
        nPage = spParam->nPage;
    }
    if (spParam->nCount.IsValid())
    {
        nCount = spParam->nCount;
    }

    auto spResult = MakeCListAddressResultPtr();
    vector<tuple<CDestination, uint8, uint512>> vDes;
    ListDestination(vDes, nPage, nCount);
    for (const auto& vd : vDes)
    {
        const CDestination dest = std::get<0>(vd);
        const uint8 nDestType = (std::get<1>(vd) >> 5);
        const uint512 pubkey512 = std::get<2>(vd);

        CListAddressResult::CAddressdata addressData;
        addressData.strAddress = dest.ToString();
        if (nDestType == CDestination::PREFIX_PUBKEY)
        {
            addressData.strType = "pubkey";
            addressData.strPubkey = pubkey512.GetHex();
        }
        else if (nDestType == CDestination::PREFIX_TEMPLATE)
        {
            addressData.strType = "template";
            CTemplatePtr ptr = pService->GetTemplate(dest);
            if (ptr == nullptr)
            {
                continue;
            }
            addressData.strTemplate = CTemplate::GetTypeName(ptr->GetTemplateType());
            auto& templateData = addressData.templatedata;
            templateData.strHex = ToHexString(ptr->Export());
            templateData.strType = ptr->GetName();
            ptr->GetTemplateData(templateData);
        }
        else
        {
            continue;
        }
        spResult->vecAddressdata.push_back(addressData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCExportWallet(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportWalletParam>(param);

#ifdef BOOST_CYGWIN_FS_PATH
    std::string strCygWinPathPrefix = "/cygdrive";
    std::size_t found = string(spParam->strPath).find(strCygWinPathPrefix);
    if (found != std::string::npos)
    {
        strCygWinPathPrefix = "";
    }
#else
    std::string strCygWinPathPrefix;
#endif

    fs::path pSave(string(strCygWinPathPrefix + spParam->strPath));
    //check if the file name given is available
    if (!pSave.is_absolute())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Must be an absolute path.");
    }
    if (is_directory(pSave))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Cannot export to a folder.");
    }
    if (exists(pSave))
    {
        throw CRPCException(RPC_WALLET_ERROR, "File has been existed.");
    }
    if (pSave.filename() == "." || pSave.filename() == "..")
    {
        throw CRPCException(RPC_WALLET_ERROR, "Cannot export to a folder.");
    }

    if (!exists(pSave.parent_path()) && !create_directories(pSave.parent_path()))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to create directories.");
    }

    Array aAddr;
    vector<tuple<CDestination, uint8, uint512>> vDes;
    ListDestination(vDes);
    for (const auto& vd : vDes)
    {
        const CDestination dest = std::get<0>(vd);
        const uint8 nDestType = (std::get<1>(vd) >> 5);
        if (nDestType == CDestination::PREFIX_PUBKEY)
        {
            Object oKey;
            oKey.push_back(Pair("type", std::to_string(nDestType)));
            oKey.push_back(Pair("address", dest.ToString()));

            vector<unsigned char> vchKey;
            if (!pService->ExportKey(dest, vchKey))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
            }
            oKey.push_back(Pair("hex", ToHexString(vchKey)));
            aAddr.push_back(oKey);
        }
        else if (nDestType == CDestination::PREFIX_TEMPLATE)
        {
            Object oTemp;
            oTemp.push_back(Pair("type", std::to_string(nDestType)));
            oTemp.push_back(Pair("address", dest.ToString()));
            CTemplatePtr ptr = pService->GetTemplate(dest);
            if (!ptr)
            {
                throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
            }
            oTemp.push_back(Pair("hex", ToHexString(ptr->Export())));
            aAddr.push_back(oTemp);
        }
    }
    //output them together to file
    try
    {
        std::ofstream ofs(pSave.string(), std::ios::out);
        if (!ofs)
        {
            throw runtime_error("write error");
        }

        write_stream(Value(aAddr), ofs, pretty_print);
        ofs.close();
    }
    catch (...)
    {
        throw CRPCException(RPC_WALLET_ERROR, "filesystem_error - failed to write.");
    }

    return MakeCExportWalletResultPtr(string("Wallet file has been saved at: ") + pSave.string());
}

CRPCResultPtr CRPCMod::RPCImportWallet(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportWalletParam>(param);

#ifdef BOOST_CYGWIN_FS_PATH
    std::string strCygWinPathPrefix = "/cygdrive";
    std::size_t found = string(spParam->strPath).find(strCygWinPathPrefix);
    if (found != std::string::npos)
    {
        strCygWinPathPrefix = "";
    }
#else
    std::string strCygWinPathPrefix;
#endif

    fs::path pLoad(string(strCygWinPathPrefix + spParam->strPath));
    //check if the file name given is available
    if (!pLoad.is_absolute())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Must be an absolute path.");
    }
    if (!exists(pLoad) || is_directory(pLoad))
    {
        throw CRPCException(RPC_WALLET_ERROR, "File name is invalid.");
    }

    Value vWallet;
    try
    {
        fs::ifstream ifs(pLoad);
        if (!ifs)
        {
            throw runtime_error("read error");
        }

        read_stream(ifs, vWallet, RPC_MAX_DEPTH);
        ifs.close();
    }
    catch (...)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Filesystem_error - failed to read.");
    }

    if (array_type != vWallet.type())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Wallet file exported is invalid, check it and try again.");
    }

    Array aAddr;
    uint32 nKey = 0;
    uint32 nTemp = 0;
    for (const auto& oAddr : vWallet.get_array())
    {
        if (oAddr.get_obj()[0].name_ != "type" || oAddr.get_obj()[1].name_ != "address" || oAddr.get_obj()[2].name_ != "hex")
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }
        string sType = oAddr.get_obj()[0].value_.get_str(); //"type" field
        string sAddr = oAddr.get_obj()[1].value_.get_str(); //"address" field
        string sHex = oAddr.get_obj()[2].value_.get_str();  //"hex" field

        uint8 nDestType = std::atoi(sType.c_str());

        CDestination addr;
        addr.ParseString(sAddr);
        if (addr.IsNull())
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }

        //import keys
        if (nDestType == CDestination::PREFIX_PUBKEY)
        {
            vector<unsigned char> vchKey = ParseHexString(sHex);
            crypto::CKey key;
            if (!key.Load(vchKey))
            {
                throw CRPCException(RPC_INVALID_PARAMS, "Failed to verify serialized key");
            }
            if (key.GetVersion() == crypto::CKey::INIT)
            {
                throw CRPCException(RPC_INVALID_PARAMS, "Can't import the key with empty passphrase");
            }
            CDestination dest(key.GetPubKey());
            if ((key.IsPrivKey() && pService->HaveKey(dest, crypto::CKey::PRIVATE_KEY))
                || (key.IsPubKey() && pService->HaveKey(dest)))
            {
                continue; //step to next one to continue importing
            }
            auto strErr = pService->AddKey(key);
            if (strErr)
            {
                throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
            }
            aAddr.push_back(key.GetPubKey().GetHex());
            ++nKey;
        }
        //import templates
        else if (nDestType == CDestination::PREFIX_TEMPLATE)
        {
            vector<unsigned char> vchTemplate = ParseHexString(sHex);
            CTemplatePtr ptr = CTemplate::Import(vchTemplate);
            if (ptr == nullptr)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
            }
            if (pService->HaveTemplate(addr))
            {
                continue; //step to next one to continue importing
            }
            if (!pService->AddTemplate(ptr))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
            }
            aAddr.push_back(CDestination(ptr->GetTemplateId()).ToString());
            ++nTemp;
        }
    }

    return MakeCImportWalletResultPtr(string("Imported ") + std::to_string(nKey)
                                      + string(" keys and ") + std::to_string(nTemp) + string(" templates."));
}

CRPCResultPtr CRPCMod::RPCMakeOrigin(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeOriginParam>(param);

    const uint8 nForkType = CProfile::ParseTypeStr(spParam->strType);
    if (!CProfile::VerifySubforkType(nForkType))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid type");
    }

    uint256 hashPrev;
    hashPrev.SetHex(spParam->strPrev);

    CDestination destOwner;
    destOwner.ParseString(spParam->strOwner);
    if (destOwner.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid owner");
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }
    uint256 nMintReward;
    if (!TokenBigFloatToCoin(spParam->strReward, nMintReward))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid reward");
    }
    if (!RewardRange(nMintReward))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid reward");
    }
    uint64 nHalvecycle = spParam->nHalvecycle;

    if (spParam->strName.empty() || spParam->strName.size() > 128
        || spParam->strSymbol.empty() || spParam->strSymbol.size() > 16)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid name or symbol");
    }
    uint64 nForkNonce = spParam->nNonce;

    CBlock blockPrev;
    CChainId nChainId;
    uint256 hashParent;
    int nJointHeight;
    if (!pService->GetBlock(hashPrev, blockPrev, nChainId, hashParent, nJointHeight))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown prev block");
    }

    if (!blockPrev.IsPrimary())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Prev block must main chain block");
    }

    int nForkHeight = pService->GetForkHeight(hashParent);
    if (nForkHeight < nJointHeight + MIN_CREATE_FORK_INTERVAL_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("The minimum confirmed height of the previous block is ") + to_string(MIN_CREATE_FORK_INTERVAL_HEIGHT));
    }
    if ((int64)nForkHeight > (int64)nJointHeight + MAX_JOINT_FORK_INTERVAL_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("Maximum fork spacing height is ") + to_string(MAX_JOINT_FORK_INTERVAL_HEIGHT));
    }

    uint256 hashBlockRef;
    uint64 nTimeRef;
    if (!pService->GetLastBlockOfHeight(pCoreProtocol->GetGenesisBlockHash(), nJointHeight + 1, hashBlockRef, nTimeRef))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Failed to query main chain reference block");
    }

    if (!pService->VerifyForkNameAndChainId(hashParent, (CChainId)(spParam->nChainid), spParam->strName))
    {
        throw CRPCException(RPC_VERIFY_ALREADY_IN_CHAIN, "Fork name or chainid is existed");
    }

    if (nForkType == CProfile::PROFILE_FORK_TYPE_CLONEMAP)
    {
        CBlockStatus statusGen;
        if (!pService->GetBlockStatus(pCoreProtocol->GetGenesisBlockHash(), statusGen))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Get genesis block status fail");
        }
        if (destOwner != statusGen.destMint)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Must be the main chain creator");
        }

        nAmount = 0;
        nMintReward = 0;
        nHalvecycle = 0;
    }

    CProfile profile;
    profile.nType = nForkType;
    profile.strName = spParam->strName;
    profile.strSymbol = spParam->strSymbol;
    profile.nChainId = spParam->nChainid;
    profile.destOwner = destOwner;
    profile.hashParent = hashParent;
    profile.nJointHeight = nJointHeight;
    profile.nAmount = nAmount;
    profile.nMintReward = nMintReward;
    profile.nMinTxFee = MIN_GAS_PRICE * TX_BASE_GAS;
    profile.nHalveCycle = nHalvecycle;

    CBlock block;
    block.nVersion = 1;
    block.nType = CBlock::BLOCK_ORIGIN;
    block.SetBlockTime(nTimeRef);
    block.hashPrev = hashPrev;

    block.AddForkProfile(profile);

    block.hashStateRoot = CCoreProtocol::CreateGenesisStateRoot(block.nType, block.GetBlockTime(), destOwner, nAmount);

    CTransaction& tx = block.txMint;

    tx.SetTxType(CTransaction::TX_GENESIS);
    tx.SetNonce(nForkNonce);
    tx.SetChainId(profile.nChainId);
    tx.SetToAddress(destOwner);
    tx.SetAmount(nAmount);

    tx.SetToAddressData(CAddressContext(CPubkeyAddressContext()));

    block.AddMintCoinProof(tx.GetAmount());

    block.hashMerkleRoot = block.CalcMerkleTreeRoot();

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(destOwner, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't sign origin block by public key");
    }
    if (fLocked)
    {
        throw CRPCException(RPC_WALLET_UNLOCK_NEEDED, "Key is locked");
    }

    uint256 hashBlock = block.GetHash();

    if (!pService->SignSignature(destOwner, hashBlock, block.vchSig))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
    }

    CBufStream ss;
    ss << block;

    auto spResult = MakeCMakeOriginResultPtr();
    spResult->strHash = hashBlock.GetHex();
    spResult->strHex = ToHexString((const unsigned char*)ss.GetData(), ss.GetSize());

    return spResult;
}

CRPCResultPtr CRPCMod::RPCCallContract(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCallContractParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CDestination from;
    from.ParseString(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to;
    to.ParseString(spParam->strTo);
    if (to.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    bytes btContractParam;
    if (spParam->strContractparam.IsValid())
    {
        btContractParam = ParseHexString(spParam->strContractparam);
    }

    uint256 nGasPrice;
    if (spParam->strGasprice.IsValid())
    {
        if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
        }
    }
    if (nGasPrice == 0)
    {
        nGasPrice = pService->GetForkMintMinGasPrice(hashFork);
    }
    uint256 nGas = uint256(spParam->nGas);
    if (nGas == 0)
    {
        nGas = DEF_TX_GAS_LIMIT;
    }

    CAddressContext ctxAddress;
    if (!pService->RetrieveAddressContext(hashFork, to, ctxAddress))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "to not is created");
    }
    if (!ctxAddress.IsContract())
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "to not is contract");
    }

    uint256 nUsedGas;
    uint64 nGasLeft = 0;
    int nStatus = 0;
    bytes btResult;
    if (!pService->CallContract(false, hashFork, uint256(), from, to, nAmount, nGasPrice, nGas, btContractParam, nUsedGas, nGasLeft, nStatus, btResult))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Call fail");
    }

    auto spResult = MakeCCallContractResultPtr();
    spResult->nStatus = nStatus;
    spResult->strResult = ToHexString(btResult);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetTransactionReceipt(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionReceiptParam>(param);

    uint256 txid;
    txid.SetHex(spParam->strTxid);
    if (txid == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txid");
    }
    if (txid == CTransaction().GetHash())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txid");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CTransactionReceiptEx receipt;
    if (!pService->GetTransactionReceipt(hashFork, txid, receipt))
    {
        throw CRPCException(RPC_DATABASE_ERROR, "Get transaction receipt fail");
    }

    auto spResult = MakeCGetTransactionReceiptResultPtr();

    spResult->strTransactionhash = txid.GetHex();
    spResult->nTransactionindex = receipt.nTxIndex;
    spResult->strBlockhash = receipt.hashBlock.GetHex();
    spResult->nBlocknumber = receipt.nBlockNumber;
    spResult->strFrom = receipt.from.ToString();
    spResult->strTo = receipt.to.ToString();
    spResult->nCumulativegasused = receipt.nBlockGasUsed.Get64();
    spResult->strEffectivegasprice = CoinToTokenBigFloat(receipt.nEffectiveGasPrice);
    spResult->strEffectivegasfee = CoinToTokenBigFloat(receipt.nEffectiveGasPrice * receipt.nTxGasUsed);
    spResult->nGasused = receipt.nTxGasUsed.Get64();
    spResult->nGastv = receipt.nTvGasUsed.Get64();
    if (!receipt.destContract.IsNull())
    {
        spResult->strContractaddress = receipt.destContract.ToString();
    }

    for (std::size_t i = 0; i < receipt.vLogs.size(); i++)
    {
        auto& txLogs = receipt.vLogs[i];
        if (!txLogs.address.IsNull()
            || !txLogs.data.empty()
            || !txLogs.topics.empty())
        {
            CGetTransactionReceiptResult::CLogs logs;

            //__optional__ CRPCBool fRemoved;
            logs.nLogindex = i;
            logs.nTransactionindex = receipt.nTxIndex;
            logs.strTransactionhash = txid.GetHex();
            logs.strBlockhash = receipt.hashBlock.GetHex();
            logs.nBlocknumber = receipt.nBlockNumber;

            logs.strAddress = txLogs.address.ToString();
            logs.strData = ToHexString(txLogs.data);
            for (auto& d : txLogs.topics)
            {
                logs.vecTopics.push_back(d.ToString());
            }
            logs.strType = "mined"; //mined  pending

            bytes btIdData;
            btIdData.assign(txid.begin(), txid.end());
            btIdData.insert(btIdData.end(), (char*)&i, (char*)&i + sizeof(i));
            bytes btIdKey = CryptoKeccakSign(btIdData.data(), btIdData.size());
            logs.strId = string("log_") + ToHexString(btIdKey).substr(2);

            spResult->vecLogs.push_back(logs);
        }
    }
    if (receipt.vLogs.size() == 0)
    {
        CGetTransactionReceiptResult::CLogs logs;
        spResult->vecLogs.push_back(logs);
    }

    if (receipt.nLogsBloom.IsNull())
    {
        spResult->strLogsbloom = "";
    }
    else
    {
        spResult->strLogsbloom = receipt.nLogsBloom.ToString();
    }
    if (!receipt.hashStatusRoot.IsNull())
    {
        spResult->strRoot = receipt.hashStatusRoot.GetHex();
    }
    spResult->strStatus = (receipt.nContractStatus == 0 ? "0x1" : "0x0");

    for (auto& vd : receipt.vTransfer)
    {
        CGetTransactionReceiptResult::CInternaltx itx;
        itx.strType = vd.GetTypeName();
        itx.strFrom = vd.destFrom.ToString();
        itx.strTo = vd.destTo.ToString();
        itx.strAmount = CoinToTokenBigFloat(vd.nAmount);
        spResult->vecInternaltx.push_back(itx);
    }
    if (receipt.vTransfer.size() == 0)
    {
        CGetTransactionReceiptResult::CInternaltx itx;
        spResult->vecInternaltx.push_back(itx);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetContractMuxCode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractMuxCodeParam>(param);

    int64 nMuxType;
    string strType;
    string strName;
    string strDescribe;
    bytes btCode;
    bytes btSource;

    if (spParam->strMuxtype == string("create"))
    {
        nMuxType = CTxContractData::CF_CREATE;
    }
    else if (spParam->strMuxtype == string("setup"))
    {
        nMuxType = CTxContractData::CF_SETUP;
    }
    else if (spParam->strMuxtype == string("upcode"))
    {
        nMuxType = CTxContractData::CF_UPCODE;
    }
    else
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid muxtype");
    }
    if (!spParam->strName.IsValid() || spParam->strName.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid name");
    }
    strName = spParam->strName;

    CDestination destOwner;
    destOwner.ParseString(spParam->strOwner);
    if (destOwner.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid owner address");
    }

    if (!spParam->strCode.IsValid() || spParam->strCode.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }

    if (nMuxType == CTxContractData::CF_SETUP)
    {
        uint256 hashCode;
        hashCode.SetHex(spParam->strCode);
        btCode.assign(hashCode.begin(), hashCode.end());
    }
    else
    {
        btCode = ParseHexString(spParam->strCode);
    }

    if (spParam->strType.IsValid())
    {
        strType = spParam->strType;
    }
    if (spParam->strDescribe.IsValid())
    {
        strDescribe = spParam->strDescribe;
    }
    if (spParam->strSource.IsValid())
    {
        btSource = ParseHexString(spParam->strSource);
    }

    CTxContractData txcd;
    if (!txcd.PacketCompress(nMuxType, strType, strName, destOwner, strDescribe, btCode, btSource))
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "Packet fail");
    }

    mtbase::CBufStream ss;
    ss << CODE_TYPE_CONTRACT << txcd;
    bytes btData;
    ss.GetData(btData);

    return MakeCGetContractMuxCodeResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCGetTemplateMuxCode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTemplateMuxCodeParam>(param);

    string strName;
    bytes btCode;

    if (!spParam->strName.IsValid() || spParam->strName.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid name");
    }
    strName = spParam->strName;

    if (!spParam->strCode.IsValid() || spParam->strCode.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }
    btCode = ParseHexString(spParam->strCode);

    CTemplatePtr ptr = CTemplate::Import(btCode);
    if (!ptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }
    CTemplateContext ctxTemplate(strName, btCode);

    mtbase::CBufStream ss;
    ss << CODE_TYPE_TEMPLATE << ctxTemplate;
    bytes btData;
    ss.GetData(btData);

    return MakeCGetTemplateMuxCodeResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCListContractCode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListContractCodeParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    uint256 hashCode;
    if (spParam->strCodehash.IsValid())
    {
        hashCode.SetHex(spParam->strCodehash);
    }

    uint256 txid;
    if (spParam->strTxid.IsValid())
    {
        txid.SetHex(spParam->strTxid);
    }

    std::map<uint256, CContractCodeContext> mapContractCode;
    if (hashCode == 0)
    {
        if (!pService->ListContractCodeContext(hashFork, hashBlock, txid, mapContractCode))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Unknown txid");
        }
    }
    else
    {
        CContractCodeContext ctxt;
        if (!pService->GetForkContractCodeContext(hashFork, hashBlock, hashCode, ctxt))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Unknown code");
        }
        mapContractCode.insert(make_pair(hashCode, ctxt));
    }

    auto spResult = MakeCListContractCodeResultPtr();

    for (const auto& kv : mapContractCode)
    {
        CListContractCodeResult::CCodedata codeData;

        codeData.strCodehash = kv.second.hashCode.ToString();
        if (kv.second.hashSource != 0)
        {
            codeData.strSourcehash = kv.second.hashSource.ToString();
        }
        if (!kv.second.strType.empty())
        {
            codeData.strType = kv.second.strType;
        }
        codeData.strOwner = kv.second.destOwner.ToString();
        if (!kv.second.strName.empty())
        {
            codeData.strName = kv.second.strName;
        }
        if (!kv.second.strDescribe.empty())
        {
            codeData.strDescribe = kv.second.strDescribe;
        }
        codeData.strTxid = kv.second.hashCreateTxid.GetHex();

        spResult->vecCodedata.push_back(codeData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCListContractAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListContractAddressParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    std::map<CDestination, CContractAddressContext> mapContractAddress;
    if (!pService->ListContractAddress(hashFork, hashBlock, mapContractAddress))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    auto spResult = MakeCListContractAddressResultPtr();

    for (const auto& kv : mapContractAddress)
    {
        CListContractAddressResult::CAddressdata addressData;

        const CDestination& address = kv.first;
        addressData.strAddress = address.ToString();
        if (!kv.second.strType.empty())
        {
            addressData.strType = kv.second.strType;
        }
        addressData.strOwner = kv.second.destCodeOwner.ToString();
        if (!kv.second.strName.empty())
        {
            addressData.strName = kv.second.strName;
        }
        if (!kv.second.strDescribe.empty())
        {
            addressData.strDescribe = kv.second.strDescribe;
        }
        addressData.strTxid = kv.second.hashCreateTxid.GetHex();
        if (kv.second.hashSourceCode != 0)
        {
            addressData.strSourcehash = kv.second.hashSourceCode.GetHex();
        }
        addressData.strCodehash = kv.second.hashContractCreateCode.GetHex();

        spResult->vecAddressdata.push_back(addressData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetDestContract(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetDestContractParam>(param);

    CDestination dest;
    dest.ParseString(spParam->strAddress);
    if (dest.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    CContractAddressContext ctxtContract;
    if (!pService->GeDestContractContext(hashFork, hashBlock, dest, ctxtContract))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Address error");
    }

    auto spResult = MakeCGetDestContractResultPtr();

    spResult->strType = ctxtContract.strType;
    spResult->strName = ctxtContract.strName;
    spResult->strDescribe = ctxtContract.strDescribe;
    spResult->strTxid = ctxtContract.hashCreateTxid.GetHex();
    spResult->strSourcehash = ctxtContract.hashSourceCode.GetHex();
    spResult->strCodehash = ctxtContract.hashContractCreateCode.GetHex();

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetContractSource(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractSourceParam>(param);

    uint256 hashSource(spParam->strSourcehash);
    if (hashSource == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid source");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    bytes btData;
    if (!pService->GetContractSource(hashFork, hashBlock, hashSource, btData))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid source");
    }

    return MakeCGetContractSourceResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCGetContractCode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractCodeParam>(param);

    uint256 hashCode(spParam->strCodehash);
    if (hashCode == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    uint256 hashBlock = GetRefBlock(hashFork, spParam->strBlock);

    bytes btData;
    if (!pService->GetContractCode(hashFork, hashBlock, hashCode, btData))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }

    return MakeCGetContractCodeResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCAddBlacklistAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddBlacklistAddressParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    pService->AddBlacklistAddress(address);

    return MakeCAddBlacklistAddressResultPtr(true);
}

CRPCResultPtr CRPCMod::RPCRemoveBlacklistAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveBlacklistAddressParam>(param);

    CDestination address;
    address.ParseString(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    pService->RemoveBlacklistAddress(address);

    return MakeCRemoveBlacklistAddressResultPtr(true);
}

CRPCResultPtr CRPCMod::RPCListBlacklistAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    set<CDestination> setAddress;
    pService->ListBlacklistAddress(setAddress);

    auto spResult = MakeCListBlacklistAddressResultPtr();
    for (auto& dest : setAddress)
    {
        spResult->vecAddress.push_back(dest.ToString());
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSetFunctionAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSetFunctionAddressParam>(param);

    if (ctxReq.hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        throw CRPCException(RPC_INVALID_REQUEST, "Only suitable for the main chain");
    }

    if (!spParam->nFuncid.IsValid() || spParam->nFuncid == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid funcid");
    }
    uint64 nFuncid = spParam->nFuncid;

    CDestination newAddress;
    newAddress.ParseString(spParam->strNewaddress);
    if (newAddress.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid newaddress");
    }

    uint64 nDisableModify = (spParam->fDisablemodify ? 1 : 0);

    CDestination destDefaultFunction;
    if (!pService->GetDefaultFunctionAddress(nFuncid, destDefaultFunction))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid funcid");
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(destDefaultFunction, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Default address not import");
    }
    if (fLocked)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Default address is locked");
    }

    std::string strErr;
    if (!pService->VerifyNewFunctionAddress(0, destDefaultFunction, nFuncid, newAddress, strErr))
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, std::string("Rejected modify: ") + strErr);
    }

    std::vector<bytes> vParamList;
    vParamList.push_back(uint256(nFuncid).ToBigEndian());
    vParamList.push_back(newAddress.ToHash().GetBytes());
    vParamList.push_back(uint256(nDisableModify).ToBigEndian());
    bytes btData = pService->MakeEthTxCallData("setFunctionAddress(uint32,address,bool)", vParamList);

    uint256 txid;
    if (!pService->SendEthTransaction(pCoreProtocol->GetGenesisBlockHash(), destDefaultFunction, FUNCTION_CONTRACT_ADDRESS, 0, 0, 0, 0, btData, FUNCTION_TX_GAS_BASE, txid))
    {
        throw CRPCException(RPC_TRANSACTION_ERROR, "Verify tx fail");
    }

    return MakeCSetFunctionAddressResultPtr(txid.ToString());
}

CRPCResultPtr CRPCMod::RPCListFunctionAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListFunctionAddressParam>(param);

    if (ctxReq.hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        throw CRPCException(RPC_INVALID_REQUEST, "Only suitable for the main chain");
    }

    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, spParam->strBlock);

    std::map<uint32, CFunctionAddressContext> mapFunctionAddress;
    pService->ListFunctionAddress(hashBlock, mapFunctionAddress);

    auto spResult = MakeCListFunctionAddressResultPtr();
    for (auto& kv : mapFunctionAddress)
    {
        CDestination destDefaultFunction;
        if (!pService->GetDefaultFunctionAddress(kv.first, destDefaultFunction))
        {
            continue;
        }

        CListFunctionAddressResult::CFunctionaddresslist item;

        item.nFuncid = kv.first;
        item.strDefaultaddress = destDefaultFunction.ToString();
        item.strFuncaddress = kv.second.GetFunctionAddress().ToString();
        item.fDisablemodify = kv.second.IsDisableModify();

        auto it = mapFunctionAddressName.find(kv.first);
        if (it != mapFunctionAddressName.end())
        {
            item.strFuncname = it->second;
        }
        else
        {
            item.strFuncname = "";
        }

        spResult->vecFunctionaddresslist.push_back(item);
    }
    return spResult;
}

/* Util */
CRPCResultPtr CRPCMod::RPCVerifyMessage(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CVerifyMessageParam>(param);

    //verifymessage <"pubkey"> <"message"> <"sig">
    crypto::CPubKey pubkey;
    if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    string strMessage = spParam->strMessage;

    if (spParam->strSig.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid sig");
    }
    vector<unsigned char> vchSig = ParseHexString(spParam->strSig);
    if (vchSig.size() == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid sig");
    }
    if (spParam->strAddr.IsValid())
    {
        CDestination addr;
        addr.ParseString(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() /*|| addr.IsPubKey()*/)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        return MakeCVerifyMessageResultPtr(pubkey.Verify(addr.ToHash(), vchSig));
    }
    else
    {
        uint256 hashStr;
        if (spParam->fHasprefix)
        {
            CBufStream ss;
            const string strMessageMagic = "MetabaseNet Signed Message:\n";
            ss << strMessageMagic;
            ss << strMessage;
            hashStr = crypto::CryptoHash(ss.GetData(), ss.GetSize());
        }
        else
        {
            hashStr = crypto::CryptoHash(strMessage.data(), strMessage.size());
        }
        return MakeCVerifyMessageResultPtr(pubkey.Verify(hashStr, vchSig));
    }
}

CRPCResultPtr CRPCMod::RPCMakeKeyPair(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeKeyPairParam>(param);

    crypto::CCryptoKey key;
    crypto::CryptoMakeNewKey(key);

    auto spResult = MakeCMakeKeyPairResultPtr();
    spResult->strPrivkey = key.secret.GetHex();
    spResult->strPubkey = key.pubkey.GetHex();
    spResult->strAddress = CDestination(crypto::CPubKey(key.pubkey)).ToString();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetPubKey(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetPubkeyParam>(param);
    uint256 nPriv;
    if (nPriv.SetHex(spParam->strPrivkey) != spParam->strPrivkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid privkey");
    }
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Get key by privkey error");
    }
    return MakeCGetPubkeyResultPtr(key.GetPubKey().ToString());
}

CRPCResultPtr CRPCMod::RPCGetPubKeyAddress(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetPubkeyAddressParam>(param);
    crypto::CPubKey pubkey;
    if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }
    CDestination dest(pubkey);
    return MakeCGetPubkeyAddressResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCMakeTemplate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeTemplateParam>(param);
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(spParam->data);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }

    auto spResult = MakeCMakeTemplateResultPtr();
    vector<unsigned char> vchTemplate = ptr->Export();
    spResult->strHex = ToHexString(vchTemplate);
    spResult->strAddress = CDestination(ptr->GetTemplateId()).ToString();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCDecodeTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CDecodeTransactionParam>(param);
    vector<unsigned char> txData(ParseHexString(spParam->strTxdata));
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    uint256 hashFork;
    if (!pService->GetForkHashByChainId(rawTx.GetChainId(), hashFork))
    {
        throw CRPCException(RPC_TRANSACTION_ERROR, "Get fork hash failed");
    }

    return MakeCDecodeTransactionResultPtr(TxToJSON(rawTx.GetHash(), rawTx, hashFork, uint256(), -1));
}

CRPCResultPtr CRPCMod::RPCGetTxFee(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionFeeParam>(param);

    bytes vchData = ParseHexString(spParam->strHexdata);

    CTransaction tx;
    tx.AddTxData(CTransaction::DF_COMMON, vchData);

    uint256 nNeedGas = tx.GetTxBaseGas();
    uint256 nFee = nNeedGas * pService->GetForkMintMinGasPrice(ctxReq.hashFork);

    auto spResult = MakeCGetTransactionFeeResultPtr();
    spResult->strTxfee = CoinToTokenBigFloat(nFee);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCMakeSha256(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeSha256Param>(param);
    vector<unsigned char> vData;
    if (spParam->strHexdata.IsValid())
    {
        vData = ParseHexString(spParam->strHexdata);
    }
    else
    {
        uint256 u;
        crypto::CryptoGetRand256(u);
        vData.assign(u.begin(), u.end());
    }

    uint256 hash = crypto::CryptoSHA256(&(vData[0]), vData.size());

    auto spResult = MakeCMakeSha256ResultPtr();
    spResult->strHexdata = ToHexString(vData);
    spResult->strSha256 = hash.GetHex();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCReverseHex(const CReqContext& ctxReq, CRPCParamPtr param)
{
    // reversehex <"hex">
    auto spParam = CastParamPtr<CReverseHexParam>(param);

    string strHex = spParam->strHex;
    if (strHex.empty() || (strHex.size() % 2 != 0))
    {
        throw CRPCException(RPC_INVALID_PARAMS, "hex string size is not even");
    }

    // regex r(R"([^0-9a-fA-F]+)");
    // smatch sm;
    // if (regex_search(strHex, sm, r))
    // {
    //     throw CRPCException(RPC_INVALID_PARAMS, string("invalid hex string: ") + sm.str());
    // }

    // for (auto itBegin = strHex.begin(), itEnd = strHex.end() - 2; itBegin < itEnd; itBegin += 2, itEnd -= 2)
    // {
    //     swap_ranges(itBegin, itBegin + 2, itEnd);
    // }

    string strOut = ReverseHexString(strHex);
    if (strOut.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMS, string("invalid hex string"));
    }
    return MakeCReverseHexResultPtr(strOut);
}

CRPCResultPtr CRPCMod::RPCFuncSign(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CFuncSignParam>(param);
    if (!spParam->strFuncname.IsValid() || spParam->strFuncname.size() == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid funcname");
    }
    std::vector<uint8> ret = crypto::CryptoKeccakSign(spParam->strFuncname.c_str(), spParam->strFuncname.size());
    if (ret.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Sign fail");
    }
    return MakeCFuncSignResultPtr(mtbase::ToHexString(ret));
}

CRPCResultPtr CRPCMod::RPCMakeHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeHashParam>(param);
    if (!spParam->strData.IsValid())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid data");
    }
    bytes btData = ParseHexString(spParam->strData);
    if (btData.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid data");
    }
    uint256 hash = crypto::CryptoHash(btData.data(), btData.size());
    return MakeCMakeHashResultPtr(hash.GetHex());
}

CRPCResultPtr CRPCMod::RPCSetMintMinGasPrice(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    auto spParam = CastParamPtr<CSetMintMinGasPriceParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    if (!spParam->strMingasprice.IsValid())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid mingasprice");
    }
    uint256 nMinGasPrice;
    if (!TokenBigFloatToCoin(spParam->strMingasprice, nMinGasPrice))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid mingasprice");
    }

    if (!pService->UpdateForkMintMinGasPrice(hashFork, nMinGasPrice))
    {
        return MakeCSetMintMinGasPriceResultPtr(false);
    }

    return MakeCSetMintMinGasPriceResultPtr(true);
}

CRPCResultPtr CRPCMod::RPCGetMintMinGasPrice(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetMintMinGasPriceParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetMintMinGasPriceResultPtr(CoinToTokenBigFloat(pService->GetForkMintMinGasPrice(hashFork)));
}

CRPCResultPtr CRPCMod::RPCListMintMinGasPrice(const CReqContext& ctxReq, CRPCParamPtr param)
{
    if (!VerifyClientOrder(ctxReq))
    {
        throw CRPCException(RPC_FORBIDDEN_BY_SAFE_MODE, "Permission denied");
    }
    auto spResult = MakeCListMintMinGasPriceResultPtr();

    vector<pair<uint256, CProfile>> vFork;
    pService->ListFork(vFork, true);
    for (size_t i = 0; i < vFork.size(); i++)
    {
        const uint256& hashFork = vFork[i].first;
        CListMintMinGasPriceResult::CForkprice forkPrice;
        forkPrice.strFork = hashFork.ToString();
        forkPrice.strMingasprice = CoinToTokenBigFloat(pService->GetForkMintMinGasPrice(hashFork));
        spResult->vecForkprice.push_back(forkPrice);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCCheckAtBloomFilter(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCheckAtBloomFilterParam>(param);
    bytes btBloomFilter = ParseHexString(spParam->strBloomfilter);
    if (btBloomFilter.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid bloomfilter");
    }
    bytes btCheckData = ParseHexString(spParam->strCheckdata);
    if (btCheckData.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid checkdata");
    }

    CNhBloomFilter bf(btBloomFilter);
    return MakeCCheckAtBloomFilterResultPtr(bf.Test(btCheckData));
}

CRPCResultPtr CRPCMod::RPCQueryStat(const CReqContext& ctxReq, CRPCParamPtr param)
{
    enum
    {
        TYPE_NON,
        TYPE_MAKER,
        TYPE_P2PSYN
    } eType
        = TYPE_NON;
    uint32 nDefQueryCount = 20;
    uint256 hashFork = ctxReq.hashFork;
    uint32 nBeginTimeValue = ((GetTime() - 60 * nDefQueryCount) % (24 * 60 * 60)) / 60;
    uint32 nGetCount = nDefQueryCount;
    bool fSetBegin = false;

    auto spParam = CastParamPtr<CQueryStatParam>(param);
    if (spParam->strType.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid type: is empty");
    }
    if (spParam->strType == "maker")
    {
        eType = TYPE_MAKER;
    }
    else if (spParam->strType == "p2psyn")
    {
        eType = TYPE_P2PSYN;
    }
    else
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid type");
    }
    if (spParam->strFork.IsValid())
    {
        if (!GetForkHashOfDef(spParam->strFork, ctxReq.hashFork, hashFork))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
        }
    }
    if (!spParam->strBegin.empty() && spParam->strBegin.size() <= 8)
    {
        //HH:MM:SS
        std::string strTempTime = spParam->strBegin;
        std::size_t found_hour = strTempTime.find(":");
        if (found_hour != std::string::npos && found_hour > 0)
        {
            std::size_t found_min = strTempTime.find(":", found_hour + 1);
            if (found_min != std::string::npos && found_min > found_hour + 1)
            {
                int hour = std::stoi(strTempTime.substr(0, found_hour));
                int minute = std::stoi(strTempTime.substr(found_hour + 1, found_min - (found_hour + 1)));
                if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59)
                {
                    nBeginTimeValue = hour * 60 + minute;
                    int64 nTimeOffset = (GetTime() - GetLocalTimeSeconds()) / 60;
                    nTimeOffset += nBeginTimeValue;
                    if (nTimeOffset >= 0)
                    {
                        nBeginTimeValue = nTimeOffset % (24 * 60);
                    }
                    else
                    {
                        nBeginTimeValue = nTimeOffset + (24 * 60);
                    }
                    fSetBegin = true;
                }
            }
        }
    }
    if (spParam->nCount.IsValid())
    {
        nGetCount = GetUint(spParam->nCount, nDefQueryCount);
        if (nGetCount == 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid count");
        }
        if (nGetCount > 24 * 60)
        {
            nGetCount = 24 * 60;
        }
    }
    if (!fSetBegin && nGetCount != nDefQueryCount)
    {
        nBeginTimeValue = ((GetTime() - 60 * nGetCount) % (24 * 60 * 60)) / 60;
    }
    else
    {
        uint32 nTempCurTimeValue = (GetTime() % (24 * 60 * 60)) / 60;
        if (nTempCurTimeValue == nBeginTimeValue)
        {
            nGetCount = 0;
        }
        else
        {
            uint32 nTempCount = 0;
            if (nTempCurTimeValue > nBeginTimeValue)
            {
                nTempCount = nTempCurTimeValue - nBeginTimeValue;
            }
            else
            {
                nTempCount = (24 * 60) - (nBeginTimeValue - nTempCurTimeValue);
            }
            if (nGetCount > nTempCount)
            {
                nGetCount = nTempCount;
            }
        }
    }

    switch (eType)
    {
    case TYPE_MAKER:
    {
        std::vector<CStatItemBlockMaker> vStatData;
        if (nGetCount > 0)
        {
            if (!pDataStat->GetBlockMakerStatData(hashFork, nBeginTimeValue, nGetCount, vStatData))
            {
                throw CRPCException(RPC_INTERNAL_ERROR, "query error");
            }
        }

        int nTimeWidth = 8 + 2;                                 //hh:mm:ss + two spaces
        int nPowBlocksWidth = string("powblocks").size() + 2;   //+ two spaces
        int nDposBlocksWidth = string("dposblocks").size() + 2; //+ two spaces
        int nTxTPSWidth = string("tps").size() + 2;
        for (const CStatItemBlockMaker& item : vStatData)
        {
            int nTempValue;
            nTempValue = to_string(item.nPOWBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nPowBlocksWidth)
            {
                nPowBlocksWidth = nTempValue;
            }
            nTempValue = to_string(item.nDPOSBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nDposBlocksWidth)
            {
                nDposBlocksWidth = nTempValue;
            }
            nTempValue = to_string(item.nTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nTxTPSWidth)
            {
                nTxTPSWidth = nTempValue;
            }
        }

        int64 nTimeOffset = GetLocalTimeSeconds() - GetTime();

        string strResult = "";
        strResult += GetWidthString("time", nTimeWidth);
        strResult += GetWidthString("powblocks", nPowBlocksWidth);
        strResult += GetWidthString("dposblocks", nDposBlocksWidth);
        strResult += GetWidthString("tps", nTxTPSWidth);
        strResult += string("\r\n");
        for (const CStatItemBlockMaker& item : vStatData)
        {
            int nLocalTimeValue = item.nTimeValue * 60 + nTimeOffset;
            if (nLocalTimeValue >= 0)
            {
                nLocalTimeValue %= (24 * 3600);
            }
            else
            {
                nLocalTimeValue += (24 * 3600);
            }
            char sTimeBuf[128] = { 0 };
            sprintf(sTimeBuf, "%2.2d:%2.2d:59", nLocalTimeValue / 3600, nLocalTimeValue % 3600 / 60);
            strResult += GetWidthString(sTimeBuf, nTimeWidth);
            strResult += GetWidthString(to_string(item.nPOWBlockCount), nPowBlocksWidth);
            strResult += GetWidthString(to_string(item.nDPOSBlockCount), nDposBlocksWidth);
            strResult += GetWidthString(item.nTxTPS, nTxTPSWidth);
            strResult += string("\r\n");
        }
        return MakeCQueryStatResultPtr(strResult);
    }
    case TYPE_P2PSYN:
    {
        std::vector<CStatItemP2pSyn> vStatData;
        if (nGetCount > 0)
        {
            if (!pDataStat->GetP2pSynStatData(hashFork, nBeginTimeValue, nGetCount, vStatData))
            {
                throw CRPCException(RPC_INTERNAL_ERROR, "query error");
            }
        }

        int nTimeWidth = 8 + 2;                                   //hh:mm:ss + two spaces
        int nRecvBlockTPSWidth = string("recvblocks").size() + 2; //+ two spaces
        int nRecvTxTPSWidth = string("recvtps").size() + 2;
        int nSendBlockTPSWidth = string("sendblocks").size() + 2;
        int nSendTxTPSWidth = string("sendtps").size() + 2;
        for (const CStatItemP2pSyn& item : vStatData)
        {
            int nTempValue;
            nTempValue = to_string(item.nRecvBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nRecvBlockTPSWidth)
            {
                nRecvBlockTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSynRecvTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nRecvTxTPSWidth)
            {
                nRecvTxTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSendBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nSendBlockTPSWidth)
            {
                nSendBlockTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSynSendTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nSendTxTPSWidth)
            {
                nSendTxTPSWidth = nTempValue;
            }
        }

        int64 nTimeOffset = GetLocalTimeSeconds() - GetTime();

        string strResult;
        strResult += GetWidthString("time", nTimeWidth);
        strResult += GetWidthString("recvblocks", nRecvBlockTPSWidth);
        strResult += GetWidthString("recvtps", nRecvTxTPSWidth);
        strResult += GetWidthString("sendblocks", nSendBlockTPSWidth);
        strResult += GetWidthString("sendtps", nSendTxTPSWidth);
        strResult += string("\r\n");
        for (const CStatItemP2pSyn& item : vStatData)
        {
            int nLocalTimeValue = item.nTimeValue * 60 + nTimeOffset;
            if (nLocalTimeValue >= 0)
            {
                nLocalTimeValue %= (24 * 3600);
            }
            else
            {
                nLocalTimeValue += (24 * 3600);
            }
            char sTimeBuf[128] = { 0 };
            sprintf(sTimeBuf, "%2.2d:%2.2d:59", nLocalTimeValue / 3600, nLocalTimeValue % 3600 / 60);
            strResult += GetWidthString(sTimeBuf, nTimeWidth);
            strResult += GetWidthString(to_string(item.nRecvBlockCount), nRecvBlockTPSWidth);
            strResult += GetWidthString(item.nSynRecvTxTPS, nRecvTxTPSWidth);
            strResult += GetWidthString(to_string(item.nSendBlockCount), nSendBlockTPSWidth);
            strResult += GetWidthString(item.nSynSendTxTPS, nSendTxTPSWidth);
            strResult += string("\r\n");
        }
        return MakeCQueryStatResultPtr(strResult);
    }
    default:
        break;
    }

    return MakeCQueryStatResultPtr(string("error"));
}

////////////////////////////////////////////////////////////////////////
/* eth rpc*/

CRPCResultPtr CRPCMod::RPCEthGetWebClientVersion(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCweb3_clientVersionResultPtr(string("null"));
}

CRPCResultPtr CRPCMod::RPCEthGetSha3(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Cweb3_sha3Param>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetSha3: Invalid paramlist");
        return nullptr;
    }
    vector<unsigned char> vData = ParseHexString(spParam->vecParamlist.at(0));
    if (vData.empty())
    {
        StdLog("CRPCMod", "RPC EthGetSha3: Invalid data");
        return nullptr;
    }
    bytes btSha3 = dev::sha3_bytes(vData);
    return MakeCweb3_sha3ResultPtr(ToHexString(btSha3));
}

CRPCResultPtr CRPCMod::RPCEthGetNetVersion(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCnet_versionResultPtr(ToHexString((uint32)NETWORK_NETID));
}

CRPCResultPtr CRPCMod::RPCEthGetNetListening(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCnet_listeningResultPtr(true);
}

CRPCResultPtr CRPCMod::RPCEthGetPeerCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCnet_peerCountResultPtr(ToHexString((uint32)(pService->GetPeerCount())));
}

CRPCResultPtr CRPCMod::RPCEthGetChainId(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCeth_chainIdResultPtr(ToHexString(ctxReq.nReqChainId));
}

CRPCResultPtr CRPCMod::RPCEthGetProtocolVersion(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCeth_protocolVersionResultPtr(string("v") + VERSION_STR);
}

CRPCResultPtr CRPCMod::RPCEthGetSyncing(const CReqContext& ctxReq, CRPCParamPtr param)
{
    uint256 hashBlock;
    int nLastHeight;
    if (!pService->GetForkLastBlock(ctxReq.hashFork, nLastHeight, hashBlock))
    {
        hashBlock = ctxReq.hashFork;
    }

    CBlockStatus status;
    if (!pService->GetBlockStatus(hashBlock, status))
    {
        StdLog("CRPCMod", "RPC EthGetSyncing: Get block status fail");
        return nullptr;
    }

    auto spResult = MakeCeth_syncingResultPtr();
    spResult->strStartingblock = "0x0";
    spResult->strCurrentblock = ToHexString(status.nBlockNumber);
    spResult->strHighestblock = ToHexString(status.nBlockNumber);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCEthGetCoinbase(const CReqContext& ctxReq, CRPCParamPtr param)
{
    std::vector<CDestination> vMintAddressList;
    pBlockMaker->GetMiningAddressList(vMintAddressList);
    if (!vMintAddressList.empty())
    {
        return MakeCeth_coinbaseResultPtr(vMintAddressList[0].ToString());
    }
    return MakeCeth_coinbaseResultPtr(string("null"));
}

CRPCResultPtr CRPCMod::RPCEthGetMining(const CReqContext& ctxReq, CRPCParamPtr param)
{
    std::vector<CDestination> vMintAddressList;
    pBlockMaker->GetMiningAddressList(vMintAddressList);
    return MakeCeth_miningResultPtr(!vMintAddressList.empty());
}

CRPCResultPtr CRPCMod::RPCEthGetHashRate(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCeth_hashrateResultPtr(string("0x0"));
}

CRPCResultPtr CRPCMod::RPCEthGetGasPrice(const CReqContext& ctxReq, CRPCParamPtr param)
{
    return MakeCeth_gasPriceResultPtr(pService->GetForkMintMinGasPrice(ctxReq.hashFork).GetValueHex());
}

CRPCResultPtr CRPCMod::RPCEthGetAccounts(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spResult = MakeCeth_accountsResultPtr();

    vector<tuple<CDestination, uint8, uint512>> vDes;
    ListDestination(vDes);
    for (const auto& vd : vDes)
    {
        spResult->vecResult.push_back(std::get<0>(vd).ToString());
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCEthGetBalance(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getBalanceParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC Eth Get Balance: Invalid paramlist");
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RPC Eth Get Balance: Invalid address, address: %s", address.ToString().c_str());
        return nullptr;
    }

    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, (spParam->vecParamlist.size() > 1 ? spParam->vecParamlist.at(1) : string()), true);

    CWalletBalance balance;
    if (!pService->GetBalance(ctxReq.hashFork, hashBlock, address, balance))
    {
        StdLog("CRPCMod", "RPC Eth Get Balance: Get balance fail, address: %s, block: %s", address.ToString().c_str(), hashBlock.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getBalanceResultPtr(balance.nAvailable.GetValueHex());
}

CRPCResultPtr CRPCMod::RPCEthGetBlockNumber(const CReqContext& ctxReq, CRPCParamPtr param)
{
    CBlockStatus status;
    if (!pService->GetLastBlockStatus(ctxReq.hashFork, status))
    {
        StdLog("CRPCMod", "RPC EthGetBlockNumber: Get last block status fail");
        return nullptr;
    }
    return MakeCeth_blockNumberResultPtr(ToHexString(status.nBlockNumber));
}

CRPCResultPtr CRPCMod::RPCEthGetStorageAt(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getStorageAtParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() != 3)
    {
        StdLog("CRPCMod", "RP EthGetStorageAt: Invalid paramlist, size: %lu", spParam->vecParamlist.size());
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RP EthGetStorageAt: Invalid address");
        return nullptr;
    }
    string strPosition = spParam->vecParamlist.at(1);
    string strRefBlock = spParam->vecParamlist.at(2);

    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, strRefBlock);
    if (hashBlock.IsNull())
    {
        StdLog("CRPCMod", "RP EthGetStorageAt: Invalid tag");
        return nullptr;
    }

    if (!(strPosition.size() > 2 && strPosition[0] == '0' && strPosition[1] == 'x'))
    {
        StdLog("CRPCMod", "RP EthGetStorageAt: Invalid pos: %s", strPosition.c_str());
        return nullptr;
    }
    if (strPosition.size() % 2 == 1)
    {
        strPosition.insert(2, "0");
    }

    uint256 key;
    bytes btPosData = ParseHexString(strPosition);
    size_t nCpSize = key.size();
    if (btPosData.size() < nCpSize)
    {
        nCpSize = btPosData.size();
    }
    memcpy(key.begin() + (key.size() - nCpSize), btPosData.data(), nCpSize);

    bytes value;
    if (!pService->RetrieveContractKvValue(ctxReq.hashFork, hashBlock, address, key, value))
    {
        StdLog("CRPCMod", "RP EthGetStorageAt: Retrieve contract kv value fail, address: %s, key: %s, block: %s",
               address.ToString().c_str(), key.ToString().c_str(), hashBlock.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getStorageAtResultPtr(ToHexString(value));
}

CRPCResultPtr CRPCMod::RPCEthGetStorageRoot(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getStorageRootParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetStorageRoot: Invalid paramlist");
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RPC EthGetStorageRoot: Invalid address");
        return nullptr;
    }
    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, (spParam->vecParamlist.size() > 1 ? spParam->vecParamlist.at(1) : string()));

    CBlockStatus status;
    if (!pService->GetBlockStatus(hashBlock, status))
    {
        StdLog("CRPCMod", "RPC EthGetStorageRoot: Get block status fail");
        return nullptr;
    }
    return MakeCeth_getStorageRootResultPtr(status.hashStateRoot.GetHex());
}

CRPCResultPtr CRPCMod::RPCEthGetTransactionCount(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getTransactionCountParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetTransactionCount: Invalid paramlist");
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RPC EthGetTransactionCount: Invalid address");
        return nullptr;
    }
    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, (spParam->vecParamlist.size() > 1 ? spParam->vecParamlist.at(1) : string()));

    CWalletBalance balance;
    if (!pService->GetBalance(ctxReq.hashFork, hashBlock, address, balance))
    {
        StdLog("CRPCMod", "RPC EthGetTransactionCount: Get balance fail, address: %s, block: %s, fork: %s", address.ToString().c_str(), hashBlock.ToString().c_str(), ctxReq.hashFork.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getTransactionCountResultPtr(ToHexString(balance.nTxNonce + 1));
}

CRPCResultPtr CRPCMod::RPCEthGetPendingTransactions(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spResult = MakeCeth_pendingTransactionsResultPtr();

    vector<CTxInfo> vTxPool;
    pService->ListTxPool(ctxReq.hashFork, CDestination(), vTxPool, 0, 0);

    for (const CTxInfo& txinfo : vTxPool)
    {
        CEthTxData txData;

        txData.strHash = txinfo.txid.GetHex();
        txData.strNonce = ToHexString(txinfo.nTxNonce);
        txData.strFrom = txinfo.destFrom.ToString();
        txData.strTo = txinfo.destTo.ToString();
        txData.strValue = txinfo.nAmount.GetValueHex();
        txData.strGasprice = txinfo.nGasPrice.GetValueHex();
        txData.strGas = txinfo.nGas.GetValueHex();
        txData.strInput = "";

        spResult->vecTransaction.push_back(txData);
        if (spResult->vecTransaction.size() >= 256)
        {
            break;
        }
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCEthGetBlockTransactionCountByHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getBlockTransactionCountByHashParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByHash: Invalid paramlist");
        return nullptr;
    }

    uint256 hashBlock;
    hashBlock.SetHex(spParam->vecParamlist.at(0));
    if (hashBlock.IsNull())
    {
        StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByHash: Invalid block hash");
        return nullptr;
    }

    CBlockStatus status, statusPrev;
    if (!pService->GetBlockStatus(hashBlock, status))
    {
        StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByHash: Get block status fail");
        return nullptr;
    }
    if (status.hashPrevBlock != 0)
    {
        if (!pService->GetBlockStatus(status.hashPrevBlock, statusPrev))
        {
            StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByHash: Get block status fail");
            return nullptr;
        }
    }

    return MakeCeth_getBlockTransactionCountByHashResultPtr(ToHexString(status.nTotalTxCount - statusPrev.nTotalTxCount));
}

CRPCResultPtr CRPCMod::RPCEthGetBlockTransactionCountByNumber(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getBlockTransactionCountByNumberParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByNumber: Invalid paramlist");
        return nullptr;
    }

    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, spParam->vecParamlist.at(0));

    CBlockStatus status, statusPrev;
    if (!pService->GetBlockStatus(hashBlock, status))
    {
        StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByNumber: Get block status fail");
        return nullptr;
    }
    if (status.hashPrevBlock != 0)
    {
        if (!pService->GetBlockStatus(status.hashPrevBlock, statusPrev))
        {
            StdLog("CRPCMod", "RPC EthGetBlockTransactionCountByNumber: Get block status fail");
            return nullptr;
        }
    }

    return MakeCeth_getBlockTransactionCountByNumberResultPtr(ToHexString(status.nTotalTxCount - statusPrev.nTotalTxCount));
}

CRPCResultPtr CRPCMod::RPCEthGetUncleCountByBlockHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    //throw CRPCException(RPC_REQUEST_FUNC_OBSOLETE, "Requested function is obsolete");
    return MakeCeth_getUncleCountByBlockHashResultPtr("0x0");
}

CRPCResultPtr CRPCMod::RPCEthGetUncleCountByBlockNumber(const CReqContext& ctxReq, CRPCParamPtr param)
{
    //throw CRPCException(RPC_REQUEST_FUNC_OBSOLETE, "Requested function is obsolete");
    return MakeCeth_getUncleCountByBlockNumberResultPtr("0x0");
}

CRPCResultPtr CRPCMod::RPCEthGetCode(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getCodeParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetCode: Invalid paramlist");
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RPC EthGetCode: Invalid address");
        return nullptr;
    }
    uint256 hashBlock = GetRefBlock(ctxReq.hashFork, (spParam->vecParamlist.size() > 1 ? spParam->vecParamlist.at(1) : string()));

    CContractAddressContext ctxtContract;
    if (!pService->GeDestContractContext(ctxReq.hashFork, hashBlock, address, ctxtContract))
    {
        StdLog("CRPCMod", "RPC EthGetCode: Get address context fail");
        return nullptr;
    }

    bytes btCode;
    if (!pService->GetContractCode(ctxReq.hashFork, hashBlock, ctxtContract.hashContractCreateCode, btCode))
    {
        StdLog("CRPCMod", "RPC EthGetCode: Get code fail");
        return nullptr;
    }

    return MakeCeth_getCodeResultPtr(ToHexString(btCode));
}

CRPCResultPtr CRPCMod::RPCEthSign(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_signParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() < 2)
    {
        StdLog("CRPCMod", "RPC EthSign: Invalid paramlist");
        return nullptr;
    }

    CDestination address;
    address.ParseString(spParam->vecParamlist.at(0));
    if (address.IsNull())
    {
        StdLog("CRPCMod", "RPC EthSign: Invalid address");
        return nullptr;
    }
    bytes btMsg = ParseHexString(spParam->vecParamlist.at(1));
    if (btMsg.empty())
    {
        StdLog("CRPCMod", "RPC EthSign: Invalid message");
        return nullptr;
    }

    uint256 hash = CryptoKeccakHash(btMsg.data(), btMsg.size());

    bytes btSig;
    if (!pService->SignSignature(address, hash, btSig))
    {
        StdLog("CRPCMod", "RPC EthSign: Failed to sign message");
        return nullptr;
    }

    return MakeCeth_signResultPtr(ToHexString(btSig));
}

CRPCResultPtr CRPCMod::RPCEthSignTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_signTransactionParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthSignTransaction: Invalid paramlist");
        return nullptr;
    }

    CDestination destFrom;
    CDestination destTo;
    uint64 nNonce = 0;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGas;
    bytes btData;

    auto txParam = spParam->vecParamlist.at(0);

    if (!txParam.strFrom.IsValid() || !destFrom.ParseString(txParam.strFrom) || destFrom.IsNull())
    {
        StdLog("CRPCMod", "RPC EthSignTransaction: Invalid from");
        return nullptr;
    }
    if (txParam.strTo.IsValid())
    {
        destTo.ParseString(txParam.strTo);
    }
    if (txParam.strNonce.IsValid())
    {
        nNonce = ParseNumericHexString(txParam.strNonce);
    }
    if (txParam.strValue.IsValid())
    {
        nAmount.SetValueHex(txParam.strValue);
    }
    if (txParam.strGasprice.IsValid())
    {
        nGasPrice.SetValueHex(txParam.strGasprice);
    }
    if (txParam.strGas.IsValid())
    {
        nGas.SetValueHex(txParam.strGas);
    }
    if (txParam.strData.IsValid())
    {
        btData = ParseHexString(txParam.strData);
    }

    uint256 txid;
    bytes btSignTxData;
    auto strErr = pService->SignEthTransaction(ctxReq.hashFork, destFrom, destTo, nAmount,
                                               nNonce, nGasPrice, nGas, btData, 0, txid, btSignTxData);
    if (strErr)
    {
        StdLog("CRPCMod", "RPC EthSignTransaction: Failed to create transaction: %s", strErr->c_str());
        return nullptr;
    }

    auto spResult = MakeCeth_signTransactionResultPtr();

    spResult->strTx = txid.GetHex();
    spResult->strRaw = ToHexString(btSignTxData);

    return spResult;
}

CRPCResultPtr CRPCMod::RPCEthSendTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_sendTransactionParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthSendTransaction: Invalid paramlist");
        uint256 txid = 0;
        return MakeCeth_sendTransactionResultPtr(txid.GetHex());
    }

    CDestination destFrom;
    CDestination destTo;
    uint64 nNonce = 0;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGas;
    bytes btData;

    auto txParam = spParam->vecParamlist.at(0);

    if (!txParam.strFrom.IsValid() || !destFrom.ParseString(txParam.strFrom) || destFrom.IsNull())
    {
        StdLog("CRPCMod", "RPC EthSendTransaction: Invalid from");
        uint256 txid = 0;
        return MakeCeth_sendTransactionResultPtr(txid.GetHex());
    }
    if (txParam.strTo.IsValid())
    {
        destTo.ParseString(txParam.strTo);
    }
    if (txParam.strNonce.IsValid())
    {
        nNonce = ParseNumericHexString(txParam.strNonce);
    }
    if (txParam.strValue.IsValid())
    {
        nAmount.SetValueHex(txParam.strValue);
    }
    if (txParam.strGasprice.IsValid())
    {
        nGasPrice.SetValueHex(txParam.strGasprice);
    }
    if (txParam.strGas.IsValid())
    {
        nGas.SetValueHex(txParam.strGas);
    }
    if (txParam.strData.IsValid())
    {
        btData = ParseHexString(txParam.strData);
    }

    const uint256& hashFork = ctxReq.hashFork;

    uint256 txid;
    bytes btSignTxData;
    auto strErr = pService->SignEthTransaction(hashFork, destFrom, destTo, nAmount,
                                               nNonce, nGasPrice, nGas, btData, 0, txid, btSignTxData);
    if (strErr)
    {
        StdLog("CRPCMod", "RPC EthSendTransaction: Failed to create transaction: %s", strErr->c_str());
        uint256 txid = 0;
        return MakeCeth_sendTransactionResultPtr(txid.GetHex());
    }

    bool fLinkToPubkeyAddress = false;
    if (!destTo.IsNull())
    {
        CAddressContext ctxAddress;
        if (!pService->RetrieveAddressContext(hashFork, destTo, ctxAddress))
        {
            fLinkToPubkeyAddress = true;
        }
    }

    CTransaction txNew;
    if (!txNew.SetEthTx(btSignTxData, fLinkToPubkeyAddress))
    {
        StdLog("CRPCMod", "RPC EthSendTransaction: Packet eth tx fail");
        uint256 txid = 0;
        return MakeCeth_sendTransactionResultPtr(txid.GetHex());
    }

    Errno err = pService->SendTransaction(hashFork, txNew);
    if (err != OK)
    {
        StdLog("CRPCMod", "RPC EthSendTransaction: Tx rejected: %s", ErrorString(err));
        uint256 txid = 0;
        return MakeCeth_sendTransactionResultPtr(txid.GetHex());
    }

    return MakeCeth_sendTransactionResultPtr(txid.GetHex());
}

CRPCResultPtr CRPCMod::RPCEthSendRawTransaction(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_sendRawTransactionParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthSendRawTransaction: Invalid paramlist");
        uint256 txid = 0;
        return MakeCeth_sendRawTransactionResultPtr(txid.GetHex());
    }
    string strTxData = spParam->vecParamlist.at(0);
    if (strTxData.empty())
    {
        StdLog("CRPCMod", "RPC EthSendRawTransaction: Invalid txdata");
        uint256 txid = 0;
        return MakeCeth_sendRawTransactionResultPtr(txid.GetHex());
    }

    bytes txData = ParseHexString(strTxData);
    if (txData.empty())
    {
        StdLog("CRPCMod", "RPC EthSendRawTransaction: Invalid raw tx");
        uint256 txid = 0;
        return MakeCeth_sendRawTransactionResultPtr(txid.GetHex());
    }

    uint256 txid;
    if (!pService->SendEthRawTransaction(txData, txid))
    {
        StdLog("CRPCMod", "RPC EthSendRawTransaction: Send raw tx fail");
        txid = 0;
    }

    return MakeCeth_sendRawTransactionResultPtr(txid.GetHex());
}

CRPCResultPtr CRPCMod::RPCEthCall(const CReqContext& ctxReq, CRPCParamPtr param)
{
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGas;
    bytes btData;
    uint256 hashBlock;

    json_spirit::Value valParam;
    if (!json_spirit::read_string(param->GetParamJson(), valParam, RPC_MAX_DEPTH))
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }
    if (valParam.type() != json_spirit::array_type)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must be an array.");
    }
    const json_spirit::Array& arrayParam = valParam.get_array();
    if (arrayParam.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must non empty.");
    }

    for (auto& v : arrayParam)
    {
        if (v.type() == json_spirit::obj_type)
        {
            const json_spirit::Value& objFrom = json_spirit::find_value(v.get_obj(), "from");
            if (!objFrom.is_null() && objFrom.type() == json_spirit::str_type)
            {
                destFrom.ParseString(objFrom.get_str());
            }
            const json_spirit::Value& objTo = json_spirit::find_value(v.get_obj(), "to");
            if (!objTo.is_null() && objTo.type() == json_spirit::str_type)
            {
                destTo.ParseString(objTo.get_str());
            }
            const json_spirit::Value& objGas = json_spirit::find_value(v.get_obj(), "gas");
            if (!objGas.is_null() && objGas.type() == json_spirit::str_type)
            {
                nGas.SetValueHex(objGas.get_str());
            }
            const json_spirit::Value& objGasPrice = json_spirit::find_value(v.get_obj(), "gasPrice");
            if (!objGasPrice.is_null() && objGasPrice.type() == json_spirit::str_type)
            {
                nGasPrice.SetValueHex(objGasPrice.get_str());
            }
            const json_spirit::Value& objData = json_spirit::find_value(v.get_obj(), "data");
            if (!objData.is_null() && objData.type() == json_spirit::str_type)
            {
                btData = ParseHexString(objData.get_str());
            }
            const json_spirit::Value& objCode = json_spirit::find_value(v.get_obj(), "code");
            if (!objCode.is_null() && objCode.type() == json_spirit::str_type)
            {
                btData = ParseHexString(objCode.get_str());
            }
        }
        else if (v.type() == json_spirit::str_type)
        {
            hashBlock = GetRefBlock(ctxReq.hashFork, v.get_str());
        }
        else if (v.type() == json_spirit::int_type)
        {
            hashBlock = GetRefBlock(ctxReq.hashFork, ToHexString((uint64)(v.get_int())));
        }
    }

    if (nGasPrice == 0)
    {
        nGasPrice = pService->GetForkMintMinGasPrice(ctxReq.hashFork);
    }
    if (nGas == 0)
    {
        nGas = DEF_TX_GAS_LIMIT;
    }
    if (hashBlock == 0)
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(ctxReq.hashFork, nLastHeight, hashBlock))
        {
            hashBlock = ctxReq.hashFork;
        }
    }

    if (destTo.IsNull())
    {
        StdLog("CRPCMod", "Rpc eth call: to is null");
        return nullptr;
    }

    const uint256& hashFork = ctxReq.hashFork;
    CAddressContext ctxAddress;
    if (!pService->RetrieveAddressContext(hashFork, destTo, ctxAddress))
    {
        StdLog("CRPCMod", "Rpc eth call: To not is created, to: %s", destTo.ToString().c_str());
        return nullptr;
    }
    if (!ctxAddress.IsContract())
    {
        StdLog("CRPCMod", "Rpc eth call: To not is contract, to: %s", destTo.ToString().c_str());
        return nullptr;
    }

    uint256 nUsedGas;
    uint64 nGasLeft = 0;
    int nStatus = 0;
    bytes btResult;
    if (!pService->CallContract(true, hashFork, hashBlock, destFrom, destTo, nAmount, nGasPrice, nGas, btData, nUsedGas, nGasLeft, nStatus, btResult))
    {
        StdLog("CRPCMod", "Rpc eth call: call fail, to: %s", destTo.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_callResultPtr(ToHexString(btResult));
}

CRPCResultPtr CRPCMod::RPCEthEstimateGas(const CReqContext& ctxReq, CRPCParamPtr param)
{
    CDestination destFrom;
    CDestination destTo;
    uint256 nAmount;
    uint256 nGasPrice;
    uint256 nGas;
    bytes btData;
    uint256 hashBlock;

    json_spirit::Value valParam;
    if (!json_spirit::read_string(param->GetParamJson(), valParam, RPC_MAX_DEPTH))
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }
    if (valParam.type() != json_spirit::array_type)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must be an array.");
    }
    const json_spirit::Array& arrayParam = valParam.get_array();
    if (arrayParam.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must non empty.");
    }

    for (auto& v : arrayParam)
    {
        if (v.type() == json_spirit::obj_type)
        {
            const json_spirit::Value& objFrom = json_spirit::find_value(v.get_obj(), "from");
            if (!objFrom.is_null() && objFrom.type() == json_spirit::str_type)
            {
                destFrom.ParseString(objFrom.get_str());
            }
            const json_spirit::Value& objTo = json_spirit::find_value(v.get_obj(), "to");
            if (!objTo.is_null() && objTo.type() == json_spirit::str_type)
            {
                destTo.ParseString(objTo.get_str());
            }
            const json_spirit::Value& objValue = json_spirit::find_value(v.get_obj(), "value");
            if (!objValue.is_null() && objValue.type() == json_spirit::str_type)
            {
                nAmount.SetValueHex(objValue.get_str());
            }
            const json_spirit::Value& objGas = json_spirit::find_value(v.get_obj(), "gas");
            if (!objGas.is_null() && objGas.type() == json_spirit::str_type)
            {
                nGas.SetValueHex(objGas.get_str());
            }
            const json_spirit::Value& objGasPrice = json_spirit::find_value(v.get_obj(), "gasPrice");
            if (!objGasPrice.is_null() && objGasPrice.type() == json_spirit::str_type)
            {
                nGasPrice.SetValueHex(objGasPrice.get_str());
            }
            const json_spirit::Value& objData = json_spirit::find_value(v.get_obj(), "data");
            if (!objData.is_null() && objData.type() == json_spirit::str_type)
            {
                btData = ParseHexString(objData.get_str());
            }
            const json_spirit::Value& objCode = json_spirit::find_value(v.get_obj(), "code");
            if (!objCode.is_null() && objCode.type() == json_spirit::str_type)
            {
                btData = ParseHexString(objCode.get_str());
            }
        }
        else if (v.type() == json_spirit::str_type)
        {
            hashBlock = GetRefBlock(ctxReq.hashFork, v.get_str());
        }
        else if (v.type() == json_spirit::int_type)
        {
            hashBlock = GetRefBlock(ctxReq.hashFork, ToHexString((uint64)(v.get_int())));
        }
    }

    if (nGasPrice == 0)
    {
        nGasPrice = pService->GetForkMintMinGasPrice(ctxReq.hashFork);
    }
    if (hashBlock == 0)
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(ctxReq.hashFork, nLastHeight, hashBlock))
        {
            hashBlock = ctxReq.hashFork;
        }
    }

    uint256 nGasUsed;
    if (destTo.IsNull())
    {
        uint256 nGasLimit = 0;
        uint256 nUsedGas = 0;
        uint64 nGasLeft = 0;
        int nStatus = 0;
        bytes btResult;
        if (!pService->CallContract(true, ctxReq.hashFork, hashBlock, destFrom, destTo, nAmount, nGasPrice, nGasLimit, btData, nUsedGas, nGasLeft, nStatus, btResult))
        {
            StdLog("rpcmod", "RPC EthEstimateGas: execution reverted");
            throw CRPCException(RPC_ETH_EXECUTION_REVERTED, "execution reverted");
        }
        else
        {
            nGasUsed = nUsedGas;
        }
    }
    else
    {
        bool fAddressCtxExist = true;
        uint8 nDestType = 0;
        CAddressContext ctxAddress;
        if (!pService->RetrieveAddressContext(ctxReq.hashFork, destTo, ctxAddress))
        {
            fAddressCtxExist = false;
            nDestType = CDestination::PREFIX_PUBKEY;
        }
        else
        {
            nDestType = ctxAddress.GetDestType();
        }
        if (nDestType == CDestination::PREFIX_PUBKEY || nDestType == CDestination::PREFIX_TEMPLATE)
        {
            CTransaction tx;
            if (!btData.empty())
            {
                tx.AddTxData(CTransaction::DF_COMMON, btData);
            }
            if (!fAddressCtxExist)
            {
                tx.SetToAddressData(CAddressContext(CPubkeyAddressContext()));
            }

            uint256 nTvGas;
            CAddressContext ctxFromAddress;
            if (pService->RetrieveAddressContext(ctxReq.hashFork, destFrom, ctxFromAddress) && ctxFromAddress.IsPubkey())
            {
                uint64 nRefBlockTime;
                CBlockStatus statusBlock;
                if (!pService->GetBlockStatus(hashBlock, statusBlock))
                {
                    nRefBlockTime = GetNetTime();
                }
                else
                {
                    nRefBlockTime = statusBlock.nBlockTime;
                }

                CTimeVault tv;
                if (!pService->RetrieveTimeVault(ctxReq.hashFork, hashBlock, destFrom, tv))
                {
                    tv.SetNull();
                }
                uint256 nTvGasFee = tv.EstimateTransTvGasFee(nRefBlockTime + ESTIMATE_TIME_VAULT_TS, nAmount);
                CTimeVault::CalcRealityTvGasFee(nGasPrice, nTvGasFee, nTvGas);
            }

            nGasUsed = tx.GetTxBaseGas() + nTvGas;
        }
        else
        {
            uint256 nGasLimit = 0;
            uint256 nUsedGas = 0;
            uint64 nGasLeft = 0;
            int nStatus = 0;
            bytes btResult;
            if (!pService->CallContract(true, ctxReq.hashFork, hashBlock, destFrom, destTo, nAmount, nGasPrice, nGasLimit, btData, nUsedGas, nGasLeft, nStatus, btResult))
            {
                StdLog("rpcmod", "RPC EthEstimateGas: execution reverted");
                throw CRPCException(RPC_ETH_EXECUTION_REVERTED, "execution reverted");
            }
            else
            {
                nGasUsed = nUsedGas;
            }
        }
    }

    return MakeCeth_estimateGasResultPtr(nGasUsed.GetValueHex());
}

CRPCResultPtr CRPCMod::RPCEthGetBlockByHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    uint256 hashBlock;
    bool fTxDetail = false;

    json_spirit::Value valParam;
    if (!json_spirit::read_string(param->GetParamJson(), valParam, RPC_MAX_DEPTH))
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }
    if (valParam.type() != json_spirit::array_type)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must be an array.");
    }
    const json_spirit::Array& arrayParam = valParam.get_array();
    if (arrayParam.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must non empty.");
    }

    for (auto& v : arrayParam)
    {
        if (v.type() == json_spirit::str_type)
        {
            hashBlock.SetHex(v.get_str());
        }
        else if (v.type() == json_spirit::bool_type)
        {
            fTxDetail = v.get_bool();
        }
    }
    if (hashBlock == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: block is null.");
    }

    CBlock block;
    CChainId nChainId;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, nChainId, fork, height))
    {
        StdLog("CRPCMod", "RPC EthGetBlockByHash: Get block fail, block: %s", hashBlock.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getBlockByHashResultPtr(EthBlockToJSON(block, fTxDetail));
}

CRPCResultPtr CRPCMod::RPCEthGetBlockByNumber(const CReqContext& ctxReq, CRPCParamPtr param)
{
    uint256 hashBlock;
    bool fTxDetail = false;

    json_spirit::Value valParam;
    if (!json_spirit::read_string(param->GetParamJson(), valParam, RPC_MAX_DEPTH))
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }
    if (valParam.type() != json_spirit::array_type)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must be an array.");
    }
    const json_spirit::Array& arrayParam = valParam.get_array();
    if (arrayParam.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse error: request must non empty.");
    }

    for (auto& v : arrayParam)
    {
        if (v.type() == json_spirit::str_type)
        {
            hashBlock = GetRefBlock(ctxReq.hashFork, v.get_str());
        }
        else if (v.type() == json_spirit::bool_type)
        {
            fTxDetail = v.get_bool();
        }
    }
    if (hashBlock == 0)
    {
        int nLastHeight;
        if (!pService->GetForkLastBlock(ctxReq.hashFork, nLastHeight, hashBlock))
        {
            hashBlock = ctxReq.hashFork;
        }
    }

    CBlock block;
    CChainId nChainId;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, nChainId, fork, height))
    {
        StdLog("CRPCMod", "RPC EthGetBlockByNumber: Get block fail, block: %s", hashBlock.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getBlockByNumberResultPtr(EthBlockToJSON(block, fTxDetail));
}

CRPCResultPtr CRPCMod::RPCEthGetTransactionByHash(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getTransactionByHashParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByHash: Invalid paramlist");
        return nullptr;
    }

    uint256 txid;
    txid.SetHex(spParam->vecParamlist.at(0));

    CTransaction tx;
    uint256 hashFork;
    uint256 hashBlock;
    uint64 nBlockNumber = 0;
    uint16 nTxIndex = 0;

    if (!pService->GetTransactionAndPosition(ctxReq.hashFork, txid, tx, hashFork, hashBlock, nBlockNumber, nTxIndex))
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByHash: No information available about transaction, txid: %s", txid.ToString().c_str());
        return nullptr;
    }

    return MakeCeth_getTransactionByHashResultPtr(EthTxToJSON(txid, tx, hashBlock, nBlockNumber, nTxIndex));
}

CRPCResultPtr CRPCMod::RPCEthGetTransactionByBlockHashAndIndex(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getTransactionByBlockHashAndIndexParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() != 2)
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByBlockHashAndIndex: Invalid paramlist");
        return nullptr;
    }

    const uint256& hashFork = ctxReq.hashFork;
    uint256 hashBlock;
    uint64 nIndex = 0;
    hashBlock = GetRefBlock(hashFork, spParam->vecParamlist.at(0));
    nIndex = ParseNumericHexString(spParam->vecParamlist.at(1));

    CTransaction tx;
    uint64 nBlockNumber = 0;
    if (!pService->GetTransactionByIndex(hashBlock, nIndex, tx, nBlockNumber))
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByBlockHashAndIndex: No information available about transaction");
        return nullptr;
    }
    uint256 txid = tx.GetHash();

    return MakeCeth_getTransactionByBlockHashAndIndexResultPtr(EthTxToJSON(txid, tx, hashBlock, nBlockNumber, nIndex));
}

CRPCResultPtr CRPCMod::RPCEthGetTransactionByBlockNumberAndIndex(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getTransactionByBlockNumberAndIndexParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() != 2)
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByBlockNumberAndIndex: Invalid paramlist");
        return nullptr;
    }

    const uint256& hashFork = ctxReq.hashFork;
    uint256 hashBlock;
    uint64 nIndex = 0;
    hashBlock = GetRefBlock(hashFork, spParam->vecParamlist.at(0));
    nIndex = ParseNumericHexString(spParam->vecParamlist.at(1));

    CTransaction tx;
    uint64 nBlockNumber = 0;
    if (!pService->GetTransactionByIndex(hashBlock, nIndex, tx, nBlockNumber))
    {
        StdLog("CRPCMod", "RPC EthGetTransactionByBlockNumberAndIndex: No information available about transaction");
        return nullptr;
    }
    uint256 txid = tx.GetHash();

    return MakeCeth_getTransactionByBlockNumberAndIndexResultPtr(EthTxToJSON(txid, tx, hashBlock, nBlockNumber, nIndex));
}

CRPCResultPtr CRPCMod::RPCEthGetTransactionReceipt(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getTransactionReceiptParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthGetTransactionReceipt: Invalid paramlist");
        return nullptr;
    }

    uint256 txid;
    txid.SetHex(spParam->vecParamlist.at(0));
    if (txid.IsNull())
    {
        StdLog("CRPCMod", "RPC EthGetTransactionReceipt: Invalid tx hash");
        return nullptr;
    }

    CTransactionReceiptEx receipt;
    if (!pService->GetTransactionReceipt(ctxReq.hashFork, txid, receipt))
    {
        StdLog("CRPCMod", "RPC EthGetTransactionReceipt: Get transaction receipt fail, txid: %s", txid.ToString().c_str());
        return nullptr;
    }

    auto spResult = MakeCeth_getTransactionReceiptResultPtr();

    spResult->strTransactionhash = txid.GetHex();
    spResult->nTransactionindex = receipt.nTxIndex;
    spResult->strBlockhash = receipt.hashBlock.GetHex();
    spResult->nBlocknumber = receipt.nBlockNumber;
    spResult->strFrom = receipt.from.ToString();
    spResult->strTo = receipt.to.ToString();
    spResult->strCumulativegasused = receipt.nBlockGasUsed.GetValueHex();
    spResult->strGasused = receipt.nTxGasUsed.GetValueHex();
    if (!receipt.destContract.IsNull())
    {
        spResult->strContractaddress = receipt.destContract.ToString();
    }

    for (std::size_t i = 0; i < receipt.vLogs.size(); i++)
    {
        auto& txLogs = receipt.vLogs[i];
        if (!txLogs.address.IsNull()
            || !txLogs.data.empty()
            || !txLogs.topics.empty())
        {
            Ceth_getTransactionReceiptResult::CLogs logs;

            //__optional__ CRPCBool fRemoved;
            logs.nLogindex = i;
            logs.nTransactionindex = receipt.nTxIndex;
            logs.strTransactionhash = txid.GetHex();
            logs.strBlockhash = receipt.hashBlock.GetHex();
            logs.nBlocknumber = receipt.nBlockNumber;

            logs.strAddress = txLogs.address.ToString();
            if (txLogs.data.empty())
            {
                logs.strData = "0x";
            }
            else
            {
                logs.strData = ToHexString(txLogs.data);
            }
            for (auto& d : txLogs.topics)
            {
                logs.vecTopics.push_back(d.ToString());
            }
            logs.strType = "mined"; //mined  pending

            bytes btIdData;
            btIdData.assign(txid.begin(), txid.end());
            btIdData.insert(btIdData.end(), (char*)&i, (char*)&i + sizeof(i));
            bytes btIdKey = CryptoKeccakSign(btIdData.data(), btIdData.size());
            logs.strId = string("log_") + ToHexString(btIdKey).substr(2);

            spResult->vecLogs.push_back(logs);
        }
    }
    // if (receipt.vLogs.size() == 0)
    // {
    //     Ceth_getTransactionReceiptResult::CLogs logs;
    //     spResult->vecLogs.push_back(logs);
    // }

    spResult->strLogsbloom = receipt.nLogsBloom.ToString();
    if (!receipt.hashStatusRoot.IsNull())
    {
        spResult->strRoot = receipt.hashStatusRoot.GetHex();
    }
    spResult->strStatus = (receipt.nContractStatus == 0 ? "0x1" : "0x0");
    spResult->strEffectivegasprice = receipt.nEffectiveGasPrice.GetValueHex();

    return spResult;
}

CRPCResultPtr CRPCMod::RPCEthGetUncleByBlockHashAndIndex(const CReqContext& ctxReq, CRPCParamPtr param)
{
    //throw CRPCException(RPC_REQUEST_FUNC_OBSOLETE, "Requested function is obsolete");
    //return MakeCeth_getUncleByBlockHashAndIndexResultPtr(CEthBlock());
    return nullptr;
}

CRPCResultPtr CRPCMod::RPCEthGetUncleByBlockNumberAndIndex(const CReqContext& ctxReq, CRPCParamPtr param)
{
    //throw CRPCException(RPC_REQUEST_FUNC_OBSOLETE, "Requested function is obsolete");
    //return MakeCeth_getUncleByBlockNumberAndIndexResultPtr(CEthBlock());
    return nullptr;
}

CRPCResultPtr CRPCMod::RPCEthNewFilter(const CReqContext& ctxReq, CRPCParamPtr param)
{
    CLogsFilter logFilter = GetLogFilterFromJson(ctxReq.hashFork, param->GetParamJson());

    // StdLog("CRPCMod", "RPC EthNewFilter: fromBlock: %s", logFilter.hashFromBlock.ToString().c_str());
    // StdLog("CRPCMod", "RPC EthNewFilter: toBlock: %s", logFilter.hashToBlock.ToString().c_str());
    // for (auto& dest : logFilter.setAddress)
    // {
    //     StdLog("CRPCMod", "RPC EthNewFilter: address: %s", dest.ToString().c_str());
    // }
    // for (int i = 0; i < 4; i++)
    // {
    //     for (auto& topics : logFilter.arrayTopics[i])
    //     {
    //         StdLog("CRPCMod", "RPC EthNewFilter: [%d] topics: %s", i, topics.ToString().c_str());
    //     }
    // }

    mtbase::CBufStream ss;
    ss << ctxReq.strPeerIp << ctxReq.nPeerPort;
    uint256 hashClient = CryptoHash(ss.GetData(), ss.GetSize());

    uint256 nFilterId = pService->AddLogsFilter(hashClient, ctxReq.hashFork, logFilter);
    return MakeCeth_newFilterResultPtr(nFilterId.ToString());
}

CRPCResultPtr CRPCMod::RPCEthNewBlockFilter(const CReqContext& ctxReq, CRPCParamPtr param)
{
    mtbase::CBufStream ss;
    ss << ctxReq.strPeerIp << ctxReq.nPeerPort;
    uint256 hashClient = CryptoHash(ss.GetData(), ss.GetSize());

    uint256 nFilterId = pService->AddBlockFilter(hashClient, ctxReq.hashFork);
    return MakeCeth_newBlockFilterResultPtr(nFilterId.ToString());
}

CRPCResultPtr CRPCMod::RPCEthNewPendingTransactionFilter(const CReqContext& ctxReq, CRPCParamPtr param)
{
    mtbase::CBufStream ss;
    ss << ctxReq.strPeerIp << ctxReq.nPeerPort;
    uint256 hashClient = CryptoHash(ss.GetData(), ss.GetSize());

    uint256 nFilterId = pService->AddPendingTxFilter(hashClient, ctxReq.hashFork);
    return MakeCeth_newPendingTransactionFilterResultPtr(nFilterId.ToString());
}

CRPCResultPtr CRPCMod::RPCEthUninstallFilter(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_uninstallFilterParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        StdLog("CRPCMod", "RPC EthUninstallFilter: Param error");
        return MakeCeth_uninstallFilterResultPtr(false);
    }

    uint256 nFilterId;
    nFilterId.SetHex(spParam->vecParamlist.at(0));

    pService->RemoveFilter(nFilterId);

    return MakeCeth_uninstallFilterResultPtr(true);
}

CRPCResultPtr CRPCMod::RPCEthGetFilterChanges(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getFilterChangesParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }

    uint256 nFilterId;
    nFilterId.SetHex(spParam->vecParamlist.at(0));

    if (CFilterId::isLogsFilter(nFilterId))
    {
        ReceiptLogsVec vReceiptLogs;
        if (!pService->GetTxReceiptLogsByFilterId(nFilterId, false, vReceiptLogs))
        {
            StdLog("CRPCMod", "RPC EthGetFilterChanges: Get logs fail");
            return MakeCeth_getFilterChangesResultPtr();
        }

        auto spResult = MakeCeth_getFilterChangesResultPtr();
        for (auto& v : vReceiptLogs)
        {
            for (auto& d : v.matchLogs)
            {
                CTxReceiptLogs txLogs;

                txLogs.fRemoved = d.fRemoved;
                txLogs.strLogindex = ToHexString((uint64)(d.nLogIndex));
                txLogs.strTransactionindex = ToHexString((uint64)(v.nTxIndex));
                txLogs.strTransactionhash = v.txid.ToString();
                txLogs.strBlocknumber = ToHexString(v.nBlockNumber);
                txLogs.strBlockhash = v.hashBlock.ToString();
                txLogs.strAddress = d.address.ToString();
                if (d.data.empty())
                {
                    txLogs.strData = "0x";
                }
                else
                {
                    txLogs.strData = ToHexString(d.data);
                }
                for (auto& t : d.topics)
                {
                    txLogs.vecTopics.push_back(t.ToString());
                }

                spResult->vecResult.push_back(txLogs);
            }
        }
        return spResult;
    }
    else if (CFilterId::isBlockFilter(nFilterId))
    {
        std::vector<uint256> vBlockHash;
        if (!pService->GetFilterBlockHashs(ctxReq.hashFork, nFilterId, false, vBlockHash))
        {
            StdLog("CRPCMod", "RPC EthGetFilterChanges: Get block fail");
            return MakeCeth_getFilterBlockTxResultPtr();
        }

        auto spResult = MakeCeth_getFilterBlockTxResultPtr();
        for (auto& hash : vBlockHash)
        {
            spResult->vecResult.push_back(hash.ToString());
        }
        return spResult;
    }
    else if (CFilterId::isTxFilter(nFilterId))
    {
        std::vector<uint256> vTxid;
        if (!pService->GetFilterTxids(ctxReq.hashFork, nFilterId, false, vTxid))
        {
            StdLog("CRPCMod", "RPC EthGetFilterChanges: Get tx fail");
            return MakeCeth_getFilterBlockTxResultPtr();
        }

        auto spResult = MakeCeth_getFilterBlockTxResultPtr();
        for (auto& hash : vTxid)
        {
            spResult->vecResult.push_back(hash.ToString());
        }
        return spResult;
    }
    return MakeCeth_getFilterChangesResultPtr();
}

CRPCResultPtr CRPCMod::RPCEthGetFilterLogs(const CReqContext& ctxReq, CRPCParamPtr param)
{
    auto spParam = CastParamPtr<Ceth_getFilterLogsParam>(param);
    if (!spParam->vecParamlist.IsValid() || spParam->vecParamlist.size() == 0)
    {
        throw CRPCException(RPC_PARSE_ERROR, "Parse Error: request json string error.");
    }

    uint256 nFilterId;
    nFilterId.SetHex(spParam->vecParamlist.at(0));

    if (CFilterId::isLogsFilter(nFilterId))
    {
        ReceiptLogsVec vReceiptLogs;
        if (!pService->GetTxReceiptLogsByFilterId(nFilterId, true, vReceiptLogs))
        {
            StdLog("CRPCMod", "RPC EthGetFilterLogs: Get logs fail");
            return MakeCeth_getFilterLogsResultPtr();
        }

        auto spResult = MakeCeth_getFilterLogsResultPtr();
        for (auto& v : vReceiptLogs)
        {
            for (auto& d : v.matchLogs)
            {
                CTxReceiptLogs txLogs;

                txLogs.fRemoved = d.fRemoved;
                txLogs.strLogindex = ToHexString((uint64)(d.nLogIndex));
                txLogs.strTransactionindex = ToHexString((uint64)(v.nTxIndex));
                txLogs.strTransactionhash = v.txid.ToString();
                txLogs.strBlocknumber = ToHexString(v.nBlockNumber);
                txLogs.strBlockhash = v.hashBlock.ToString();
                txLogs.strAddress = d.address.ToString();
                if (d.data.empty())
                {
                    txLogs.strData = "0x";
                }
                else
                {
                    txLogs.strData = ToHexString(d.data);
                }
                for (auto& t : d.topics)
                {
                    txLogs.vecTopics.push_back(t.ToString());
                }

                spResult->vecResult.push_back(txLogs);
            }
        }
        return spResult;
    }
    else if (CFilterId::isBlockFilter(nFilterId))
    {
        std::vector<uint256> vBlockHash;
        if (!pService->GetFilterBlockHashs(ctxReq.hashFork, nFilterId, true, vBlockHash))
        {
            StdLog("CRPCMod", "RPC EthGetFilterLogs: Get block fail");
            return MakeCeth_getFilterBlockTxResultPtr();
        }

        auto spResult = MakeCeth_getFilterBlockTxResultPtr();
        for (auto& hash : vBlockHash)
        {
            spResult->vecResult.push_back(hash.ToString());
        }
        return spResult;
    }
    else if (CFilterId::isTxFilter(nFilterId))
    {
        std::vector<uint256> vTxid;
        if (!pService->GetFilterTxids(ctxReq.hashFork, nFilterId, true, vTxid))
        {
            StdLog("CRPCMod", "RPC EthGetFilterChanges: Get tx fail");
            return MakeCeth_getFilterBlockTxResultPtr();
        }

        auto spResult = MakeCeth_getFilterBlockTxResultPtr();
        for (auto& hash : vTxid)
        {
            spResult->vecResult.push_back(hash.ToString());
        }
        return spResult;
    }
    return MakeCeth_getFilterLogsResultPtr();
}

CRPCResultPtr CRPCMod::RPCEthGetLogs(const CReqContext& ctxReq, CRPCParamPtr param)
{
    CLogsFilter logsFilter = GetLogFilterFromJson(ctxReq.hashFork, param->GetParamJson());

    // StdLog("CRPCMod", "RPC EthGetLogs: fromBlock: %s", logsFilter.hashFromBlock.ToString().c_str());
    // StdLog("CRPCMod", "RPC EthGetLogs: toBlock: %s", logsFilter.hashToBlock.ToString().c_str());
    // for (auto& dest : logsFilter.setAddress)
    // {
    //     StdLog("CRPCMod", "RPC EthGetLogs: address: %s", dest.ToString().c_str());
    // }
    // for (int i = 0; i < 4; i++)
    // {
    //     for (auto& topics : logsFilter.arrayTopics[i])
    //     {
    //         StdLog("CRPCMod", "RPC EthGetLogs: [%d] topics: %s", i, topics.ToString().c_str());
    //     }
    // }

    ReceiptLogsVec vReceiptLogs;
    if (!pService->GetTxReceiptsByLogsFilter(ctxReq.hashFork, logsFilter, vReceiptLogs))
    {
        StdLog("CRPCMod", "RPC EthGetLogs: Get logs fail");
        return MakeCeth_getLogsResultPtr();
    }

    auto spResult = MakeCeth_getLogsResultPtr();
    for (auto& v : vReceiptLogs)
    {
        for (auto& d : v.matchLogs)
        {
            CTxReceiptLogs txLogs;

            txLogs.fRemoved = d.fRemoved;
            txLogs.strLogindex = ToHexString((uint64)(d.nLogIndex));
            txLogs.strTransactionindex = ToHexString((uint64)(v.nTxIndex));
            txLogs.strTransactionhash = v.txid.ToString();
            txLogs.strBlocknumber = ToHexString(v.nBlockNumber);
            txLogs.strBlockhash = v.hashBlock.ToString();
            txLogs.strAddress = d.address.ToString();
            if (d.data.empty())
            {
                txLogs.strData = "0x";
            }
            else
            {
                txLogs.strData = ToHexString(d.data);
            }
            for (auto& t : d.topics)
            {
                txLogs.vecTopics.push_back(t.ToString());
            }

            spResult->vecResult.push_back(txLogs);
        }
    }
    return spResult;
}

} // namespace metabasenet

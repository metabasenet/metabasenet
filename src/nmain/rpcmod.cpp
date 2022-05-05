// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcmod.h"

#include "json/json_spirit_reader_template.h"
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/regex.hpp>
#include <regex>
//#include <algorithm>

#include "rpc/auto_protocol.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/template.h"
#include "util.h"
#include "version.h"

using namespace std;
using namespace hnbase;
using namespace json_spirit;
using namespace metabasenet::rpc;
using namespace metabasenet;
namespace fs = boost::filesystem;

#define UNLOCKKEY_RELEASE_DEFAULT_TIME 60

const char* GetGitVersion();

///////////////////////////////
// static function

static CBlockData BlockToJSON(const uint256& hashBlock, const CBlock& block, const uint256& hashFork, int nHeight)
{
    CBlockData data;
    data.strHash = hashBlock.GetHex();
    data.strHashprev = block.hashPrev.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.nType);
    data.nTime = block.GetBlockTime();
    data.nNumber = block.GetBlockNumber();
    data.strStateroot = block.hashStateRoot.GetHex();
    data.strReceiptsroot = block.hashReceiptsRoot.GetHex();
    data.strBloom = block.nBloom.GetHex();
    if (block.hashPrev != 0)
    {
        data.strPrev = block.hashPrev.GetHex();
    }
    data.strFork = hashFork.GetHex();
    data.nHeight = nHeight;

    data.strTxmint = block.txMint.GetHash().GetHex();
    for (const CTransaction& tx : block.vtx)
    {
        data.vecTx.push_back(tx.GetHash().GetHex());
    }
    return data;
}

CTransactionData TxToJSON(const uint256& txid, const CTransaction& tx, const uint256& blockHash, const int nDepth, const uint64 nTxGasLeft)
{
    CTransactionData ret;
    ret.strTxid = txid.GetHex();
    ret.nVersion = tx.nVersion;
    ret.strType = tx.GetTypeString();
    ret.nTime = tx.nTimeStamp;
    ret.nNonce = tx.nTxNonce;
    ret.strFrom = tx.from.ToString();
    ret.strTo = tx.to.ToString();
    ret.strAmount = CoinToTokenBigFloat(tx.nAmount);

    uint256 nUsedGas = tx.nGasLimit - nTxGasLeft;
    ret.nGaslimit = tx.nGasLimit.Get64();
    ret.strGasprice = CoinToTokenBigFloat(tx.nGasPrice);
    ret.nGasused = nUsedGas.Get64();
    ret.strTxfee = CoinToTokenBigFloat(nUsedGas * tx.nGasPrice);

    bytes btTxData;
    tx.GetTxData(btTxData);
    ret.strData = hnbase::ToHexString(btTxData);

    ret.strSig = hnbase::ToHexString(tx.vchSig);
    ret.strFork = tx.hashFork.GetHex();
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

static CWalletTxData TxInfoToJSON(const CDestination& destLocal, const CDestTxInfo& tx)
{
    CWalletTxData data;

    data.strTxid = tx.txid.GetHex();
    data.nTxindex = tx.nTxIndex;
    data.nBlocknumber = tx.nBlockNumber;
    data.strTxtype = CTransaction::GetTypeStringStatic(tx.nTxType);
    data.nTime = (boost::int64_t)tx.nTimeStamp;
    data.strAmount = CoinToTokenBigFloat(tx.nAmount);
    data.strFee = CoinToTokenBigFloat(tx.nTxFee);

    if (tx.nDirection == CDestTxInfo::DXI_DIRECTION_CIN
        || tx.nDirection == CDestTxInfo::DXI_DIRECTION_COUT
        || tx.nDirection == CDestTxInfo::DXI_DIRECTION_CINOUT)
    {
        data.strTransfertype = "contract";
    }
    else if (tx.nDirection == CDestTxInfo::DXI_DIRECTION_CODEREWARD)
    {
        data.strTransfertype = "codereward";
    }
    else
    {
        data.strTransfertype = "common";
    }

    switch (tx.nDirection)
    {
    case CDestTxInfo::DXI_DIRECTION_IN:         // local is to, peer is from
    case CDestTxInfo::DXI_DIRECTION_CIN:        // contract transfer
    case CDestTxInfo::DXI_DIRECTION_CODEREWARD: // code reward
        data.strFrom = tx.destPeer.ToString();
        data.strTo = destLocal.ToString();
        break;
    case CDestTxInfo::DXI_DIRECTION_OUT:    // local is from, peer is to
    case CDestTxInfo::DXI_DIRECTION_COUT:   // contract transfer
    case CDestTxInfo::DXI_DIRECTION_INOUT:  // from equal to
    case CDestTxInfo::DXI_DIRECTION_CINOUT: // contract transfer from equal to
        data.strFrom = destLocal.ToString();
        data.strTo = tx.destPeer.ToString();
        break;
    }
    return data;
}

namespace metabasenet
{

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
        ("listcontractcode", &CRPCMod::RPCListContractCode)
        //
        ("listcontractaddress", &CRPCMod::RPCListContractAddress)
        //
        ("getdestcontract", &CRPCMod::RPCGetDestContract)
        //
        ("getcontractsource", &CRPCMod::RPCGetContractSource)
        //
        ("getcontractcode", &CRPCMod::RPCGetContractCode)
        /* Util */
        ("verifymessage", &CRPCMod::RPCVerifyMessage)
        //
        ("makekeypair", &CRPCMod::RPCMakeKeyPair)
        //
        ("getpubkey", &CRPCMod::RPCGetPubKey)
        //
        ("getpubkeyaddress", &CRPCMod::RPCGetPubKeyAddress)
        //
        ("getaddresskey", &CRPCMod::RPCGetAddressKey)
        //
        ("gettemplateaddress", &CRPCMod::RPCGetTemplateAddress)
        //
        ("getcontractaddress", &CRPCMod::RPCGetContractAddress)
        //
        ("maketemplate", &CRPCMod::RPCMakeTemplate)
        //
        ("decodetransaction", &CRPCMod::RPCDecodeTransaction)
        //
        ("gettxfee", &CRPCMod::RPCGetTxFee)
        //
        ("makesha256", &CRPCMod::RPCMakeSha256)
        //
        ("aesencrypt", &CRPCMod::RPCAesEncrypt)
        //
        ("aesdecrypt", &CRPCMod::RPCAesDecrypt)
        //
        ("reversehex", &CRPCMod::RPCReverseHex)
        //
        ("funcsign", &CRPCMod::RPCFuncSign)
        //
        ("makehash", &CRPCMod::RPCMakeHash)
        /* Mint */
        ("getwork", &CRPCMod::RPCGetWork)
        //
        ("submitwork", &CRPCMod::RPCSubmitWork)
        /* tool */
        ("querystat", &CRPCMod::RPCQueryStat);
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
    fWriteRPCLog = RPCServerConfig()->fRPCLogEnable;

    return true;
}

void CRPCMod::HandleDeinitialize()
{
    pHttpServer = nullptr;
    pCoreProtocol = nullptr;
    pService = nullptr;
    pDataStat = nullptr;
    pForkManager = nullptr;
}

bool CRPCMod::HandleEvent(CEventHttpReq& eventHttpReq)
{
    auto lmdMask = [](const string& data) -> string
    {
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

        bool fArray;
        CRPCReqVec vecReq = DeserializeCRPCReq(eventHttpReq.data.strContent, fArray);
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

                if (fWriteRPCLog)
                {
                    Debug("request : %s ", lmdMask(spReq->Serialize()).c_str());
                }

                spResult = (this->*(*it).second)(spReq->spParam);
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

bool CRPCMod::CheckWalletError(Errno err)
{
    switch (err)
    {
    case ERR_WALLET_NOT_FOUND:
        throw CRPCException(RPC_INVALID_REQUEST, "Missing wallet");
        break;
    case ERR_WALLET_IS_LOCKED:
        throw CRPCException(RPC_WALLET_UNLOCK_NEEDED,
                            "Wallet is locked,enter the wallet passphrase with walletpassphrase first.");
    case ERR_WALLET_IS_UNLOCKED:
        throw CRPCException(RPC_WALLET_ALREADY_UNLOCKED, "Wallet is already unlocked");
        break;
    case ERR_WALLET_IS_ENCRYPTED:
        throw CRPCException(RPC_WALLET_WRONG_ENC_STATE, "Running with an encrypted wallet, "
                                                        "but encryptwallet was called");
        break;
    case ERR_WALLET_IS_UNENCRYPTED:
        throw CRPCException(RPC_WALLET_WRONG_ENC_STATE, "Running with an unencrypted wallet, "
                                                        "but walletpassphrasechange/walletlock was called.");
        break;
    default:
        break;
    }
    return (err == OK);
}

void CRPCMod::ListDestination(vector<CDestination>& vDestination, const uint64 nPage, const uint64 nCount)
{
    set<crypto::CPubKey> setPubKey;
    size_t nPubkeyCount = pService->GetPubKeys(setPubKey, nCount * nPage, nCount);

    vDestination.clear();
    for (const crypto::CPubKey& pubkey : setPubKey)
    {
        vDestination.push_back(CDestination(pubkey));
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

    set<CTemplateId> setTid;
    pService->GetTemplateIds(setTid, nTidStartPos, nTidCount);
    for (const CTemplateId& tid : setTid)
    {
        vDestination.push_back(CDestination(tid));
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

/* System */
CRPCResultPtr CRPCMod::RPCHelp(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CHelpParam>(param);
    string command = spParam->strCommand;
    return MakeCHelpResultPtr(RPCHelpInfo(EModeType::MODE_CONSOLE, command));
}

CRPCResultPtr CRPCMod::RPCStop(CRPCParamPtr param)
{
    pService->Stop();
    return MakeCStopResultPtr("metabasenet server stopping");
}

CRPCResultPtr CRPCMod::RPCVersion(CRPCParamPtr param)
{
    string strVersion = string("MetabaseNet server version is v") + VERSION_STR + string(", git commit id is ") + GetGitVersion();
    return MakeCVersionResultPtr(strVersion);
}

/* Network */
CRPCResultPtr CRPCMod::RPCGetPeerCount(CRPCParamPtr param)
{
    return MakeCGetPeerCountResultPtr(pService->GetPeerCount());
}

CRPCResultPtr CRPCMod::RPCListPeer(CRPCParamPtr param)
{
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

CRPCResultPtr CRPCMod::RPCAddNode(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->AddNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to add node.");
    }

    return MakeCAddNodeResultPtr(string("Add node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCRemoveNode(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->RemoveNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to remove node.");
    }

    return MakeCRemoveNodeResultPtr(string("Remove node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCGetForkCount(CRPCParamPtr param)
{
    return MakeCGetForkCountResultPtr(pService->GetForkCount());
}

CRPCResultPtr CRPCMod::RPCListFork(CRPCParamPtr param)
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

        CListForkResult::CProfile displayProfile;
        displayProfile.strFork = hashFork.GetHex();
        displayProfile.strName = profile.strName;
        displayProfile.strSymbol = profile.strSymbol;
        displayProfile.strAmount = CoinToTokenBigFloat(profile.nAmount);
        displayProfile.strReward = CoinToTokenBigFloat(profile.nMintReward);
        displayProfile.nHalvecycle = (uint64)(profile.nHalveCycle);
        displayProfile.fIsolated = profile.IsIsolated();
        displayProfile.fPrivate = profile.IsPrivate();
        displayProfile.fEnclosed = profile.IsEnclosed();
        displayProfile.strOwner = profile.destOwner.ToString();
        displayProfile.strCreatetxid = forkContext.txidEmbedded.GetHex();
        displayProfile.nCreateforkheight = forkContext.nJointHeight;
        displayProfile.strParentfork = forkContext.hashParent.GetHex();
        displayProfile.nForkheight = lastStatus.nBlockHeight;
        displayProfile.nLastnumber = lastStatus.nBlockNumber;
        displayProfile.strLastblock = lastStatus.hashBlock.ToString();
        displayProfile.nTotaltxcount = lastStatus.nTxCount;
        displayProfile.strMoneysupply = CoinToTokenBigFloat(lastStatus.nMoneySupply);
        displayProfile.strMoneydestroy = CoinToTokenBigFloat(lastStatus.nMoneyDestroy);

        spResult->vecProfile.push_back(displayProfile);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetForkGenealogy(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetGenealogyParam>(param);

    //getgenealogy (-f="fork")
    uint256 fork;
    if (!GetForkHashOfDef(spParam->strFork, fork))
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

CRPCResultPtr CRPCMod::RPCGetBlockLocation(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockLocationParam>(param);

    //getblocklocation <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    uint256 fork;
    int height;
    if (!pService->GetBlockLocation(hashBlock, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    auto spResult = MakeCGetBlockLocationResultPtr();
    spResult->strFork = fork.GetHex();
    spResult->nHeight = height;
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockCount(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockCountParam>(param);

    //getblockcount (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetBlockCountResultPtr(pService->GetBlockCount(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetBlockHash(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockHashParam>(param);

    //getblockhash <height> (-f="fork")
    int nHeight = spParam->nHeight;

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<uint256> vBlockHash;
    if (!pService->GetBlockHash(hashFork, nHeight, vBlockHash))
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

CRPCResultPtr CRPCMod::RPCGetBlockNumberHash(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockNumberHashParam>(param);

    uint64 nNumber = spParam->nNumber;

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
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

CRPCResultPtr CRPCMod::RPCGetBlock(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockParam>(param);

    //getblock <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlock block;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    return MakeCGetBlockResultPtr(BlockToJSON(hashBlock, block, fork, height));
}

CRPCResultPtr CRPCMod::RPCGetBlockDetail(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CgetblockdetailParam>(param);

    //getblockdetail <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlockEx block;
    uint256 fork;
    int height;
    if (!pService->GetBlockEx(hashBlock, block, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    Cblockdatadetail data;

    data.strHash = hashBlock.GetHex();
    data.strHashprev = block.hashPrev.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.nType);
    data.nTime = block.GetBlockTime();
    data.nNumber = block.GetBlockNumber();
    data.strStateroot = block.hashStateRoot.GetHex();
    data.strReceiptsroot = block.hashReceiptsRoot.GetHex();
    data.strBloom = block.nBloom.GetHex();
    if (block.hashPrev != 0)
    {
        data.strPrev = block.hashPrev.GetHex();
    }
    data.strFork = fork.GetHex();
    data.nHeight = height;
    int nDepth = height < 0 ? 0 : pService->GetForkHeight(fork) - height;
    //if (fork != pCoreProtocol->GetGenesisBlockHash())
    //{
    //    nDepth = nDepth * 30;
    //}
    data.txmint = TxToJSON(block.txMint.GetHash(), block.txMint, hashBlock, nDepth, 0);
    if (block.IsProofOfWork())
    {
        CProofOfHashWork proof;
        if (!proof.Load(block.vchProof))
        {
            throw CRPCException(RPC_PARSE_ERROR, "Load proof fail");
        }
        data.nBits = proof.nBits;
    }
    else
    {
        data.nBits = 0;
    }
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        uint64 nTxGasLeft = 0;
        if (tx.to.IsContract())
        {
            CTransactionReceipt receipt;
            uint256 hashTempBlock;
            uint256 nBlockGasUsed;
            if (pService->GetTransactionReceipt(tx.hashFork, tx.GetHash(), receipt, hashTempBlock, nBlockGasUsed))
            {
                nTxGasLeft = receipt.nContractGasLeft.Get64();
            }
        }
        data.vecTx.push_back(TxToJSON(tx.GetHash(), tx, hashBlock, nDepth, nTxGasLeft));
    }
    return MakeCgetblockdetailResultPtr(data);
}

CRPCResultPtr CRPCMod::RPCGetBlockData(CRPCParamPtr param)
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
        if (!GetForkHashOfDef(spParam->strFork, hashFork))
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
                int nBlockHeight = spParam->nHeight;
                if (!pService->GetBlockHash(hashFork, nBlockHeight, hashBlock))
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "Invalid height");
                }
            }
        }
    }
    if (hashBlock == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid block");
    }

    CBlock block;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    return MakeCGetBlockDataResultPtr(BlockToJSON(hashBlock, block, fork, height));
}

CRPCResultPtr CRPCMod::RPCGetTxPool(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTxPoolParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
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

CRPCResultPtr CRPCMod::RPCGetTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionParam>(param);
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

    CTransaction tx;
    uint256 hashBlock;

    if (!pService->GetTransaction(txid, tx, hashBlock))
    {
        throw CRPCException(RPC_INVALID_REQUEST, "No information available about transaction");
    }

    uint64 nTxGasLeft = 0;
    if (tx.to.IsContract())
    {
        CTransactionReceipt receipt;
        uint256 hashTempBlock;
        uint256 nBlockGasUsed;
        if (pService->GetTransactionReceipt(tx.hashFork, txid, receipt, hashTempBlock, nBlockGasUsed))
        {
            nTxGasLeft = receipt.nContractGasLeft.Get64();
            //throw CRPCException(RPC_INVALID_PARAMETER, "Get transaction receipt fail");
        }
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
    int nDepth = nHeight < 0 ? 0 : pService->GetForkHeight(tx.hashFork) - nHeight;
    //if (tx.hashFork != pCoreProtocol->GetGenesisBlockHash())
    //{
    //    nDepth = nDepth * 30;
    //}

    spResult->transaction = TxToJSON(txid, tx, hashBlock, nDepth, nTxGasLeft);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendTransaction(CRPCParamPtr param)
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

    Errno err = pService->SendTransaction(rawTx);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ") + ErrorString(err));
    }

    return MakeCSendTransactionResultPtr(rawTx.GetHash().GetHex());
}

CRPCResultPtr CRPCMod::RPCGetForkHeight(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetForkHeightParam>(param);

    //getforkheight (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetForkHeightResultPtr(pService->GetForkHeight(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetVotes(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetVotesParam>(param);

    CDestination destDelegate(spParam->strAddress);
    if (destDelegate.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint256 nVotesToken;
    string strFailCause;
    if (!pService->GetVotes(destDelegate, nVotesToken, strFailCause))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, strFailCause);
    }

    return MakeCGetVotesResultPtr(CoinToTokenBigFloat(nVotesToken));
}

CRPCResultPtr CRPCMod::RPCListDelegate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListDelegateParam>(param);

    std::multimap<uint256, CDestination> mapVotes;
    if (!pService->ListDelegate(spParam->nCount, mapVotes))
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

/* Wallet */
CRPCResultPtr CRPCMod::RPCListKey(CRPCParamPtr param)
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
            p.strKey = pubkey.GetHex();
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

CRPCResultPtr CRPCMod::RPCGetNewKey(CRPCParamPtr param)
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

    return MakeCGetNewKeyResultPtr(pubkey.ToString());
}

CRPCResultPtr CRPCMod::RPCEncryptKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CEncryptKeyParam>(param);

    //encryptkey <"pubkey"> <-new="passphrase"> <-old="oldpassphrase">
    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);

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

    if (!pService->HaveKey(pubkey, crypto::CKey::PRIVATE_KEY))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (!pService->EncryptKey(pubkey, strPassphrase, strOldPassphrase))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCEncryptKeyResultPtr(string("Encrypt key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCLockKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CLockKeyParam>(param);

    CDestination address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't lock public key");
    }
    if (!fLocked && !pService->Lock(pubkey))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to lock key");
    }
    return MakeCLockKeyResultPtr(string("Lock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCUnlockKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CUnlockKeyParam>(param);

    CDestination address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
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
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
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

    if (!pService->Unlock(pubkey, strPassphrase, nTimeout))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCUnlockKeyResultPtr(string("Unlock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCRemoveKey(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveKeyParam>(param);
    CDestination address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }

    if (!fPublic)
    {
        pService->Lock(pubkey);
        crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
        if (!pService->Unlock(pubkey, strPassphrase, UNLOCKKEY_RELEASE_DEFAULT_TIME))
        {
            throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "Can't remove key with incorrect passphrase");
        }
    }

    auto strErr = pService->RemoveKey(pubkey);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_REMOVE_KEY_ERROR, *strErr);
    }

    return MakeCRemoveKeyResultPtr(string("Remove key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCImportPrivKey(CRPCParamPtr param)
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
    if (!pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
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

    return MakeCImportPrivKeyResultPtr(key.GetPubKey().GetHex());
}

CRPCResultPtr CRPCMod::RPCImportPubKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportPubKeyParam>(param);

    //importpubkey <"pubkey"> or importpubkey <"pubkeyaddress">
    CDestination address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Template id is not allowed");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    crypto::CKey key;
    key.Load(pubkey, crypto::CKey::PUBLIC_KEY, crypto::CCryptoCipher());
    if (!pService->HaveKey(key.GetPubKey()))
    {
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    CDestination dest(pubkey);
    return MakeCImportPubKeyResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCImportKey(CRPCParamPtr param)
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
    if ((key.IsPrivKey() && !pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
        || (key.IsPubKey() && !pService->HaveKey(key.GetPubKey())))
    {
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    return MakeCImportKeyResultPtr(key.GetPubKey().GetHex());
}

CRPCResultPtr CRPCMod::RPCExportKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportKeyParam>(param);

    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);

    if (!pService->HaveKey(pubkey))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    vector<unsigned char> vchKey;
    if (!pService->ExportKey(pubkey, vchKey))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
    }

    return MakeCExportKeyResultPtr(ToHexString(vchKey));
}

CRPCResultPtr CRPCMod::RPCAddNewTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddNewTemplateParam>(param);
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(spParam->data);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }
    if (!pService->HaveTemplate(ptr->GetTemplateId()))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCAddNewTemplateResultPtr(CDestination(ptr->GetTemplateId()).ToString());
}

CRPCResultPtr CRPCMod::RPCImportTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportTemplateParam>(param);
    vector<unsigned char> vchTemplate = ParseHexString(spParam->strData);
    CTemplatePtr ptr = CTemplate::Import(vchTemplate);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }
    if (!pService->HaveTemplate(ptr->GetTemplateId()))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCImportTemplateResultPtr(CDestination(ptr->GetTemplateId()).ToString());
}

CRPCResultPtr CRPCMod::RPCExportTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportTemplateParam>(param);
    CDestination address(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    CTemplateId tid = address.GetTemplateId();
    if (!tid)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    CTemplatePtr ptr = pService->GetTemplate(tid);
    if (!ptr)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
    }

    vector<unsigned char> vchTemplate = ptr->Export();
    return MakeCExportTemplateResultPtr(ToHexString(vchTemplate));
}

CRPCResultPtr CRPCMod::RPCRemoveTemplate(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveTemplateParam>(param);
    CDestination address(spParam->strAddress);
    if (address.IsNull() || !address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid template address");
    }

    if (!pService->RemoveTemplate(address.GetTemplateId()))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Remove template address fail");
    }

    return MakeCRemoveTemplateResultPtr("Success");
}

CRPCResultPtr CRPCMod::RPCValidateAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CValidateAddressParam>(param);

    CDestination address(spParam->strAddress);
    bool isValid = !address.IsNull();

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    auto spResult = MakeCValidateAddressResultPtr();
    spResult->fIsvalid = isValid;
    if (isValid)
    {
        auto& addressData = spResult->addressdata;

        addressData.strAddress = address.ToString();
        if (address.IsPubKey())
        {
            crypto::CPubKey pubkey;
            address.GetPubKey(pubkey);
            bool isMine = pService->HaveKey(pubkey);
            addressData.fIsmine = isMine;
            addressData.strType = "pubkey";
            addressData.strPubkey = pubkey.GetHex();
        }
        else if (address.IsTemplate())
        {
            CTemplateId tid = address.GetTemplateId();
            uint16 nType = tid.GetType();
            CTemplatePtr ptr = pService->GetTemplate(tid);
            if (ptr == nullptr)
            {
                bytes btTemplateData;
                if (pService->GetDestTemplateData(hashFork, address, btTemplateData))
                {
                    ptr = CTemplate::CreateTemplatePtr(address.GetTemplateId().GetType(), btTemplateData);
                    if (ptr == nullptr)
                    {
                        StdLog("CRPCMod", "RPC Validate Address: Create template fail");
                    }
                }
            }
            addressData.fIsmine = false;
            addressData.strType = "template";
            addressData.strTemplate = CTemplate::GetTypeName(nType);
            if (ptr)
            {
                addressData.fIsmine = true;

                auto& templateData = addressData.templatedata;
                templateData.strHex = ToHexString(ptr->Export());
                templateData.strType = ptr->GetName();
                ptr->GetTemplateData(templateData);
            }
        }
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBalance(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBalanceParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<CDestination> vDest;
    if (spParam->strAddress.IsValid())
    {
        CDestination address(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
        vDest.push_back(static_cast<CDestination&>(address));
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
        ListDestination(vDest, nPage, nCount);
    }

    int nLastHeight;
    uint256 hashLastBlock;
    if (!pService->GetForkLastBlock(hashFork, nLastHeight, hashLastBlock))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Get last block error");
    }

    auto spResult = MakeCGetBalanceResultPtr();
    for (const CDestination& dest : vDest)
    {
        CWalletBalance balance;
        if (!pService->GetBalance(hashFork, hashLastBlock, dest, balance))
        {
            balance.SetNull();
        }

        CGetBalanceResult::CBalance b;
        b.strAddress = dest.ToString();
        b.nNonce = balance.nTxNonce;
        b.strAvail = CoinToTokenBigFloat(balance.nAvailable);
        b.strLocked = CoinToTokenBigFloat(balance.nLocked);
        b.strUnconfirmedin = CoinToTokenBigFloat(balance.nUnconfirmedIn);
        b.strUnconfirmedout = CoinToTokenBigFloat(balance.nUnconfirmedOut);
        spResult->vecBalance.push_back(b);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCListTransaction(CRPCParamPtr param)
{
    if (!BasicConfig()->fFullDb)
    {
        throw CRPCException(RPC_INVALID_REQUEST, "If you need this function, please set config 'fulldb=true' and restart");
    }

    auto spParam = CastParamPtr<CListTransactionParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CDestination dest(spParam->strAddress);
    if (dest.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    vector<CDestTxInfo> vTx;
    if (!pService->ListTransaction(hashFork, dest, spParam->nOffset, spParam->nCount, spParam->fReverse, vTx))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to list transactions");
    }

    auto spResult = MakeCListTransactionResultPtr();
    for (const CDestTxInfo& tx : vTx)
    {
        spResult->vecTransaction.push_back(TxInfoToJSON(dest, tx));
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendFrom(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSendFromParam>(param);
    CDestination from(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to;
    if (spParam->strTo.IsValid() && !spParam->strTo.empty() && spParam->strTo.c_str()[0] == '0')
    {
        to.SetContractId(uint256());
    }
    else
    {
        if (!to.ParseString(spParam->strTo))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
    }
    if (to.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint64 nTxType = CTransaction::TX_TOKEN;
    if (spParam->nTxtype.IsValid())
    {
        nTxType = spParam->nTxtype;
        if (!CTransaction::IsUserTx(nTxType))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txtype");
        }
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
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
            auto hex = hnbase::ToHexString((const unsigned char*)strDataTmp.c_str(), strlen(strDataTmp.c_str()));
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
    if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
    }
    uint256 nGas = uint256(spParam->nGas);

    vector<uint8> vchToData;
    if (to.IsTemplate() && spParam->strTodata.IsValid())
    {
        vchToData = ParseHexString(spParam->strTodata);
    }

    bytes btContractCode;
    bytes btContractParam;
    if (to.IsContract())
    {
        if (spParam->strContractcode.IsValid())
        {
            if (spParam->strContractcode.size() == CDestination::ADDRESS_LEN)
            {
                CDestination dest(spParam->strContractcode);
                if (dest.IsContract())
                {
                    hnbase::CBufStream ss;
                    ss << dest;
                    ss.GetData(btContractCode);
                }
            }
            if (btContractCode.empty())
            {
                btContractCode = ParseHexString(spParam->strContractcode);
            }
        }
        if (spParam->strContractparam.IsValid())
        {
            btContractParam = ParseHexString(spParam->strContractparam);
        }
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransaction(hashFork, from, to, vchToData, nTxType, nAmount, nNonce, nGasPrice, nGas, vchData, btFormatData, btContractCode, btContractParam, txNew);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr);
    }

    if (!pService->SignTransaction(txNew))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }

    Errno err = pService->SendTransaction(txNew);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ") + ErrorString(err));
    }

    uint256 txid = txNew.GetHash();

    StdDebug("[SendFrom][DEBUG]", "Sendfrom success, txid: %s, to: %s", txid.GetHex().c_str(), to.ToString().c_str());

    return MakeCSendFromResultPtr(txid.GetHex());
}

CRPCResultPtr CRPCMod::RPCCreateTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCreateTransactionParam>(param);

    CDestination from(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to;
    if (spParam->strTo.IsValid() && !spParam->strTo.empty() && spParam->strTo.c_str()[0] == '0')
    {
        to.SetContractId(uint256());
    }
    else
    {
        if (!to.ParseString(spParam->strTo))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
        }
    }
    if (to.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint64 nTxType = CTransaction::TX_TOKEN;
    if (spParam->nTxtype.IsValid())
    {
        nTxType = spParam->nTxtype;
        if (!CTransaction::IsUserTx(nTxType))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txtype");
        }
    }

    uint256 nAmount;
    if (!TokenBigFloatToCoin(spParam->strAmount, nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
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
            auto hex = hnbase::ToHexString((const unsigned char*)strDataTmp.c_str(), strlen(strDataTmp.c_str()));
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
    if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
    }
    uint256 nGas = uint256(spParam->nGas);

    vector<uint8> vchToData;
    if (to.IsTemplate() && spParam->strTodata.IsValid())
    {
        vchToData = ParseHexString(spParam->strTodata);
    }

    bytes btContractCode;
    bytes btContractParam;
    if (to.IsContract())
    {
        if (spParam->strContractcode.IsValid())
        {
            if (spParam->strContractcode.size() == CDestination::ADDRESS_LEN)
            {
                CDestination dest(spParam->strContractcode);
                if (dest.IsContract())
                {
                    hnbase::CBufStream ss;
                    ss << dest;
                    ss.GetData(btContractCode);
                }
            }
            if (btContractCode.empty())
            {
                btContractCode = ParseHexString(spParam->strContractcode);
            }
        }
        if (spParam->strContractparam.IsValid())
        {
            btContractParam = ParseHexString(spParam->strContractparam);
        }
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransaction(hashFork, from, to, vchToData, nTxType, nAmount, nNonce, nGasPrice, nGas, vchData, btFormatData, btContractCode, btContractParam, txNew);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr);
    }

    CBufStream ss;
    ss << txNew;

    return MakeCCreateTransactionResultPtr(
        ToHexString((const unsigned char*)ss.GetData(), ss.GetSize()));
}

CRPCResultPtr CRPCMod::RPCSignTransaction(CRPCParamPtr param)
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

    if (!pService->SignTransaction(rawTx))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }

    CBufStream ssNew;
    ssNew << rawTx;

    return MakeCSignTransactionResultPtr(
        ToHexString((const unsigned char*)ssNew.GetData(), ssNew.GetSize()));
}

CRPCResultPtr CRPCMod::RPCSignMessage(CRPCParamPtr param)
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
        if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
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
        CDestination addr(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() || addr.IsPubKey())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        if (!!pubkey)
        {
            if (!pService->SignSignature(pubkey, addr.GetTemplateId(), vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
        else
        {
            if (!privkey.Sign(addr.GetTemplateId(), vchSig))
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
            if (!pService->SignSignature(pubkey, hashStr, vchSig))
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

CRPCResultPtr CRPCMod::RPCListAddress(CRPCParamPtr param)
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
    vector<CDestination> vDes;
    ListDestination(vDes, nPage, nCount);
    for (const auto& des : vDes)
    {
        CListAddressResult::CAddressdata addressData;
        addressData.strAddress = des.ToString();
        if (des.IsPubKey())
        {
            addressData.strType = "pubkey";
            crypto::CPubKey pubkey;
            des.GetPubKey(pubkey);
            addressData.strPubkey = pubkey.GetHex();
        }
        else if (des.IsTemplate())
        {
            addressData.strType = "template";

            CTemplateId tid = des.GetTemplateId();
            uint16 nType = tid.GetType();
            CTemplatePtr ptr = pService->GetTemplate(tid);
            addressData.strTemplate = CTemplate::GetTypeName(nType);

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

CRPCResultPtr CRPCMod::RPCExportWallet(CRPCParamPtr param)
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
    vector<CDestination> vDes;
    ListDestination(vDes, 0, 0);
    for (const auto& des : vDes)
    {
        if (des.IsPubKey())
        {
            Object oKey;
            oKey.push_back(Pair("address", des.ToString()));

            crypto::CPubKey pubkey;
            des.GetPubKey(pubkey);
            vector<unsigned char> vchKey;
            if (!pService->ExportKey(pubkey, vchKey))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
            }
            oKey.push_back(Pair("hex", ToHexString(vchKey)));
            aAddr.push_back(oKey);
        }

        if (des.IsTemplate())
        {
            Object oTemp;
            CDestination address(des);

            oTemp.push_back(Pair("address", address.ToString()));

            CTemplateId tid;
            if (!address.GetTemplateId(tid))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid template address");
            }
            CTemplatePtr ptr = pService->GetTemplate(tid);
            if (!ptr)
            {
                throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
            }
            vector<unsigned char> vchTemplate = ptr->Export();

            oTemp.push_back(Pair("hex", ToHexString(vchTemplate)));

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

CRPCResultPtr CRPCMod::RPCImportWallet(CRPCParamPtr param)
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
        if (oAddr.get_obj()[0].name_ != "address" || oAddr.get_obj()[1].name_ != "hex")
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }
        string sAddr = oAddr.get_obj()[0].value_.get_str(); //"address" field
        string sHex = oAddr.get_obj()[1].value_.get_str();  //"hex" field

        CDestination addr(sAddr);
        if (addr.IsNull())
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }

        //import keys
        if (addr.IsPubKey())
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
            if ((key.IsPrivKey() && pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
                || (key.IsPubKey() && pService->HaveKey(key.GetPubKey())))
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
        if (addr.IsTemplate())
        {
            vector<unsigned char> vchTemplate = ParseHexString(sHex);
            CTemplatePtr ptr = CTemplate::Import(vchTemplate);
            if (ptr == nullptr)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
            }
            if (pService->HaveTemplate(addr.GetTemplateId()))
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

CRPCResultPtr CRPCMod::RPCMakeOrigin(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeOriginParam>(param);

    uint256 hashPrev;
    hashPrev.SetHex(spParam->strPrev);

    CDestination destOwner(spParam->strOwner);
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
    //if (!RewardRange(nMintReward))
    //{
    //    throw CRPCException(RPC_INVALID_PARAMETER, "Invalid reward");
    //}

    if (spParam->strName.empty() || spParam->strName.size() > 128
        || spParam->strSymbol.empty() || spParam->strSymbol.size() > 16)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid name or symbol");
    }
    uint64 nForkNonce = spParam->nNonce;

    CBlock blockPrev;
    uint256 hashParent;
    int nJointHeight;
    if (!pService->GetBlock(hashPrev, blockPrev, hashParent, nJointHeight))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown prev block");
    }

    if (blockPrev.IsExtended() || blockPrev.IsVacant())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Prev block should not be extended/vacant block");
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
    int64 nTimeRef;
    if (!pService->GetLastBlockOfHeight(pCoreProtocol->GetGenesisBlockHash(), nJointHeight + 1, hashBlockRef, nTimeRef))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Failed to query main chain reference block");
    }

    CProfile profile;
    profile.strName = spParam->strName;
    profile.strSymbol = spParam->strSymbol;
    profile.destOwner = destOwner;
    profile.hashParent = hashParent;
    profile.nJointHeight = nJointHeight;
    profile.nAmount = nAmount;
    profile.nMintReward = nMintReward;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = spParam->nHalvecycle;
    profile.SetFlag(spParam->fIsolated, spParam->fPrivate, spParam->fEnclosed);

    CBlock block;
    block.nVersion = 1;
    block.nType = CBlock::BLOCK_ORIGIN;
    block.nTimeStamp = nTimeRef;
    block.hashPrev = hashPrev;
    profile.Save(block.vchProof);

    if (spParam->fIsolated)
    {
        std::map<CDestination, CDestState> mapBlockState;
        mapBlockState[destOwner] = CDestState(nAmount);
        if (!storage::CStateDB::CreateStaticStateRoot(mapBlockState, block.hashStateRoot))
        {
            throw CRPCException(RPC_INTERNAL_ERROR, "Create state trie root fail");
        }
    }
    else
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "not support non isolated fork");
    }

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.to = destOwner;
    tx.nAmount = nAmount;
    //tx.vchData.assign(profile.strName.begin(), profile.strName.end());

    {
        hnbase::CBufStream ss;
        ss << tx.nAmount;
        bytes btTempData;
        ss.GetData(btTempData);
        tx.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
    }

    {
        hnbase::CBufStream sm;
        sm << profile.strName;
        bytes btTempData;
        sm.GetData(btTempData);
        tx.AddTxData(CTransaction::DF_FORKNAME, btTempData);
    }

    {
        hnbase::CBufStream sm;
        sm << nForkNonce;
        bytes btTempData;
        sm.GetData(btTempData);
        tx.AddTxData(CTransaction::DF_FORKNONCE, btTempData);
    }

    crypto::CPubKey pubkey;
    if (!destOwner.GetPubKey(pubkey))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Owner' address should be pubkey address");
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
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

    if (!pService->VerifyForkName(hashBlock, profile.strName))
    {
        throw CRPCException(RPC_VERIFY_ALREADY_IN_CHAIN, "Fork name is existed");
    }

    if (!pService->SignSignature(pubkey, hashBlock, block.vchSig))
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

CRPCResultPtr CRPCMod::RPCCallContract(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCallContractParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CDestination from(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CDestination to(spParam->strTo);
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
    if (!TokenBigFloatToCoin(spParam->strGasprice, nGasPrice))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid gasprice");
    }
    if (nGasPrice == 0)
    {
        nGasPrice = DEF_TX_GAS_PRICE;
    }
    uint256 nGas = uint256(spParam->nGas);
    if (nGas == 0)
    {
        nGas = DEF_TX_GAS_LIMIT;
    }

    int nStatus = 0;
    bytes btResult;
    if (!pService->CallContract(hashFork, from, to, nAmount, nGasPrice, nGas, btContractParam, nStatus, btResult))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Call fail");
    }

    auto spResult = MakeCCallContractResultPtr();
    spResult->nStatus = nStatus;
    spResult->strResult = ToHexString(btResult);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetTransactionReceipt(rpc::CRPCParamPtr param)
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
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CTransactionReceipt receipt;
    uint256 hashBlock;
    uint256 nBlockGasUsed;
    if (!pService->GetTransactionReceipt(hashFork, txid, receipt, hashBlock, nBlockGasUsed))
    {
        throw CRPCException(RPC_DATABASE_ERROR, "Get transaction receipt fail");
    }

    auto spResult = MakeCGetTransactionReceiptResultPtr();

    spResult->strTxid = txid.GetHex();
    spResult->nTxindex = receipt.nTxIndex;
    spResult->strBlockhash = hashBlock.GetHex();
    spResult->nBlocknumber = receipt.nBlockNumber;
    spResult->strFrom = receipt.from.ToString();
    spResult->strTo = receipt.to.ToString();
    spResult->nBlockgasused = nBlockGasUsed.Get64();
    spResult->nTxgasused = receipt.nTxGasUsed.Get64();
    spResult->strContractaddress = receipt.destContract.ToString();
    spResult->strContractcodehash = receipt.hashContractCode.GetHex();
    spResult->nContractstatus = receipt.nContractStatus;
    spResult->nContractgasleft = receipt.nContractGasLeft.Get64();
    spResult->strContractresult = ToHexString(receipt.btContractResult);

    spResult->logs.strAddress = ToHexString(receipt.logs.address);
    spResult->logs.strData = ToHexString(receipt.logs.data);
    for (const auto& vd : receipt.logs.topics)
    {
        spResult->logs.vecTopics.push_back(ToHexString(vd));
    }
    spResult->strLogsbloom = receipt.nLogsBloom.GetHex();

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetContractMuxCode(rpc::CRPCParamPtr param)
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

    CDestination destOwner(spParam->strOwner);
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

    hnbase::CBufStream ss;
    ss << txcd;
    bytes btData;
    ss.GetData(btData);

    return MakeCGetContractMuxCodeResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCListContractCode(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListContractCodeParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

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
        if (!pService->ListContractCodeContext(hashFork, txid, mapContractCode))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
        }
    }
    else
    {
        CContractCodeContext ctxt;
        if (!pService->GetContractCodeContext(hashFork, hashCode, ctxt))
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
        }
        mapContractCode.insert(make_pair(hashCode, ctxt));
    }

    auto spResult = MakeCListContractCodeResultPtr();

    for (const auto& kv : mapContractCode)
    {
        CListContractCodeResult::CCodedata codeData;

        codeData.strCodehash = kv.second.hashCode.ToString();
        codeData.strSourcehash = kv.second.hashSource.ToString();
        codeData.strType = kv.second.strType;
        codeData.strOwner = kv.second.destOwner.ToString();
        codeData.strName = kv.second.strName;
        codeData.strDescribe = kv.second.strDescribe;
        codeData.strTxid = kv.second.hashCreateTxid.GetHex();
        codeData.nStatus = kv.second.nStatus;

        spResult->vecCodedata.push_back(codeData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCListContractAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListContractAddressParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    std::map<uint256, CContractAddressContext> mapContractAddress;
    if (!pService->ListContractAddress(hashFork, mapContractAddress))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    auto spResult = MakeCListContractAddressResultPtr();

    for (const auto& kv : mapContractAddress)
    {
        CListContractAddressResult::CAddressdata addressData;

        CDestination address(CContractId(kv.first));
        addressData.strAddress = address.ToString();
        addressData.strType = kv.second.strType;
        addressData.strOwner = kv.second.destCodeOwner.ToString();
        addressData.strName = kv.second.strName;
        addressData.strDescribe = kv.second.strDescribe;
        addressData.strTxid = kv.second.hashCreateTxid.GetHex();
        addressData.strSourcehash = kv.second.hashSourceCode.GetHex();
        addressData.strCodehash = kv.second.hashWasmCreateCode.GetHex();

        spResult->vecAddressdata.push_back(addressData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetDestContract(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetDestContractParam>(param);

    CDestination dest(spParam->strAddress);
    if (dest.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CContractAddressContext ctxtContract;
    if (!pService->GeDestContractContext(hashFork, dest, ctxtContract))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Address error");
    }

    auto spResult = MakeCGetDestContractResultPtr();

    spResult->strType = ctxtContract.strType;
    spResult->strName = ctxtContract.strName;
    spResult->strDescribe = ctxtContract.strDescribe;
    spResult->strTxid = ctxtContract.hashCreateTxid.GetHex();
    spResult->strSourcehash = ctxtContract.hashSourceCode.GetHex();
    spResult->strCodehash = ctxtContract.hashWasmCreateCode.GetHex();

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetContractSource(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractSourceParam>(param);

    uint256 hashSource(spParam->strSourcehash);
    if (hashSource == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid source");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    bytes btData;
    if (!pService->GetContractSource(hashFork, hashSource, btData))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid source");
    }

    return MakeCGetContractSourceResultPtr(ToHexString(btData));
}

CRPCResultPtr CRPCMod::RPCGetContractCode(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractCodeParam>(param);

    uint256 hashCode(spParam->strCodehash);
    if (hashCode == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    bytes btData;
    if (!pService->GetContractCode(hashFork, hashCode, btData))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid code");
    }

    return MakeCGetContractCodeResultPtr(ToHexString(btData));
}

/* Util */
CRPCResultPtr CRPCMod::RPCVerifyMessage(CRPCParamPtr param)
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
        CDestination addr(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() || addr.IsPubKey())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        return MakeCVerifyMessageResultPtr(
            pubkey.Verify(addr.GetTemplateId(), vchSig));
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

CRPCResultPtr CRPCMod::RPCMakeKeyPair(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeKeyPairParam>(param);

    crypto::CCryptoKey key;
    crypto::CryptoMakeNewKey(key);

    auto spResult = MakeCMakeKeyPairResultPtr();
    spResult->strPrivkey = key.secret.GetHex();
    spResult->strPubkey = key.pubkey.GetHex();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetPubKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetPubkeyParam>(param);
    crypto::CPubKey pubkey;
    {
        CDestination address(spParam->strPrivkeyaddress);
        if (!address.IsNull())
        {
            if (!address.GetPubKey(pubkey))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey address");
            }
            return MakeCGetPubkeyResultPtr(pubkey.ToString());
        }
    }
    {
        uint256 nPriv;
        if (nPriv.SetHex(spParam->strPrivkeyaddress) == spParam->strPrivkeyaddress.size())
        {
            crypto::CKey key;
            if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Get pubkey by privkey error");
            }
            return MakeCGetPubkeyResultPtr(key.GetPubKey().ToString());
        }
    }

    throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address or privkey");
}

CRPCResultPtr CRPCMod::RPCGetPubKeyAddress(CRPCParamPtr param)
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

CRPCResultPtr CRPCMod::RPCGetAddressKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetAddressKeyParam>(param);
    CDestination address(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }
    return MakeCGetAddressKeyResultPtr(address.data.GetHex());
}

CRPCResultPtr CRPCMod::RPCGetTemplateAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTemplateAddressParam>(param);
    CTemplateId tid;
    if (tid.SetHex(spParam->strTid) != spParam->strTid.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid tid");
    }

    CDestination dest(tid);

    return MakeCGetTemplateAddressResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCGetContractAddress(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetContractAddressParam>(param);
    CContractId cid;
    if (cid.SetHex(spParam->strCid) != spParam->strCid.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid cid");
    }

    CDestination dest(cid);

    return MakeCGetContractAddressResultPtr(dest.ToString());
}

CRPCResultPtr CRPCMod::RPCMakeTemplate(CRPCParamPtr param)
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

CRPCResultPtr CRPCMod::RPCDecodeTransaction(CRPCParamPtr param)
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

    //uint256 hashFork = rawTx.hashFork;
    /*int nHeight;
    if (!pService->GetBlockLocation(rawTx.hashFork, hashFork, nHeight))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown anchor block");
    }*/

    return MakeCDecodeTransactionResultPtr(TxToJSON(rawTx.GetHash(), rawTx, uint256(), -1, 0));
}

CRPCResultPtr CRPCMod::RPCGetTxFee(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionFeeParam>(param);

    bytes vchData = ParseHexString(spParam->strHexdata);

    CTransaction tx;
    tx.AddTxData(CTransaction::DF_COMMON, vchData);

    uint256 nNeedGas = TX_BASE_GAS + tx.GetTxDataGas();
    uint256 nFee = nNeedGas * DEF_TX_GAS_PRICE;

    auto spResult = MakeCGetTransactionFeeResultPtr();
    spResult->strTxfee = CoinToTokenBigFloat(nFee);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCMakeSha256(rpc::CRPCParamPtr param)
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

CRPCResultPtr CRPCMod::RPCAesEncrypt(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAesEncryptParam>(param);

    CDestination addressLocal(spParam->strLocaladdress);
    if (addressLocal.IsNull() || !addressLocal.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid local address");
    }

    CDestination addressRemote(spParam->strRemoteaddress);
    if (addressRemote.IsNull() || !addressRemote.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid remote address");
    }

    crypto::CPubKey pubkeyLocal;
    addressLocal.GetPubKey(pubkeyLocal);

    crypto::CPubKey pubkeyRemote;
    addressRemote.GetPubKey(pubkeyRemote);

    vector<uint8> vMessage = ParseHexString(spParam->strMessage);
    if (vMessage.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid message");
    }

    vector<uint8> vCiphertext;
    if (!pService->AesEncrypt(pubkeyLocal, pubkeyRemote, vMessage, vCiphertext))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Encrypt fail");
    }

    auto spResult = MakeCAesEncryptResultPtr();
    spResult->strResult = ToHexString(vCiphertext);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCAesDecrypt(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAesDecryptParam>(param);

    CDestination addressLocal(spParam->strLocaladdress);
    if (addressLocal.IsNull() || !addressLocal.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid local address");
    }

    CDestination addressRemote(spParam->strRemoteaddress);
    if (addressRemote.IsNull() || !addressRemote.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid remote address");
    }

    crypto::CPubKey pubkeyLocal;
    addressLocal.GetPubKey(pubkeyLocal);

    crypto::CPubKey pubkeyRemote;
    addressRemote.GetPubKey(pubkeyRemote);

    vector<uint8> vCiphertext = ParseHexString(spParam->strCiphertext);
    if (vCiphertext.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid ciphertext");
    }

    vector<uint8> vMessage;
    if (!pService->AesDecrypt(pubkeyLocal, pubkeyRemote, vCiphertext, vMessage))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Decrypt fail");
    }

    auto spResult = MakeCAesDecryptResultPtr();
    spResult->strResult = ToHexString(vMessage);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCReverseHex(rpc::CRPCParamPtr param)
{
    // reversehex <"hex">
    auto spParam = CastParamPtr<CReverseHexParam>(param);

    string strHex = spParam->strHex;
    if (strHex.empty() || (strHex.size() % 2 != 0))
    {
        throw CRPCException(RPC_INVALID_PARAMS, "hex string size is not even");
    }

    regex r(R"([^0-9a-fA-F]+)");
    smatch sm;
    if (regex_search(strHex, sm, r))
    {
        throw CRPCException(RPC_INVALID_PARAMS, string("invalid hex string: ") + sm.str());
    }

    for (auto itBegin = strHex.begin(), itEnd = strHex.end() - 2; itBegin < itEnd; itBegin += 2, itEnd -= 2)
    {
        swap_ranges(itBegin, itBegin + 2, itEnd);
    }

    return MakeCReverseHexResultPtr(strHex);
}

CRPCResultPtr CRPCMod::RPCFuncSign(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CFuncSignParam>(param);

    std::vector<uint8> ret = crypto::CryptoKeccak(spParam->strFuncname.c_str(), spParam->strFuncname.size());

    auto spResult = MakeCFuncSignResultPtr();
    spResult->strSign = hnbase::ToHexString(std::vector<uint8_t>(ret.end() - 4, ret.end()));

    return spResult;
}

CRPCResultPtr CRPCMod::RPCMakeHash(rpc::CRPCParamPtr param)
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

// /* Mint */
CRPCResultPtr CRPCMod::RPCGetWork(CRPCParamPtr param)
{
    //getwork <"spent"> <"privkey"> ("prev")
    auto spParam = CastParamPtr<CGetWorkParam>(param);

    CDestination addrSpent(spParam->strSpent);
    uint256 nPriv(spParam->strPrivkey);
    if (addrSpent.IsNull() || !addrSpent.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address");
    }
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }
    crypto::CPubKey pubkeySpent;
    if (addrSpent.GetPubKey(pubkeySpent) && pubkeySpent == key.GetPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address or private key");
    }
    CTemplateMintPtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(key.GetPubKey(), static_cast<CDestination&>(addrSpent)));
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid mint template");
    }

    auto spResult = MakeCGetWorkResultPtr();

    vector<unsigned char> vchWorkData;
    int nPrevBlockHeight;
    uint256 hashPrev;
    uint32 nPrevTime;
    int nAlgo, nBits;
    if (!pService->GetWork(vchWorkData, nPrevBlockHeight, hashPrev, nPrevTime, nAlgo, nBits, ptr))
    {
        spResult->fResult = false;
        return spResult;
    }

    spResult->fResult = true;

    spResult->work.nPrevblockheight = nPrevBlockHeight;
    spResult->work.strPrevblockhash = hashPrev.GetHex();
    spResult->work.nPrevblocktime = nPrevTime;
    spResult->work.nAlgo = nAlgo;
    spResult->work.nBits = nBits;
    spResult->work.strData = ToHexString(vchWorkData);

    return spResult;
}

CRPCResultPtr CRPCMod::RPCSubmitWork(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSubmitWorkParam>(param);
    vector<unsigned char> vchWorkData(ParseHexString(spParam->strData));
    CDestination addrSpent(spParam->strSpent);
    uint256 nPriv(spParam->strPrivkey);
    if (addrSpent.IsNull() || !addrSpent.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address");
    }
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }

    CTemplateMintPtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(key.GetPubKey(), static_cast<CDestination&>(addrSpent)));
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid mint template");
    }
    uint256 hashBlock;
    Errno err = pService->SubmitWork(vchWorkData, ptr, key, hashBlock);
    if (err != OK)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("Block rejected : ") + ErrorString(err));
    }

    return MakeCSubmitWorkResultPtr(hashBlock.GetHex());
}

CRPCResultPtr CRPCMod::RPCQueryStat(rpc::CRPCParamPtr param)
{
    enum
    {
        TYPE_NON,
        TYPE_MAKER,
        TYPE_P2PSYN
    } eType
        = TYPE_NON;
    uint32 nDefQueryCount = 20;
    uint256 hashFork;
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
    if (!spParam->strFork.empty())
    {
        if (hashFork.SetHex(spParam->strFork) != spParam->strFork.size())
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

} // namespace metabasenet

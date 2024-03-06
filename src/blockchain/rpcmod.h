// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_RPCMOD_H
#define METABASENET_RPCMOD_H

#include "json/json_spirit.h"
#include <boost/function.hpp>

#include "base.h"
#include "mtbase.h"
#include "rpc/rpc.h"

namespace metabasenet
{

class CReqContext
{
public:
    CReqContext() {}
    CReqContext(const uint64 nNonceIn, const uint256& hashForkIn, const uint8 nReqSourceTypeIn, const CChainId nReqChainIdIn, const uint16 nListenRpcPortIn, const std::string& strPeerIpIn, const uint16 nPeerPortIn)
      : nNonce(nNonceIn), hashFork(hashForkIn), nReqSourceType(nReqSourceTypeIn), nReqChainId(nReqChainIdIn), nListenRpcPort(nListenRpcPortIn), strPeerIp(strPeerIpIn), nPeerPort(nPeerPortIn) {}

public:
    uint64 nNonce;
    uint256 hashFork;
    uint8 nReqSourceType;
    CChainId nReqChainId;
    uint16 nListenRpcPort;
    std::string strPeerIp;
    uint16 nPeerPort;

    std::string strMethod;
};

class CRPCMod : public mtbase::IIOModule, virtual public mtbase::CHttpEventListener
{
public:
    typedef rpc::CRPCResultPtr (CRPCMod::*RPCFunc)(const CReqContext& ctxReq, rpc::CRPCParamPtr param);

    CRPCMod();
    ~CRPCMod();

    bool HandleEvent(mtbase::CEventHttpReq& eventHttpReq) override;
    bool HandleEvent(mtbase::CEventHttpBroken& eventHttpBroken) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    const CBasicConfig* BasicConfig()
    {
        return dynamic_cast<const CBasicConfig*>(mtbase::IBase::Config());
    }
    const CNetworkConfig* Config()
    {
        return dynamic_cast<const CNetworkConfig*>(mtbase::IBase::Config());
    }
    const CRPCServerConfig* RPCServerConfig()
    {
        return dynamic_cast<const CRPCServerConfig*>(IBase::Config());
    }

    void JsonReply(const uint8 nReqSourceType, const CChainId nChainId, const uint64 nNonce, const std::string& result);

    int GetInt(const rpc::CRPCInt64& i, int valDefault);
    unsigned int GetUint(const rpc::CRPCUint64& i, unsigned int valDefault);
    uint64 GetUint64(const rpc::CRPCUint64& i, uint64 valDefault);
    bool GetForkHashOfDef(const rpc::CRPCString& hex, const uint256& hashDefFork, uint256& hashFork);
    void ListDestination(std::vector<tuple<CDestination, uint8, uint512>>& vDestination, const uint64 nPage = 0, const uint64 nCount = 0);
    bool CheckVersion(std::string& strVersion);
    std::string GetWidthString(const std::string& strIn, int nWidth);
    std::string GetWidthString(uint64 nCount, int nWidth);
    uint256 GetRefBlock(const uint256& hashFork, const string& strRefBlock, const bool fDefZero = false);
    bool VerifyClientOrder(const CReqContext& ctxReq);
    CLogsFilter GetLogFilterFromJson(const uint256& hashFork, const std::string& strJsonValue);
    bool GetVmExecResultInfo(const int nStatus, const bytes& btResult, string& strError);

private:
    /* System */
    rpc::CRPCResultPtr RPCHelp(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCStop(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCVersion(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* Network */
    rpc::CRPCResultPtr RPCGetPeerCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListPeer(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddNode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveNode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetForkPort(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* Worldline & TxPool */
    rpc::CRPCResultPtr RPCGetForkCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListFork(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetForkGenealogy(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockLocation(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockNumberHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlock(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockDetail(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockData(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTxPool(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetForkHeight(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetVotes(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListDelegate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetDelegateVotes(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetUserVotes(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTimeVault(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetAddressCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* Wallet */
    rpc::CRPCResultPtr RPCListKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetNewKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEncryptKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCLockKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCUnlockKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportPrivKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportPubKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddNewTemplate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportTemplate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportTemplate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveTemplate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCValidateAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBalance(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendFrom(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCCreateTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSignTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSignMessage(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddUserCoin(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddContractCoin(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetCoinInfo(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListCoinInfo(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetDexCoinPair(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListDexCoinPair(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendDexOrderTx(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListDexOrder(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetDexSymbolType(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListRealtimeDexOrder(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendCrossTransferTx(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetCrossTransferAmount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportWallet(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportWallet(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeOrigin(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCCallContract(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTransactionReceipt(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractMuxCode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTemplateMuxCode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListContractCode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListContractAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetDestContract(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractSource(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractCode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddBlacklistAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveBlacklistAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListBlacklistAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSetFunctionAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListFunctionAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* Util */
    rpc::CRPCResultPtr RPCVerifyMessage(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeKeyPair(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetPubKey(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetPubKeyAddress(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeTemplate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCDecodeTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTxFee(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeSha256(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCReverseHex(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCFuncSign(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSetMintMinGasPrice(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetMintMinGasPrice(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListMintMinGasPrice(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCCheckAtBloomFilter(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* tool */
    rpc::CRPCResultPtr RPCQueryStat(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    /* eth rpc */
    rpc::CRPCResultPtr RPCEthGetWebClientVersion(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetSha3(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetNetVersion(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetNetListening(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetPeerCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetChainId(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetProtocolVersion(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetSyncing(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetCoinbase(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetMining(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetHashRate(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetGasPrice(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetAccounts(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBalance(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBlockNumber(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetStorageAt(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetStorageRoot(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetTransactionCount(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetPendingTransactions(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBlockTransactionCountByHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBlockTransactionCountByNumber(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetUncleCountByBlockHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetUncleCountByBlockNumber(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetCode(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthSign(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthSignTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthSendTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthSendRawTransaction(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthCall(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthEstimateGas(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBlockByHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetBlockByNumber(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetTransactionByHash(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetTransactionByBlockHashAndIndex(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetTransactionByBlockNumberAndIndex(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetTransactionReceipt(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetUncleByBlockHashAndIndex(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetUncleByBlockNumberAndIndex(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthNewFilter(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthNewBlockFilter(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthNewPendingTransactionFilter(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthUninstallFilter(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetFilterChanges(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetFilterLogs(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthGetLogs(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthSubscribe(const CReqContext& ctxReq, rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEthUnsubscribe(const CReqContext& ctxReq, rpc::CRPCParamPtr param);

protected:
    mtbase::IIOProc* pHttpServer;
    ICoreProtocol* pCoreProtocol;
    IService* pService;
    IDataStat* pDataStat;
    IForkManager* pForkManager;
    IBlockMaker* pBlockMaker;
    IWsService* pWsService;

    boost::mutex mutexDec;

private:
    std::map<std::string, RPCFunc> mapRPCFunc;
    bool fWriteRPCLog;
};

} // namespace metabasenet

#endif //METABASENET_RPCMOD_H

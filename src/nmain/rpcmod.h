// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_RPCMOD_H
#define METABASENET_RPCMOD_H

#include "json/json_spirit.h"
#include <boost/function.hpp>

#include "base.h"
#include "hcode.h"
#include "rpc/rpc.h"

namespace metabasenet
{

class CRPCMod : public hcode::IIOModule, virtual public hcode::CHttpEventListener
{
public:
    typedef rpc::CRPCResultPtr (CRPCMod::*RPCFunc)(rpc::CRPCParamPtr param);
    CRPCMod();
    ~CRPCMod();
    bool HandleEvent(hcode::CEventHttpReq& eventHttpReq) override;
    bool HandleEvent(hcode::CEventHttpBroken& eventHttpBroken) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    const CBasicConfig* BasicConfig()
    {
        return dynamic_cast<const CBasicConfig*>(hcode::IBase::Config());
    }
    const CNetworkConfig* Config()
    {
        return dynamic_cast<const CNetworkConfig*>(hcode::IBase::Config());
    }
    const CRPCServerConfig* RPCServerConfig()
    {
        return dynamic_cast<const CRPCServerConfig*>(IBase::Config());
    }

    void JsonReply(uint64 nNonce, const std::string& result);

    int GetInt(const rpc::CRPCInt64& i, int valDefault)
    {
        return i.IsValid() ? int(i) : valDefault;
    }
    unsigned int GetUint(const rpc::CRPCUint64& i, unsigned int valDefault)
    {
        return i.IsValid() ? uint64(i) : valDefault;
    }
    uint64 GetUint64(const rpc::CRPCUint64& i, uint64 valDefault)
    {
        return i.IsValid() ? uint64(i) : valDefault;
    }
    bool GetForkHashOfDef(const rpc::CRPCString& hex, uint256& hashFork)
    {
        if (!hex.empty())
        {
            if (hashFork.SetHex(hex) != hex.size())
            {
                return false;
            }
        }
        else
        {
            hashFork = pCoreProtocol->GetGenesisBlockHash();
        }
        return true;
    }
    bool CheckWalletError(Errno err);
    void ListDestination(std::vector<CDestination>& vDestination, const uint64 nPage, const uint64 nCount);
    bool CheckVersion(std::string& strVersion);
    std::string GetWidthString(const std::string& strIn, int nWidth);
    std::string GetWidthString(uint64 nCount, int nWidth);

private:
    /* System */
    rpc::CRPCResultPtr RPCHelp(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCStop(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCVersion(rpc::CRPCParamPtr param);
    /* Network */
    rpc::CRPCResultPtr RPCGetPeerCount(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListPeer(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddNode(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveNode(rpc::CRPCParamPtr param);
    /* Worldline & TxPool */
    rpc::CRPCResultPtr RPCGetForkCount(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListFork(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetForkGenealogy(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockLocation(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockCount(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockHash(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockNumberHash(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlock(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockDetail(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBlockData(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTxPool(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetForkHeight(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetVotes(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListDelegate(rpc::CRPCParamPtr param);
    /* Wallet */
    rpc::CRPCResultPtr RPCListKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetNewKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCEncryptKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCLockKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCUnlockKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportPrivKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportPubKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAddNewTemplate(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportTemplate(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportTemplate(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCRemoveTemplate(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCValidateAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetBalance(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSendFrom(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCCreateTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSignTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSignMessage(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCExportWallet(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCImportWallet(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeOrigin(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCCallContract(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTransactionReceipt(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractMuxCode(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListContractCode(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCListContractAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetDestContract(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractSource(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractCode(rpc::CRPCParamPtr param);
    /* Util */
    rpc::CRPCResultPtr RPCVerifyMessage(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeKeyPair(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetPubKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetPubKeyAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetAddressKey(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTemplateAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetContractAddress(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeTemplate(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCDecodeTransaction(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr GetDefiRelation(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCGetTxFee(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeSha256(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAesEncrypt(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCAesDecrypt(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCReverseHex(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCFuncSign(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCMakeHash(rpc::CRPCParamPtr param);
    /* Mint */
    rpc::CRPCResultPtr RPCGetWork(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCSubmitWork(rpc::CRPCParamPtr param);
    rpc::CRPCResultPtr RPCQueryStat(rpc::CRPCParamPtr param);

protected:
    hcode::IIOProc* pHttpServer;
    ICoreProtocol* pCoreProtocol;
    IService* pService;
    IDataStat* pDataStat;
    IForkManager* pForkManager;

private:
    std::map<std::string, RPCFunc> mapRPCFunc;
    bool fWriteRPCLog;
};

} // namespace metabasenet

#endif //METABASENET_RPCMOD_H

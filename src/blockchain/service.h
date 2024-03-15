// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_SERVICE_H
#define METABASENET_SERVICE_H

#include "base.h"
#include "mtbase.h"
#include "network.h"

namespace metabasenet
{

class CService : public IService
{
public:
    CService();
    ~CService();
    /* System */
    void Stop() override;
    /* Network */
    CChainId GetChainId() override;
    int GetPeerCount() override;
    void GetPeers(std::vector<network::CBbPeerInfo>& vPeerInfo) override;
    bool AddNode(const mtbase::CNetHost& node) override;
    bool RemoveNode(const mtbase::CNetHost& node) override;
    bool GetForkRpcPort(const uint256& hashFork, uint16& nRpcPort) override;
    /* Blockchain & Tx Pool*/
    int GetForkCount() override;
    bool HaveFork(const uint256& hashFork) override;
    int GetForkHeight(const uint256& hashFork) override;
    bool GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock) override;
    void ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll = false) override;
    bool GetForkContext(const uint256& hashFork, CForkContext& ctxtFork, const uint256& hashMainChainRefBlock = uint256()) override;
    bool VerifyForkFlag(const uint256& hashNewFork, const CChainId nChainIdIn, const std::string& strForkSymbol, const std::string& strForkName, const uint256& hashBlock = uint256()) override;
    bool GetForkCoinCtxByForkSymbol(const std::string& strForkSymbol, CCoinContext& ctxCoin, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainIdIn, uint256& hashFork) override;
    bool GetCoinContext(const std::string& strCoinSymbol, CCoinContext& ctxCoin, const uint256& hashLastBlock = uint256()) override;
    bool ListCoinContext(std::map<std::string, CCoinContext>& mapSymbolCoin, const uint256& hashLastBlock = uint256()) override;
    bool GetDexCoinPairBySymbolPair(const std::string& strSymbol1, const std::string& strSymbol2, uint32& nCoinPair, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetSymbolPairByDexCoinPair(const uint32 nCoinPair, std::string& strSymbol1, std::string& strSymbol2, const uint256& hashMainChainRefBlock = uint256()) override;
    bool ListDexCoinPair(const uint32 nCoinPair, const std::string& strCoinSymbol, std::map<uint32, std::pair<std::string, std::string>>& mapDexCoinPair, const uint256& hashMainChainRefBlock = uint256()) override;
    bool GetForkGenealogy(const uint256& hashFork, std::vector<std::pair<uint256, int>>& vAncestry,
                          std::vector<std::pair<int, uint256>>& vSubline) override;
    bool GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight) override;
    int GetBlockCount(const uint256& hashFork) override;
    bool GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock) override;
    bool GetBlockHashList(const uint256& hashFork, const uint32 nHeight, std::vector<uint256>& vBlockHash) override;
    bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) override;
    bool GetBlock(const uint256& hashBlock, CBlock& block, CChainId& nChainId, uint256& hashFork, int& nHeight) override;
    bool GetBlockEx(const uint256& hashBlock, CBlockEx& block, CChainId& nChainId, uint256& hashFork, int& nHeight) override;
    bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime) override;
    bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) override;
    bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) override;
    bool IsBlockConfirm(const uint256& hashBlock) override;
    void GetTxPool(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) override;
    void ListTxPool(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) override;
    bool GetTransactionAndPosition(const uint256& hashRefFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, uint256& hashAtBlock, uint64& nBlockNumber, uint16& nTxSeq) override;
    bool GetTransactionByIndex(const uint256& hashBlock, const uint64 nTxIndex, CTransaction& tx, uint64& nBlockNumber) override;
    Errno SendTransaction(const uint256& hashFork, CTransaction& tx) override;
    bool GetVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes, string& strFailCause) override;
    bool ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) override;
    bool GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes) override;
    bool GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight) override;
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex) override;
    bool CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                      const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult) override;
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value) override;
    uint256 AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter) override;
    void RemoveFilter(const uint256& nFilterId) override;
    bool GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs) override;
    bool GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs) override;
    uint256 AddBlockFilter(const uint256& hashClient, const uint256& hashFork) override;
    bool GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash) override;
    uint256 AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork) override;
    bool GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid) override;

    /* Wallet */
    bool HaveKey(const CDestination& dest, const int32 nVersion = -1) override;
    bool GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const override;
    std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) override;
    bool GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) override;
    boost::optional<std::string> MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey) override;
    boost::optional<std::string> AddKey(const crypto::CKey& key) override;
    boost::optional<std::string> RemoveKey(const CDestination& dest) override;
    bool ImportKey(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) override;
    bool ExportKey(const CDestination& dest, std::vector<unsigned char>& vchKey) override;
    bool EncryptKey(const CDestination& dest, const crypto::CCryptoString& strPassphrase, const crypto::CCryptoString& strCurrentPassphrase) override;
    bool Lock(const CDestination& dest) override;
    bool Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout) override;
    bool GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, const CCoinContext& ctxCoin, CWalletBalance& balance) override;
    bool SignSignature(const CDestination& dest, const uint256& hash, std::vector<unsigned char>& vchSig) override;
    bool SignTransaction(const uint256& hashFork, CTransaction& tx) override;
    bool HaveTemplate(const CDestination& dest) override;
    void GetTemplateIds(std::set<pair<uint8, CTemplateId>>& setTid, const uint64 nPos, const uint64 nCount) override;
    bool AddTemplate(CTemplatePtr& ptr) override;
    CTemplatePtr GetTemplate(const CDestination& dest) override;
    bool RemoveTemplate(const CDestination& dest) override;
    bool ListTransaction(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, std::vector<CDestTxInfo>& vTx) override;
    boost::optional<std::string> CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData,
                                                   const uint256& nAmount, const uint64 nNonce, const uint256& nGasPriceIn, const uint256& nGasIn, const bytes& vchData,
                                                   const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew) override;
    boost::optional<std::string> SignEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                                                    const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid, bytes& btSignTxData) override;
    bool SendEthRawTransaction(const bytes& btRawTxData, uint256& txid) override;
    bool SendEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                            const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid) override;
    void GetWalletDestinations(std::set<CDestination>& setDest) override;
    bool GetForkContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) override;
    bool ListContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode) override;
    bool ListContractAddress(const uint256& hashFork, const uint256& hashRefBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress) override;
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount) override;
    bool GeDestContractContext(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, CContractAddressContext& ctxtContract) override;
    bool GetContractSource(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashSource, bytes& btSource) override;
    bool GetContractCode(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashCode, bytes& btCode) override;
    bool GetDestTemplateData(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, bytes& btTemplateData) override;
    bool RetrieveAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashBlock = uint256()) override;
    uint64 GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest) override;

    bool GetCompleteOrder(const uint256& hashBlock, const CDestination& destOrder, const CChainId nChainIdOwner, const std::string& strCoinSymbolOwner,
                          const std::string& strCoinSymbolPeer, const uint64 nOrderNumber, uint256& nCompleteAmount, uint64& nCompleteOrderCount) override;
    bool GetCompleteOrder(const uint256& hashBlock, const uint256& hashDexOrder, uint256& nCompleteAmount, uint64& nCompleteOrderCount);
    bool ListAddressDexOrder(const uint256& hashBlock, const CDestination& destOrder, const std::string& strCoinSymbolOwner, const std::string& strCoinSymbolPeer,
                             const uint64 nBeginOrderNumber, const uint32 nGetCount, std::map<CDexOrderHeader, CDexOrderSave>& mapDexOrder) override;
    bool ListMatchDexOrder(const uint256& hashBlock, const std::string& strCoinSymbolSell, const std::string& strCoinSymbolBuy, const uint64 nGetCount, CRealtimeDexOrder& realDexOrder) override;

    bool AddBlacklistAddress(const CDestination& dest) override;
    void RemoveBlacklistAddress(const CDestination& dest) override;
    void ListBlacklistAddress(set<CDestination>& setAddressOut) override;
    bool ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress) override;
    bool RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress) override;
    bool GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction) override;
    bool VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr) override;

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice) override;
    uint256 GetForkMintMinGasPrice(const uint256& hashFork) override;

    /* Mint */
    bool GetWork(std::vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                 uint256& hashPrev, int& nAlgo, int& nBits,
                 const CTemplateMintPtr& templMint) override;
    Errno SubmitWork(const std::vector<unsigned char>& vchWorkData, const CTemplateMintPtr& templMint,
                     crypto::CKey& keyMint, uint256& hashBlock) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool SetContractTransaction(const uint256& hashFork, const uint256& hashLastBlock, const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew, std::string& strErr);
    bool SetTemplateTransaction(const uint256& hashFork, const uint256& hashLastBlock, const uint8 nToTemplateType, const bytes& btFormatData, const bytes& vchData, const bytes& btToData, CTransaction& txNew, std::string& strErr);

    const CRPCServerConfig* RPCServerConfig()
    {
        return dynamic_cast<const CRPCServerConfig*>(IBase::Config());
    }

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    IDispatcher* pDispatcher;
    IWallet* pWallet;
    CNetwork* pNetwork;
    IForkManager* pForkManager;
    network::INetChannel* pNetChannel;
    IBlockFilter* pBlockFilter;
};

} // namespace metabasenet

#endif //METABASENET_SERVICE_H

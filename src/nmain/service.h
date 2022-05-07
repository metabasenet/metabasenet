// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_SERVICE_H
#define METABASENET_SERVICE_H

#include "base.h"
#include "hcode.h"
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
    int GetPeerCount() override;
    void GetPeers(std::vector<network::CBbPeerInfo>& vPeerInfo) override;
    bool AddNode(const hcode::CNetHost& node) override;
    bool RemoveNode(const hcode::CNetHost& node) override;
    /* Blockchain & Tx Pool*/
    int GetForkCount() override;
    bool HaveFork(const uint256& hashFork) override;
    int GetForkHeight(const uint256& hashFork) override;
    bool GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock) override;
    void ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll = false) override;
    bool GetForkContext(const uint256& hashFork, CForkContext& ctxtFork) override;
    bool VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock = uint256()) override;
    bool GetForkGenealogy(const uint256& hashFork, std::vector<std::pair<uint256, int>>& vAncestry,
                          std::vector<std::pair<int, uint256>>& vSubline) override;
    bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight) override;
    int GetBlockCount(const uint256& hashFork) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, std::vector<uint256>& vBlockHash) override;
    bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) override;
    bool GetBlock(const uint256& hashBlock, CBlock& block, uint256& hashFork, int& nHeight) override;
    bool GetBlockEx(const uint256& hashBlock, CBlockEx& block, uint256& hashFork, int& nHeight) override;
    bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime) override;
    bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) override;
    bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) override;
    void GetTxPool(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) override;
    void ListTxPool(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) override;
    bool GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashBlock) override;
    Errno SendTransaction(CTransaction& tx) override;
    bool GetVotes(const CDestination& destDelegate, uint256& nVotes, string& strFailCause) override;
    bool ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) override;
    bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed) override;
    bool CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult) override;

    /* Wallet */
    bool HaveKey(const crypto::CPubKey& pubkey, const int32 nVersion = -1) override;
    std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) override;
    bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) override;
    boost::optional<std::string> MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey) override;
    boost::optional<std::string> AddKey(const crypto::CKey& key) override;
    boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) override;
    bool ImportKey(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) override;
    bool ExportKey(const crypto::CPubKey& pubkey, std::vector<unsigned char>& vchKey) override;
    bool EncryptKey(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                    const crypto::CCryptoString& strCurrentPassphrase) override;
    bool Lock(const crypto::CPubKey& pubkey) override;
    bool Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout) override;
    bool GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, CWalletBalance& balance) override;
    bool SignSignature(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<unsigned char>& vchSig) override;
    bool SignTransaction(CTransaction& tx) override;
    bool HaveTemplate(const CTemplateId& tid) override;
    void GetTemplateIds(std::set<CTemplateId>& setTid, const uint64 nPos, const uint64 nCount) override;
    bool AddTemplate(CTemplatePtr& ptr) override;
    CTemplatePtr GetTemplate(const CTemplateId& tid) override;
    bool RemoveTemplate(const CTemplateId& tid) override;
    bool ListTransaction(const uint256& hashFork, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, std::vector<CDestTxInfo>& vTx) override;
    boost::optional<std::string> CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData, const uint64 nTxType,
                                                   const uint256& nAmount, const uint64 nNonce, const uint256& nGasPriceIn, const uint256& mGasIn, const bytes& vchData,
                                                   const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew) override;
    bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) override;
    bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) override;
    void GetWalletDestinations(std::set<CDestination>& setDest) override;
    bool GetContractCodeContext(const uint256& hashFork, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) override;
    bool ListContractCodeContext(const uint256& hashFork, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode) override;
    bool ListContractAddress(const uint256& hashFork, std::map<uint256, CContractAddressContext>& mapContractAddress) override;
    bool GeDestContractContext(const uint256& hashFork, const CDestination& dest, CContractAddressContext& ctxtContract) override;
    bool GetContractSource(const uint256& hashFork, const uint256& hashSource, bytes& btSource) override;
    bool GetContractCode(const uint256& hashFork, const uint256& hashCode, bytes& btCode) override;
    bool GetDestTemplateData(const uint256& hashFork, const CDestination& dest, bytes& btTemplateData) override;
    bool RetrieveInviteParent(const uint256& hashFork, const CDestination& destSub, CDestination& destParent) override;

    /* Mint */
    bool GetWork(std::vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                 uint256& hashPrev, uint32& nPrevTime, int& nAlgo, int& nBits,
                 const CTemplateMintPtr& templMint) override;
    Errno SubmitWork(const std::vector<unsigned char>& vchWorkData, const CTemplateMintPtr& templMint,
                     crypto::CKey& keyMint, uint256& hashBlock) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    bool SetContractTransaction(const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew, std::string& strErr);
    bool SetTemplateTransaction(const bytes& btFormatData, const bytes& vchData, const bytes& btToData, CTransaction& txNew, std::string& strErr);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    IDispatcher* pDispatcher;
    IWallet* pWallet;
    CNetwork* pNetwork;
    IForkManager* pForkManager;
    network::INetChannel* pNetChannel;
};

} // namespace metabasenet

#endif //METABASENET_SERVICE_H

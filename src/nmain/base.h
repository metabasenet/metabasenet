// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_BASE_H
#define METABASENET_BASE_H

#include <boost/optional.hpp>
#include <hnbase.h>
#include <map>
#include <set>

#include "block.h"
#include "blockbase.h"
#include "config.h"
#include "crypto.h"
#include "destination.h"
#include "error.h"
#include "key.h"
#include "param.h"
#include "peer.h"
#include "profile.h"
#include "struct.h"
#include "template/mint.h"
#include "template/template.h"
#include "transaction.h"
#include "uint256.h"

namespace metabasenet
{

class ICoreProtocol : public hnbase::IBase
{
public:
    ICoreProtocol()
      : IBase("coreprotocol") {}
    virtual void InitializeGenesisBlock() = 0;
    virtual const uint256& GetGenesisBlockHash() = 0;
    virtual void GetGenesisBlock(CBlock& block) = 0;
    virtual Errno ValidateTransaction(const CTransaction& tx, int nHeight) = 0;
    virtual Errno ValidateBlock(const CBlock& block) = 0;
    virtual Errno VerifyForkCreateTx(const CTransaction& tx, const int nHeight, const bytes& btToAddressData) = 0;
    virtual Errno VerifyForkRedeemTx(const CTransaction& tx, const uint256& hashPrevBlock, const CDestState& state, const bytes& btFromAddressData) = 0;
    virtual Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) = 0;
    virtual Errno VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev) = 0;
    virtual Errno VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                              const CDelegateAgreement& agreement)
        = 0;
    virtual Errno VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                   const CDelegateAgreement& agreement)
        = 0;
    virtual Errno VerifyTransaction(const CTransaction& tx, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<uint256, CAddressContext>& mapBlockAddress) = 0;
    virtual bool GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, std::size_t nEnrollTrust = 0) = 0;
    virtual bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits) = 0;
    virtual void GetDelegatedBallot(const uint256& nAgreement, const std::size_t& nWeight, const std::map<CDestination, std::size_t>& mapBallot,
                                    const std::vector<std::pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, std::vector<CDestination>& vBallot, std::size_t& nEnrollTrust, int nBlockHeight)
        = 0;
    virtual uint256 MinEnrollAmount() = 0;
    virtual uint32 DPoSTimestamp(const CBlockIndex* pIndexPrev) = 0;
    virtual uint32 GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType) = 0;
    virtual uint32 CalcSingleBlockDistributeVoteRewardTxCount() = 0;
};

class IBlockChain : public hnbase::IBase
{
public:
    class CCheckPoint
    {
    public:
        CCheckPoint()
          : nHeight(-1)
        {
        }
        CCheckPoint(int nHeightIn, const uint256& nBlockHashIn)
          : nHeight(nHeightIn), nBlockHash(nBlockHashIn)
        {
        }
        CCheckPoint(const CCheckPoint& point)
          : nHeight(point.nHeight), nBlockHash(point.nBlockHash)
        {
        }
        CCheckPoint& operator=(const CCheckPoint& point)
        {
            nHeight = point.nHeight;
            nBlockHash = point.nBlockHash;
            return *this;
        }
        bool IsNull() const
        {
            return (nHeight == -1 || !nBlockHash);
        }

    public:
        int nHeight;
        uint256 nBlockHash;
    };

public:
    IBlockChain()
      : IBase("blockchain") {}
    virtual void GetForkStatus(std::map<uint256, CForkStatus>& mapForkStatus) = 0;
    virtual bool GetForkProfile(const uint256& hashFork, CProfile& profile) = 0;
    virtual bool GetForkContext(const uint256& hashFork, CForkContext& ctxt) = 0;
    virtual bool GetForkAncestry(const uint256& hashFork, std::vector<std::pair<uint256, uint256>> vAncestry) = 0;
    virtual int GetBlockCount(const uint256& hashFork) = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight, uint256& hashNext) = 0;
    virtual bool GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock) = 0;
    virtual bool GetBlockHash(const uint256& hashFork, int nHeight, std::vector<uint256>& vBlockHash) = 0;
    virtual bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) = 0;
    virtual bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) = 0;
    virtual bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime) = 0;
    virtual bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) = 0;
    virtual bool GetLastBlockTime(const uint256& hashFork, int nDepth, std::vector<int64>& vTime) = 0;
    virtual bool GetBlock(const uint256& hashBlock, CBlock& block) = 0;
    virtual bool GetBlockEx(const uint256& hashBlock, CBlockEx& block) = 0;
    virtual bool GetOrigin(const uint256& hashFork, CBlock& block) = 0;
    virtual bool Exists(const uint256& hashBlock) = 0;
    virtual bool GetTransaction(const uint256& txid, CTransaction& tx) = 0;
    virtual bool GetTransaction(const uint256& hashFork, const uint256& hashLastBlock, const uint256& txid, CTransaction& tx) = 0;
    virtual bool GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight) = 0;
    virtual bool ExistsTx(const uint256& txid) = 0;
    virtual bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256()) = 0;
    virtual bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock) = 0;
    virtual int GetForkCreatedHeight(const uint256& hashFork) = 0;
    virtual bool GetForkStorageMaxHeight(const uint256& hashFork, uint32& nMaxHeight) = 0;
    virtual Errno AddNewBlock(const CBlock& block, CBlockChainUpdate& update) = 0;
    virtual Errno AddNewOrigin(const CBlock& block, CBlockChainUpdate& update) = 0;
    virtual bool GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits) = 0;
    virtual bool GetBlockMintReward(const uint256& hashPrev, uint256& nReward) = 0;
    virtual bool GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep) = 0;
    virtual bool GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, std::vector<uint256>& vBlockHash, std::size_t nMaxCount) = 0;
    virtual int64 GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::vector<CTxInfo>& vTx) = 0;
    virtual bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress) = 0;
    virtual bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress) = 0;
    virtual bool RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode) = 0;
    virtual bool GetBlockSourceCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CTxContractData& txcdCode) = 0;
    virtual bool GetBlockWasmCreateCodeData(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CTxContractData& txcdCode) = 0;
    virtual bool GetContractCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) = 0;
    virtual bool ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapCreateCode) = 0;
    virtual bool ListAddressTxInfo(const uint256& hashFork, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo) = 0;
    virtual bool GetCreateForkLockedAmount(const CDestination& dest, const uint256& hashPrevBlock, const bytes& btAddressData, uint256& nLockedAmount) = 0;
    virtual bool VerifyAddressVoteRedeem(const CDestination& dest, const uint256& hashPrevBlock) = 0;
    virtual bool GetVoteRewardLockedAmount(const uint256& hashFork, const uint256& hashPrevBlock, const CDestination& dest, uint256& nLockedAmount) = 0;
    virtual bool VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock = uint256()) = 0;

    /////////////    CheckPoints    /////////////////////
    virtual bool HasCheckPoints(const uint256& hashFork) const = 0;
    virtual bool GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point) = 0;
    virtual std::vector<CCheckPoint> CheckPoints(const uint256& hashFork) const = 0;
    virtual CCheckPoint LatestCheckPoint(const uint256& hashFork) const = 0;
    virtual CCheckPoint UpperBoundCheckPoint(const uint256& hashFork, int nHeight) const = 0;
    virtual bool VerifyCheckPoint(const uint256& hashFork, int nHeight, const uint256& nBlockHash) = 0;
    virtual bool FindPreviousCheckPointBlock(const uint256& hashFork, CBlock& block) = 0;
    virtual bool IsSameBranch(const uint256& hashFork, const CBlock& block) = 0;

    virtual bool GetVotes(const CDestination& destDelegate, uint256& nVotes) = 0;
    virtual bool ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) = 0;
    virtual bool VerifyRepeatBlock(const uint256& hashFork, const CBlock& block, const uint256& hashBlockRef) = 0;
    virtual bool GetBlockDelegateVote(const uint256& hashBlock, std::map<CDestination, uint256>& mapVote) = 0;
    virtual uint256 GetDelegateMinEnrollAmount(const uint256& hashBlock) = 0;
    virtual bool GetDelegateCertTxCount(const uint256& hashLastBlock, std::map<CDestination, int>& mapVoteCert) = 0;
    virtual bool GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled) = 0;
    virtual bool GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement) = 0;
    virtual uint256 GetBlockMoneySupply(const uint256& hashBlock) = 0;
    virtual uint32 DPoSTimestamp(const uint256& hashPrev) = 0;
    virtual Errno VerifyPowBlock(const CBlock& block, bool& fLongChain) = 0;
    virtual bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt) = 0;
    virtual bool CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update) = 0;
    virtual bool VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock) = 0;
    virtual bool GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime) = 0;
    virtual bool IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block) = 0;

    virtual bool CalcBlockVoteRewardTx(const uint256& hashPrev, const uint16 nBlockType, const int nBlockHeight, const uint32 nBlockTime, vector<CTransaction>& vVoteRewardTx) = 0;
    virtual uint256 GetPrimaryBlockReward(const uint256& hashPrev) = 0;
    virtual bool CreateBlockStateRoot(const uint256& hashFork, const CBlock& block, uint256& hashStateRoot, uint256& hashReceiptRoot, uint256& nBlockGasUsed,
                                      uint2048& nBlockBloom, const bool fCreateBlock, const bool fMintRatio, uint256& nTotalMintRewardOut, uint256& nBlockMintRewardOut)
        = 0;
    virtual bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CDestState& state) = 0;
    virtual bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed) = 0;
    virtual bool CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult) = 0;
    virtual bool VerifyContractAddress(const uint256& hashFork, const uint256& hashBlock, const CDestination& destContract) = 0;
    virtual bool VerifyCreateContractTx(const uint256& hashFork, const uint256& hashBlock, const CTransaction& tx) = 0;

    const CBasicConfig* Config()
    {
        return dynamic_cast<const CBasicConfig*>(hnbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(hnbase::IBase::Config());
    }
};

class ITxPool : public hnbase::IBase
{
public:
    ITxPool()
      : IBase("txpool") {}
    virtual bool Exists(const uint256& hashFork, const uint256& txid) = 0;
    virtual bool CheckTxNonce(const uint256& hashFork, const CDestination& destFrom, const uint64 nTxNonce) = 0;
    virtual std::size_t Count(const uint256& hashFork) const = 0;
    virtual Errno Push(const CTransaction& tx) = 0;
    //virtual void Pop(const uint256& txid) = 0;
    virtual bool Get(const uint256& txid, CTransaction& tx) const = 0;
    virtual void ListTx(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) = 0;
    virtual void ListTx(const uint256& hashFork, std::vector<uint256>& vTxPool) = 0;
    virtual bool ListTx(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) = 0;
    virtual bool FetchArrangeBlockTx(const uint256& hashFork, const uint256& hashPrev, const int64 nBlockTime,
                                     const std::size_t nMaxSize, std::vector<CTransaction>& vtx, uint256& nTotalTxFee)
        = 0;
    virtual bool SynchronizeBlockChain(const CBlockChainUpdate& update) = 0;
    virtual void AddDestDelegate(const CDestination& destDeleage) = 0;
    virtual int GetDestTxpoolTxCount(const CDestination& dest) = 0;
    virtual void GetDestBalance(const uint256& hashFork, const CDestination& dest, uint64& nNonce, uint256& nAvail, uint256& nUnconfirmedIn, uint256& nUnconfirmedOut) = 0;
    virtual uint64 GetDestNextTxNonce(const uint256& hashFork, const CDestination& dest) = 0;
    virtual uint64 GetCertTxNextNonce(const CDestination& dest) = 0;
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(hnbase::IBase::Config());
    }
};

class IForkManager : public hnbase::IBase
{
public:
    IForkManager()
      : IBase("forkmanager") {}
    virtual bool GetActiveFork(std::vector<uint256>& vActive) = 0;
    virtual bool IsAllowed(const uint256& hashFork) const = 0;
    virtual void ForkUpdate(const CBlockChainUpdate& update, std::vector<uint256>& vActive, std::vector<uint256>& vDeactive) = 0;
    const CForkConfig* ForkConfig()
    {
        return dynamic_cast<const CForkConfig*>(hnbase::IBase::Config());
    }
};

class IConsensus : public hnbase::IBase
{
public:
    IConsensus()
      : IBase("consensus") {}
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(hnbase::IBase::Config());
    }
    virtual void PrimaryUpdate(const CBlockChainUpdate& update, CDelegateRoutine& routine) = 0;
    virtual bool AddNewDistribute(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchDistribute) = 0;
    virtual bool AddNewPublish(const uint256& hashDistributeAnchor, const CDestination& destFrom, const std::vector<unsigned char>& vchPublish) = 0;
    virtual void GetAgreement(int nTargetHeight, uint256& nAgreement, std::size_t& nWeight, std::vector<CDestination>& vBallot) = 0;
    virtual void GetProof(int nTargetHeight, std::vector<unsigned char>& vchProof) = 0;
    virtual bool GetNextConsensus(CAgreementBlock& consParam) = 0;
    virtual bool LoadConsensusData(int& nStartHeight, CDelegateRoutine& routine) = 0;
};

class IBlockMaker : public hnbase::CEventProc
{
public:
    IBlockMaker()
      : CEventProc("blockmaker") {}
    const CMintConfig* MintConfig()
    {
        return dynamic_cast<const CMintConfig*>(hnbase::IBase::Config());
    }
};

class IWallet : public hnbase::IBase
{
public:
    IWallet()
      : IBase("wallet") {}
    /* Key store */
    virtual boost::optional<std::string> AddKey(const crypto::CKey& key) = 0;
    virtual boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) = 0;
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const = 0;
    virtual bool Have(const crypto::CPubKey& pubkey, const int32 nVersion = -1) const = 0;
    virtual bool Export(const crypto::CPubKey& pubkey, std::vector<unsigned char>& vchKey) const = 0;
    virtual bool Import(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) = 0;
    virtual bool Encrypt(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                         const crypto::CCryptoString& strCurrentPassphrase)
        = 0;
    virtual bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const = 0;
    virtual bool IsLocked(const crypto::CPubKey& pubkey) const = 0;
    virtual bool Lock(const crypto::CPubKey& pubkey) = 0;
    virtual bool Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout) = 0;
    virtual bool Sign(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<uint8>& vchSig) = 0;
    virtual bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) = 0;
    virtual bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) = 0;
    /* Template */
    virtual void GetTemplateIds(std::set<CTemplateId>& setTemplateId, const uint64 nPos, const uint64 nCount) const = 0;
    virtual bool Have(const CTemplateId& tid) const = 0;
    virtual bool AddTemplate(CTemplatePtr& ptr) = 0;
    virtual CTemplatePtr GetTemplate(const CTemplateId& tid) const = 0;
    virtual bool RemoveTemplate(const CTemplateId& tid) = 0;
    /* Destination */
    virtual void GetDestinations(std::set<CDestination>& setDest) = 0;
    /* Wallet Tx */
    virtual bool SignTransaction(const CDestination& destIn, CTransaction& tx, const uint256& hashFork, const int32 nForkHeight) = 0;

    const CBasicConfig* Config()
    {
        return dynamic_cast<const CBasicConfig*>(hnbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(hnbase::IBase::Config());
    }
};

class IDispatcher : public hnbase::IBase
{
public:
    IDispatcher()
      : IBase("dispatcher") {}
    virtual Errno AddNewBlock(const CBlock& block, uint64 nNonce = 0) = 0;
    virtual Errno AddNewTx(const CTransaction& tx, uint64 nNonce = 0) = 0;
    virtual bool AddNewDistribute(const uint256& hashAnchor, const CDestination& dest,
                                  const std::vector<unsigned char>& vchDistribute)
        = 0;
    virtual bool AddNewPublish(const uint256& hashAnchor, const CDestination& dest,
                               const std::vector<unsigned char>& vchPublish)
        = 0;
    virtual void SetConsensus(const CAgreementBlock& agreeBlock) = 0;
    virtual void CheckAllSubForkLastBlock() = 0;
};

class IService : public hnbase::IBase
{
public:
    IService()
      : IBase("service") {}
    /* System */
    virtual void Stop() = 0;
    /* Network */
    virtual int GetPeerCount() = 0;
    virtual void GetPeers(std::vector<network::CBbPeerInfo>& vPeerInfo) = 0;
    virtual bool AddNode(const hnbase::CNetHost& node) = 0;
    virtual bool RemoveNode(const hnbase::CNetHost& node) = 0;
    /* Blockchain & Tx Pool*/
    virtual int GetForkCount() = 0;
    virtual bool HaveFork(const uint256& hashFork) = 0;
    virtual int GetForkHeight(const uint256& hashFork) = 0;
    virtual bool GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock) = 0;
    virtual void ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll = false) = 0;
    virtual bool GetForkContext(const uint256& hashFork, CForkContext& ctxtFork) = 0;
    virtual bool VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock = uint256()) = 0;
    virtual bool GetForkGenealogy(const uint256& hashFork, std::vector<std::pair<uint256, int>>& vAncestry,
                                  std::vector<std::pair<int, uint256>>& vSubline)
        = 0;
    virtual bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight) = 0;
    virtual int GetBlockCount(const uint256& hashFork) = 0;
    virtual bool GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock) = 0;
    virtual bool GetBlockHash(const uint256& hashFork, int nHeight, std::vector<uint256>& vBlockHash) = 0;
    virtual bool GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock) = 0;
    virtual bool GetBlock(const uint256& hashBlock, CBlock& block, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetBlockEx(const uint256& hashBlock, CBlockEx& block, uint256& hashFork, int& nHeight) = 0;
    virtual bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime) = 0;
    virtual bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) = 0;
    virtual bool GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status) = 0;
    virtual void GetTxPool(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) = 0;
    virtual void ListTxPool(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) = 0;
    virtual bool GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashBlock) = 0;
    virtual Errno SendTransaction(CTransaction& tx) = 0;
    //virtual bool RemovePendingTx(const uint256& txid) = 0;
    virtual bool GetVotes(const CDestination& destDelegate, uint256& nVotes, string& strFailCause) = 0;
    virtual bool ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes) = 0;
    virtual bool GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed) = 0;
    virtual bool CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult) = 0;

    /* Wallet */
    virtual bool HaveKey(const crypto::CPubKey& pubkey, const int32 nVersion = -1) = 0;
    virtual std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) = 0;
    virtual bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) = 0;
    virtual boost::optional<std::string> MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey) = 0;
    virtual boost::optional<std::string> AddKey(const crypto::CKey& key) = 0;
    virtual boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) = 0;
    virtual bool ImportKey(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) = 0;
    virtual bool ExportKey(const crypto::CPubKey& pubkey, std::vector<unsigned char>& vchKey) = 0;
    virtual bool EncryptKey(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                            const crypto::CCryptoString& strCurrentPassphrase)
        = 0;
    virtual bool Lock(const crypto::CPubKey& pubkey) = 0;
    virtual bool Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout) = 0;
    virtual bool GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, CWalletBalance& balance) = 0;
    virtual bool SignSignature(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<unsigned char>& vchSig) = 0;
    virtual bool SignTransaction(CTransaction& tx) = 0;
    virtual bool HaveTemplate(const CTemplateId& tid) = 0;
    virtual void GetTemplateIds(std::set<CTemplateId>& setTid, const uint64 nPos, const uint64 nCount) = 0;
    virtual bool AddTemplate(CTemplatePtr& ptr) = 0;
    virtual CTemplatePtr GetTemplate(const CTemplateId& tid) = 0;
    virtual bool RemoveTemplate(const CTemplateId& tid) = 0;
    virtual bool ListTransaction(const uint256& hashFork, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, std::vector<CDestTxInfo>& vTx) = 0;
    virtual boost::optional<std::string> CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData, const uint64 nTxType,
                                                           const uint256& nAmount, const uint64 nNonce, const uint256& nGasPriceIn, const uint256& mGasIn, const bytes& vchData,
                                                           const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew)
        = 0;
    virtual bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) = 0;
    virtual bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) = 0;
    virtual void GetWalletDestinations(std::set<CDestination>& setDest) = 0;
    virtual bool GetContractCodeContext(const uint256& hashFork, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode) = 0;
    virtual bool ListContractCodeContext(const uint256& hashFork, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode) = 0;
    virtual bool ListContractAddress(const uint256& hashFork, std::map<uint256, CContractAddressContext>& mapContractAddress) = 0;
    virtual bool GeDestContractContext(const uint256& hashFork, const CDestination& dest, CContractAddressContext& ctxtContract) = 0;
    virtual bool GetContractSource(const uint256& hashFork, const uint256& hashSource, bytes& btSource) = 0;
    virtual bool GetContractCode(const uint256& hashFork, const uint256& hashCode, bytes& btCode) = 0;
    virtual bool GetDestTemplateData(const uint256& hashFork, const CDestination& dest, bytes& btTemplateData) = 0;
    /* Mint */
    virtual bool GetWork(std::vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                         uint256& hashPrev, uint32& nPrevTime, int& nAlgo, int& nBits,
                         const CTemplateMintPtr& templMint)
        = 0;
    virtual Errno SubmitWork(const std::vector<unsigned char>& vchWorkData, const CTemplateMintPtr& templMint,
                             crypto::CKey& keyMint, uint256& hashBlock)
        = 0;
};

class IDataStat : public hnbase::IIOModule
{
public:
    IDataStat()
      : IIOModule("datastat") {}
    virtual bool AddBlockMakerStatData(const uint256& hashFork, bool fPOW, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynRecvStatData(const uint256& hashFork, uint64 nBlockCountIn, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynSendStatData(const uint256& hashFork, uint64 nBlockCountIn, uint64 nTxCountIn) = 0;
    virtual bool AddP2pSynTxSynStatData(const uint256& hashFork, bool fRecv) = 0;
    virtual bool GetBlockMakerStatData(const uint256& hashFork, uint32 nBeginTime, uint32 nGetCount, std::vector<CStatItemBlockMaker>& vStatData) = 0;
    virtual bool GetP2pSynStatData(const uint256& hashFork, uint32 nBeginTime, uint32 nGetCount, std::vector<CStatItemP2pSyn>& vStatData) = 0;
};

class IRecovery : public hnbase::IBase
{
public:
    IRecovery()
      : IBase("recovery") {}
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(hnbase::IBase::Config());
    }
};

} // namespace metabasenet

#endif //METABASENET_BASE_H

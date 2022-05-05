// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "service.h"

#include "event.h"
#include "template/activatecode.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/vote.h"

using namespace std;
using namespace hnbase;

extern void Shutdown();

namespace metabasenet
{

//////////////////////////////
// CService

CService::CService()
  : pCoreProtocol(nullptr), pBlockChain(nullptr), pTxPool(nullptr), pDispatcher(nullptr), pWallet(nullptr), pNetwork(nullptr), pForkManager(nullptr), pNetChannel(nullptr)
{
}

CService::~CService()
{
}

bool CService::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool");
        return false;
    }

    if (!GetObject("dispatcher", pDispatcher))
    {
        Error("Failed to request dispatcher");
        return false;
    }

    if (!GetObject("wallet", pWallet))
    {
        Error("Failed to request wallet");
        return false;
    }

    if (!GetObject("peernet", pNetwork))
    {
        Error("Failed to request network");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }

    if (!GetObject("netchannel", pNetChannel))
    {
        Error("Failed to request netchannel");
        return false;
    }

    return true;
}

void CService::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pWallet = nullptr;
    pNetwork = nullptr;
    pForkManager = nullptr;
    pNetChannel = nullptr;
}

bool CService::HandleInvoke()
{
    return true;
}

void CService::HandleHalt()
{
}

void CService::Stop()
{
    Shutdown();
}

int CService::GetPeerCount()
{
    CEventPeerNetGetCount eventGetPeerCount(0);
    if (pNetwork->DispatchEvent(&eventGetPeerCount))
    {
        return eventGetPeerCount.result;
    }
    return 0;
}

void CService::GetPeers(vector<network::CBbPeerInfo>& vPeerInfo)
{
    vPeerInfo.clear();
    CEventPeerNetGetPeers eventGetPeers(0);
    if (pNetwork->DispatchEvent(&eventGetPeers))
    {
        vPeerInfo.reserve(eventGetPeers.result.size());
        for (unsigned int i = 0; i < eventGetPeers.result.size(); i++)
        {
            vPeerInfo.push_back(static_cast<network::CBbPeerInfo&>(eventGetPeers.result[i]));
        }
    }
}

bool CService::AddNode(const CNetHost& node)
{
    CEventPeerNetAddNode eventAddNode(0);
    eventAddNode.data = node;
    return pNetwork->DispatchEvent(&eventAddNode);
}

bool CService::RemoveNode(const CNetHost& node)
{
    CEventPeerNetRemoveNode eventRemoveNode(0);
    eventRemoveNode.data = node;
    return pNetwork->DispatchEvent(&eventRemoveNode);
}

int CService::GetForkCount()
{
    map<uint256, CForkContext> mapForkCtxt;
    pBlockChain->ListForkContext(mapForkCtxt);
    return mapForkCtxt.size();
}

bool CService::HaveFork(const uint256& hashFork)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock) || hashLastBlock == 0)
    {
        StdLog("CService", "Have Fork: Retrieve fork last fail, fork: %s, last: %s",
               hashFork.GetHex().c_str(), hashLastBlock.GetHex().c_str());
        return false;
    }
    return true;
}

int CService::GetForkHeight(const uint256& hashFork)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        return 0;
    }
    return CBlock::GetBlockHeightByHash(hashLastBlock);
}

bool CService::GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock)
{
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    nLastHeight = CBlock::GetBlockHeightByHash(hashLastBlock);
    return true;
}

void CService::ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!pBlockChain->ListForkContext(mapForkCtxt))
    {
        return;
    }
    if (fAll)
    {
        for (const auto& kv : mapForkCtxt)
        {
            vFork.push_back(make_pair(kv.first, kv.second.GetProfile()));
        }
    }
    else
    {
        for (const auto& kv : mapForkCtxt)
        {
            if (pForkManager->IsAllowed(kv.first))
            {
                vFork.push_back(make_pair(kv.first, kv.second.GetProfile()));
            }
        }
    }
}

bool CService::GetForkContext(const uint256& hashFork, CForkContext& ctxtFork)
{
    return pBlockChain->GetForkContext(hashFork, ctxtFork);
}

bool CService::VerifyForkName(const uint256& hashFork, const std::string& strForkName, const uint256& hashBlock)
{
    return pBlockChain->VerifyForkName(hashFork, strForkName, hashBlock);
}

bool CService::GetForkGenealogy(const uint256& hashFork, vector<pair<uint256, int>>& vAncestry, vector<pair<int, uint256>>& vSubline)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!pBlockChain->ListForkContext(mapForkCtxt))
    {
        return false;
    }

    auto getJoint = [&](const uint256& hashForkIn, uint256& hashParentOut, uint256& hashJointOut, int& nJointHeightOut) -> bool
    {
        auto it = mapForkCtxt.find(hashForkIn);
        if (it == mapForkCtxt.end())
        {
            return false;
        }
        hashParentOut = it->second.hashParent;
        hashJointOut = it->second.hashJoint;
        nJointHeightOut = it->second.nJointHeight;
        return true;
    };

    uint256 hashParent, hashJoint;
    int nJointHeight;
    if (!getJoint(hashFork, hashParent, hashJoint, nJointHeight))
    {
        return false;
    }
    while (hashParent != 0)
    {
        vAncestry.push_back(make_pair(hashParent, nJointHeight));
        if (!getJoint(hashParent, hashParent, hashJoint, nJointHeight))
        {
            return false;
        }
    }

    for (const auto& kv : mapForkCtxt)
    {
        if (kv.second.hashParent == hashFork)
        {
            vSubline.push_back(make_pair(kv.second.nJointHeight, kv.second.hashFork));
        }
    }
    return true;
}

bool CService::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

int CService::GetBlockCount(const uint256& hashFork)
{
    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        return (GetForkHeight(hashFork) + 1);
    }
    return pBlockChain->GetBlockCount(hashFork);
}

bool CService::GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock)
{
    if (nHeight < 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashBlock))
        {
            StdTrace("CService", "Create Transaction: Retrieve fork last fail");
            return false;
        }
        return true;
    }
    return pBlockChain->GetBlockHash(hashFork, nHeight, hashBlock);
}

bool CService::GetBlockHash(const uint256& hashFork, int nHeight, vector<uint256>& vBlockHash)
{
    return pBlockChain->GetBlockHash(hashFork, nHeight, vBlockHash);
}

bool CService::GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock)
{
    return pBlockChain->GetBlockNumberHash(hashFork, nNumber, hashBlock);
}

bool CService::GetBlock(const uint256& hashBlock, CBlock& block, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlock(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

bool CService::GetBlockEx(const uint256& hashBlock, CBlockEx& block, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockEx(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

bool CService::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime)
{
    return pBlockChain->GetLastBlockOfHeight(hashFork, nHeight, hashBlock, nTime);
}

bool CService::GetBlockStatus(const uint256& hashBlock, CBlockStatus& status)
{
    return pBlockChain->GetBlockStatus(hashBlock, status);
}

bool CService::GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status)
{
    return pBlockChain->GetLastBlockStatus(hashFork, status);
}

void CService::GetTxPool(const uint256& hashFork, vector<pair<uint256, size_t>>& vTxPool)
{
    vTxPool.clear();
    pTxPool->ListTx(hashFork, vTxPool);
}

void CService::ListTxPool(const uint256& hashFork, const CDestination& dest, vector<CTxInfo>& vTxPool, const int64 nGetOffset, const int64 nGetCount)
{
    vTxPool.clear();
    pTxPool->ListTx(hashFork, dest, vTxPool, nGetOffset, nGetCount);
}

bool CService::GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashBlock)
{
    if (!pTxPool->Get(txid, tx))
    {
        uint256 hashFork;
        int nHeight;
        if (!pBlockChain->GetTransaction(txid, tx, hashFork, nHeight))
        {
            return false;
        }
        std::vector<uint256> vHashBlock;
        if (!GetBlockHash(hashFork, nHeight, vHashBlock))
        {
            StdLog("CService", "Get Transaction: GetBlockHash fail, txid: %s, fork: %s, height: %d",
                   txid.GetHex().c_str(), hashFork.GetHex().c_str(), nHeight);
            return false;
        }
        for (const auto& hash : vHashBlock)
        {
            CBlockEx block;
            uint256 tempHashFork;
            int tempHeight = 0;
            if (!GetBlockEx(hash, block, tempHashFork, tempHeight))
            {
                StdLog("CService", "Get Transaction: GetBlockEx fail, txid: %s, block: %s",
                       txid.GetHex().c_str(), hash.GetHex().c_str());
                return false;
            }
            if (txid == block.txMint.GetHash())
            {
                hashBlock = hash;
                break;
            }
            for (int i = 0; i < block.vtx.size(); i++)
            {
                if (txid == block.vtx[i].GetHash())
                {
                    hashBlock = hash;
                    break;
                }
            }
            if (hashBlock != 0)
            {
                break;
            }
        }
    }
    return true;
}

Errno CService::SendTransaction(CTransaction& tx)
{
    return pDispatcher->AddNewTx(tx);
}

bool CService::GetVotes(const CDestination& destDelegate, uint256& nVotes, string& strFailCause)
{
    CTemplateId tid;
    if (!destDelegate.GetTemplateId(tid)
        || (tid.GetType() != TEMPLATE_DELEGATE && tid.GetType() != TEMPLATE_VOTE))
    {
        strFailCause = "Not a delegate or vote template address";
        return false;
    }
    if (tid.GetType() == TEMPLATE_DELEGATE)
    {
        if (!pBlockChain->GetVotes(destDelegate, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    else
    {
        CTemplatePtr ptr = pWallet->GetTemplate(tid);
        if (ptr == nullptr)
        {
            strFailCause = "Vote template address not imported";
            return false;
        }
        CDestination destDelegateTemplateOut = boost::dynamic_pointer_cast<CTemplateVote>(ptr)->destDelegate;
        if (destDelegateTemplateOut.IsNull())
        {
            strFailCause = "Vote template address not imported";
            return false;
        }
        if (!pBlockChain->GetVotes(destDelegateTemplateOut, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    return true;
}

bool CService::ListDelegate(uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    return pBlockChain->ListDelegate(nCount, mapVotes);
}

bool CService::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& receipt, uint256& hashBlock, uint256& nBlockGasUsed)
{
    return pBlockChain->GetTransactionReceipt(hashFork, txid, receipt, hashBlock, nBlockGasUsed);
}

bool CService::CallContract(const uint256& hashFork, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice, const uint256& nGas, const bytes& btContractParam, int& nStatus, bytes& btResult)
{
    return pBlockChain->CallContract(hashFork, from, to, nAmount, nGasPrice, nGas, btContractParam, nStatus, btResult);
}

bool CService::HaveKey(const crypto::CPubKey& pubkey, const int32 nVersion)
{
    return pWallet->Have(pubkey, nVersion);
}

size_t CService::GetPubKeys(set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount)
{
    return pWallet->GetPubKeys(setPubKey, nPos, nCount);
}

bool CService::GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic)
{
    return pWallet->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic);
}

boost::optional<std::string> CService::MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey)
{
    crypto::CKey key;
    if (!key.Renew())
    {
        return std::string("Renew Key Failed");
    }
    if (!strPassphrase.empty())
    {
        if (!key.Encrypt(strPassphrase))
        {
            return std::string("Encrypt Key failed");
        }
        key.Lock();
    }
    pubkey = key.GetPubKey();
    return pWallet->AddKey(key);
}

boost::optional<std::string> CService::AddKey(const crypto::CKey& key)
{
    return pWallet->AddKey(key);
}

boost::optional<std::string> CService::RemoveKey(const crypto::CPubKey& pubkey)
{
    return pWallet->RemoveKey(pubkey);
}

bool CService::ImportKey(const vector<unsigned char>& vchKey, crypto::CPubKey& pubkey)
{
    return pWallet->Import(vchKey, pubkey);
}

bool CService::ExportKey(const crypto::CPubKey& pubkey, vector<unsigned char>& vchKey)
{
    return pWallet->Export(pubkey, vchKey);
}

bool CService::EncryptKey(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                          const crypto::CCryptoString& strCurrentPassphrase)
{
    return pWallet->Encrypt(pubkey, strPassphrase, strCurrentPassphrase);
}

bool CService::Lock(const crypto::CPubKey& pubkey)
{
    return pWallet->Lock(pubkey);
}

bool CService::Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout)
{
    return pWallet->Unlock(pubkey, strPassphrase, nTimeout);
}

bool CService::GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, CWalletBalance& balance)
{
    pTxPool->GetDestBalance(hashFork, dest, balance.nTxNonce, balance.nAvailable, balance.nUnconfirmedIn, balance.nUnconfirmedOut);
    if (balance.nAvailable > 0)
    {
        uint32 nDestTemplateType = (dest.IsTemplate() ? dest.GetTemplateId().GetType() : 0);
        if (nDestTemplateType == TEMPLATE_FORK)
        {
            uint256 nLockedAmount;
            if (pBlockChain->GetCreateForkLockedAmount(dest, hashLastBlock, bytes(), nLockedAmount))
            {
                balance.nLocked += nLockedAmount;
            }
        }
        else if (nDestTemplateType == TEMPLATE_DELEGATE || nDestTemplateType == TEMPLATE_VOTE)
        {
            if (!pBlockChain->VerifyAddressVoteRedeem(dest, hashLastBlock))
            {
                balance.nLocked += balance.nAvailable;
            }
        }
        if (balance.nAvailable > balance.nLocked)
        {
            uint256 nLockedAmount;
            if (pBlockChain->GetVoteRewardLockedAmount(hashFork, hashLastBlock, dest, nLockedAmount))
            {
                balance.nLocked += nLockedAmount;
            }
        }

        if (balance.nLocked > 0)
        {
            if (balance.nAvailable > balance.nLocked)
            {
                balance.nAvailable -= balance.nLocked;
            }
            else
            {
                balance.nAvailable = 0;
            }
        }
    }
    return true;
}

bool CService::SignSignature(const crypto::CPubKey& pubkey, const uint256& hash, vector<unsigned char>& vchSig)
{
    return pWallet->Sign(pubkey, hash, vchSig);
}

bool CService::SignTransaction(CTransaction& tx)
{
    uint256 hashFork;
    int nHeight;
    if (!pBlockChain->GetBlockLocation(tx.hashFork, hashFork, nHeight))
    {
        StdError("CService", "Sign Transaction: GetBlockLocation fail, txid: %s, hashFork: %s", tx.GetHash().GetHex().c_str(), tx.hashFork.GetHex().c_str());
        return false;
    }

    int32 nForkHeight;
    uint256 hashLastBlock;
    if (!GetForkLastBlock(hashFork, nForkHeight, hashLastBlock))
    {
        StdError("CService", "Sign Transaction: GetForkLastBlock fail, txid: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    if (hashFork == pCoreProtocol->GetGenesisBlockHash()
        && tx.to.IsTemplate() && tx.to.GetTemplateId().GetType() == TEMPLATE_FORK)
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        if (!pBlockChain->VerifyBlockForkTx(hashLastBlock, tx, vForkCtxt) || vForkCtxt.empty())
        {
            StdError("CService", "Sign Transaction: Verify block fork tx fail, txid: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    if (!pWallet->SignTransaction(tx.from, tx, hashFork, nForkHeight))
    {
        StdError("CService", "Sign ransaction: Sign Transaction fail, txid: %s, from: %s", tx.GetHash().GetHex().c_str(), tx.from.ToString().c_str());
        return false;
    }
    return true;
}

bool CService::HaveTemplate(const CTemplateId& tid)
{
    return pWallet->Have(tid);
}

void CService::GetTemplateIds(set<CTemplateId>& setTid, const uint64 nPos, const uint64 nCount)
{
    pWallet->GetTemplateIds(setTid, nPos, nCount);
}

bool CService::AddTemplate(CTemplatePtr& ptr)
{
    return pWallet->AddTemplate(ptr);
}

CTemplatePtr CService::GetTemplate(const CTemplateId& tid)
{
    return pWallet->GetTemplate(tid);
}

bool CService::RemoveTemplate(const CTemplateId& tid)
{
    return pWallet->RemoveTemplate(tid);
}

bool CService::ListTransaction(const uint256& hashFork, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, vector<CDestTxInfo>& vTx)
{
    return pBlockChain->ListAddressTxInfo(hashFork, dest, nOffset, nCount, fReverse, vTx);
}

boost::optional<std::string> CService::CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData, const uint64 nTxType,
                                                         const uint256& nAmount, const uint64 nNonce, const uint256& nGasPriceIn, const uint256& mGasIn, const bytes& vchData,
                                                         const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew)
{
    uint64 nNewTxNonce = 0;
    if (nNonce == 0)
    {
        nNewTxNonce = pTxPool->GetDestNextTxNonce(hashFork, destFrom);
        if (nNewTxNonce == 0)
        {
            StdError("CService", "Create Transaction: Get tx nonce fail, fork: %s", hashFork.GetHex().c_str());
            return std::string("Get tx nonce fail, fork: ") + hashFork.GetHex();
        }
    }
    else
    {
        nNewTxNonce = nNonce;
    }
    if (!CTransaction::IsUserTx(nTxType))
    {
        StdLog("CService", "Create Transaction: tx type error, tx type: %ld", nTxType);
        return std::string("tx type error");
    }

    txNew.SetNull();
    txNew.nType = nTxType;
    txNew.nTimeStamp = GetNetTime();
    txNew.hashFork = hashFork;
    txNew.nTxNonce = nNewTxNonce;
    txNew.from = destFrom;
    txNew.to = destTo;
    txNew.nAmount = nAmount;
    txNew.nGasPrice = nGasPriceIn;
    txNew.nGasLimit = mGasIn;

    if (txNew.nGasPrice == 0)
    {
        txNew.nGasPrice = DEF_TX_GAS_PRICE;
    }
    if (txNew.nGasLimit == 0)
    {
        txNew.nGasLimit = DEF_TX_GAS_LIMIT;
    }

    if (nTxType == CTransaction::TX_DEFI_RELATION)
    {
        if (txNew.from == txNew.to)
        {
            StdLog("CService", "Create Transaction: DeFi relation tx from address must be not equal to to address");
            return std::string("DeFi relation tx from address must be not equal to to address");
        }
        if (!txNew.from.IsPubKey() || !txNew.to.IsPubKey())
        {
            StdLog("CService", "Create Transaction: DeFi tx to Address and from must be public key address");
            return std::string("DeFi tx to Address and from must be public key address");
        }
        // vchData: shared_pubkey + sub_sig + parent_sig
        if (vchData.size() != 160)
        {
            StdLog("CService", "Create Transaction: DeFi tx length of vchData is not 160");
            return std::string("DeFi tx length of vchData is not 160");
        }
        txNew.AddTxData(CTransaction::DF_COMMON, vchData);
    }
    else
    {
        if (txNew.to.IsContract())
        {
            std::string strErr;
            if (!SetContractTransaction(btFormatData, btContractCode, btContractParam, txNew, strErr))
            {
                return strErr;
            }
        }
        else if (txNew.to.IsTemplate())
        {
            std::string strErr;
            if (!SetTemplateTransaction(btFormatData, vchData, btToData, txNew, strErr))
            {
                return strErr;
            }
        }
        else
        {
            if (vchData.size() > 0)
            {
                txNew.AddTxData(CTransaction::DF_COMMON, vchData);
            }
        }
    }

    uint256 nNeedGas = TX_BASE_GAS + txNew.GetTxDataGas();
    if (!txNew.to.IsContract() && mGasIn == 0)
    {
        txNew.nGasLimit = nNeedGas;
    }
    else
    {
        if (txNew.nGasLimit < nNeedGas)
        {
            StdError("CService", "Create Transaction: gas not enough, need gas: 0x%s", nNeedGas.GetHex().c_str());
            return std::string("gas not enough");
        }
    }

    return boost::optional<std::string>{};
}

bool CService::AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext)
{
    return pWallet->AesEncrypt(pubkeyLocal, pubkeyRemote, vMessage, vCiphertext);
}

bool CService::AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage)
{
    return pWallet->AesDecrypt(pubkeyLocal, pubkeyRemote, vCiphertext, vMessage);
}

void CService::GetWalletDestinations(std::set<CDestination>& setDest)
{
    pWallet->GetDestinations(setDest);
}

bool CService::GetContractCodeContext(const uint256& hashFork, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    return pBlockChain->GetContractCodeContext(hashFork, hashLastBlock, hashContractCode, ctxtContractCode);
}

bool CService::ListContractCodeContext(const uint256& hashFork, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    return pBlockChain->ListWasmCreateCodeContext(hashFork, hashLastBlock, txid, mapContractCode);
}

bool CService::ListContractAddress(const uint256& hashFork, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        return false;
    }
    return pBlockChain->ListContractAddress(hashFork, hashLastBlock, mapContractAddress);
}

bool CService::GeDestContractContext(const uint256& hashFork, const CDestination& dest, CContractAddressContext& ctxtContract)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CService", "Get dest contract context: Get last block fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    CAddressContext ctxtAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest.data, ctxtAddress))
    {
        StdLog("CService", "Get dest contract context: Contract dest error1, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!ctxtAddress.IsContract())
    {
        StdLog("CService", "Get dest contract context: Contract dest error2, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!ctxtAddress.GetContractAddressContext(ctxtContract))
    {
        StdLog("CService", "Get dest contract context: Get contract dest fail, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    return true;
}

bool CService::GetContractSource(const uint256& hashFork, const uint256& hashSource, bytes& btSource)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CService", "Get contract source: Get last block fail, source: %s", hashSource.GetHex().c_str());
        return false;
    }
    CTxContractData txcd;
    if (!pBlockChain->GetBlockSourceCodeData(hashFork, hashLastBlock, hashSource, txcd))
    {
        StdLog("CService", "Get contract source: Get source code fail, source: %s", hashSource.GetHex().c_str());
        return false;
    }
    btSource = txcd.GetSource();
    return true;
}

bool CService::GetContractCode(const uint256& hashFork, const uint256& hashCode, bytes& btCode)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CService", "Get contract code: Get last block fail, code: %s", hashCode.GetHex().c_str());
        return false;
    }
    CTxContractData txcd;
    if (!pBlockChain->GetBlockWasmCreateCodeData(hashFork, hashLastBlock, hashCode, txcd))
    {
        StdLog("CService", "Get contract code: Get code fail, code: %s", hashCode.GetHex().c_str());
        return false;
    }
    if (txcd.IsSetup())
    {
        if (!pBlockChain->GetBlockWasmCreateCodeData(hashFork, hashLastBlock, txcd.GetWasmCreateCodeHash(), txcd))
        {
            StdLog("CService", "Get contract code: Get code fail, code: %s", hashCode.GetHex().c_str());
            return false;
        }
    }
    btCode = txcd.GetCode();
    return true;
}

bool CService::GetDestTemplateData(const uint256& hashFork, const CDestination& dest, bytes& btTemplateData)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
    {
        StdLog("CService", "Get dest template data: Retrieve fork last fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    CAddressContext ctxtAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest.data, ctxtAddress))
    {
        StdLog("CService", "Get dest template data: Retrieve address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxtAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdLog("CService", "Get dest template data: Get template address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    btTemplateData = ctxtTemplate.btData;
    return true;
}

bool CService::GetWork(vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                       uint256& hashPrev, uint32& nPrevTime, int& nAlgo,
                       int& nBits, const CTemplateMintPtr& templMint)
{
    CBlock block;
    block.nType = CBlock::BLOCK_PRIMARY;

    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        StdTrace("CService", "GetWork: Get last block status fail");
        return false;
    }
    hashPrev = status.hashBlock;
    nPrevTime = status.nBlockTime;
    nPrevBlockHeight = status.nBlockHeight;
    block.hashPrev = hashPrev;
    block.nNumber = status.nBlockNumber + 1;

    nPrevTime = pCoreProtocol->GetNextBlockTimeStamp(status.nMintType, status.nBlockTime, CTransaction::TX_WORK);
    block.nTimeStamp = std::max((uint32)GetNetTime(), nPrevTime);

    bool fIsDpos = false;
    if (pNetChannel->IsLocalCachePowBlock(nPrevBlockHeight + 1, fIsDpos))
    {
        StdTrace("CService", "GetWork: IsLocalCachePowBlock pow exist");
        return false;
    }
    if (fIsDpos)
    {
        if (block.nTimeStamp < nPrevTime + 5)
        {
            StdTrace("CService", "GetWork: Wait dpos block......");
            return false;
        }
    }

    nAlgo = CM_CRYPTONIGHT;
    if (!pBlockChain->GetProofOfWorkTarget(block.hashPrev, nAlgo, nBits))
    {
        StdError("CService", "GetWork: Get ProofOfWork Target fail");
        return false;
    }

    CProofOfHashWork proof;
    proof.nWeight = 0;
    proof.nAgreement = 0;
    proof.nAlgo = nAlgo;
    proof.nBits = nBits;
    proof.destMint = CDestination(templMint->GetTemplateId());
    proof.nNonce = 0;
    proof.Save(block.vchProof);

    block.GetSerializedProofOfWorkData(vchWorkData);

    if (fIsDpos)
    {
        nAlgo = CM_MPVSS;
    }
    return true;
}

Errno CService::SubmitWork(const vector<unsigned char>& vchWorkData,
                           const CTemplateMintPtr& templMint, crypto::CKey& keyMint, uint256& hashBlock)
{
    if (vchWorkData.empty())
    {
        StdError("CService", "SubmitWork: vchWorkData is empty");
        return FAILED;
    }
    CBlock block;
    CProofOfHashWork proof;
    CBufStream ss;
    ss.Write((const char*)&vchWorkData[0], vchWorkData.size());
    try
    {
        ss >> block.nVersion >> block.nType >> block.nTimeStamp >> block.nNumber >> block.hashPrev >> block.vchProof;
        if (!proof.Load(block.vchProof))
        {
            StdError("CService", "SubmitWork: Load proof fail");
            return ERR_BLOCK_PROOF_OF_WORK_INVALID;
        }
        if (proof.nAlgo != CM_CRYPTONIGHT)
        {
            StdError("CService", "SubmitWork: nAlgo error");
            return ERR_BLOCK_PROOF_OF_WORK_INVALID;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return FAILED;
    }

    int nBits;
    if (!pBlockChain->GetProofOfWorkTarget(block.hashPrev, proof.nAlgo, nBits))
    {
        StdError("CService", "SubmitWork: Get ProofOfWork Target fail");
        return FAILED;
    }

    uint256 nReward = 0;
    if (!pBlockChain->GetBlockMintReward(block.hashPrev, nReward))
    {
        StdError("CService", "SubmitWork: Get block mint reward fail");
        return FAILED;
    }

    CTransaction& txMint = block.txMint;
    txMint.nType = CTransaction::TX_WORK;
    txMint.nTimeStamp = block.nTimeStamp;
    txMint.nTxNonce = block.nNumber;
    txMint.hashFork = pCoreProtocol->GetGenesisBlockHash();
    txMint.to = CDestination(templMint->GetTemplateId());
    txMint.nAmount = nReward;
    if (txMint.nAmount == 0)
    {
        StdError("CService", "SubmitWork: Get block invest total reward fail");
        return FAILED;
    }

    CAddressContext ctxtAddress;
    if (!pBlockChain->RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, txMint.to.data, ctxtAddress))
    {
        txMint.AddTxData(CTransaction::DF_TEMPLATEDATA, templMint->GetTemplateData());
    }

    size_t nSigSize = templMint->GetTemplateData().size() + 64 + 2;
    size_t nMaxTxSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - nSigSize;

    vector<CTransaction> vVoteRewardTx;
    if (!pBlockChain->CalcBlockVoteRewardTx(block.hashPrev, block.nType, block.GetBlockHeight(), block.GetBlockTime(), vVoteRewardTx))
    {
        StdError("CService", "SubmitWork: Get block invest reward fail");
        return FAILED;
    }
    uint256 nVoteRewardTxFee;
    for (const CTransaction& tx : vVoteRewardTx)
    {
        size_t nTxSize = GetSerializeSize(tx);
        if (nMaxTxSize < nTxSize)
        {
            StdError("CService", "SubmitWork: Invest reward tx size error");
            return FAILED;
        }
        block.vtx.push_back(tx);
        nVoteRewardTxFee += tx.GetTxFee();
        nMaxTxSize -= nTxSize;
    }

    uint256 nTotalTxFee;
    if (!pTxPool->FetchArrangeBlockTx(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, block.nTimeStamp, nMaxTxSize, block.vtx, nTotalTxFee))
    {
        StdError("CService", "SubmitWork: Fetch arrange block tx fail");
        return FAILED;
    }
    uint256 nMintCoin(block.txMint.nAmount);
    block.txMint.nAmount += (nTotalTxFee + nVoteRewardTxFee);
    block.hashMerkleRoot = block.CalcMerkleTreeRoot();
    block.nGasLimit = MAX_BLOCK_GAS_LIMIT;

    uint256 nTotalMintReward;
    if (!pBlockChain->CreateBlockStateRoot(pCoreProtocol->GetGenesisBlockHash(), block, block.hashStateRoot, block.hashReceiptsRoot, block.nGasUsed,
                                           block.nBloom, true, false, nTotalMintReward, block.txMint.nAmount))
    {
        StdError("CService", "SubmitWork: Get block state root fail, block: %s", block.GetHash().ToString().c_str());
        return FAILED;
    }

    {
        hnbase::CBufStream ss;
        ss << nMintCoin;
        bytes btTempData;
        ss.GetData(btTempData);
        block.txMint.AddTxData(CTransaction::DF_MINTCOIN, btTempData);
    }

    hashBlock = block.GetHash();
    if (!keyMint.Sign(hashBlock, block.vchSig))
    {
        StdError("CService", "SubmitWork: Sign fail");
        return ERR_BLOCK_SIGNATURE_INVALID;
    }

    Errno err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        StdError("CService", "SubmitWork: ValidateBlock fail");
        return err;
    }

    if (!pNetChannel->AddCacheLocalPowBlock(block))
    {
        StdError("CService", "SubmitWork: AddCacheLocalPowBlock fail");
        return FAILED;
    }
    return OK;
}

////////////////////////////////////////////////////////////////////////
bool CService::SetContractTransaction(const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew, std::string& strErr)
{
    if (txNew.to.IsNullContract())
    {
        if (btFormatData.size() > 0)
        {
            CTxContractData txcdDux;
            try
            {
                hnbase::CBufStream ss(btFormatData);
                ss >> txcdDux;
            }
            catch (std::exception& e)
            {
                strErr = "Contract dux data error";
                StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), txNew.hashFork.GetHex().c_str());
                return false;
            }
            uint256 hashWasmCreateCode = txcdDux.GetWasmCreateCodeHash();
            uint256 hashLastBlock;
            if (!pBlockChain->RetrieveForkLast(txNew.hashFork, hashLastBlock))
            {
                strErr = "Get last block fail";
                StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), txNew.hashFork.GetHex().c_str());
                return false;
            }
            bool fExistWasmCode = true;
            CWasmCreateCodeContext ctxtCode;
            if (!pBlockChain->RetrieveWasmCreateCodeContext(txNew.hashFork, hashLastBlock, hashWasmCreateCode, ctxtCode))
            {
                fExistWasmCode = false;
            }
            if (!txcdDux.IsUpcode())
            {
                if (!fExistWasmCode || ctxtCode.nStatus == CWasmCreateCodeContext::ST_INITIALIZE)
                {
                    if (!fExistWasmCode)
                    {
                        strErr = "Contract not exist";
                    }
                    else
                    {
                        strErr = "Contract not activated";
                    }
                    StdLog("CService", "Set contract transaction: %s, contract hash: %s, fork: %s",
                           strErr.c_str(), hashWasmCreateCode.GetHex().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
            }
            if (txcdDux.IsSetup())
            {
                if (!fExistWasmCode)
                {
                    strErr = "Contract not exist";
                    StdLog("CService", "Set contract transaction: %s, contract hash: %s, fork: %s",
                           strErr.c_str(), hashWasmCreateCode.GetHex().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
            }
            else
            {
                if (fExistWasmCode)
                {
                    txcdDux.SetSetupHash(hashWasmCreateCode);
                }
            }
            hnbase::CBufStream ss;
            ss << txcdDux;
            bytes btCodeData;
            ss.GetData(btCodeData);
            txNew.AddTxData(CTransaction::DF_CONTRACTCODE, btCodeData);
        }
        else
        {
            if (btContractCode.size() == 32)
            {
                bytes btTempData(btContractCode.begin(), btContractCode.end());
                std::reverse(btTempData.begin(), btTempData.end());
                uint256 hashContract(btTempData);

                uint256 hashLastBlock;
                if (!pBlockChain->RetrieveForkLast(txNew.hashFork, hashLastBlock))
                {
                    strErr = "Get last block fail";
                    StdLog("CService", "Set contract transaction: %s, fork: %s",
                           strErr.c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                CWasmCreateCodeContext ctxtCode;
                if (!pBlockChain->RetrieveWasmCreateCodeContext(txNew.hashFork, hashLastBlock, hashContract, ctxtCode))
                {
                    strErr = "Contract not exist";
                    StdLog("CService", "Set contract transaction: %s, contract hash: %s, fork: %s",
                           strErr.c_str(), hashContract.GetHex().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }

                CTxContractData txcd(hashContract);
                hnbase::CBufStream ss;
                ss << txcd;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CONTRACTCODE, btCodeData);
            }
            else if (btContractCode.size() == CDestination::DESTINATION_SIZE)
            {
                CDestination destContract;
                try
                {
                    hnbase::CBufStream ss(btContractCode);
                    ss >> destContract;
                }
                catch (std::exception& e)
                {
                    strErr = "Contract code error";
                    StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                if (!destContract.IsContract() || destContract.IsNullContract())
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error1, dest: %s, fork: %s",
                           destContract.ToString().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }

                uint256 hashLastBlock;
                if (!pBlockChain->RetrieveForkLast(txNew.hashFork, hashLastBlock))
                {
                    strErr = "Get last block fail";
                    StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                CAddressContext ctxtAddress;
                if (!pBlockChain->RetrieveAddressContext(txNew.hashFork, hashLastBlock, destContract.data, ctxtAddress))
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error2, dest: %s, fork: %s",
                           destContract.ToString().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                if (!ctxtAddress.IsContract())
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error3, dest: %s, fork: %s",
                           destContract.ToString().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                CContractAddressContext ctxtContract;
                if (!ctxtAddress.GetContractAddressContext(ctxtContract))
                {
                    strErr = "Get contract dest fail";
                    StdLog("CService", "Set contract transaction: Get contract dest fail, dest: %s, fork: %s",
                           destContract.ToString().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }
                CWasmCreateCodeContext ctxtCode;
                if (!pBlockChain->RetrieveWasmCreateCodeContext(txNew.hashFork, hashLastBlock, ctxtContract.hashWasmCreateCode, ctxtCode))
                {
                    strErr = "Contract not exist";
                    StdLog("CService", "Set contract transaction: Contract not exist, contract hash: %s, fork: %s",
                           ctxtContract.hashWasmCreateCode.GetHex().c_str(), txNew.hashFork.GetHex().c_str());
                    return false;
                }

                CTxContractData txcd(ctxtContract.hashWasmCreateCode);
                hnbase::CBufStream ss;
                ss << txcd;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CONTRACTCODE, btCodeData);
            }
            else if (btContractCode.size() > CDestination::DESTINATION_SIZE)
            {
                uint256 hashWasmCreateCode = metabasenet::crypto::CryptoHash(btContractCode.data(), btContractCode.size());

                uint256 hashLastBlock;
                if (!pBlockChain->RetrieveForkLast(txNew.hashFork, hashLastBlock))
                {
                    strErr = "Get last block fail";
                    StdLog("CService", "Set contract transaction: Get last block fail, fork: %s", txNew.hashFork.GetHex().c_str());
                    return false;
                }
                CWasmCreateCodeContext ctxtCode;
                if (pBlockChain->RetrieveWasmCreateCodeContext(txNew.hashFork, hashLastBlock, hashWasmCreateCode, ctxtCode))
                {
                    CTxContractData txcd(hashWasmCreateCode);
                    hnbase::CBufStream ss;
                    ss << txcd;
                    bytes btCodeData;
                    ss.GetData(btCodeData);
                    txNew.AddTxData(CTransaction::DF_CONTRACTCODE, btCodeData);
                }
                else
                {
                    bytes btCompressCode;
                    if (!BtCompress(btContractCode, btCompressCode) || btCompressCode.size() == 0)
                    {
                        strErr = "Contract code compress fail";
                        StdLog("CService", "Set contract transaction: Contract code compress fail, fork: %s", txNew.hashFork.GetHex().c_str());
                        return false;
                    }
                    CTxContractData txcd(btCompressCode, true);
                    //CTxContractData txcd(btContractCode, false);
                    hnbase::CBufStream ss;
                    ss << txcd;
                    bytes btCodeData;
                    ss.GetData(btCodeData);
                    txNew.AddTxData(CTransaction::DF_CONTRACTCODE, btCodeData);
                }
            }
            else
            {
                strErr = "Contract data error";
                StdError("CService", "Set contract transaction: Contract data error");
                return false;
            }
        }
    }
    if (btContractParam.size() > 0)
    {
        txNew.AddTxData(CTransaction::DF_CONTRACTPARAM, btContractParam);
    }
    return true;
}

bool CService::SetTemplateTransaction(const bytes& btFormatData, const bytes& vchData, const bytes& btToData, CTransaction& txNew, std::string& strErr)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(txNew.hashFork, hashLastBlock))
    {
        strErr = "fork error";
        StdError("CService", "Set template transaction: find fork fail, fork: %s", txNew.hashFork.GetHex().c_str());
        return false;
    }
    CAddressContext ctxtAddress;
    if (!pBlockChain->RetrieveAddressContext(txNew.hashFork, hashLastBlock, txNew.to.data, ctxtAddress))
    {
        CTemplatePtr ptr = pWallet->GetTemplate(txNew.to.GetTemplateId());
        if (ptr == nullptr)
        {
            bool fRet = true;
            if (btToData.empty())
            {
                fRet = false;
            }
            else
            {
                ptr = CTemplate::Import(btToData);
                if (ptr == nullptr)
                {
                    fRet = false;
                }
                else
                {
                    if (ptr->GetTemplateId() != txNew.to.GetTemplateId())
                    {
                        fRet = false;
                    }
                }
            }
            if (!fRet)
            {
                strErr = "to address no template data";
                StdError("CService", "Set template transaction: to address no template data, txid: %s", txNew.GetHash().ToString().c_str());
                return false;
            }
        }
        txNew.AddTxData(CTransaction::DF_TEMPLATEDATA, ptr->GetTemplateData());
    }
    if (txNew.to.GetTemplateId().GetType() == TEMPLATE_FORK)
    {
        if (vchData.size() > 0)
        {
            txNew.AddTxData(CTransaction::DF_FORKDATA, vchData);
        }
    }
    else if (txNew.to.GetTemplateId().GetType() == TEMPLATE_ACTIVATECODE)
    {
        if (vchData.size() == 0)
        {
            strErr = "no activate code template data";
            StdLog("CService", "Set template transaction: no activatecode template data, txid: %s", txNew.GetHash().ToString().c_str());
            return false;
        }
        uint256 hashCode;
        try
        {
            CBufStream ss(vchData);
            ss >> hashCode;
        }
        catch (std::exception& e)
        {
            strErr = "activate code data error";
            StdLog("CService", "Set template transaction: activatecode data error, txid: %s", txNew.GetHash().ToString().c_str());
            return false;
        }
        CWasmCreateCodeContext ctxtCode;
        if (!pBlockChain->RetrieveWasmCreateCodeContext(txNew.hashFork, hashLastBlock, hashCode, ctxtCode))
        {
            strErr = "code not exist";
            StdLog("CService", "Set template transaction: code not exist, code: %s, txid: %s",
                   hashCode.ToString().c_str(), txNew.GetHash().ToString().c_str());
            return false;
        }
        if (ctxtCode.nStatus != 0)
        {
            strErr = "code is activated";
            StdLog("CService", "Set template transaction: code is activated, code: %s, txid: %s",
                   hashCode.ToString().c_str(), txNew.GetHash().ToString().c_str());
            return false;
        }
        txNew.AddTxData(CTransaction::DF_ACTIVATECODE, vchData);
    }
    else
    {
        if (vchData.size() > 0)
        {
            txNew.AddTxData(CTransaction::DF_COMMON, vchData);
        }
    }
    return true;
}

} // namespace metabasenet

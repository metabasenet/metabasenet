// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "service.h"

#include "devcommon/util.h"
#include "event.h"
#include "libethcore/TransactionBase.h"
#include "template/activatecode.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/pledge.h"
#include "template/vote.h"

using namespace std;
using namespace mtbase;
using namespace dev;
using namespace dev::eth;
using namespace metabasenet::crypto;

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

CChainId CService::GetChainId()
{
    return pCoreProtocol->GetGenesisChainId();
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

bool CService::GetForkRpcPort(const uint256& hashFork, uint16& nRpcPort)
{
    auto pRpcSrv = RPCServerConfig();
    if (!pRpcSrv)
    {
        return false;
    }
    CForkContext ctxFork;
    if (!pBlockChain->GetForkContext(hashFork, ctxFork))
    {
        return false;
    }
    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        nRpcPort = pRpcSrv->nRPCPort;
        return true;
    }
    for (auto& vd : pRpcSrv->vecChainIdRpcPort)
    {
        if (vd.first == ctxFork.nChainId)
        {
            nRpcPort = vd.second;
            return true;
        }
    }
    return false;
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
    std::map<uint32, uint256> mapForkIndex;
    for (const auto& kv : mapForkCtxt)
    {
        mapForkIndex.insert(std::make_pair(kv.second.nJointHeight, kv.first));
    }
    for (auto& kv : mapForkIndex)
    {
        if (fAll || pForkManager->IsAllowed(kv.second))
        {
            auto it = mapForkCtxt.find(kv.second);
            if (it != mapForkCtxt.end())
            {
                vFork.push_back(make_pair(it->first, it->second.GetProfile()));
            }
        }
    }
}

bool CService::GetForkContext(const uint256& hashFork, CForkContext& ctxtFork, const uint256& hashMainChainRefBlock)
{
    return pBlockChain->GetForkContext(hashFork, ctxtFork, hashMainChainRefBlock);
}

bool CService::VerifyForkNameAndChainId(const uint256& hashFork, const CChainId nChainIdIn, const std::string& strForkName, const uint256& hashBlock)
{
    return pBlockChain->VerifyForkNameAndChainId(hashFork, nChainIdIn, strForkName, hashBlock);
}

bool CService::GetForkGenealogy(const uint256& hashFork, vector<pair<uint256, int>>& vAncestry, vector<pair<int, uint256>>& vSubline)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!pBlockChain->ListForkContext(mapForkCtxt))
    {
        return false;
    }

    auto getJoint = [&](const uint256& hashForkIn, uint256& hashParentOut, uint256& hashJointOut, int& nJointHeightOut) -> bool {
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

bool CService::GetBlockLocation(const uint256& hashBlock, CChainId& nChainId, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockLocation(hashBlock, nChainId, hashFork, nHeight);
}

int CService::GetBlockCount(const uint256& hashFork)
{
    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        return (GetForkHeight(hashFork) + 1);
    }
    return pBlockChain->GetBlockCount(hashFork);
}

bool CService::GetBlockHashByHeightSlot(const uint256& hashFork, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock)
{
    return pBlockChain->GetBlockHashByHeightSlot(hashFork, nHeight, nSlot, hashBlock);
}

bool CService::GetBlockHashList(const uint256& hashFork, const uint32 nHeight, vector<uint256>& vBlockHash)
{
    return pBlockChain->GetBlockHashList(hashFork, nHeight, vBlockHash);
}

bool CService::GetBlockNumberHash(const uint256& hashFork, const uint64 nNumber, uint256& hashBlock)
{
    return pBlockChain->GetBlockNumberHash(hashFork, nNumber, hashBlock);
}

bool CService::GetBlock(const uint256& hashBlock, CBlock& block, CChainId& nChainId, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlock(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, nChainId, hashFork, nHeight);
}

bool CService::GetBlockEx(const uint256& hashBlock, CBlockEx& block, CChainId& nChainId, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockEx(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, nChainId, hashFork, nHeight);
}

bool CService::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, uint64& nTime)
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

bool CService::GetTransactionAndPosition(const uint256& hashRefFork, const uint256& txid, CTransaction& tx, uint256& hashAtFork, uint256& hashAtBlock, uint64& nBlockNumber, uint16& nTxSeq)
{
    if (pTxPool->Get(hashRefFork, txid, tx, hashAtFork))
    {
        hashAtBlock = 0;
        nBlockNumber = 0;
        nTxSeq = 0;
    }
    else
    {
        CTxIndex txIndex;
        if (!pBlockChain->GetTransactionAndIndex(hashRefFork, txid, tx, hashAtFork, txIndex))
        {
            StdLog("CService", "Get Transaction: Get transaction and index fail, txid: %s", txid.GetHex().c_str());
            return false;
        }
        if (!pBlockChain->GetBlockNumberHash(hashAtFork, txIndex.nBlockNumber, hashAtBlock))
        {
            StdLog("CService", "Get Transaction: Get block hash fail, fork: %s, number: %lu, txid: %s",
                   hashAtFork.GetHex().c_str(), txIndex.nBlockNumber, txid.GetHex().c_str());
            return false;
        }
        nBlockNumber = txIndex.nBlockNumber;
        nTxSeq = txIndex.nTxSeq;
    }
    return true;
}

bool CService::GetTransactionByIndex(const uint256& hashBlock, const uint64 nTxIndex, CTransaction& tx, uint64& nBlockNumber)
{
    CBlockEx block;
    CChainId nChainId;
    uint256 tempHashFork;
    int tempHeight = 0;
    if (!GetBlockEx(hashBlock, block, nChainId, tempHashFork, tempHeight))
    {
        StdLog("CService", "Get Transaction: Get blockex fail, block: %s",
               hashBlock.GetHex().c_str());
        return false;
    }
    if (nTxIndex == 0)
    {
        tx = block.txMint;
    }
    else
    {
        if (nTxIndex - 1 >= block.vtx.size())
        {
            StdLog("CService", "Get Transaction: nTxIndex error, nTxIndex: %ld, block: %s",
                   nTxIndex, hashBlock.GetHex().c_str());
            return false;
        }
        tx = block.vtx[nTxIndex - 1];
    }
    nBlockNumber = block.GetBlockNumber();
    return true;
}

Errno CService::SendTransaction(const uint256& hashFork, CTransaction& tx)
{
    return pDispatcher->AddNewTx(hashFork, tx);
}

bool CService::GetVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes, string& strFailCause)
{
    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), hashRefBlock, destDelegate, ctxAddress))
    {
        strFailCause = "Retrieve address context failed";
        return false;
    }
    if (ctxAddress.IsTemplate() && ctxAddress.GetTemplateType() == TEMPLATE_DELEGATE)
    {
        if (!pBlockChain->GetDelegateVotes(hashRefBlock, destDelegate, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    else if (ctxAddress.IsTemplate() && (ctxAddress.GetTemplateType() == TEMPLATE_VOTE || ctxAddress.GetTemplateType() == TEMPLATE_PLEDGE))
    {
        CTemplateAddressContext ctxTemplate;
        if (!ctxAddress.GetTemplateAddressContext(ctxTemplate))
        {
            strFailCause = "Template address data error";
            return false;
        }
        CTemplatePtr ptr = CTemplate::Import(ctxTemplate.btData);
        if (ptr == nullptr || (ptr->GetTemplateType() != TEMPLATE_VOTE && ptr->GetTemplateType() != TEMPLATE_PLEDGE))
        {
            strFailCause = "Template address import error";
            return false;
        }
        CDestination destDelegateTemplateOut;
        if (ptr->GetTemplateType() == TEMPLATE_VOTE)
        {
            destDelegateTemplateOut = boost::dynamic_pointer_cast<CTemplateVote>(ptr)->destDelegate;
            if (destDelegateTemplateOut.IsNull())
            {
                strFailCause = "Vote template address not imported";
                return false;
            }
        }
        else
        {
            destDelegateTemplateOut = boost::dynamic_pointer_cast<CTemplatePledge>(ptr)->destDelegate;
            if (destDelegateTemplateOut.IsNull())
            {
                strFailCause = "Pledge template address not imported";
                return false;
            }
        }
        if (!pBlockChain->GetDelegateVotes(hashRefBlock, destDelegateTemplateOut, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    else
    {
        strFailCause = "Not a delegate or vote template address";
        return false;
    }
    return true;
}

bool CService::ListDelegate(const uint256& hashRefBlock, const uint32 nStartIndex, const uint32 nCount, std::multimap<uint256, CDestination>& mapVotes)
{
    return pBlockChain->ListDelegate(hashRefBlock, nStartIndex, nCount, mapVotes);
}

bool CService::GetDelegateVotes(const uint256& hashRefBlock, const CDestination& destDelegate, uint256& nVotes)
{
    return pBlockChain->GetDelegateVotes(hashRefBlock, destDelegate, nVotes);
}

bool CService::GetUserVotes(const uint256& hashRefBlock, const CDestination& destVote, uint32& nTemplateType, uint256& nVotes, uint32& nUnlockHeight)
{
    return pBlockChain->GetUserVotes(hashRefBlock, destVote, nTemplateType, nVotes, nUnlockHeight);
}

bool CService::GetTransactionReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceiptEx& receiptex)
{
    return pBlockChain->GetTransactionReceipt(hashFork, txid, receiptex);
}

bool CService::CallContract(const bool fEthCall, const uint256& hashFork, const uint256& hashBlock, const CDestination& from, const CDestination& to, const uint256& nAmount, const uint256& nGasPrice,
                            const uint256& nGas, const bytes& btContractParam, uint256& nUsedGas, uint64& nGasLeft, int& nStatus, bytes& btResult)
{
    return pBlockChain->CallContract(fEthCall, hashFork, hashBlock, from, to, nAmount, nGasPrice, nGas, btContractParam, nUsedGas, nGasLeft, nStatus, btResult);
}

bool CService::GetForkHashByChainId(const CChainId nChainIdIn, uint256& hashFork)
{
    uint256 hashLastBlock;
    if (!pBlockChain->RetrieveForkLast(pCoreProtocol->GetGenesisBlockHash(), hashLastBlock))
    {
        return false;
    }
    return pBlockChain->GetForkHashByChainId(nChainIdIn, hashFork, hashLastBlock);
}

bool CService::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint256& key, bytes& value)
{
    return pBlockChain->RetrieveContractKvValue(hashFork, hashBlock, dest, key, value);
}

uint256 CService::AddLogsFilter(const uint256& hashClient, const uint256& hashFork, const CLogsFilter& logsFilter)
{
    return pBlockChain->AddLogsFilter(hashClient, hashFork, logsFilter);
}

void CService::RemoveFilter(const uint256& nFilterId)
{
    pBlockChain->RemoveFilter(nFilterId);
}

bool CService::GetTxReceiptLogsByFilterId(const uint256& nFilterId, const bool fAll, ReceiptLogsVec& receiptLogs)
{
    return pBlockChain->GetTxReceiptLogsByFilterId(nFilterId, fAll, receiptLogs);
}

bool CService::GetTxReceiptsByLogsFilter(const uint256& hashFork, const CLogsFilter& logsFilter, ReceiptLogsVec& vReceiptLogs)
{
    return pBlockChain->GetTxReceiptsByLogsFilter(hashFork, logsFilter, vReceiptLogs);
}

uint256 CService::AddBlockFilter(const uint256& hashClient, const uint256& hashFork)
{
    return pBlockChain->AddBlockFilter(hashClient, hashFork);
}

bool CService::GetFilterBlockHashs(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vBlockHash)
{
    return pBlockChain->GetFilterBlockHashs(hashFork, nFilterId, fAll, vBlockHash);
}

uint256 CService::AddPendingTxFilter(const uint256& hashClient, const uint256& hashFork)
{
    return pBlockChain->AddPendingTxFilter(hashClient, hashFork);
}

bool CService::GetFilterTxids(const uint256& hashFork, const uint256& nFilterId, const bool fAll, std::vector<uint256>& vTxid)
{
    return pBlockChain->GetFilterTxids(hashFork, nFilterId, fAll, vTxid);
}

bool CService::HaveKey(const CDestination& dest, const int32 nVersion)
{
    return pWallet->HaveKey(dest, nVersion);
}

bool CService::GetPubkey(const CDestination& dest, crypto::CPubKey& pubkey) const
{
    return pWallet->GetPubkey(dest, pubkey);
}

size_t CService::GetPubKeys(set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount)
{
    return pWallet->GetPubKeys(setPubKey, nPos, nCount);
}

bool CService::GetKeyStatus(const CDestination& dest, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic)
{
    return pWallet->GetKeyStatus(dest, nVersion, fLocked, nAutoLockTime, fPublic);
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

boost::optional<std::string> CService::RemoveKey(const CDestination& dest)
{
    return pWallet->RemoveKey(dest);
}

bool CService::ImportKey(const vector<unsigned char>& vchKey, crypto::CPubKey& pubkey)
{
    return pWallet->Import(vchKey, pubkey);
}

bool CService::ExportKey(const CDestination& dest, vector<unsigned char>& vchKey)
{
    return pWallet->Export(dest, vchKey);
}

bool CService::EncryptKey(const CDestination& dest, const crypto::CCryptoString& strPassphrase,
                          const crypto::CCryptoString& strCurrentPassphrase)
{
    return pWallet->Encrypt(dest, strPassphrase, strCurrentPassphrase);
}

bool CService::Lock(const CDestination& dest)
{
    return pWallet->Lock(dest);
}

bool CService::Unlock(const CDestination& dest, const crypto::CCryptoString& strPassphrase, int64 nTimeout)
{
    return pWallet->Unlock(dest, strPassphrase, nTimeout);
}

bool CService::GetBalance(const uint256& hashFork, const uint256& hashLastBlock, const CDestination& dest, CWalletBalance& balance)
{
    CAddressContext ctxAddress;
    pTxPool->GetDestBalance(hashFork, dest, balance.nDestType, balance.nTemplateType, balance.nTxNonce, balance.nAvailable, balance.nUnconfirmedIn, balance.nUnconfirmedOut, ctxAddress, hashLastBlock);
    if (balance.nAvailable > 0)
    {
        uint256 hashRefBlock;
        if (hashLastBlock == 0)
        {
            if (!pBlockChain->RetrieveForkLast(hashFork, hashRefBlock))
            {
                StdLog("CService", "Get balance: Retrieve fork last fail, fork: %s", hashFork.ToString().c_str());
                return false;
            }
        }
        else
        {
            hashRefBlock = hashLastBlock;
        }

        if (!pBlockChain->GetAddressLockedAmount(hashFork, hashRefBlock, dest, ctxAddress, balance.nAvailable, balance.nLocked))
        {
            StdLog("CService", "Get balance: Get address locked amount fail, dest: %s", dest.ToString().c_str());
            return false;
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

bool CService::SignSignature(const CDestination& dest, const uint256& hash, vector<unsigned char>& vchSig)
{
    return pWallet->Sign(dest, hash, vchSig);
}

bool CService::SignTransaction(const uint256& hashFork, CTransaction& tx)
{
    int32 nForkHeight;
    uint256 hashLastBlock;
    if (!GetForkLastBlock(hashFork, nForkHeight, hashLastBlock))
    {
        StdError("CService", "Sign Transaction: GetForkLastBlock fail, txid: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        CAddressContext ctxAddress;
        if (pBlockChain->GetTxToAddressContext(hashFork, hashLastBlock, tx, ctxAddress)
            && ctxAddress.IsTemplate() && ctxAddress.GetTemplateType() == TEMPLATE_FORK)
        {
            vector<pair<CDestination, CForkContext>> vForkCtxt;
            if (!pBlockChain->VerifyBlockForkTx(hashLastBlock, tx, vForkCtxt) || vForkCtxt.empty())
            {
                StdError("CService", "Sign Transaction: Verify block fork tx fail, txid: %s", tx.GetHash().GetHex().c_str());
                return false;
            }
        }
    }

    if (!pWallet->SignTransaction(hashFork, hashLastBlock, tx))
    {
        StdError("CService", "Sign ransaction: Sign Transaction fail, txid: %s, from: %s",
                 tx.GetHash().GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return false;
    }
    return true;
}

bool CService::HaveTemplate(const CDestination& dest)
{
    return pWallet->HaveTemplate(dest);
}

void CService::GetTemplateIds(set<pair<uint8, CTemplateId>>& setTid, const uint64 nPos, const uint64 nCount)
{
    pWallet->GetTemplateIds(setTid, nPos, nCount);
}

bool CService::AddTemplate(CTemplatePtr& ptr)
{
    return pWallet->AddTemplate(ptr);
}

CTemplatePtr CService::GetTemplate(const CDestination& dest)
{
    return pWallet->GetTemplate(dest);
}

bool CService::RemoveTemplate(const CDestination& dest)
{
    return pWallet->RemoveTemplate(dest);
}

bool CService::ListTransaction(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, const uint64 nOffset, const uint64 nCount, const bool fReverse, vector<CDestTxInfo>& vTx)
{
    return pBlockChain->ListAddressTxInfo(hashFork, hashRefBlock, dest, nOffset, nCount, fReverse, vTx);
}

boost::optional<std::string> CService::CreateTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const bytes& btToData,
                                                         const uint256& nAmount, const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData,
                                                         const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew)
{
    auto funcReturnError = [&](const std::string& strErr) -> std::string {
        StdError("CService", "Create transaction: %s, fork: %s", strErr.c_str(), hashFork.GetHex().c_str());
        return strErr + std::string(", fork: ") + hashFork.GetHex();
    };

    CBlockStatus statusFork;
    if (!pBlockChain->GetLastBlockStatus(hashFork, statusFork))
    {
        return funcReturnError("Get fork status fail");
    }

    uint64 nNewTxNonce = 0;
    if (nNonce == 0)
    {
        nNewTxNonce = pTxPool->GetDestNextTxNonce(hashFork, destFrom);
        if (nNewTxNonce == 0)
        {
            return funcReturnError("Get tx nonce fail");
        }
    }
    else
    {
        nNewTxNonce = nNonce;
    }

    CForkContext ctxFork;
    if (!pBlockChain->GetForkContext(hashFork, ctxFork))
    {
        return funcReturnError("Get fork context fail");
    }
    if (ctxFork.nChainId == 0)
    {
        return funcReturnError("chainid is 0");
    }

    txNew.SetNull();

    txNew.SetTxType(CTransaction::TX_TOKEN);
    txNew.SetChainId(ctxFork.nChainId);
    txNew.SetNonce(nNewTxNonce);
    txNew.SetFromAddress(destFrom);
    txNew.SetToAddress(destTo);
    txNew.SetAmount(nAmount);
    txNew.SetGasPrice(nGasPrice);
    txNew.SetGasLimit(nGas);

    if (txNew.GetGasPrice() == 0)
    {
        txNew.SetGasPrice(pBlockChain->GetForkMintMinGasPrice(hashFork));
    }
    if (txNew.GetGasLimit() == 0)
    {
        txNew.SetGasLimit(DEF_TX_GAS_LIMIT);
    }

    bool fToContractAddress = false;
    uint8 nToTemplateType = 0;
    if (txNew.GetToAddress().IsNull())
    {
        // Create contract or template
        fToContractAddress = true;
    }
    else
    {
        CAddressContext ctxAddress;
        if (!pBlockChain->RetrieveAddressContext(hashFork, statusFork.hashBlock, txNew.GetToAddress(), ctxAddress))
        {
            CTemplatePtr ptr = pWallet->GetTemplate(txNew.GetToAddress());
            if (ptr == nullptr)
            {
                if (!btToData.empty())
                {
                    ptr = CTemplate::Import(btToData);
                    if (ptr && txNew.GetToAddress() != CDestination(ptr->GetTemplateId()))
                    {
                        ptr = nullptr;
                    }
                }
            }
            if (ptr)
            {
                CTemplateAddressContext ctxTemplate(string(), string(), ptr->GetTemplateType(), ptr->Export());
                txNew.SetToAddressData(CAddressContext(ctxTemplate));
                nToTemplateType = ptr->GetTemplateType();
            }
            else
            {
                txNew.SetToAddressData(CAddressContext(CPubkeyAddressContext()));
            }
        }
        else
        {
            if (ctxAddress.IsPubkey())
            {
                CTemplatePtr ptr = pWallet->GetTemplate(txNew.GetToAddress());
                if (ptr == nullptr)
                {
                    if (!btToData.empty())
                    {
                        ptr = CTemplate::Import(btToData);
                        if (ptr && txNew.GetToAddress() != CDestination(ptr->GetTemplateId()))
                        {
                            ptr = nullptr;
                        }
                    }
                }
                if (ptr)
                {
                    // When the address type in the database is inconsistent with the address type in the wallet, it is necessary to change the address data again to correct the address data error.
                    CTemplateAddressContext ctxTemplate(string(), string(), ptr->GetTemplateType(), ptr->Export());
                    ctxAddress = CAddressContext(ctxTemplate);
                    txNew.SetToAddressData(ctxAddress);
                }
            }
            if (ctxAddress.IsTemplate())
            {
                nToTemplateType = ctxAddress.GetTemplateType();
            }
            else if (ctxAddress.IsContract())
            {
                fToContractAddress = true;
            }
        }
    }

    if (fToContractAddress)
    {
        std::string strErr;
        if (!SetContractTransaction(hashFork, statusFork.hashBlock, btFormatData, btContractCode, btContractParam, txNew, strErr))
        {
            return strErr;
        }
    }
    else if (nToTemplateType != 0)
    {
        std::string strErr;
        if (!SetTemplateTransaction(hashFork, statusFork.hashBlock, nToTemplateType, btFormatData, btData, btToData, txNew, strErr))
        {
            return strErr;
        }
    }
    else
    {
        if (btData.size() > 0)
        {
            txNew.AddTxData(CTransaction::DF_COMMON, btData);
        }
    }

    uint256 nTvGas;
    if (!txNew.GetFromAddress().IsNull())
    {
        CAddressContext ctxFromAddress;
        if (!pTxPool->GetAddressContext(hashFork, txNew.GetFromAddress(), ctxFromAddress))
        {
            return funcReturnError(std::string("From address error, from: ") + txNew.GetFromAddress().ToString());
        }
        if (ctxFromAddress.IsPubkey())
        {
            CTimeVault tv;
            if (!pBlockChain->RetrieveTimeVault(hashFork, statusFork.hashBlock, txNew.GetFromAddress(), tv))
            {
                tv.SetNull();
            }
            uint256 nTvFee = tv.EstimateTransTvGasFee(GetNetTime() + ESTIMATE_TIME_VAULT_TS, txNew.GetAmount());
            CTimeVault::CalcRealityTvGasFee(txNew.GetGasPrice(), nTvFee, nTvGas);
        }
    }

    uint256 nNeedGas = txNew.GetTxBaseGas() + nTvGas;
    if (!fToContractAddress && nGas == 0)
    {
        txNew.SetGasLimit(nNeedGas);
    }
    else
    {
        if (txNew.GetGasLimit() < nNeedGas)
        {
            return funcReturnError(std::string("Gas not enough, need gas: 0x") + nNeedGas.GetHex());
        }
    }

    return boost::optional<std::string>{};
}

boost::optional<std::string> CService::SignEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                                                          const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid, bytes& btSignTxData)
{
    auto funcReturnError = [&](const std::string& strErr) -> std::string {
        StdError("CService", "Create eth transaction: %s, fork: %s", strErr.c_str(), hashFork.GetHex().c_str());
        return strErr + std::string(", fork: ") + hashFork.GetHex();
    };

    if (destFrom.IsNull())
    {
        return funcReturnError("From address is null");
    }

    uint64 nNonceUse = nNonce;
    if (nNonceUse == 0)
    {
        nNonceUse = pTxPool->GetDestNextTxNonce(hashFork, destFrom);
        if (nNonceUse == 0)
        {
            return funcReturnError("Get address nonce fail");
        }
    }

    CForkContext ctxFork;
    if (!pBlockChain->GetForkContext(hashFork, ctxFork, 0))
    {
        return funcReturnError("Get fork context fail");
    }

    CBlockStatus statusFork;
    if (!pBlockChain->GetLastBlockStatus(hashFork, statusFork))
    {
        return funcReturnError("Get fork status fail");
    }
    StdDebug("CService", "Sign Eth Transaction: nChainId: %d", ctxFork.nChainId);

    crypto::CEthTxSkeleton ets;

    ets.chainId = ctxFork.nChainId;
    ets.creation = destTo.IsNull();
    ets.from = static_cast<uint160>(destFrom);
    ets.to = static_cast<uint160>(destTo);
    ets.value = nAmount;
    ets.data = btData;
    ets.nonce = nNonceUse;
    if (nGasPrice == 0)
        ets.gasPrice = pBlockChain->GetForkMintMinGasPrice(hashFork);
    else
        ets.gasPrice = nGasPrice;

    uint256 nTvGas;
    if (!destFrom.IsNull())
    {
        CAddressContext ctxFromAddress;
        if (!pTxPool->GetAddressContext(hashFork, destFrom, ctxFromAddress))
        {
            return funcReturnError(std::string("From address error, from: ") + destFrom.ToString());
        }
        if (ctxFromAddress.IsPubkey())
        {
            CTimeVault tv;
            if (!pBlockChain->RetrieveTimeVault(hashFork, statusFork.hashBlock, destFrom, tv))
            {
                tv.SetNull();
            }
            uint256 nTvFee = tv.EstimateTransTvGasFee(GetNetTime() + ESTIMATE_TIME_VAULT_TS, nAmount);
            CTimeVault::CalcRealityTvGasFee(ets.gasPrice, nTvFee, nTvGas);
            if (nTvGas == 0)
            {
                nTvGas = 1;
            }
        }
    }

    uint256 nNeedGas = CTransaction::GetTxBaseGasStatic(btData.size()) + nTvGas + nAddGas;
    if (nGas == 0)
    {
        ets.gas = nNeedGas;
    }
    else
    {
        if (nGas < nNeedGas)
        {
            return funcReturnError(std::string("Gas not enough, need gas: 0x") + nNeedGas.GetHex());
        }
        ets.gas = nGas;
    }

    if (!pWallet->SignEthTx(destFrom, ets, txid, btSignTxData))
    {
        return funcReturnError("Sign eth tx fail");
    }
    return boost::optional<std::string>{};
}

bool CService::SendEthRawTransaction(const bytes& btRawTxData, uint256& txid)
{
    try
    {
        TransactionBase txEth(btRawTxData, CheckTransaction::Everything);

        // StdDebug("CService", "eth tx hash: 0x%s", txEth.sha3().hex().c_str());
        // StdDebug("CService", "chainId: %lu", txEth.getChainId());
        // StdDebug("CService", "nonce: %ld", (uint64)(txEth.nonce()));
        // StdDebug("CService", "from: %s", address_to_hex(txEth.from()).c_str());
        // StdDebug("CService", "to: %s", address_to_hex(txEth.to()).c_str());
        // StdDebug("CService", "value: %s", u256_to_hex(txEth.value()).c_str());
        // StdDebug("CService", "gas: %s", u256_to_hex(txEth.gas()).c_str());
        // StdDebug("CService", "gasPrice: %s", u256_to_hex(txEth.gasPrice()).c_str());
        // StdDebug("CService", "data: %s", ToHexString(txEth.data()).c_str());
        // StdDebug("CService", "sig: %s", ToHexString(txEth.getSignature()).c_str());

        // StdDebug("CService", "rlp: %s", ToHexString(txEth.rlp()).c_str());

        // if (nNonce == 0)
        // {
        //     nNonce = pTxPool->GetDestNextTxNonce(pCoreProtocol->GetGenesisBlockHash(), destFrom);
        //     if (nNonce == 0)
        //     {
        //         StdError("CService", "Send eth raw tx: Nonce error");
        //         return false;
        //     }
        // }

        uint256 hashFork;
        uint64 nChainId = txEth.getChainId();
        if (/*nChainId == 0 || nChainId == 1 ||*/ nChainId == pCoreProtocol->GetGenesisChainId())
        {
            hashFork = pCoreProtocol->GetGenesisBlockHash();
        }
        else
        {
            if (!pBlockChain->GetForkHashByChainId((CChainId)nChainId, hashFork))
            {
                StdError("CService", "Send eth raw tx: Get fork hash error");
                return false;
            }
        }

        bool fLinkToPubkeyAddress = false;
        if (txEth.to() != ZeroAddress)
        {
            CDestination destTo(address_to_hex(txEth.to()));
            CAddressContext ctxAddress;
            if (!pBlockChain->RetrieveAddressContext(hashFork, uint256(), destTo, ctxAddress))
            {
                fLinkToPubkeyAddress = true;
            }
        }

        CTransaction tx;
        if (!tx.SetEthTx(btRawTxData, fLinkToPubkeyAddress))
        {
            StdError("CService", "Send eth raw tx: Nonce error");
            return false;
        }
        txid = tx.GetHash();

        CTransactionReceiptEx receiptex;
        if (pBlockChain->GetTransactionReceipt(hashFork, txid, receiptex))
        {
            StdLog("CService", "Send eth raw tx: tx is existed, txid: %s", txid.GetHex().c_str());
            return true;
        }

        if (SendTransaction(hashFork, tx) != OK)
        {
            StdError("CService", "Send eth raw tx: send tx fail");
            return false;
        }

        StdLog("CService", "Send eth raw tx success, txid: %s", txid.GetHex().c_str());
    }
    catch (exception& e)
    {
        StdError("CService", "Send eth raw tx: Parse eth raw tx fail: %s", e.what());
        return false;
    }
    return true;
}

bool CService::SendEthTransaction(const uint256& hashFork, const CDestination& destFrom, const CDestination& destTo, const uint256& nAmount,
                                  const uint64 nNonce, const uint256& nGasPrice, const uint256& nGas, const bytes& btData, const uint64 nAddGas, uint256& txid)
{
    bytes btSignTxData;
    auto strErr = SignEthTransaction(hashFork, destFrom, destTo, nAmount, nNonce, nGasPrice, nGas, btData, nAddGas, txid, btSignTxData);
    if (strErr)
    {
        StdLog("CRPCMod", "Send eth transaction: Failed to create transaction: %s", strErr->c_str());
        return false;
    }
    return SendEthRawTransaction(btSignTxData, txid);
}

void CService::GetWalletDestinations(std::set<CDestination>& setDest)
{
    pWallet->GetDestinations(setDest);
}

bool CService::GetForkContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashContractCode, CContractCodeContext& ctxtContractCode)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            return false;
        }
    }
    return pBlockChain->GetForkContractCodeContext(hashFork, hashLastBlock, hashContractCode, ctxtContractCode);
}

bool CService::ListContractCodeContext(const uint256& hashFork, const uint256& hashRefBlock, const uint256& txid, std::map<uint256, CContractCodeContext>& mapContractCode)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            return false;
        }
    }
    return pBlockChain->ListContractCreateCodeContext(hashFork, hashLastBlock, txid, mapContractCode);
}

bool CService::ListContractAddress(const uint256& hashFork, const uint256& hashRefBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            return false;
        }
    }
    return pBlockChain->ListContractAddress(hashFork, hashLastBlock, mapContractAddress);
}

bool CService::RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    return pBlockChain->RetrieveTimeVault(hashFork, hashBlock, dest, tv);
}

bool CService::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return pBlockChain->GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CService::GeDestContractContext(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, CContractAddressContext& ctxtContract)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            StdLog("CService", "Get dest contract context: Get last block fail, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
    }
    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest, ctxAddress))
    {
        StdLog("CService", "Get dest contract context: Contract dest error1, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!ctxAddress.IsContract())
    {
        StdLog("CService", "Get dest contract context: Contract dest error2, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!ctxAddress.GetContractAddressContext(ctxtContract))
    {
        StdLog("CService", "Get dest contract context: Get contract dest fail, dest: %s, fork: %s",
               dest.ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    return true;
}

bool CService::GetContractSource(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashSource, bytes& btSource)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            StdLog("CService", "Get contract source: Get last block fail, source: %s", hashSource.GetHex().c_str());
            return false;
        }
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

bool CService::GetContractCode(const uint256& hashFork, const uint256& hashRefBlock, const uint256& hashCode, bytes& btCode)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            StdLog("CService", "Get contract code: Get last block fail, code: %s", hashCode.GetHex().c_str());
            return false;
        }
    }
    CTxContractData txcd;
    if (!pBlockChain->GetBlockContractCreateCodeData(hashFork, hashLastBlock, hashCode, txcd))
    {
        StdLog("CService", "Get contract code: Get code fail1, code: %s", hashCode.GetHex().c_str());
        return false;
    }
    if (txcd.IsSetup())
    {
        if (!pBlockChain->GetBlockContractCreateCodeData(hashFork, hashLastBlock, txcd.GetContractCreateCodeHash(), txcd))
        {
            StdLog("CService", "Get contract code: Get code fail2, code: %s", hashCode.GetHex().c_str());
            return false;
        }
    }
    btCode = txcd.GetCode();
    return true;
}

bool CService::GetDestTemplateData(const uint256& hashFork, const uint256& hashRefBlock, const CDestination& dest, bytes& btTemplateData)
{
    uint256 hashLastBlock = hashRefBlock;
    if (hashRefBlock == 0)
    {
        if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        {
            StdLog("CService", "Get dest template data: Retrieve fork last fail, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
    }
    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, dest, ctxAddress))
    {
        StdLog("CService", "Get dest template data: Retrieve address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    CTemplateAddressContext ctxtTemplate;
    if (!ctxAddress.GetTemplateAddressContext(ctxtTemplate))
    {
        StdLog("CService", "Get dest template data: Get template address context fail, dest: %s", dest.ToString().c_str());
        return false;
    }
    btTemplateData = ctxtTemplate.btData;
    return true;
}

bool CService::RetrieveAddressContext(const uint256& hashFork, const CDestination& dest, CAddressContext& ctxAddress, const uint256& hashBlock)
{
    return pTxPool->GetAddressContext(hashFork, dest, ctxAddress, hashBlock);
}

bool CService::AddBlacklistAddress(const CDestination& dest)
{
    return pBlockChain->AddBlacklistAddress(dest);
}

void CService::RemoveBlacklistAddress(const CDestination& dest)
{
    pBlockChain->RemoveBlacklistAddress(dest);
}

void CService::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    pBlockChain->ListBlacklistAddress(setAddressOut);
}

bool CService::ListFunctionAddress(const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    return pBlockChain->ListFunctionAddress(hashBlock, mapFunctionAddress);
}

bool CService::RetrieveFunctionAddress(const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    return pBlockChain->RetrieveFunctionAddress(hashBlock, nFuncId, ctxFuncAddress);
}

bool CService::GetDefaultFunctionAddress(const uint32 nFuncId, CDestination& destDefFunction)
{
    return pBlockChain->GetDefaultFunctionAddress(nFuncId, destDefFunction);
}

bool CService::VerifyNewFunctionAddress(const uint256& hashBlock, const CDestination& destFrom, const uint32 nFuncId, const CDestination& destNewFunction, std::string& strErr)
{
    return pBlockChain->VerifyNewFunctionAddress(hashBlock, destFrom, nFuncId, destNewFunction, strErr);
}

bool CService::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return pBlockChain->UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

uint256 CService::GetForkMintMinGasPrice(const uint256& hashFork)
{
    return pBlockChain->GetForkMintMinGasPrice(hashFork);
}

bytes CService::MakeEthTxCallData(const std::string& strFunction, const std::vector<bytes>& vParamList)
{
    bytes btData;
    bytes btFuncSign = CryptoKeccakSign(strFunction);
    btData.insert(btData.end(), btFuncSign.begin(), btFuncSign.end());
    for (auto& p : vParamList)
    {
        btData.insert(btData.end(), p.begin(), p.end());
    }
    return btData;
}

bool CService::GetWork(vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                       uint256& hashPrev, int& nAlgo,
                       int& nBits, const CTemplateMintPtr& templMint)
{
    CBlock block;
    block.nType = CBlock::BLOCK_PRIMARY;

    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        StdTrace("CService", "Get work: Get last block status fail");
        return false;
    }
    hashPrev = status.hashBlock;
    nPrevBlockHeight = status.nBlockHeight;
    block.hashPrev = hashPrev;
    block.nNumber = status.nBlockNumber + 1;

    // uint32 nNextTimestamp = pCoreProtocol->GetNextBlockTimestamp(status.nBlockTime);
    // block.nTimeStamp = nNextTimestamp;
    // uint32 nCurrTime = GetNetTime();
    // if (nCurrTime > block.nTimeStamp)
    // {
    //     block.nTimeStamp += (((nCurrTime - block.nTimeStamp) / BLOCK_TARGET_SPACING) * BLOCK_TARGET_SPACING);
    // }

    uint64 nNextTimestamp = pCoreProtocol->GetNextBlockTimestamp(status.nBlockTime);
    block.SetBlockTime(nNextTimestamp);
    uint64 nCurrTime = GetNetTime();
    if (nCurrTime > block.GetBlockTime())
    {
        block.SetBlockTime(block.GetBlockTime() + (((nCurrTime - block.GetBlockTime()) / BLOCK_TARGET_SPACING) * BLOCK_TARGET_SPACING));
    }

    bool fIsDpos = false;
    if (pNetChannel->IsLocalCachePowBlock(nPrevBlockHeight + 1, fIsDpos))
    {
        StdTrace("CService", "Get work: IsLocalCachePowBlock poa exist");
        return false;
    }
    if (fIsDpos)
    {
        if (block.GetBlockTime() < nNextTimestamp + BLOCK_TARGET_SPACING)
        {
            StdTrace("CService", "Get work: Wait pos block......");
            return false;
        }
    }

    nAlgo = CM_CRYPTONIGHT;
    if (!pBlockChain->GetProofOfWorkTarget(block.hashPrev, nAlgo, nBits))
    {
        StdError("CService", "Get work: Get ProofOfWork Target fail");
        return false;
    }

    CProofOfHashWork proof;
    proof.nWeight = 0;
    proof.nAgreement = 0;
    proof.nAlgo = nAlgo;
    proof.nBits = nBits;
    proof.destMint = CDestination(templMint->GetTemplateId());
    proof.nNonce = 0;

    block.AddHashWorkProof(proof);

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
        StdError("CService", "Submit work: vchWorkData is empty");
        return FAILED;
    }
    CBlock block;
    CProofOfHashWork proof;
    CBufStream ss;
    ss.Write((const char*)&vchWorkData[0], vchWorkData.size());
    try
    {
        uint64 nBlockTime;
        ss >> block.nVersion >> block.nType >> nBlockTime >> block.nNumber >> block.nSlot >> block.hashPrev >> block.mapProof;
        block.SetBlockTime(nBlockTime);
        if (!block.GetHashWorkProof(proof))
        {
            StdError("CService", "Submit work: Load proof fail");
            return ERR_BLOCK_PROOF_OF_WORK_INVALID;
        }
        if (proof.nAlgo != CM_CRYPTONIGHT)
        {
            StdError("CService", "Submit work: nAlgo error");
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
        StdError("CService", "Submit work: Get ProofOfWork Target fail");
        return FAILED;
    }

    uint256 nReward = 0;
    if (!pBlockChain->GetBlockMintReward(block.hashPrev, true, nReward, block.hashPrev))
    {
        StdError("CService", "Submit work: Get block mint reward fail");
        return FAILED;
    }

    CTransaction& txMint = block.txMint;

    txMint.SetTxType(CTransaction::TX_WORK);
    txMint.SetNonce(block.nNumber);
    txMint.SetChainId(pCoreProtocol->GetGenesisChainId());
    txMint.SetToAddress(CDestination(templMint->GetTemplateId()));
    txMint.SetAmount(nReward);
    txMint.SetGasPrice(0);
    txMint.SetGasLimit(0);

    CAddressContext ctxAddress;
    if (!pBlockChain->RetrieveAddressContext(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, txMint.GetToAddress(), ctxAddress))
    {
        CTemplateAddressContext ctxTemplate(string(), string(), templMint->GetTemplateType(), templMint->Export());
        txMint.SetToAddressData(CAddressContext(ctxTemplate));
    }

    size_t nSigSize = templMint->Export().size() + crypto::SIGN_DATA_SIZE + 2 + 32 * 5;
    size_t nMaxTxSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - nSigSize;

    vector<CTransaction> vVoteRewardTx;
    if (!pBlockChain->CalcBlockVoteRewardTx(block.hashPrev, block.nType, block.GetBlockHeight(), block.GetBlockTime(), vVoteRewardTx))
    {
        StdError("CService", "Submit work: Get block invest reward fail");
        return FAILED;
    }
    uint256 nVoteRewardTxFee;
    for (const CTransaction& tx : vVoteRewardTx)
    {
        size_t nTxSize = GetSerializeSize(tx);
        if (nMaxTxSize < nTxSize)
        {
            StdError("CService", "Submit work: Invest reward tx size error");
            return FAILED;
        }
        block.vtx.push_back(tx);
        nVoteRewardTxFee += tx.GetTxFee();
        nMaxTxSize -= nTxSize;
    }

    uint256 nTotalTxFee;
    if (!pTxPool->FetchArrangeBlockTx(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, block.GetBlockTime(), nMaxTxSize, block.vtx, nTotalTxFee))
    {
        StdError("CService", "Submit work: Fetch arrange block tx fail");
        return FAILED;
    }
    // uint256 nMintCoin(block.txMint.GetAmount());
    // block.txMint.SetAmount(block.txMint.GetAmount() + nTotalTxFee + nVoteRewardTxFee);
    block.AddMintCoinProof(block.txMint.GetAmount());
    block.txMint.SetAmount(0);
    block.nGasLimit = MAX_BLOCK_GAS_LIMIT;

    uint256 nTotalMintReward;
    if (!pBlockChain->CreateBlockStateRoot(pCoreProtocol->GetGenesisBlockHash(), block, block.hashStateRoot, block.hashReceiptsRoot,
                                           block.nGasUsed, block.btBloomData, nTotalMintReward))
    {
        StdError("CService", "Submit work: Get block state root fail, block: %s", block.GetHash().ToString().c_str());
        pTxPool->ClearTxPool(pCoreProtocol->GetGenesisBlockHash());
        return FAILED;
    }
    //block.txMint.SetAmount(0);

    //block.AddMintCoinProof(nMintCoin);
    block.AddMintRewardProof(nTotalMintReward);

    block.hashMerkleRoot = block.CalcMerkleTreeRoot();

    hashBlock = block.GetHash();
    bytes btSigData;
    if (!keyMint.Sign(hashBlock, btSigData))
    {
        StdError("CService", "Submit work: Sign fail");
        return ERR_BLOCK_SIGNATURE_INVALID;
    }
    block.SetSignData(btSigData);

    Errno err = pCoreProtocol->ValidateBlock(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, block);
    if (err != OK)
    {
        StdError("CService", "Submit work: Validate block fail");
        return err;
    }

    if (!pNetChannel->AddCacheLocalPowBlock(block))
    {
        StdError("CService", "Submit work: AddCacheLocalPowBlock fail");
        return FAILED;
    }
    return OK;
}

////////////////////////////////////////////////////////////////////////
bool CService::SetContractTransaction(const uint256& hashFork, const uint256& hashLastBlock, const bytes& btFormatData, const bytes& btContractCode, const bytes& btContractParam, CTransaction& txNew, std::string& strErr)
{
    auto funcContractCreateContext = [&](const uint256& hashContractCreateCode) -> bool {
        // uint256 hashLastBlock;
        // if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        // {
        //     strErr = "Get last block fail";
        //     StdLog("CService", "Set contract transaction: %s, fork: %s",
        //            strErr.c_str(), hashFork.GetHex().c_str());
        //     return false;
        // }
        // CContractCreateCodeContext ctxtCode;
        // bool fLinkGenesisFork;
        // if (!pBlockChain->RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashLastBlock, hashContractCreateCode, ctxtCode, fLinkGenesisFork))
        // {
        //     strErr = "Contract not exist";
        //     StdLog("CService", "Set contract transaction: %s, contract create code hash: %s, fork: %s",
        //            strErr.c_str(), hashContractCreateCode.GetHex().c_str(), hashFork.GetHex().c_str());
        //     return false;
        // }
        // if (!ctxtCode.IsActivated())
        // {
        //     strErr = "Contract not activated";
        //     StdLog("CService", "Set contract transaction: %s, contract create code hash: %s, fork: %s",
        //            strErr.c_str(), hashContractCreateCode.GetHex().c_str(), hashFork.GetHex().c_str());
        //     return false;
        // }
        return true;
    };
    auto funcQueryCreateContext = [&](const uint256& hashContractCreateCode, bool& fLinkGenesisFork) -> bool {
        CContractCreateCodeContext ctxtCode;
        if (!pBlockChain->RetrieveLinkGenesisContractCreateCodeContext(hashFork, hashLastBlock, hashContractCreateCode, ctxtCode, fLinkGenesisFork))
        {
            return false;
        }
        return true;
    };

    if (txNew.GetToAddress().IsNull())
    {
        if (btFormatData.size() > 0)
        {
            uint8 nCodeType;
            CTemplateContext ctxTemplate;
            CTxContractData txcdDux;
            try
            {
                mtbase::CBufStream ss(btFormatData);
                ss >> nCodeType;
                if (nCodeType == CODE_TYPE_TEMPLATE)
                {
                    ss >> ctxTemplate;
                }
                else if (nCodeType == CODE_TYPE_CONTRACT)
                {
                    ss >> txcdDux;
                }
                else
                {
                    strErr = "Code type error";
                    StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), hashFork.GetHex().c_str());
                    return false;
                }
            }
            catch (std::exception& e)
            {
                strErr = "Contract dux data error";
                StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), hashFork.GetHex().c_str());
                return false;
            }
            if (nCodeType == CODE_TYPE_TEMPLATE)
            {
                mtbase::CBufStream ss;
                ss << nCodeType << ctxTemplate;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CREATE_CODE, btCodeData);
            }
            else
            {
                if (txcdDux.IsUpcode())
                {
                    uint256 hashContractCreateCode = txcdDux.GetContractCreateCodeHash();
                    bool fLinkGenesisFork = false;
                    if (funcQueryCreateContext(hashContractCreateCode, fLinkGenesisFork) && fLinkGenesisFork)
                    {
                        strErr = "Contract at genesis fork";
                        StdLog("CService", "Set contract transaction: %s, contract create code hash: %s, fork: %s",
                               strErr.c_str(), hashContractCreateCode.GetHex().c_str(), hashFork.GetHex().c_str());
                        return false;
                    }
                }
                else
                {
                    uint256 hashContractCreateCode = txcdDux.GetContractCreateCodeHash();
                    if (fEnableContractCodeVerify)
                    {
                        if (!funcContractCreateContext(hashContractCreateCode))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (txcdDux.IsSetup())
                        {
                            bool fLinkGenesisFork = false;
                            if (!funcQueryCreateContext(hashContractCreateCode, fLinkGenesisFork) && fLinkGenesisFork)
                            {
                                strErr = "Contract not exist";
                                StdLog("CService", "Set contract transaction: %s, contract create code hash: %s, fork: %s",
                                       strErr.c_str(), hashContractCreateCode.GetHex().c_str(), hashFork.GetHex().c_str());
                                return false;
                            }
                        }
                    }
                    if (txcdDux.IsCreate())
                    {
                        txcdDux.SetSetupHash(hashContractCreateCode);
                    }
                }
                mtbase::CBufStream ss;
                ss << nCodeType << txcdDux;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CREATE_CODE, btCodeData);
            }
        }
        else
        {
            if (btContractCode.size() == uint256::size())
            {
                //bytes btTempData(btContractCode.begin(), btContractCode.end());
                //std::reverse(btTempData.begin(), btTempData.end());
                //uint256 hashContractCreateCode(btTempData);
                uint256 hashContractCreateCode(btContractCode);

                if (!funcContractCreateContext(hashContractCreateCode))
                {
                    return false;
                }

                CTxContractData txcd(hashContractCreateCode); //CF_SETUP
                mtbase::CBufStream ss;
                ss << CODE_TYPE_CONTRACT << txcd;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CREATE_CODE, btCodeData);
            }
            else if (btContractCode.size() == CDestination::size())
            {
                CDestination destContract;
                try
                {
                    mtbase::CBufStream ss(btContractCode);
                    ss >> destContract;
                }
                catch (std::exception& e)
                {
                    strErr = "Contract code error";
                    StdLog("CService", "Set contract transaction: %s, fork: %s", strErr.c_str(), hashFork.GetHex().c_str());
                    return false;
                }
                //if (!destContract.IsContract() || destContract.IsNullContract())
                if (destContract.IsNull())
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error1, dest: %s, fork: %s",
                           destContract.ToString().c_str(), hashFork.GetHex().c_str());
                    return false;
                }

                CAddressContext ctxAddress;
                if (!pBlockChain->RetrieveAddressContext(hashFork, hashLastBlock, destContract, ctxAddress))
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error2, dest: %s, fork: %s",
                           destContract.ToString().c_str(), hashFork.GetHex().c_str());
                    return false;
                }
                if (!ctxAddress.IsContract())
                {
                    strErr = "Contract dest error";
                    StdLog("CService", "Set contract transaction: Contract dest error3, dest: %s, fork: %s",
                           destContract.ToString().c_str(), hashFork.GetHex().c_str());
                    return false;
                }
                CContractAddressContext ctxtContract;
                if (!ctxAddress.GetContractAddressContext(ctxtContract))
                {
                    strErr = "Get contract dest fail";
                    StdLog("CService", "Set contract transaction: Get contract dest fail, dest: %s, fork: %s",
                           destContract.ToString().c_str(), hashFork.GetHex().c_str());
                    return false;
                }

                if (!funcContractCreateContext(ctxtContract.hashContractCreateCode))
                {
                    return false;
                }

                CTxContractData txcd(ctxtContract.hashContractCreateCode); //CF_SETUP
                mtbase::CBufStream ss;
                ss << CODE_TYPE_CONTRACT << txcd;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CREATE_CODE, btCodeData);
            }
            else if (btContractCode.size() > CDestination::size())
            {
                uint256 hashContractCreateCode = metabasenet::crypto::CryptoKeccakHash(btContractCode.data(), btContractCode.size());

                if (!funcContractCreateContext(hashContractCreateCode))
                {
                    return false;
                }

                bytes btCompressCode;
                if (!BtCompress(btContractCode, btCompressCode) || btCompressCode.size() == 0)
                {
                    strErr = "Contract code compress fail";
                    StdLog("CService", "Set contract transaction: %s, fork: %s",
                           strErr.c_str(), hashFork.GetHex().c_str());
                    return false;
                }
                CTxContractData txcd(btCompressCode, true); //CF_CREATE
                mtbase::CBufStream ss;
                ss << CODE_TYPE_CONTRACT << txcd;
                bytes btCodeData;
                ss.GetData(btCodeData);
                txNew.AddTxData(CTransaction::DF_CREATE_CODE, btCodeData);
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

bool CService::SetTemplateTransaction(const uint256& hashFork, const uint256& hashLastBlock, const uint8 nToTemplateType, const bytes& btFormatData, const bytes& vchData, const bytes& btToData, CTransaction& txNew, std::string& strErr)
{
    if (nToTemplateType == TEMPLATE_FORK)
    {
        if (vchData.size() > 0)
        {
            txNew.AddTxData(CTransaction::DF_FORKDATA, vchData);
        }
    }
    else if (nToTemplateType == TEMPLATE_ACTIVATECODE)
    {
        // if (!fEnableContractCodeVerify)
        // {
        //     strErr = "No activation required";
        //     StdLog("CService", "Set template transaction: %s, txid: %s", strErr.c_str(), txNew.GetHash().ToString().c_str());
        //     return false;
        // }
        // if (vchData.size() == 0)
        // {
        //     StdLog("CService", "Set template transaction: %s, txid: %s", strErr.c_str(), txNew.GetHash().ToString().c_str());
        //     return false;
        // }
        // bytes btCodeHashData = vchData;
        // std::reverse(btCodeHashData.begin(), btCodeHashData.end());
        // uint256 hashCode;
        // try
        // {
        //     CBufStream ss(btCodeHashData);
        //     ss >> hashCode;
        // }
        // catch (std::exception& e)
        // {
        //     strErr = "activate code data error";
        //     StdLog("CService", "Set template transaction: activatecode data error, txid: %s", txNew.GetHash().ToString().c_str());
        //     return false;
        // }
        // uint256 hashLastBlock;
        // if (!pBlockChain->RetrieveForkLast(hashFork, hashLastBlock))
        // {
        //     strErr = "fork error";
        //     StdError("CService", "Set template transaction: find fork fail, fork: %s", hashFork.GetHex().c_str());
        //     return false;
        // }
        // CContractCreateCodeContext ctxtCode;
        // if (!pBlockChain->RetrieveForkContractCreateCodeContext(hashFork, hashLastBlock, hashCode, ctxtCode))
        // {
        //     strErr = "code not exist";
        //     StdLog("CService", "Set template transaction: code not exist, code: %s, txid: %s",
        //            hashCode.ToString().c_str(), txNew.GetHash().ToString().c_str());
        //     return false;
        // }
        // // if (ctxtCode.IsActivated())
        // // {
        // //     strErr = "code is activated";
        // //     StdLog("CService", "Set template transaction: code is activated, code: %s, txid: %s",
        // //            hashCode.ToString().c_str(), txNew.GetHash().ToString().c_str());
        // //     return false;
        // // }
        // txNew.AddTxData(CTransaction::DF_ACTIVATECODE, btCodeHashData);
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

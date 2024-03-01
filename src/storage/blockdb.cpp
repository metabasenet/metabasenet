// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"

#include "util.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CBlockDB

CBlockDB::CBlockDB()
  : fCfgFullDb(false)
{
}

CBlockDB::~CBlockDB()
{
}

bool CBlockDB::Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn, const bool fFullDbIn)
{
    fCfgFullDb = fFullDbIn;
    if (!dbVerify.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbVerify initialize fail");
        return false;
    }
    if (!dbFork.Initialize(pathData, hashGenesisBlockIn))
    {
        StdLog("CBlockDB", "Initialize: dbFork initialize fail");
        return false;
    }
    if (!dbBlockIndex.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbBlockIndex initialize fail");
        return false;
    }
    if (!dbTxIndex.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbTxIndex initialize fail");
        return false;
    }
    if (!dbVote.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbVote initialize fail");
        return false;
    }
    if (!dbState.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbState initialize fail");
        return false;
    }
    if (!dbAddress.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbAddress initialize fail");
        return false;
    }
    if (!dbContract.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbContract initialize fail");
        return false;
    }
    if (!dbAddressBlacklist.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbAddressBlacklist initialize fail");
        return false;
    }
    if (!dbMintMinGasPrice.Initialize(pathData))
    {
        StdLog("CBlockDB", "Initialize: dbMintMinGasPrice initialize fail");
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.Initialize(pathData))
        {
            StdLog("CBlockDB", "Initialize: dbAddressTxInfo initialize fail");
            return false;
        }
    }
    return LoadAllFork();
}

void CBlockDB::Deinitialize()
{
    dbContract.Deinitialize();
    dbAddress.Deinitialize();
    dbState.Deinitialize();
    dbVote.Deinitialize();
    dbTxIndex.Deinitialize();
    dbBlockIndex.Deinitialize();
    dbFork.Deinitialize();
    dbVerify.Deinitialize();
    dbAddressBlacklist.Deinitialize();
    dbMintMinGasPrice.Deinitialize();
    if (fCfgFullDb)
    {
        dbAddressTxInfo.Deinitialize();
    }
}

void CBlockDB::RemoveAll()
{
    dbContract.Clear();
    dbAddress.Clear();
    dbState.Clear();
    dbVote.Clear();
    dbTxIndex.Clear();
    dbBlockIndex.Clear();
    dbFork.Clear();
    dbVerify.Clear();
    dbAddressBlacklist.Remove();
    dbMintMinGasPrice.Remove();
    if (fCfgFullDb)
    {
        dbAddressTxInfo.Clear();
    }
}

bool CBlockDB::AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, uint256& hashNewRoot)
{
    return dbFork.AddForkContext(hashPrevBlock, hashBlock, mapForkCtxt, hashNewRoot);
}

bool CBlockDB::ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock)
{
    return dbFork.ListForkContext(mapForkCtxt, hashBlock);
}

bool CBlockDB::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock)
{
    return dbFork.RetrieveForkContext(hashFork, ctxt, hashMainChainRefBlock);
}

bool CBlockDB::UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock)
{
    return dbFork.UpdateForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbFork.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkHashByForkName(strForkName, hashFork, hashMainChainRefBlock);
}

bool CBlockDB::GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock)
{
    return dbFork.GetForkHashByChainId(nChainId, hashFork, hashMainChainRefBlock);
}

bool CBlockDB::AddNewFork(const uint256& hashFork)
{
    if (!dbFork.UpdateForkLast(hashFork, hashFork))
    {
        return false;
    }
    if (!dbTxIndex.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbState.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbAddress.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbContract.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.AddNewFork(hashFork))
        {
            RemoveFork(hashFork);
            return false;
        }
    }
    return true;
}

bool CBlockDB::LoadFork(const uint256& hashFork)
{
    if (!dbTxIndex.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbState.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbAddress.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbContract.LoadFork(hashFork))
    {
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.LoadFork(hashFork))
        {
            return false;
        }
    }
    return true;
}

bool CBlockDB::RemoveFork(const uint256& hashFork)
{
    dbTxIndex.RemoveFork(hashFork);
    dbState.RemoveFork(hashFork);
    dbAddress.RemoveFork(hashFork);
    dbContract.RemoveFork(hashFork);
    if (fCfgFullDb)
    {
        dbAddressTxInfo.RemoveFork(hashFork);
    }
    return dbFork.RemoveFork(hashFork);
}

bool CBlockDB::ListFork(vector<pair<uint256, uint256>>& vFork)
{
    std::map<uint256, CForkContext> mapForkCtxt;
    if (!dbFork.ListForkContext(mapForkCtxt))
    {
        return false;
    }
    vFork.clear();
    for (const auto& kv : mapForkCtxt)
    {
        uint256 hashLastBlock;
        if (!dbFork.RetrieveForkLast(kv.first, hashLastBlock))
        {
            hashLastBlock = 0;
        }
        vFork.push_back(make_pair(kv.first, hashLastBlock));
    }
    return true;
}

bool CBlockDB::AddNewBlockIndex(const CBlockOutline& outline)
{
    return dbBlockIndex.AddNewBlockIndex(outline);
}

bool CBlockDB::RemoveBlockIndex(const uint256& hashBlock)
{
    return dbBlockIndex.RemoveBlockIndex(hashBlock);
}

bool CBlockDB::RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline)
{
    return dbBlockIndex.RetrieveBlockIndex(hashBlock, outline);
}

bool CBlockDB::AddNewBlockNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint64 nBlockNumber, const uint256& hashBlock, uint256& hashNewRoot)
{
    return dbBlockIndex.AddBlockNumber(hashFork, nChainId, hashPrevBlock, nBlockNumber, hashBlock, hashNewRoot);
}

bool CBlockDB::RetrieveBlockHashByNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashLastBlock, const uint64 nBlockNumber, uint256& hashBlock)
{
    return dbBlockIndex.RetrieveBlockHashByNumber(hashFork, nChainId, hashLastBlock, nBlockNumber, hashBlock);
}

bool CBlockDB::AddBlockVerify(const CBlockOutline& outline, const uint32 nRootCrc)
{
    return dbVerify.AddBlockVerify(outline, nRootCrc);
}

bool CBlockDB::RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock)
{
    return dbVerify.RetrieveBlockVerify(hashBlock, verifyBlock);
}

std::size_t CBlockDB::GetBlockVerifyCount()
{
    return dbVerify.GetBlockVerifyCount();
}

bool CBlockDB::GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock)
{
    return dbVerify.GetBlockVerify(pos, verifyBlock);
}

bool CBlockDB::UpdateDelegateContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                                     const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot)
{
    return dbVote.AddNewDelegate(hashPrevBlock, hashBlock, mapVote, mapEnrollTx, hashDelegateRoot);
}

bool CBlockDB::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote)
{
    return dbVote.RetrieveDestDelegateVote(hashBlock, dest, nVote);
}

bool CBlockDB::WalkThroughBlockIndex(CBlockDBWalker& walker)
{
    return dbBlockIndex.WalkThroughBlockIndex(walker);
}

bool CBlockDB::RetrieveTxIndex(const uint256& hashFork, const uint256& txid, CTxIndex& txIndex)
{
    return dbTxIndex.RetrieveTxIndex(hashFork, txid, txIndex);
}

bool CBlockDB::RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt)
{
    return dbTxIndex.RetrieveTxReceipt(hashFork, txid, txReceipt);
}

bool CBlockDB::RetrieveDelegate(const uint256& hash, map<CDestination, uint256>& mapDelegate)
{
    return dbVote.RetrieveDelegatedVote(hash, mapDelegate);
}

bool CBlockDB::RetrieveRangeEnroll(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbVote.RetrieveRangeEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockDB::AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                            const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight, uint256& hashVoteRoot)
{
    return dbVote.AddBlockVote(hashPrev, hashBlock, mapBlockVote, mapAddPledgeFinalHeight, mapRemovePledgeFinalHeight, hashVoteRoot);
}

bool CBlockDB::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbVote.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockDB::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbVote.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockDB::ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight)
{
    return dbVote.ListPledgeFinalHeight(hashBlock, nFinalHeight, mapPledgeFinalHeight);
}

bool CBlockDB::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbVote.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockDB::AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot)
{
    return dbState.AddBlockState(hashFork, hashPrevRoot, statusBlockRoot, mapBlockState, hashBlockRoot);
}

bool CBlockDB::CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot)
{
    return dbState.CreateCacheStateTrie(hashFork, hashPrevRoot, statusBlockRoot, mapBlockState, hashBlockRoot);
}

bool CBlockDB::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbState.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

bool CBlockDB::ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState)
{
    return dbState.ListDestState(hashFork, hashBlockRoot, mapBlockState);
}

bool CBlockDB::AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::map<uint256, CTransactionReceipt>& mapBlockTxReceipts)
{
    return dbTxIndex.AddBlockTxIndexReceipt(hashFork, hashBlock, mapBlockTxIndex, mapBlockTxReceipts);
}

bool CBlockDB::UpdateBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx)
{
    return dbTxIndex.UpdateBlockLongChain(hashFork, vRemoveTx, mapNewTx);
}

bool CBlockDB::AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState)
{
    return dbContract.AddBlockContractKvValue(hashFork, hashPrevRoot, hashContractRoot, mapContractState);
}

bool CBlockDB::RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value)
{
    return dbContract.RetrieveContractKvValue(hashFork, hashContractRoot, key, value);
}

bool CBlockDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                                 const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, uint256& hashNewRoot)
{
    return dbAddress.AddAddressContext(hashFork, hashPrevBlock, hashBlock, mapAddress, nNewAddressCount, mapTimeVault, mapFunctionAddress, hashNewRoot);
}

bool CBlockDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress)
{
    uint256 hashRefBlock;
    if (hashBlock == 0)
    {
        if (!dbFork.RetrieveForkLast(hashFork, hashRefBlock))
        {
            StdLog("CBlockDB", "Retrieve address context: Retrieve fork last fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }
    }
    else
    {
        hashRefBlock = hashBlock;
    }
    return dbAddress.RetrieveAddressContext(hashFork, hashRefBlock, dest, ctxAddress);
}

bool CBlockDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress)
{
    return dbAddress.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockDB::RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv)
{
    return dbAddress.RetrieveTimeVault(hashFork, hashBlock, dest, tv);
}

bool CBlockDB::GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount)
{
    return dbAddress.GetAddressCount(hashFork, hashBlock, nAddressCount, nNewAddressCount);
}

bool CBlockDB::ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress)
{
    return dbAddress.ListFunctionAddress(hashFork, hashBlock, mapFunctionAddress);
}

bool CBlockDB::RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress)
{
    return dbAddress.RetrieveFunctionAddress(hashFork, hashBlock, nFuncId, ctxFuncAddress);
}

bool CBlockDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                              const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                              const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                              const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                              const std::map<uint256, CTemplateContext>& mapTemplateData,
                              uint256& hashCodeRoot)
{
    return dbContract.AddCodeContext(hashFork, hashPrevBlock, hashBlock, mapSourceCode, mapContractCreateCode, mapContractRunCode, mapTemplateData, hashCodeRoot);
}

bool CBlockDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode)
{
    return dbContract.RetrieveSourceCodeContext(hashFork, hashBlock, hashSourceCode, ctxtCode);
}

bool CBlockDB::RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode)
{
    return dbContract.RetrieveContractCreateCodeContext(hashFork, hashBlock, hashContractCreateCode, ctxtCode);
}

bool CBlockDB::RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode)
{
    return dbContract.RetrieveContractRunCodeContext(hashFork, hashBlock, hashContractRunCode, ctxtCode);
}

bool CBlockDB::ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode)
{
    return dbContract.ListContractCreateCodeContext(hashFork, hashBlock, mapContractCreateCode);
}

bool CBlockDB::AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.AddAddressTxInfo(hashFork, hashPrevBlock, hashBlock, nBlockNumber, mapAddressTxInfo, hashNewRoot);
    }
    return false;
}

bool CBlockDB::GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.GetAddressTxCount(hashFork, hashBlock, dest, nTxCount);
    }
    return false;
}

bool CBlockDB::RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.RetrieveAddressTxInfo(hashFork, hashBlock, dest, nTxIndex, ctxtAddressTxInfo);
    }
    return false;
}

bool CBlockDB::ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo)
{
    if (fCfgFullDb)
    {
        return dbAddressTxInfo.ListAddressTxInfo(hashFork, hashBlock, dest, nBeginTxIndex, nGetTxCount, fReverse, vAddressTxInfo);
    }
    return false;
}

bool CBlockDB::AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot)
{
    return dbVote.AddVoteReward(hashFork, nChainId, hashPrevBlock, hashBlock, nBlockHeight, mapVoteReward, hashNewRoot);
}

bool CBlockDB::ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    return dbVote.ListVoteReward(nChainId, hashBlock, dest, nGetCount, vVoteReward);
}

bool CBlockDB::AddBlacklistAddress(const CDestination& dest)
{
    return dbAddressBlacklist.AddAddress(dest);
}

void CBlockDB::RemoveBlacklistAddress(const CDestination& dest)
{
    dbAddressBlacklist.RemoveAddress(dest);
}

bool CBlockDB::IsExistBlacklistAddress(const CDestination& dest)
{
    return dbAddressBlacklist.IsExist(dest);
}

void CBlockDB::ListBlacklistAddress(set<CDestination>& setAddressOut)
{
    dbAddressBlacklist.ListAddress(setAddressOut);
}

bool CBlockDB::UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice)
{
    return dbMintMinGasPrice.UpdateForkMintMinGasPrice(hashFork, nMinGasPrice);
}

bool CBlockDB::GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice)
{
    return dbMintMinGasPrice.GetForkMintMinGasPrice(hashFork, nMinGasPrice);
}

bool CBlockDB::VerifyBlockRoot(const bool fPrimary, const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                               const uint256& hashLocalStateRoot, CBlockRoot& localBlockRoot, const bool fVerifyAllNode)
{
    if (fPrimary)
    {
        if (!dbFork.VerifyForkContext(hashPrevBlock, hashBlock, localBlockRoot.hashForkContextRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify fork context fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
        if (!dbVote.VerifyDelegateVote(hashPrevBlock, hashBlock, localBlockRoot.hashDelegateRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify delegate fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
        if (!dbVote.VerifyVote(hashPrevBlock, hashBlock, localBlockRoot.hashVoteRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify vote fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }
    if (!dbState.VerifyState(hashFork, hashLocalStateRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify state fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    localBlockRoot.hashStateRoot = hashLocalStateRoot;
    if (!dbAddress.VerifyAddressContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashAddressRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify address context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbContract.VerifyCodeContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashCodeRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify code context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbBlockIndex.VerifyBlockNumberContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashBlockNumberRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify blocknumber fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbTxIndex.VerifyTxIndex(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashTxIndexRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify txindex fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbVote.VerifyVoteReward(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashVoteRewardRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify reward lock fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (fCfgFullDb)
    {
        uint256 hashAddressTxInfoRoot;
        if (!dbAddressTxInfo.VerifyAddressTxInfo(hashFork, hashPrevBlock, hashBlock, hashAddressTxInfoRoot, fVerifyAllNode))
        {
            StdError("CBlockDB", "Verify block root: Verify address tx info fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////
bool CBlockDB::LoadAllFork()
{
    map<uint256, CForkContext> mapForkCtxt;
    if (!dbFork.ListForkContext(mapForkCtxt))
    {
        StdLog("CBlockDB", "Load all fork: ListForkContext fail");
        return false;
    }

    for (const auto& kv : mapForkCtxt)
    {
        if (!dbTxIndex.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbTxIndex LoadFork fail");
            return false;
        }
        if (!dbState.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbState LoadFork fail");
            return false;
        }
        if (!dbAddress.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbAddress LoadFork fail");
            return false;
        }
        if (!dbContract.LoadFork(kv.first))
        {
            StdLog("CBlockDB", "Load all fork: dbContract LoadFork fail");
            return false;
        }
        if (fCfgFullDb)
        {
            if (!dbAddressTxInfo.LoadFork(kv.first))
            {
                StdLog("CBlockDB", "Load all fork: dbAddressTxInfo LoadFork fail");
                return false;
            }
        }
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

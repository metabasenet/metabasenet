// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"

#include "util.h"

using namespace std;
using namespace hcode;

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
        return false;
    }
    if (!dbFork.Initialize(pathData, hashGenesisBlockIn))
    {
        return false;
    }
    if (!dbBlockIndex.Initialize(pathData))
    {
        return false;
    }
    if (!dbTxIndex.Initialize(pathData))
    {
        return false;
    }
    if (!dbDelegate.Initialize(pathData))
    {
        return false;
    }
    if (!dbVote.Initialize(pathData))
    {
        return false;
    }
    if (!dbState.Initialize(pathData))
    {
        return false;
    }
    if (!dbAddress.Initialize(pathData))
    {
        return false;
    }
    if (!dbCode.Initialize(pathData))
    {
        return false;
    }
    if (!dbWasm.Initialize(pathData))
    {
        return false;
    }
    if (!dbVoteReward.Initialize(pathData))
    {
        return false;
    }
    if (!dbInvite.Initialize(pathData))
    {
        return false;
    }
    if (fCfgFullDb)
    {
        if (!dbAddressTxInfo.Initialize(pathData))
        {
            return false;
        }
    }
    return LoadAllFork();
}

void CBlockDB::Deinitialize()
{
    dbInvite.Deinitialize();
    dbVoteReward.Deinitialize();
    dbWasm.Deinitialize();
    dbCode.Deinitialize();
    dbAddress.Deinitialize();
    dbState.Deinitialize();
    dbVote.Deinitialize();
    dbDelegate.Deinitialize();
    dbTxIndex.Deinitialize();
    dbBlockIndex.Deinitialize();
    dbFork.Deinitialize();
    dbVerify.Deinitialize();
    if (fCfgFullDb)
    {
        dbAddressTxInfo.Deinitialize();
    }
}

void CBlockDB::RemoveAll()
{
    dbInvite.Clear();
    dbVoteReward.Clear();
    dbWasm.Clear();
    dbCode.Clear();
    dbAddress.Clear();
    dbState.Clear();
    dbVote.Clear();
    dbDelegate.Clear();
    dbTxIndex.Clear();
    dbBlockIndex.Clear();
    dbFork.Clear();
    dbVerify.Clear();
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

bool CBlockDB::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashBlock)
{
    return dbFork.RetrieveForkContext(hashFork, ctxt, hashBlock);
}

bool CBlockDB::UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock)
{
    return dbFork.UpdateForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock)
{
    return dbFork.RetrieveForkLast(hashFork, hashLastBlock);
}

bool CBlockDB::GetForkIdByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashBlock)
{
    return dbFork.GetForkIdByForkName(strForkName, hashFork, hashBlock);
}

bool CBlockDB::GetForkIdByForkSn(const uint16 nForkSn, uint256& hashFork, const uint256& hashBlock)
{
    return dbFork.GetForkIdByForkSn(nForkSn, hashFork, hashBlock);
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
    if (!dbCode.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbWasm.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbVoteReward.AddNewFork(hashFork))
    {
        RemoveFork(hashFork);
        return false;
    }
    if (!dbInvite.AddNewFork(hashFork))
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
    if (!dbCode.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbWasm.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbVoteReward.LoadFork(hashFork))
    {
        return false;
    }
    if (!dbInvite.LoadFork(hashFork))
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
    dbCode.RemoveFork(hashFork);
    dbWasm.RemoveFork(hashFork);
    dbVoteReward.RemoveFork(hashFork);
    dbInvite.RemoveFork(hashFork);
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
    return dbDelegate.AddNewDelegate(hashPrevBlock, hashBlock, mapVote, mapEnrollTx, hashDelegateRoot);
}

bool CBlockDB::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote)
{
    return dbDelegate.RetrieveDestDelegateVote(hashBlock, dest, nVote);
}

bool CBlockDB::WalkThroughBlockIndex(CBlockDBWalker& walker)
{
    return dbBlockIndex.WalkThroughBlockIndex(walker);
}

bool CBlockDB::RetrieveTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex)
{
    return dbTxIndex.RetrieveTxIndex(hashFork, hashBlock, txid, txIndex);
}

bool CBlockDB::RetrieveDelegate(const uint256& hash, map<CDestination, uint256>& mapDelegate)
{
    return dbDelegate.RetrieveDelegatedVote(hash, mapDelegate);
}

bool CBlockDB::RetrieveEnroll(const uint256& hash, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos)
{
    return dbDelegate.RetrieveDelegatedEnrollTx(hash, mapEnrollTxPos);
}

bool CBlockDB::RetrieveEnroll(int height, const vector<uint256>& vBlockRange,
                              map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbDelegate.RetrieveEnrollTx(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockDB::AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote, uint256& hashVoteRoot)
{
    return dbVote.AddBlockVote(hashPrev, hashBlock, mapBlockVote, hashVoteRoot);
}

bool CBlockDB::RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote)
{
    return dbVote.RetrieveAllDelegateVote(hashBlock, mapDelegateVote);
}

bool CBlockDB::RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote)
{
    return dbVote.RetrieveDestVoteContext(hashBlock, destVote, ctxtVote);
}

bool CBlockDB::WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker)
{
    return dbVote.WalkThroughDayVote(hashBeginBlock, hashTailBlock, walker);
}

bool CBlockDB::AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    return dbState.AddBlockState(hashFork, hashPrevRoot, hashBlockRoot, mapBlockState);
}

bool CBlockDB::CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState)
{
    return dbState.CreateCacheStateTrie(hashFork, hashPrevRoot, hashBlockRoot, mapBlockState);
}

bool CBlockDB::RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state)
{
    return dbState.RetrieveDestState(hashFork, hashBlockRoot, dest, state);
}

bool CBlockDB::ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState)
{
    return dbState.ListDestState(hashFork, hashBlockRoot, mapBlockState);
}

bool CBlockDB::AddBlockTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot)
{
    return dbTxIndex.AddBlockTxIndex(hashFork, hashPrevBlock, hashBlock, mapBlockTxIndex, hashNewRoot);
}

bool CBlockDB::AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState)
{
    return dbWasm.AddBlockWasmState(hashFork, hashPrevRoot, hashWasmRoot, mapWasmState);
}

bool CBlockDB::RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value)
{
    return dbWasm.RetrieveWasmState(hashFork, hashWasmRoot, key, value);
}

bool CBlockDB::AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CAddressContext>& mapAddress, uint256& hashNewRoot)
{
    return dbAddress.AddAddressContext(hashFork, hashPrevBlock, hashBlock, mapAddress, hashNewRoot);
}

bool CBlockDB::RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress)
{
    return dbAddress.RetrieveAddressContext(hashFork, hashBlock, dest, ctxtAddress);
}

bool CBlockDB::ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress)
{
    return dbAddress.ListContractAddress(hashFork, hashBlock, mapContractAddress);
}

bool CBlockDB::AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                              const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                              const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                              const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                              uint256& hashCodeRoot)
{
    return dbCode.AddCodeContext(hashFork, hashPrevBlock, hashBlock, mapSourceCode, mapWasmCreateCode, mapWasmRunCode, hashCodeRoot);
}

bool CBlockDB::RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode)
{
    return dbCode.RetrieveSourceCodeContext(hashFork, hashBlock, hashSourceCode, ctxtCode);
}

bool CBlockDB::RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode)
{
    return dbCode.RetrieveWasmCreateCodeContext(hashFork, hashBlock, hashWasmCreateCode, ctxtCode);
}

bool CBlockDB::RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode)
{
    return dbCode.RetrieveWasmRunCodeContext(hashFork, hashBlock, hashWasmRunCode, ctxtCode);
}

bool CBlockDB::ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode)
{
    return dbCode.ListWasmCreateCodeContext(hashFork, hashBlock, mapWasmCreateCode);
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

bool CBlockDB::AddVoteReward(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot)
{
    return dbVoteReward.AddVoteReward(hashFork, hashPrevBlock, hashBlock, nBlockHeight, mapVoteReward, hashNewRoot);
}

bool CBlockDB::ListVoteReward(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward)
{
    return dbVoteReward.ListVoteReward(hashFork, hashBlock, dest, nGetCount, vVoteReward);
}

bool CBlockDB::AddInviteRelation(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CDestination>& mapInviteContext, uint256& hashNewRoot)
{
    return dbInvite.AddInviteRelation(hashFork, hashPrevBlock, hashBlock, mapInviteContext, hashNewRoot);
}

bool CBlockDB::RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CDestination& destParent)
{
    return dbInvite.RetrieveInviteParent(hashFork, hashBlock, destSub, destParent);
}

bool CBlockDB::ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CDestination>& mapInviteContext)
{
    return dbInvite.ListInviteRelation(hashFork, hashBlock, mapInviteContext);
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
        if (!dbDelegate.VerifyDelegate(hashPrevBlock, hashBlock, localBlockRoot.hashDelegateRoot, fVerifyAllNode))
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
    if (!dbCode.VerifyCodeContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashCodeRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify code context fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbTxIndex.VerifyTxIndex(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashTxIndexRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify txindex fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbVoteReward.VerifyVoteReward(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashVoteRewardRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify reward lock fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!dbInvite.VerifyInviteContext(hashFork, hashPrevBlock, hashBlock, localBlockRoot.hashInviteRoot, fVerifyAllNode))
    {
        StdError("CBlockDB", "Verify block root: Verify invite fail, block: %s", hashBlock.GetHex().c_str());
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
        return false;
    }

    for (const auto& kv : mapForkCtxt)
    {
        if (!dbTxIndex.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbState.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbAddress.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbCode.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbWasm.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbVoteReward.LoadFork(kv.first))
        {
            return false;
        }
        if (!dbInvite.LoadFork(kv.first))
        {
            return false;
        }
        if (fCfgFullDb)
        {
            if (!dbAddressTxInfo.LoadFork(kv.first))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

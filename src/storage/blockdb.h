// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKDB_H
#define STORAGE_BLOCKDB_H

#include "addressblacklistdb.h"
#include "addressdb.h"
#include "addresstxinfodb.h"
#include "block.h"
#include "blockindexdb.h"
#include "cfgmintmingasprice.h"
#include "contractdb.h"
#include "forkcontext.h"
#include "forkdb.h"
#include "statedb.h"
#include "transaction.h"
#include "txindexdb.h"
#include "verifydb.h"
#include "votedb.h"

namespace metabasenet
{
namespace storage
{

class CBlockDB
{
public:
    CBlockDB();
    ~CBlockDB();
    bool Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn, const bool fFullDbIn);
    void Deinitialize();
    void RemoveAll();
    bool AddForkContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CForkContext>& mapForkCtxt, uint256& hashNewRoot);
    bool ListForkContext(std::map<uint256, CForkContext>& mapForkCtxt, const uint256& hashBlock = uint256());
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashMainChainRefBlock = uint256());
    bool UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock);
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkHashByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());
    bool GetForkHashByChainId(const CChainId nChainId, uint256& hashFork, const uint256& hashMainChainRefBlock = uint256());

    bool AddNewFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    bool RemoveFork(const uint256& hashFork);
    bool ListFork(std::vector<std::pair<uint256, uint256>>& vFork);
    bool AddNewBlockIndex(const CBlockOutline& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline);
    bool AddNewBlockNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint64 nBlockNumber, const uint256& hashBlock, uint256& hashNewRoot);
    bool RetrieveBlockHashByNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashLastBlock, const uint64 nBlockNumber, uint256& hashBlock);
    bool AddBlockVerify(const CBlockOutline& outline, const uint32 nRootCrc);
    bool RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock);
    std::size_t GetBlockVerifyCount();
    bool GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock);
    bool UpdateDelegateContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                               const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);
    bool RetrieveTxIndex(const uint256& hashFork, const uint256& txid, CTxIndex& txIndex);
    bool RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt);
    bool RetrieveDelegate(const uint256& hash, std::map<CDestination, uint256>& mapDelegate);
    bool RetrieveRangeEnroll(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote,
                      const std::map<CDestination, std::pair<uint32, uint32>>& mapAddPledgeFinalHeight, const std::map<CDestination, uint32>& mapRemovePledgeFinalHeight, uint256& hashVoteRoot);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool ListPledgeFinalHeight(const uint256& hashBlock, const uint32 nFinalHeight, std::map<CDestination, std::pair<uint32, uint32>>& mapPledgeFinalHeight);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, const CBlockRootStatus& statusBlockRoot, const std::map<CDestination, CDestState>& mapBlockState, uint256& hashBlockRoot);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::map<uint256, CTransactionReceipt>& mapBlockTxReceipts);
    bool UpdateBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx);
    bool AddBlockContractKvValue(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashContractRoot, const std::map<uint256, bytes>& mapContractState);
    bool RetrieveContractKvValue(const uint256& hashFork, const uint256& hashContractRoot, const uint256& key, bytes& value);
    bool AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CAddressContext>& mapAddress, const uint64 nNewAddressCount,
                           const std::map<CDestination, CTimeVault>& mapTimeVault, const std::map<uint32, CFunctionAddressContext>& mapFunctionAddress, uint256& hashNewRoot);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CAddressContext& ctxAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CContractAddressContext>& mapContractAddress);
    bool RetrieveTimeVault(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, CTimeVault& tv);
    bool GetAddressCount(const uint256& hashFork, const uint256& hashBlock, uint64& nAddressCount, uint64& nNewAddressCount);
    bool ListFunctionAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint32, CFunctionAddressContext>& mapFunctionAddress);
    bool RetrieveFunctionAddress(const uint256& hashFork, const uint256& hashBlock, const uint32 nFuncId, CFunctionAddressContext& ctxFuncAddress);
    bool AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContractSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode,
                        const std::map<uint256, CContractRunCodeContext>& mapContractRunCode,
                        const std::map<uint256, CTemplateContext>& mapTemplateData,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContractSourceCodeContext& ctxtCode);
    bool RetrieveContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractCreateCode, CContractCreateCodeContext& ctxtCode);
    bool RetrieveContractRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashContractRunCode, CContractRunCodeContext& ctxtCode);
    bool ListContractCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractCreateCodeContext>& mapContractCreateCode);

    bool AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool AddVoteReward(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint256& hashBlock, const uint32 nBlockHeight, const std::map<CDestination, uint256>& mapVoteReward, uint256& hashNewRoot);
    bool ListVoteReward(const uint32 nChainId, const uint256& hashBlock, const CDestination& dest, const uint32 nGetCount, std::vector<std::pair<uint32, uint256>>& vVoteReward);

    bool AddBlacklistAddress(const CDestination& dest);
    void RemoveBlacklistAddress(const CDestination& dest);
    bool IsExistBlacklistAddress(const CDestination& dest);
    void ListBlacklistAddress(set<CDestination>& setAddressOut);

    bool UpdateForkMintMinGasPrice(const uint256& hashFork, const uint256& nMinGasPrice);
    bool GetForkMintMinGasPrice(const uint256& hashFork, uint256& nMinGasPrice);

    bool VerifyBlockRoot(const bool fPrimary, const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                         const uint256& hashLocalStateRoot, CBlockRoot& localBlockRoot, const bool fVerifyAllNode = true);

protected:
    bool LoadAllFork();

protected:
    bool fCfgFullDb;

    CForkDB dbFork;
    CBlockIndexDB dbBlockIndex;
    CTxIndexDB dbTxIndex;
    CVoteDB dbVote;
    CStateDB dbState;
    CAddressDB dbAddress;
    CVerifyDB dbVerify;
    CContractDB dbContract;
    CAddressBlacklistDB dbAddressBlacklist;
    CCfgMintMinGasPriceDB dbMintMinGasPrice;

    CAddressTxInfoDB dbAddressTxInfo;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_BLOCKDB_H

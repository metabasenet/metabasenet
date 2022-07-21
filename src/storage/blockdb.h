// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_BLOCKDB_H
#define STORAGE_BLOCKDB_H

#include "addressdb.h"
#include "addresstxinfodb.h"
#include "block.h"
#include "blockindexdb.h"
#include "codedb.h"
#include "delegatedb.h"
#include "forkcontext.h"
#include "forkdb.h"
#include "invitedb.h"
#include "statedb.h"
#include "transaction.h"
#include "txindexdb.h"
#include "verifydb.h"
#include "votedb.h"
#include "voteredeemdb.h"
#include "wasmdb.h"

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
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt, const uint256& hashBlock = uint256());
    bool UpdateForkLast(const uint256& hashFork, const uint256& hashLastBlock);
    bool RetrieveForkLast(const uint256& hashFork, uint256& hashLastBlock);
    bool GetForkIdByForkName(const std::string& strForkName, uint256& hashFork, const uint256& hashBlock = uint256());
    bool GetForkIdByForkSn(const uint16 nForkSn, uint256& hashFork, const uint256& hashBlock = uint256());

    bool AddNewFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    bool RemoveFork(const uint256& hashFork);
    bool ListFork(std::vector<std::pair<uint256, uint256>>& vFork);
    bool AddNewBlockIndex(const CBlockOutline& outline);
    bool RemoveBlockIndex(const uint256& hashBlock);
    bool RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline);
    bool AddBlockVerify(const CBlockOutline& outline, const uint32 nRootCrc);
    bool RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock);
    std::size_t GetBlockVerifyCount();
    bool GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock);
    bool UpdateDelegateContext(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                               const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot);
    bool RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote);
    bool WalkThroughBlockIndex(CBlockDBWalker& walker);
    bool RetrieveTxIndex(const uint256& hashFork, const uint256& hashBlock, const uint256& txid, CTxIndex& txIndex);
    bool RetrieveDelegate(const uint256& hash, std::map<CDestination, uint256>& mapDelegate);
    bool RetrieveEnroll(const uint256& hash, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos);
    bool RetrieveEnroll(int height, const std::vector<uint256>& vBlockRange,
                        std::map<CDestination, CDiskPos>& mapEnrollTxPos);
    bool AddBlockVote(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteContext>& mapBlockVote, uint256& hashVoteRoot);
    bool RetrieveAllDelegateVote(const uint256& hashBlock, std::map<CDestination, std::map<CDestination, CVoteContext>>& mapDelegateVote);
    bool RetrieveDestVoteContext(const uint256& hashBlock, const CDestination& destVote, CVoteContext& ctxtVote);
    bool WalkThroughDayVote(const uint256& hashBeginBlock, const uint256& hashTailBlock, CDayVoteWalker& walker);
    bool AddBlockVoteRedeem(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteRedeemContext>& mapBlockVoteRedeem, uint256& hashVoteRedeemRoot);
    bool RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destVoteRedeem, CVoteRedeemContext& ctxtVoteRedeem);
    bool AddBlockState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState);
    bool CreateCacheStateTrie(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashBlockRoot, const std::map<CDestination, CDestState>& mapBlockState);
    bool RetrieveDestState(const uint256& hashFork, const uint256& hashBlockRoot, const CDestination& dest, CDestState& state);
    bool ListDestState(const uint256& hashFork, const uint256& hashBlockRoot, std::map<CDestination, CDestState>& mapBlockState);
    bool AddBlockTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, uint256& hashNewRoot);
    bool AddBlockWasmState(const uint256& hashFork, const uint256& hashPrevRoot, uint256& hashWasmRoot, const std::map<uint256, bytes>& mapWasmState);
    bool RetrieveWasmState(const uint256& hashFork, const uint256& hashWasmRoot, const uint256& key, bytes& value);
    bool AddAddressContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<uint256, CAddressContext>& mapAddress, uint256& hashNewRoot);
    bool RetrieveAddressContext(const uint256& hashFork, const uint256& hashBlock, const uint256& dest, CAddressContext& ctxtAddress);
    bool ListContractAddress(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CContractAddressContext>& mapContractAddress);
    bool AddCodeContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                        const std::map<uint256, CContracSourceCodeContext>& mapSourceCode,
                        const std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode,
                        const std::map<uint256, CWasmRunCodeContext>& mapWasmRunCode,
                        uint256& hashCodeRoot);
    bool RetrieveSourceCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashSourceCode, CContracSourceCodeContext& ctxtCode);
    bool RetrieveWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmCreateCode, CWasmCreateCodeContext& ctxtCode);
    bool RetrieveWasmRunCodeContext(const uint256& hashFork, const uint256& hashBlock, const uint256& hashWasmRunCode, CWasmRunCodeContext& ctxtCode);
    bool ListWasmCreateCodeContext(const uint256& hashFork, const uint256& hashBlock, std::map<uint256, CWasmCreateCodeContext>& mapWasmCreateCode);

    bool AddAddressTxInfo(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const uint64 nBlockNumber, const std::map<CDestination, std::vector<CDestTxInfo>>& mapAddressTxInfo, uint256& hashNewRoot);
    bool GetAddressTxCount(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, uint64& nTxCount);
    bool RetrieveAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nTxIndex, CDestTxInfo& ctxtAddressTxInfo);
    bool ListAddressTxInfo(const uint256& hashFork, const uint256& hashBlock, const CDestination& dest, const uint64 nBeginTxIndex, const uint64 nGetTxCount, const bool fReverse, std::vector<CDestTxInfo>& vAddressTxInfo);

    bool AddInviteRelation(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, CInviteContext>& mapInviteContext, uint256& hashNewRoot);
    bool RetrieveInviteParent(const uint256& hashFork, const uint256& hashBlock, const CDestination& destSub, CInviteContext& ctxInvite);
    bool ListInviteRelation(const uint256& hashFork, const uint256& hashBlock, std::map<CDestination, CInviteContext>& mapInviteContext);

    bool VerifyBlockRoot(const bool fPrimary, const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock,
                         const uint256& hashLocalStateRoot, CBlockRoot& localBlockRoot, const bool fVerifyAllNode = true);

protected:
    bool LoadAllFork();

protected:
    bool fCfgFullDb;

    CForkDB dbFork;
    CBlockIndexDB dbBlockIndex;
    CTxIndexDB dbTxIndex;
    CDelegateDB dbDelegate;
    CVoteDB dbVote;
    CVoteRedeemDB dbVoteRedeem;
    CStateDB dbState;
    CAddressDB dbAddress;
    CCodeDB dbCode;
    CInviteDB dbInvite;
    CVerifyDB dbVerify;
    CWasmDB dbWasm;

    CAddressTxInfoDB dbAddressTxInfo;
};

} // namespace storage
} // namespace metabasenet

#endif //STORAGE_BLOCKDB_H

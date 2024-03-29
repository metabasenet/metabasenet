// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include "bloomfilter/bloomfilter.h"
#include "contractdb.h"
#include "crypto.h"
#include "destination.h"
#include "param.h"
#include "statedb.h"
#include "structure/merkletree.h"
#include "template/activatecode.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/mint.h"
#include "template/pledge.h"
#include "template/proof.h"
#include "template/vote.h"
#include "wallet.h"

using namespace std;
using namespace mtbase;

#define DEBUG(err, ...) Debug((err), __FUNCTION__, __VA_ARGS__)

namespace metabasenet
{
///////////////////////////////
// CCoreProtocol

CCoreProtocol::CCoreProtocol()
{
    pBlockChain = nullptr;
    pForkManager = nullptr;
}

CCoreProtocol::~CCoreProtocol()
{
}

bool CCoreProtocol::HandleInitialize()
{
    InitializeGenesisBlock();
    if (!GetObject("blockchain", pBlockChain))
    {
        return false;
    }
    if (!GetObject("forkmanager", pForkManager))
    {
        return false;
    }
    return true;
}

Errno CCoreProtocol::Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...)
{
    string strFormat(pszFunc);
    strFormat += string(", ") + string(ErrorString(err)) + string(" : ") + string(pszFormat);
    va_list ap;
    va_start(ap, pszFormat);
    VDebug(strFormat.c_str(), ap);
    va_end(ap);
    return err;
}

void CCoreProtocol::InitializeGenesisBlock()
{
    CBlock block;
    GetGenesisBlock(block);
    hashGenesisBlock = block.GetHash();
}

const uint256& CCoreProtocol::GetGenesisBlockHash()
{
    return hashGenesisBlock;
}

CChainId CCoreProtocol::GetGenesisChainId() const
{
    return GENESIS_CHAINID;
}

uint256 CCoreProtocol::CreateGenesisStateRoot(const uint16 nBlockType, const uint64 nBlockTimeStamp, const CDestination& destOwner, const uint256& nAmount)
{
    CDestination destFuncContract(FUNCTION_CONTRACT_ADDRESS);
    CDestState stateContractDest;
    {
        uint256 hashCreateCode = crypto::CryptoKeccakHash(getFunctionContractCreateCode());
        uint256 hashRuntimeCode = crypto::CryptoKeccakHash(getFunctionContractRuntimeCode());

        CContractDestCodeContext ctxDestCode(hashCreateCode, hashRuntimeCode, destFuncContract);
        bytes btDestCodeData;
        CBufStream ss;
        ss << ctxDestCode;
        ss.GetData(btDestCodeData);

        std::map<uint256, bytes> mapContractState;
        mapContractState[destFuncContract.ToHash()] = btDestCodeData;

        uint256 hashKvRoot;
        if (!storage::CContractDB::CreateStaticContractStateRoot(mapContractState, hashKvRoot))
        {
            StdError("CCoreProtocol", "Create genesis state root: Create static contract state root fail");
            return {};
        }

        stateContractDest.SetType(CDestination::PREFIX_CONTRACT);
        stateContractDest.SetCodeHash(hashRuntimeCode);
        stateContractDest.SetStorageRoot(hashKvRoot);
    }

    std::map<CDestination, CDestState> mapBlockState;
    mapBlockState[destOwner] = CDestState(CDestination::PREFIX_PUBKEY, 0, nAmount);
    mapBlockState[destFuncContract] = stateContractDest;

    CBlockRootStatus statusBlockRoot(nBlockType, nBlockTimeStamp, destOwner);

    uint256 hashStateRoot;
    if (!storage::CStateDB::CreateStaticStateRoot(statusBlockRoot, mapBlockState, hashStateRoot))
    {
        StdError("CCoreProtocol", "Create genesis state root: Create static state root fail");
        return {};
    }
    return hashStateRoot;
}

void CCoreProtocol::CreateGenesisBlock(const bool fMainnet, const CChainId nChainIdIn, const string& strOwnerAddress, CBlock& block)
{
    const CDestination destOwner(strOwnerAddress);

    block.SetNull();

    block.nVersion = 1;
    block.nType = CBlock::BLOCK_GENESIS;
    block.SetBlockTime(1691539200); // ToDo
    block.nNumber = 0;
    block.nHeight = 0;
    block.nSlot = 0;
    block.hashPrev = 0;

    CTransaction& tx = block.txMint;
    tx.SetTxType(CTransaction::TX_GENESIS);
    tx.SetChainId(nChainIdIn);
    tx.SetToAddress(destOwner);
    tx.SetAmount(BBCP_TOKEN_INIT);

    tx.SetToAddressData(CAddressContext(CPubkeyAddressContext()));
    // ToDo
    string strData("Blockchain is more than just a financial instrument; it's a transformative technology. August 9, 2023, marks not only the debut of Hash Ahead but also the dawn of a new era in the widespread adoption of blockchain technology.");
    tx.AddTxData(CTransaction::DF_COMMON, bytes(strData.begin(), strData.end()));

    CProfile profile;
    profile.nType = CProfile::PROFILE_FORK_TYPE_MAIN;
    if (fMainnet)
    {
        profile.strName = "HAH Core";
        profile.strSymbol = "HAH";
    }
    else
    {
        profile.strName = "HAH Core test";
        profile.strSymbol = "HAHT";
    }
    profile.nChainId = nChainIdIn;
    profile.destOwner = destOwner;
    profile.nAmount = tx.GetAmount();
    profile.nMintReward = BBCP_REWARD_INIT;
    if (TESTNET_FLAG)
    {
        profile.nMinTxFee = MIN_GAS_PRICE * TX_BASE_GAS;
    }
    else
    {
        profile.nMinTxFee = MIN_GAS_PRICE_0 * TX_BASE_GAS;
    }
    profile.nHalveCycle = 0;

    block.AddForkProfile(profile);
    block.AddMintCoinProof(tx.GetAmount());

    block.hashStateRoot = CreateGenesisStateRoot(block.nType, block.GetBlockTime(), destOwner, tx.GetAmount());
    block.UpdateMerkleRoot();
}

void CCoreProtocol::GetGenesisBlock(CBlock& block)
{
    if (TESTMAINNET_FLAG)
    {
        // "privkey" : "0x3833f2e2681130850a10f2d1331430ef9e284b813a92ed094ff09533e8a89be4",
        // "pubkey" : "0x419877cf6a153bc4f26262cd8534a777c63cb998b60fb169036b19d3973c3878ba8fbf5918965a3f098555d34630457015875aaa6848bcfaab62b9d2304e1e7f",
        // "address" : "0xa7581ec78f00f8e30da9dd5ead10c2abbf125dcb"
        CreateGenesisBlock(true, GetGenesisChainId(), "0xa7581ec78f00f8e30da9dd5ead10c2abbf125dcb", block);
    }
    else
    {
        CreateGenesisBlock(true, GetGenesisChainId(), "0x6080e4a1775b482fd9ee04e59aa697848d583fef", block);
    }
}

Errno CCoreProtocol::ValidateTransaction(const uint256& hashTxAtFork, const uint256& hashMainChainRefBlock, const CTransaction& tx)
{
    if (tx.GetTxType() != CTransaction::TX_GENESIS)
    {
        uint256 hashTempFork;
        if (!pBlockChain->GetForkHashByChainId(tx.GetChainId(), hashTempFork, hashMainChainRefBlock))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx chainid error, chainid: %d, txid: %s", tx.GetChainId(), tx.GetHash().GetHex().c_str());
        }
        if (hashTempFork != hashTxAtFork)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx fork hash error, txid: %s", tx.GetHash().GetHex().c_str());
        }
    }
    if (tx.GetTxType() == CTransaction::TX_WORK)
    {
        if (tx.GetTxDataCount() > 0)
        {
            CTransaction txTemp;
            const CDestination destMint(crypto::CPubKey(~uint512()));
            const CDestination destOwner(crypto::CPubKey(~uint512()));
            CTemplatePtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(destMint, destOwner));
            CTemplateAddressContext ctxTemplate(string(), string(), ptr->GetTemplateType(), ptr->Export());
            txTemp.SetToAddressData(CAddressContext(ctxTemplate));

            if (tx.GetTxDataCount() > 1 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
            }
        }
    }
    else if (tx.GetTxType() == CTransaction::TX_STAKE)
    {
        if (tx.GetTxDataCount() > 0)
        {
            CTransaction txTemp;

            const CDestination destMint(crypto::CPubKey(~uint512()));
            const CDestination destOwner(crypto::CPubKey(~uint512()));
            CTemplatePtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateDelegate(destMint, destOwner, 500));
            CTemplateAddressContext ctxTemplate(string(), string(), ptr->GetTemplateType(), ptr->Export());
            txTemp.SetToAddressData(CAddressContext(ctxTemplate));

            if (tx.GetTxDataCount() > 1 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
            }
        }
    }
    else if (tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
    {
        CTransaction txTemp;

        const CDestination destDelegate(CTemplateId(~uint256()));
        const CDestination destOwner(crypto::CPubKey(~uint512()));
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(new CTemplateVote(destDelegate, destOwner, 0));
        if (!ptr)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "create template fail");
        }
        CTemplateAddressContext ctxTemplate(string(), string(), ptr->GetTemplateType(), ptr->Export());
        txTemp.SetToAddressData(CAddressContext(ctxTemplate));

        if (tx.GetTxDataCount() > 1 || tx.GetTxDataSize() > txTemp.GetTxDataSize())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx data error, tx type: %s", tx.GetTypeString().c_str());
        }
    }
    if (!tx.GetSignData().empty() && tx.IsRewardTx())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid signature");
    }
    if (tx.GetToAddress().IsNull())
    {
        uint8 nCodeType;
        CTemplateContext ctxTemplate;
        CTxContractData ctxContract;
        if (!tx.GetCreateCodeContext(nCodeType, ctxTemplate, ctxContract))
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "send to null address");
        }
    }
    if (!MoneyRange(tx.GetAmount()))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "amount overflow, amount: %s", CoinToTokenBigFloat(tx.GetAmount()).c_str());
    }
    if (!MoneyRange(tx.GetTxFee()))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee overflow, fee: %s", CoinToTokenBigFloat(tx.GetTxFee()).c_str());
    }
    if (tx.GetTxType() == CTransaction::TX_TOKEN
        || tx.GetTxType() == CTransaction::TX_ETH_CREATE_CONTRACT
        || tx.GetTxType() == CTransaction::TX_ETH_MESSAGE_CALL)
    {
        if (tx.GetGasPrice() < MIN_GAS_PRICE)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "gas price invalid, tx gas price: %s, min gas price: %s",
                         CoinToTokenBigFloat(tx.GetGasPrice()).c_str(), CoinToTokenBigFloat(MIN_GAS_PRICE).c_str());
        }
        uint256 nNeedGas = tx.GetTxBaseGas();
        if (tx.GetGasLimit() < nNeedGas)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "gas invalid, tx gas: %lu, need gas: %lu",
                         tx.GetGasLimit().Get64(), nNeedGas.Get64());
        }
    }
    else
    {
        if (tx.GetTxFee() != 0)
        {
            return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "non token tx, txfee nonzero, fee: %s",
                         CoinToTokenBigFloat(tx.GetTxFee()).c_str());
        }
    }

    if (isFunctionContractAddress(tx.GetFromAddress()))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "from is function contract error");
    }
    if (isFunctionContractAddress(tx.GetToAddress()))
    {
        if (tx.GetAmount() > 0)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx is function contract, amount error, amount: %s", CoinToTokenBigFloat(tx.GetAmount()).c_str());
        }
        bytes btTxData;
        if (tx.IsEthTx())
        {
            if (!tx.GetTxData(CTransaction::DF_ETH_TX_DATA, btTxData) || btTxData.size() < 4)
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx is function contract, tx data error");
            }
        }
        else
        {
            if (!tx.GetTxData(CTransaction::DF_CONTRACTPARAM, btTxData) || btTxData.size() < 4)
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx is function contract, tx data error");
            }
        }
    }

    if (GetSerializeSize(tx) > MAX_TX_SIZE)
    {
        return DEBUG(ERR_TRANSACTION_OVERSIZE, "tx size: %u", GetSerializeSize(tx));
    }
    return OK;
}

Errno CCoreProtocol::ValidateBlock(const uint256& hashFork, const uint256& hashMainChainRefBlock, const CBlock& block)
{
    // These are checks that are independent of context
    // Only allow CBlock::BLOCK_PRIMARY type in v1.0.0
    /*if (block.nType != CBlock::BLOCK_PRIMARY)
    {
        return DEBUG(ERR_BLOCK_TYPE_INVALID, "Block type error");
    }*/
    // Check timestamp
    if (block.GetBlockTime() > GetNetTime() + MAX_CLOCK_DRIFT)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "%ld", block.GetBlockTime());
    }

    if (!block.VerifyBlockHeight())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "block height error, height: %d, prev height: %d, block type: %d",
                     block.nHeight, CBlock::GetBlockHeightByHash(block.hashPrev), block.nType);
    }
    if (!block.VerifyBlockProof())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "block proof error");
    }

    // validate vacant block
    if (block.nType == CBlock::BLOCK_VACANT)
    {
        if (block.txMint.GetAmount() != 0 || block.txMint.GetTxFee() != 0 || block.txMint.GetTxType() != CTransaction::TX_STAKE || block.txMint.GetToAddress().IsNull())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, nAmount: %lu, tx fee: %lu, nType: %d, to: %s",
                         block.txMint.GetAmount(), block.txMint.GetTxFee(), block.txMint.GetTxType(),
                         (block.txMint.GetToAddress().IsNull() ? "null" : block.txMint.GetToAddress().ToString().c_str()));
        }
        if (!block.vtx.empty())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block vtx is not empty");
        }
    }

    // Validate mint tx
    if (!block.txMint.IsMintTx() || ValidateTransaction(hashFork, hashMainChainRefBlock, block.txMint) != OK)
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, tx type: %d", block.txMint.GetTxType());
    }

    size_t nBlockSize = GetSerializeSize(block);
    if (nBlockSize > MAX_BLOCK_SIZE)
    {
        return DEBUG(ERR_BLOCK_OVERSIZE, "size overflow size=%u vtx=%u", nBlockSize, block.vtx.size());
    }

    if (block.nType == CBlock::BLOCK_ORIGIN && !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "origin block vtx is not empty");
    }

    if (!block.VerifyBlockMerkleTreeRoot())
    {
        return DEBUG(ERR_BLOCK_TXHASH_MISMATCH, "tx merkleroot mismatched");
    }

    set<uint256> setTx;
    for (auto& tx : block.vtx)
    {
        setTx.insert(tx.GetHash());
    }
    if (setTx.size() != block.vtx.size())
    {
        return DEBUG(ERR_BLOCK_DUPLICATED_TRANSACTION, "duplicate tx");
    }

    for (const CTransaction& tx : block.vtx)
    {
        if (tx.IsMintTx() || ValidateTransaction(hashFork, hashMainChainRefBlock, tx) != OK)
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid tx %s", tx.GetHash().GetHex().c_str());
        }
    }

    if (!CheckBlockSignature(hashFork, block))
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "Check block signature fail");
    }
    return OK;
}

Errno CCoreProtocol::VerifyForkCreateTx(const uint256& hashTxAtFork, const CTransaction& tx, const int nHeight, const uint256& hashMainChainPrevBlock, const bytes& btToAddressData)
{
    if (hashTxAtFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork");
    }
    if (tx.GetToAddress() == tx.GetFromAddress())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }

    bytes btTempData;
    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid tx data");
    }
    CBlock block;
    try
    {
        CBufStream ss(btTempData);
        ss >> block;
    }
    catch (std::exception& e)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork data");
    }
    if (!block.IsOrigin() || block.IsPrimary())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
    }
    CProfile profile;
    if (!block.GetForkProfile(profile))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    uint256 hashNewFork = block.GetHash();

    if (profile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    if (!MoneyRange(profile.nAmount))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    }
    if (!RewardRange(profile.nMintReward))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    }
    if (block.txMint.GetToAddress() != profile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork to");
    }

    if (profile.IsClonemapFork())
    {
        CBlockStatus statusGen;
        if (!pBlockChain->GetBlockStatus(GetGenesisBlockHash(), statusGen))
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "Get genesis block status fail");
        }
        if (profile.destOwner != statusGen.destMint || tx.GetFromAddress() != statusGen.destMint)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "Must be the main chain creator");
        }

        if (profile.nAmount != 0 || profile.nMintReward != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "Must amount and mintreward is 0");
        }
        if (tx.GetAmount() != 0)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Must tx amount is 0");
        }
    }
    else
    {
        if (!fCreateUserForkEnable)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "disable create user fork");
        }
        if (tx.GetAmount() < MORTGAGE_BASE)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid tx amount");
        }
    }

    if (ValidateBlock(hashNewFork, hashMainChainPrevBlock, block) != OK)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
    }

    CTemplatePtr ptr = CTemplate::Import(btToAddressData);
    if (!ptr)
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid vchSig");
    }
    if (tx.GetToAddress() != CDestination(ptr->GetTemplateId()))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid template id");
    }
    if (boost::dynamic_pointer_cast<CTemplateFork>(ptr)->hashFork != hashNewFork)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid forkid");
    }
    return OK;
}

Errno CCoreProtocol::VerifyForkRedeemTx(const uint256& hashTxAtFork, const CTransaction& tx, const uint256& hashPrevBlock, const CDestState& state, const bytes& btFromAddressData)
{
    if (hashTxAtFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid fork");
    }
    if (tx.GetToAddress() == tx.GetFromAddress())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }

    uint256 nLockedAmount;
    if (!pBlockChain->GetCreateForkLockedAmount(tx.GetFromAddress(), hashPrevBlock, btFromAddressData, nLockedAmount))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid locked coin template destination");
    }

    // locked coin template: nValueIn >= tx.GetAmount() + tx.GetTxFee() + nLockedAmount
    if (state.GetBalance() < tx.GetAmount() + tx.GetTxFee() + nLockedAmount)
    {
        return DEBUG(ERR_TRANSACTION_IS_LOCKED, "balance is not enough to locked coin, balance: %s, use: %s",
                     CoinToTokenBigFloat(state.GetBalance()).c_str(), CoinToTokenBigFloat(tx.GetAmount() + tx.GetTxFee() + nLockedAmount).c_str());
    }
    return OK;
}

Errno CCoreProtocol::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile)
{
    if (!block.GetForkProfile(forkProfile))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "load profile error");
    }
    if (forkProfile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    if (forkProfile.strSymbol.empty() || forkProfile.strSymbol.size() > MAX_COIN_SYMBOL_SIZE)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid symbol");
    }
    if (forkProfile.strName.empty() || forkProfile.strName.size() > MAX_FORK_NAME_SIZE)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid name");
    }
    if (!MoneyRange(forkProfile.nAmount))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    }
    if (!RewardRange(forkProfile.nMintReward))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    }
    if (block.txMint.GetToAddress() != forkProfile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork to");
    }
    CBlockVoteSig proofVote;
    if (block.GetBlockVoteSig(proofVote))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block vote");
    }
    if (forkProfile.nChainId != block.txMint.GetChainId())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid chainid");
    }
    if (!CProfile::VerifySubforkType(forkProfile.nType))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid type");
    }
    if (forkProfile.IsClonemapFork())
    {
        if (forkProfile.nAmount != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
        }
        if (forkProfile.nMintReward != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
        }
    }
    else
    {
        if (!fCreateUserForkEnable)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "disable create user fork");
        }
    }
    return OK;
}

Errno CCoreProtocol::VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev)
{
    uint64 nNextTimestamp = GetNextBlockTimestamp(pIndexPrev->GetBlockTime());
    if (block.GetBlockTime() < nNextTimestamp)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Verify proof work: Timestamp out of range 2, height: %d, block time: %lu, next time: %lu, prev minttype: 0x%x, prev time: %lu, block: %s.",
                     block.GetBlockHeight(), block.GetBlockTime(), nNextTimestamp,
                     pIndexPrev->nMintType, pIndexPrev->GetBlockTime(), block.GetHash().GetHex().c_str());
    }

    CProofOfHashWork proof;
    if (!block.GetHashWorkProof(proof))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "load proof fail.");
    }

    // int nBits = 0;
    // if (!GetProofOfWorkTarget(pIndexPrev, proof.nAlgo, nBits))
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "get target fail.");
    // }

    // if (nBits != proof.nBits || proof.nAlgo != CM_CRYPTONIGHT)
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "algo or bits error, nAlgo: %d, nBits: %d.", proof.nAlgo, proof.nBits);
    // }
    // if (proof.destMint != block.txMint.GetToAddress())
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "destMint error, destMint: %s.", proof.destMint.ToString().c_str());
    // }

    // uint256 hashTarget = (~uint256(uint64(0)) >> nBits);

    // vector<unsigned char> vchProofOfWork;
    // block.GetSerializedProofOfWorkData(vchProofOfWork);
    // uint256 hash = crypto::CryptoPowHash(&vchProofOfWork[0], vchProofOfWork.size());

    // if (hash > hashTarget)
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "hash error: proof[%s] vs. target[%s] with bits[%d]",
    //                  hash.ToString().c_str(), hashTarget.ToString().c_str(), nBits);
    // }

    return OK;
}

Errno CCoreProtocol::VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                                 const CDelegateAgreement& agreement)
{
    uint64 nTime = GetNextBlockTimestamp(pIndexPrev->GetBlockTime());
    if (block.GetBlockTime() < nTime)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range. block time %lu is not equal %lu", block.GetBlockTime(), nTime);
    }
    if (block.txMint.GetToAddress() != agreement.vBallot[0])
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint to error.");
    }
    // if (block.txMint.GetTxTime() != block.GetBlockTime())
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    // }
    return OK;
}

Errno CCoreProtocol::VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                      const CDelegateAgreement& agreement)
{
    if (block.GetBlockTime() <= pIndexPrev->GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range.");
    }

    if (block.IsSubsidiary())
    {
        if (block.GetBlockSlot() != 0)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Subsidiary slot out of range.");
        }
        if (block.GetBlockTime() != pIndexRef->GetBlockTime())
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Subsidiary timestamp out of range.");
        }
    }
    else
    {
        if (block.GetBlockSlot() >= BLOCK_TARGET_SPACING / EXTENDED_BLOCK_SPACING)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended slot out of range.");
        }
        if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
            || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp out of range.");
        }
        if (((block.GetBlockTime() - pIndexRef->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp error.");
        }
        if (((block.GetBlockTime() - pIndexRef->GetBlockTime()) / EXTENDED_BLOCK_SPACING) != block.GetBlockSlot())
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp no align slot.");
        }
    }

    if (block.txMint.GetToAddress() != agreement.GetBallot(0))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint to error.");
    }

    // if (block.txMint.GetTxTime() != block.GetBlockTime())
    // {
    //     return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    // }
    return OK;
}

Errno CCoreProtocol::VerifyTransaction(const uint256& txid, const CTransaction& tx, const uint256& hashFork, const uint256& hashPrevBlock, const int nAtHeight, const CDestState& stateFrom, const std::map<CDestination, CAddressContext>& mapBlockAddress)
{
    Errno err = OK;
    if (stateFrom.GetBalance() < tx.GetAmount() + tx.GetTxFee())
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "balance is not enough, balance: %s, tx amount: %s, from: %s, txid: %s",
                     CoinToTokenBigFloat(stateFrom.GetBalance()).c_str(), CoinToTokenBigFloat(tx.GetAmount() + tx.GetTxFee()).c_str(),
                     tx.GetFromAddress().ToString().c_str(), txid.ToString().c_str());
    }
    if (tx.GetTxType() != CTransaction::TX_CERT && !tx.GetFromAddress().IsNull())
    {
        if (!stateFrom.VerifyTxNonce(tx.GetNonce()))
        {
            return DEBUG(ERR_MISSING_PREV, "invalid nonce, tx nonce: %ld, state nonce: %ld, from: %s, to: %s, txid: %s",
                         tx.GetNonce(), stateFrom.GetTxNonce(), tx.GetFromAddress().ToString().c_str(),
                         tx.GetToAddress().ToString().c_str(), txid.ToString().c_str());
        }
    }

    if (tx.GetTxType() == CTransaction::TX_CERT)
    {
        if ((err = VerifyCertTx(hashFork, tx, mapBlockAddress)) != OK)
        {
            return DEBUG(err, "invalid cert tx, txid: %s", txid.ToString().c_str());
        }
    }

    uint16 nFromTemplateType = 0;
    uint16 nToTemplateType = 0;
    bool fToContract = false;
    CTemplateAddressContext ctxFromTemplate;
    CTemplateAddressContext ctxToTemplate;

    if (!tx.GetFromAddress().IsNull())
    {
        auto it = mapBlockAddress.find(tx.GetFromAddress());
        if (it != mapBlockAddress.end())
        {
            const CAddressContext& ctxAddress = it->second;
            if (ctxAddress.IsTemplate())
            {
                if (!ctxAddress.GetTemplateAddressContext(ctxFromTemplate))
                {
                    return DEBUG(ERR_TRANSACTION_INVALID, "invalid from template address, txid: %s", txid.ToString().c_str());
                }
                nFromTemplateType = ctxAddress.GetTemplateType();
            }
        }
    }
    if (!tx.GetToAddress().IsNull())
    {
        auto mt = mapBlockAddress.find(tx.GetToAddress());
        if (mt != mapBlockAddress.end())
        {
            const CAddressContext& ctxAddress = mt->second;
            if (ctxAddress.IsTemplate())
            {
                if (!ctxAddress.GetTemplateAddressContext(ctxToTemplate))
                {
                    return DEBUG(ERR_TRANSACTION_INVALID, "invalid to template address, txid: %s", txid.ToString().c_str());
                }
                nToTemplateType = ctxAddress.GetTemplateType();
            }
            else if (ctxAddress.IsContract())
            {
                fToContract = true;
            }
        }
    }

    if (nToTemplateType == TEMPLATE_VOTE)
    {
        err = VerifyVoteTx(hashFork, tx, hashPrevBlock, ctxToTemplate);
        if (err != OK)
        {
            return DEBUG(err, "invalid vote tx, txid: %s", txid.ToString().c_str());
        }
    }
    if (nToTemplateType == TEMPLATE_PLEDGE)
    {
        err = VerifyPledgeTx(hashFork, tx, hashPrevBlock, ctxToTemplate);
        if (err != OK)
        {
            return DEBUG(err, "invalid pledge tx, txid: %s", txid.ToString().c_str());
        }
    }
    if ((nToTemplateType != TEMPLATE_DELEGATE && nToTemplateType != TEMPLATE_VOTE && nToTemplateType != TEMPLATE_PLEDGE)
        && (nFromTemplateType == TEMPLATE_DELEGATE || nFromTemplateType == TEMPLATE_VOTE || nFromTemplateType == TEMPLATE_PLEDGE))
    {
        err = VerifyVoteRedeemTx(hashFork, tx, nFromTemplateType, hashPrevBlock);
        if (err != OK)
        {
            return DEBUG(err, "invalid redeem tx, txid: %s", txid.ToString().c_str());
        }
    }

    err = VerifyVoteRewardLockTx(hashFork, tx, hashPrevBlock, stateFrom);
    if (err != OK)
    {
        return DEBUG(err, "invalid vote reward redeem tx, txid: %s", txid.ToString().c_str());
    }

    if (nFromTemplateType == TEMPLATE_FORK)
    {
        err = VerifyForkRedeemTx(hashFork, tx, hashPrevBlock, stateFrom, ctxFromTemplate.btData);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork redeem fail, txid: %s", txid.ToString().c_str());
        }
    }

    if (nToTemplateType == TEMPLATE_FORK)
    {
        err = VerifyForkCreateTx(hashFork, tx, nAtHeight, hashPrevBlock, ctxToTemplate.btData);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork tx fail, txid: %s", txid.ToString().c_str());
        }
    }

    if (nToTemplateType == TEMPLATE_ACTIVATECODE && fEnableContractCodeVerify)
    {
        err = VerifyActivateCodeTx(hashFork, hashPrevBlock, tx);
        if (err != OK)
        {
            return DEBUG(err, "Verify activate code tx fail, txid: %s", txid.ToString().c_str());
        }
    }

    if (tx.GetToAddress().IsNull())
    {
        if (!pBlockChain->VerifyCreateCodeTx(hashFork, hashPrevBlock, tx))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Verify create code tx fail, txid: %s", txid.ToString().c_str());
        }
    }
    else if (fToContract)
    {
        if (!pBlockChain->VerifyContractAddress(hashFork, hashPrevBlock, tx.GetToAddress()))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Verify contract address fail, txid: %s", txid.ToString().c_str());
        }
    }

    CDestination destSign;
    if (nFromTemplateType != 0)
    {
        auto ptr = CTemplate::Import(ctxFromTemplate.btData);
        if (!ptr)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Import template data fail, txid: %s", txid.ToString().c_str());
        }
        if (!ptr->GetSignDestination(tx, destSign))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Get sign destination fail, txid: %s", txid.ToString().c_str());
        }
    }
    else
    {
        destSign = tx.GetFromAddress();
    }
    if (!tx.VerifyTxSignature(destSign))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "Invalid signature, txid: %s", txid.ToString().c_str());
    }
    return OK;
}

bool CCoreProtocol::GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev, const CDelegateAgreement& agreement, const CBlockIndex* pIndexRef, const uint256& nEnrollTrust)
{
    //int32 nHeight = block.GetBlockHeight();
    if (block.IsGenesis())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsVacant())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsPrimary())
    {
        if (block.IsProofOfWork())
        {
            // uint256 v(1);
            // nChainTrust = (v << 8);
            CBufStream ss;
            ss << block.GetBlockHeight() << block.txMint.GetToAddress();
            uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());
            nChainTrust = hash.GetUint16Index();
            if (nChainTrust == 0)
            {
                StdLog("CoreProtocol", "Get Block Trust: Proof of work, nChainTrust is 0");
                CBufStream ss;
                ss << (block.GetBlockHeight() - 1) << block.txMint.GetToAddress();
                uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());
                nChainTrust = hash.GetUint16Index();
                if (nChainTrust == 0)
                {
                    nChainTrust = 1;
                    StdLog("CoreProtocol", "Get Block Trust: Proof of work, nChainTrust is 0, set nChainTrust is 1");
                }
            }
        }
        else if (pIndexPrev != nullptr)
        {
            // Get the last PoW block nAlgo
            int nAlgo;
            const CBlockIndex* pIndex = pIndexPrev;
            while (!pIndex->IsProofOfWork() && (pIndex->pPrev != nullptr))
            {
                pIndex = pIndex->pPrev;
            }
            if (!pIndex->IsProofOfWork())
            {
                nAlgo = CM_CRYPTONIGHT;
            }
            else
            {
                nAlgo = pIndex->nProofAlgo;
            }

            int nBits;
            if (GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits))
            {
                if (agreement.nWeight == 0 || nBits <= 0)
                {
                    StdError("CoreProtocol", "Get Block Trust: nWeight or nBits error, nWeight: %lu, nBits: %d", agreement.nWeight, nBits);
                    return false;
                }
                // if (nEnrollTrust <= 0)
                // {
                //     StdError("CoreProtocol", "Get Block Trust: nEnrollTrust error, nEnrollTrust: %lu", nEnrollTrust);
                //     return false;
                // }

                // PoS difficulty = 2 ^ (nBits + weight)
                //nChainTrust = uint256(1) << (int(nEnrollTrust) + nBits + 20);
                nChainTrust = nEnrollTrust;
            }
            else
            {
                StdError("CoreProtocol", "Get Block Trust: Get ProofOfWork Target fail");
                return false;
            }
        }
        else
        {
            StdError("CoreProtocol", "Get Block Trust: Primary pIndexPrev is null");
            return false;
        }
    }
    else if (block.IsOrigin())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsSubsidiary() || block.IsExtended())
    {
        if (pIndexRef == nullptr)
        {
            StdError("CoreProtocol", "Get Block Trust: pIndexRef is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        if (pIndexRef->pPrev == nullptr)
        {
            StdError("CoreProtocol", "Get Block Trust: Subsidiary or Extended block pPrev is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        nChainTrust = pIndexRef->nChainTrust - pIndexRef->pPrev->nChainTrust;
    }
    else
    {
        StdError("CoreProtocol", "Get Block Trust: block type error");
        return false;
    }
    return true;
}

bool CCoreProtocol::GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits)
{
    nBits = 8;
    return true;
}

uint256 CCoreProtocol::GetDelegatedBallot(const int nBlockHeight, const uint256& nAgreement, const size_t& nWeight, const map<CDestination, size_t>& mapBallot,
                                          const vector<pair<CDestination, uint256>>& vecAmount, const uint256& nMoneySupply, vector<CDestination>& vBallot)
{
    vBallot.clear();
    if (nAgreement == 0 || mapBallot.size() == 0)
    {
        StdTrace("CoreProtocol", "Get delegated ballot: height: %d, nAgreement: %s, mapBallot.size: %ld", nBlockHeight, nAgreement.GetHex().c_str(), mapBallot.size());
        return 0;
    }
    if (nMoneySupply == 0)
    {
        StdTrace("CoreProtocol", "Get delegated ballot: nMoneySupply == 0");
        return 0;
    }
    if (vecAmount.size() != mapBallot.size())
    {
        StdError("CoreProtocol", "Get delegated ballot: dest ballot size %llu is not equal amount size %llu", mapBallot.size(), vecAmount.size());
    }

    uint32 nSelected = 0;
    for (uint32 i = 0; i < nAgreement.size() / sizeof(uint32); ++i)
    {
        nSelected ^= nAgreement.Get32(i);
    }

    map<CDestination, size_t> mapSelectBallot;
    uint256 nEnrollTrust;
    size_t nEnrollWeight = 0;

    for (auto& amount : vecAmount)
    {
        if (mapBallot.find(amount.first) != mapBallot.end())
        {
            uint256 nMinAmount;
            if (amount.second < DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT)
            {
                nMinAmount = amount.second;
            }
            else
            {
                nMinAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT;
            }
            size_t nDestWeight = (nMinAmount / DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT).Get64();
            mapSelectBallot[amount.first] = nDestWeight;
            nEnrollWeight += nDestWeight;
            nEnrollTrust += nMinAmount;

            StdTrace("CoreProtocol", "Get delegated ballot: Enrolled, height: %d, ballot dest: %s, weight: %lld, vote amount: %s",
                     nBlockHeight, amount.first.ToString().c_str(), nDestWeight, CoinToTokenBigFloat(amount.second).c_str());
        }
        else
        {
            StdTrace("CoreProtocol", "Get delegated ballot: Not enrolled, height: %d, vote dest: %s, vote amount: %s",
                     nBlockHeight, amount.first.ToString().c_str(), CoinToTokenBigFloat(amount.second).c_str());
        }
    }
    if (nEnrollWeight == 0)
    {
        StdError("CoreProtocol", "Get delegated ballot: Enroll weight error, dest ballot size %llu is not equal amount size %llu", mapBallot.size(), vecAmount.size());
        return 0;
    }

    size_t n = nSelected % nEnrollWeight;
    for (auto it = mapSelectBallot.begin(); it != mapSelectBallot.end(); ++it)
    {
        if (n < it->second)
        {
            vBallot.push_back(it->first);
            break;
        }
        n -= it->second;
    }

    StdTrace("CoreProtocol", "Get delegated ballot: consensus: %s, height: %d, ballot count: %lu, enroll trust: %s, ballot address: %s",
             (vBallot.size() > 0 ? "pos" : "poa"), nBlockHeight, mapSelectBallot.size(),
             CoinToTokenBigFloat(nEnrollTrust).c_str(), (vBallot.size() > 0 ? vBallot[0].ToString().c_str() : ""));

    return nEnrollTrust;
}

uint64 CCoreProtocol::GetNextBlockTimestamp(const uint64 nPrevTimeStamp)
{
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

bool CCoreProtocol::CheckBlockSignature(const uint256& hashFork, const CBlock& block)
{
    const uint256 hashBlock = block.GetHash();
    if (hashBlock != GetGenesisBlockHash())
    {
        // if (!block.txMint.GetToAddress().IsTemplate())
        // {
        //     StdLog("CoreProtocol", "Check Block Signature: txMint not template address, block: %s", hashBlock.GetHex().c_str());
        //     return false;
        // }

        if (block.txMint.GetTxType() == CTransaction::TX_GENESIS)
        {
            if (!block.VerifyBlockSignature(block.txMint.GetToAddress()))
            {
                StdLog("CoreProtocol", "Check Block Signature: Verify block sign fail, block: %s", hashBlock.GetHex().c_str());
                return false;
            }
        }
        else
        {
            CTemplatePtr ptr = pBlockChain->GetTxToAddressTemplatePtr(hashFork, block.hashPrev, block.txMint);
            if (!ptr || (ptr->GetTemplateType() != TEMPLATE_PROOF && ptr->GetTemplateType() != TEMPLATE_DELEGATE))
            {
                if (!ptr)
                {
                    StdLog("CoreProtocol", "Check Block Signature: Get template fail, block: %s, to: %s", hashBlock.GetHex().c_str(), block.txMint.GetToAddress().ToString().c_str());
                }
                else
                {
                    StdLog("CoreProtocol", "Check Block Signature: Template error, template type: %d, block: %s, to: %s", ptr->GetTemplateType(), hashBlock.GetHex().c_str(), block.txMint.GetToAddress().ToString().c_str());
                }
                return false;
            }
            CTemplateMintPtr ptrMint = boost::dynamic_pointer_cast<CTemplateMint>(ptr);

            CDestination destBlockSign;
            ptrMint->GetBlockSignDestination(destBlockSign);
            if (!block.VerifyBlockSignature(destBlockSign))
            {
                StdLog("CoreProtocol", "Check Block Signature: Verify block sign fail, block: %s", hashBlock.GetHex().c_str());
                return false;
            }
        }
    }
    return true;
}

Errno CCoreProtocol::ValidateVacantBlock(const CBlock& block)
{
    if (block.hashMerkleRoot != 0 || block.txMint != CTransaction() || !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block tx is not empty.");
    }

    if (!block.IsProofEmpty() || !block.vchSig.empty())
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "vacant block proof or signature is not empty.");
    }

    return OK;
}

Errno CCoreProtocol::VerifyCertTx(const uint256& hashFork, const CTransaction& tx, const std::map<CDestination, CAddressContext>& mapBlockAddress)
{
    // CERT transaction must be on the main chain
    if (hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify cert tx: CERT tx is not on the main chain, fork: %s", hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // the from address must be equal to the to address of cert tx
    if (tx.GetToAddress().IsNull() || tx.GetFromAddress() != tx.GetToAddress())
    {
        StdLog("CoreProtocol", "Verify cert tx: From address is not equal the to address of CERT tx, from: %s, to: %s",
               tx.GetFromAddress().ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    auto it = mapBlockAddress.find(tx.GetToAddress());
    if (it == mapBlockAddress.end())
    {
        StdLog("CoreProtocol", "Verify cert tx: Find address context fail, from: %s, to: %s",
               tx.GetFromAddress().ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    if (!it->second.IsTemplate() || it->second.GetTemplateType() != TEMPLATE_DELEGATE)
    {
        StdLog("CoreProtocol", "Verify cert tx: not template address, from: %s, to: %s",
               tx.GetFromAddress().ToString().c_str(), tx.GetToAddress().ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    return OK;
}

Errno CCoreProtocol::VerifyVoteTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CTemplateAddressContext& ctxToTemplate)
{
    uint256 txid = tx.GetHash();

    if (!fEnableStakeVote)
    {
        StdLog("CoreProtocol", "Verify Vote Tx: Disable vote, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // VOTE transaction must be on the main chain
    if (hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify Vote Tx: From or to vote template address tx is not on the main chain, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.GetToAddress() == tx.GetFromAddress())
    {
        StdLog("CoreProtocol", "Verify Vote Tx: From and to addresses cannot be the same, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.GetAmount() == 0)
    {
        StdLog("CoreProtocol", "Verify Vote Tx: Amount quantity is 0, nAmount: 0, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // CTemplatePtr ptr = CTemplate::Import(ctxToTemplate.btData);
    // if (!ptr || ptr->GetTemplateType() != TEMPLATE_VOTE)
    // {
    //     StdLog("CoreProtocol", "Verify Vote Tx: Import vote template fail or template type error, txid: %s", txid.GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    // auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);

    // CAddressContext ctxOwnerAddress;
    // if (!pBlockChain->RetrieveAddressContext(hashFork, hashPrev, objVote->destOwner, ctxOwnerAddress))
    // {
    //     StdLog("CoreProtocol", "Verify Vote Tx: Vote owner address not created, owner address: %s, txid: %s", objVote->destOwner.ToString().c_str(), txid.GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    // if (!ctxOwnerAddress.IsPubkey())
    // {
    //     StdLog("CoreProtocol", "Verify Vote Tx: Vote owner address not public key address, owner address: %s, txid: %s", objVote->destOwner.ToString().c_str(), txid.GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    return OK;
}

Errno CCoreProtocol::VerifyPledgeTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CTemplateAddressContext& ctxToTemplate)
{
    uint256 txid = tx.GetHash();

    if (!fEnableStakePledge)
    {
        StdLog("CoreProtocol", "Verify Vote Tx: Disable pledge, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // Pledge transaction must be on the main chain
    if (hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify Pledge Tx: From or to pledge template address tx is not on the main chain, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.GetToAddress() == tx.GetFromAddress())
    {
        StdLog("CoreProtocol", "Verify Pledge Tx: From and to addresses cannot be the same, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.GetAmount() == 0)
    {
        StdLog("CoreProtocol", "Verify Pledge Tx: Amount quantity is 0, nAmount: 0, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CTemplatePtr ptr = CTemplate::Import(ctxToTemplate.btData);
    if (!ptr || ptr->GetTemplateType() != TEMPLATE_PLEDGE)
    {
        StdLog("CoreProtocol", "Verify Pledge Tx: Import pledge template fail or template type error, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    auto objPledge = boost::dynamic_pointer_cast<CTemplatePledge>(ptr);
    uint32 nFinalHeight = 0;
    if (!objPledge->GetPledgeFinalHeight(CBlock::GetBlockHeightByHash(hashPrev) + 1, nFinalHeight))
    {
        StdLog("CoreProtocol", "Verify Pledge Tx: Pledge days error, txid: %s", txid.GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    // CAddressContext ctxOwnerAddress;
    // if (!pBlockChain->RetrieveAddressContext(hashFork, hashPrev, objPledge->destOwner, ctxOwnerAddress))
    // {
    //     StdLog("CoreProtocol", "Verify Pledge Tx: Pledge owner address not created, owner address: %s, txid: %s", objPledge->destOwner.ToString().c_str(), txid.GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    // if (!ctxOwnerAddress.IsPubkey())
    // {
    //     StdLog("CoreProtocol", "Verify Pledge Tx: Pledge owner address not public key address, owner address: %s, txid: %s", objPledge->destOwner.ToString().c_str(), txid.GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    return OK;
}

Errno CCoreProtocol::VerifyVoteRedeemTx(const uint256& hashFork, const CTransaction& tx, const uint16 nFromTemplateType, const uint256& hashPrev)
{
    uint256 txid = tx.GetHash();
    if (hashFork != GetGenesisBlockHash())
    {
        StdLog("CoreProtocol", "Verify Vote Redeem Tx: tx fork error, txid: %s, fork: %s",
               txid.GetHex().c_str(), hashFork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    if (nFromTemplateType == TEMPLATE_PLEDGE)
    {
        StdDebug("CoreProtocol", "Verify Vote Redeem Tx: Pledge locked, txid: %s, from: %s",
                 txid.GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return ERR_TRANSACTION_IS_LOCKED;
    }
    if (!pBlockChain->VerifyAddressVoteRedeem(tx.GetFromAddress(), hashPrev))
    {
        StdDebug("CoreProtocol", "Verify Vote Redeem Tx: Vote locked, txid: %s, from: %s",
                 txid.GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return ERR_TRANSACTION_IS_LOCKED;
    }
    return OK;
}

Errno CCoreProtocol::VerifyVoteRewardLockTx(const uint256& hashFork, const CTransaction& tx, const uint256& hashPrev, const CDestState& state)
{
    uint256 nLockedAmount;
    if (!pBlockChain->GetVoteRewardLockedAmount(hashFork, hashPrev, tx.GetFromAddress(), nLockedAmount))
    {
        StdLog("CoreProtocol", "Verify vote reward lock tx: Get reward locked amount failed, txid: %s, from: %s",
               tx.GetHash().GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return ERR_TRANSACTION_IS_LOCKED;
    }
    if (nLockedAmount > 0 && state.GetBalance() < nLockedAmount + tx.GetAmount() + tx.GetTxFee())
    {
        StdLog("CoreProtocol", "Verify vote reward lock tx: Balance locked, balance: %s, lock amount: %s, txid: %s, from: %s",
               CoinToTokenBigFloat(state.GetBalance()).c_str(), CoinToTokenBigFloat(nLockedAmount).c_str(),
               tx.GetHash().GetHex().c_str(), tx.GetFromAddress().ToString().c_str());
        return ERR_TRANSACTION_IS_LOCKED;
    }
    return OK;
}

Errno CCoreProtocol::VerifyActivateCodeTx(const uint256& hashFork, const uint256& hashPrev, const CTransaction& tx)
{
    if (mapCodeGrantAddress.find(tx.GetFromAddress().ToString()) == mapCodeGrantAddress.end())
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: Code grant address error, from: %s, txid: %s",
               tx.GetFromAddress().ToString().c_str(), tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    CTemplatePtr ptr = pBlockChain->GetTxToAddressTemplatePtr(hashFork, hashPrev, tx);
    if (!ptr || ptr->GetTemplateType() != TEMPLATE_ACTIVATECODE)
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: Template data error2, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    if (tx.GetFromAddress() != boost::dynamic_pointer_cast<CTemplateActivateCode>(ptr)->destGrant)
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: from not is grant, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    bytes btActData;
    if (!tx.GetTxData(CTransaction::DF_ACTIVATECODE, btActData))
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: data not activate code, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    uint256 hashCode;
    try
    {
        mtbase::CBufStream ss(btActData);
        ss >> hashCode;
    }
    catch (std::exception& e)
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: code hash error, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    CContractCreateCodeContext ctxtCode;
    if (!pBlockChain->RetrieveForkContractCreateCodeContext(hashFork, hashPrev, hashCode, ctxtCode))
    {
        StdLog("CoreProtocol", "Verify Activate Code Tx: code not exist, txid: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    // if (ctxtCode.IsActivated())
    // {
    //     StdLog("CoreProtocol", "Verify Activate Code Tx: code is activated, txid: %s", tx.GetHash().GetHex().c_str());
    //     return ERR_TRANSACTION_INVALID;
    // }
    return OK;
}

///////////////////////////////
// CTestNetCoreProtocol

CTestNetCoreProtocol::CTestNetCoreProtocol()
{
}

// "privkey" : "0xd8b8db5ef512bd8e5a04f83ccfcfe6470c83d3f2e31480828ecf93a9ac0a54e2",
// "pubkey" : "0xa9cd6869b7d2d41c56c3573e43ca0bc666353ec387821857d5b0ad1bbd3377a1a62e4d69cc18c4a393b5ecd22513af25722a4db955650aa91a11ce4d7a1a3765",
// "address" : "0x5962974eeb0b17b43edabfc9b747839317aa852f"

void CTestNetCoreProtocol::GetGenesisBlock(CBlock& block)
{
    CreateGenesisBlock(false, GetGenesisChainId(), "0x5962974eeb0b17b43edabfc9b747839317aa852f", block);
}

///////////////////////////////
// CProofOfWorkParam

CProofOfWorkParam::CProofOfWorkParam(const bool fTestnetIn)
{
    fTestnet = fTestnetIn;
    nDelegateProofOfStakeEnrollMinimumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
    nDelegateProofOfStakeEnrollMaximumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT;

    if (fTestnet)
    {
        pCoreProtocol = (ICoreProtocol*)new CTestNetCoreProtocol();
    }
    else
    {
        pCoreProtocol = (ICoreProtocol*)new CCoreProtocol();
    }
    pCoreProtocol->InitializeGenesisBlock();

    hashGenesisBlock = pCoreProtocol->GetGenesisBlockHash();
}

CProofOfWorkParam::~CProofOfWorkParam()
{
    if (pCoreProtocol)
    {
        if (fTestnet)
        {
            delete (CTestNetCoreProtocol*)pCoreProtocol;
        }
        else
        {
            delete (CCoreProtocol*)pCoreProtocol;
        }
        pCoreProtocol = nullptr;
    }
}

uint64 CProofOfWorkParam::GetNextBlockTimestamp(const uint64 nPrevTimeStamp)
{
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

Errno CProofOfWorkParam::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const
{
    return pCoreProtocol->ValidateOrigin(block, parentProfile, forkProfile);
}

} // namespace metabasenet

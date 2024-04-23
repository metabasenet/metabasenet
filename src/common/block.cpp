// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "block.h"

#include "crypto.h"
#include "mtbase.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

namespace metabasenet
{

//////////////////////////////
// CBlock

void CBlock::SetNull()
{
    nVersion = 1;
    nType = 0;
    nTimeStamp = 0;
    nNumber = 0;
    nHeight = 0;
    nSlot = 0;
    hashPrev = 0;
    hashMerkleRoot = 0;
    hashStateRoot = 0;
    hashReceiptsRoot = 0;
    hashCrosschainMerkleRoot = 0;
    nGasLimit = 0;
    nGasUsed = 0;
    mapProof.clear();
    txMint.SetNull();
    vchSig.clear();

    btBloomData.clear();
    vtx.clear();
    mapProve.clear();
}

bool CBlock::IsNull() const
{
    return (nType == 0 || nTimeStamp == 0 || txMint.IsNull());
}

bool CBlock::IsGenesis() const
{
    return (nType == BLOCK_GENESIS);
}

bool CBlock::IsOrigin() const
{
    return (nType == BLOCK_GENESIS || nType == BLOCK_ORIGIN);
}

bool CBlock::IsPrimary() const
{
    return ((nType >> 4) == 0);
}

bool CBlock::IsSubsidiary() const
{
    return (nType == BLOCK_SUBSIDIARY);
}

bool CBlock::IsExtended() const
{
    return (nType == BLOCK_EXTENDED);
}

bool CBlock::IsVacant() const
{
    return (nType == BLOCK_VACANT);
}

bool CBlock::IsProofOfWork() const
{
    return (txMint.GetTxType() == CTransaction::TX_WORK);
}

bool CBlock::IsProofEmpty() const
{
    return mapProof.empty();
}

uint256 CBlock::GetHash() const
{
    // mtbase::CBufStream ss;
    // ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << hashCrosschainMerkleRoot << nGasLimit << nGasUsed << mapProof << txMint;
    // return uint256(GetChainId(), GetBlockHeight(), nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));

    return uint256(GetChainId(), GetBlockHeight(), nSlot, CalcMerkleTreeRoot());
}

std::size_t CBlock::GetTxSerializedOffset() const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    mtbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof;
    return ss.GetSize();
}

void CBlock::GetSerializedProofOfWorkData(std::vector<unsigned char>& vchProofOfWork) const
{
    mtbase::CBufStream ss;
    ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << mapProof;
    vchProofOfWork.assign(ss.GetData(), ss.GetData() + ss.GetSize());
}

uint32 CBlock::GetChainId() const
{
    if (IsOrigin())
    {
        CProfile profile;
        if (!GetForkProfile(profile))
        {
            return 0;
        }
        return profile.nChainId;
    }
    return hashPrev.GetB1();
}

uint64 CBlock::GetBlockTime() const
{
    return nTimeStamp;
}

uint64 CBlock::GetBlockNumber() const
{
    return nNumber;
}

uint32 CBlock::GetBlockSlot() const
{
    return nSlot;
}

uint32 CBlock::GetBlockHeight() const
{
    // if (IsGenesis())
    // {
    //     return 0;
    // }
    // else if (IsExtended())
    // {
    //     return hashPrev.GetB2();
    // }
    // else
    // {
    //     return hashPrev.GetB2() + 1;
    // }
    return nHeight;
}

uint256 CBlock::GetRefBlock() const
{
    uint256 hashRefBlock;
    if (!IsPrimary() && !IsOrigin())
    {
        CProofOfPiggyback proof;
        if (GetPiggybackProof(proof))
        {
            hashRefBlock = proof.hashRefBlock;
        }
    }
    return hashRefBlock;
}

uint64 CBlock::GetBlockBeacon(int idx) const
{
    if (mapProof.empty())
    {
        return hashPrev.Get64(idx & 3);
    }
    return 0;
}

bool CBlock::GetBlockMint(uint256& nMintCoin) const
{
    if (nType == BLOCK_EXTENDED || nType == BLOCK_VACANT)
    {
        return true;
    }
    return GetMintCoinProof(nMintCoin);
}

uint256 CBlock::GetBlockTotalReward() const
{
    uint256 nTotalReward;
    if (!GetMintRewardProof(nTotalReward))
    {
        return 0;
    }
    return nTotalReward;
}

uint256 CBlock::GetBlockMoneyDestroy() const
{
    uint256 nDestroyAmount;
    for (auto& tx : vtx)
    {
        if (tx.GetToAddress() == FUNCTION_BLACKHOLE_ADDRESS)
        {
            nDestroyAmount += tx.GetAmount();
        }
    }
    return nDestroyAmount;
}

void CBlock::SetBlockTime(const uint64 nTime)
{
    nTimeStamp = nTime;
}

void CBlock::SetSignData(const bytes& btSigData)
{
    vchSig = btSigData;
}

void CBlock::UpdateMerkleRoot()
{
    hashMerkleRoot = CalcMerkleTreeRoot();
}

bool CBlock::VerifyBlockHeight() const
{
    uint32 nCalcHeight = 0;
    if (IsGenesis())
    {
        nCalcHeight = 0;
    }
    else if (IsExtended())
    {
        nCalcHeight = hashPrev.GetB2();
    }
    else
    {
        nCalcHeight = hashPrev.GetB2() + 1;
    }
    return (nHeight == nCalcHeight);
}

bool CBlock::VerifyBlockMerkleTreeRoot() const
{
    return (hashMerkleRoot == CalcMerkleTreeRoot());
}

bool CBlock::VerifyBlockSignature(const CDestination& destBlockSign) const
{
    uint256 hash = GetHash();
    uint512 pub = CrytoGetSignPubkey(hash, vchSig);
    if (destBlockSign != CDestination(CPubKey(pub)))
    {
        return false;
    }
    return true;
}

bool CBlock::VerifyBlockProof() const
{
    for (const auto& [k, v] : mapProof)
    {
        std::size_t nDataSize = 0;
        switch (k)
        {
        case BP_FORK_PROFILE:
        {
            if (!IsOrigin())
            {
                return false;
            }
            CProfile profile;
            if (!profile.Load(v))
            {
                return false;
            }
            bytes btData;
            profile.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_DELEGATE:
        {
            if (nType != BLOCK_PRIMARY || txMint.GetTxType() != CTransaction::TX_STAKE)
            {
                return false;
            }
            CProofOfDelegate proof;
            if (!proof.Load(v))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_PIGGYBACK:
        {
            if (nType != BLOCK_SUBSIDIARY && nType != BLOCK_EXTENDED && nType != BLOCK_VACANT)
            {
                return false;
            }
            CProofOfPiggyback proof;
            if (!proof.Load(v))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_HASH_WORK:
        {
            if (nType != BLOCK_PRIMARY || txMint.GetTxType() != CTransaction::TX_WORK)
            {
                return false;
            }
            CProofOfHashWork proof;
            if (!proof.Load(v))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        case BP_MINTCOIN:
        case BP_MINTREWARD:
        {
            nDataSize = uint256().size();
            break;
        }
        case BP_BLOCK_VOTE_SIG:
        {
            CBlockVoteSig proof;
            if (!proof.Load(v))
            {
                return false;
            }
            bytes btData;
            proof.Save(btData);
            nDataSize = btData.size();
            break;
        }
        default:
            return false;
        }
        if (v.size() != nDataSize)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------
void CBlock::AddProofData(const uint8 nProofId, const bytes& btData)
{
    mapProof[nProofId] = btData;
}

bool CBlock::GetProofData(const uint8 nProofId, bytes& btData) const
{
    auto it = mapProof.find(nProofId);
    if (it == mapProof.end())
    {
        return false;
    }
    btData = it->second;
    return true;
}

//------------------------------------------------
void CBlock::AddForkProfile(const CProfile& profile)
{
    bytes btPf;
    profile.Save(btPf);
    mapProof[BP_FORK_PROFILE] = btPf;
}

void CBlock::AddDelegateProof(const CProofOfDelegate& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_DELEGATE] = btPf;
}

void CBlock::AddPiggybackProof(const CProofOfPiggyback& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_PIGGYBACK] = btPf;
}

void CBlock::AddHashWorkProof(const CProofOfHashWork& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_HASH_WORK] = btPf;
}

void CBlock::AddMintCoinProof(const uint256& nMintCoin)
{
    mapProof[BP_MINTCOIN] = nMintCoin.GetBytes();
}

void CBlock::AddMintRewardProof(const uint256& nMintReward)
{
    mapProof[BP_MINTREWARD] = nMintReward.GetBytes();
}

void CBlock::AddBlockVoteSig(const CBlockVoteSig& proof)
{
    bytes btPf;
    proof.Save(btPf);
    mapProof[BP_BLOCK_VOTE_SIG] = btPf;
}

//------------------------------------------------
bool CBlock::GetForkProfile(CProfile& profile) const
{
    auto it = mapProof.find(BP_FORK_PROFILE);
    if (it == mapProof.end())
    {
        return false;
    }
    return profile.Load(it->second);
}

bool CBlock::GetDelegateProof(CProofOfDelegate& proof) const
{
    auto it = mapProof.find(BP_DELEGATE);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetPiggybackProof(CProofOfPiggyback& proof) const
{
    auto it = mapProof.find(BP_PIGGYBACK);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetHashWorkProof(CProofOfHashWork& proof) const
{
    auto it = mapProof.find(BP_HASH_WORK);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetMintCoinProof(uint256& nMintCoin) const
{
    auto it = mapProof.find(BP_MINTCOIN);
    if (it == mapProof.end())
    {
        return false;
    }
    return (nMintCoin.SetBytes(it->second) == nMintCoin.size());
}

bool CBlock::GetMintRewardProof(uint256& nMintReward) const
{
    auto it = mapProof.find(BP_MINTREWARD);
    if (it == mapProof.end())
    {
        return false;
    }
    return (nMintReward.SetBytes(it->second) == nMintReward.size());
}

bool CBlock::GetBlockVoteSig(CBlockVoteSig& proof) const
{
    auto it = mapProof.find(BP_BLOCK_VOTE_SIG);
    if (it == mapProof.end())
    {
        return false;
    }
    return proof.Load(it->second);
}

bool CBlock::GetMerkleProve(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const
{
    return (CalcMerkleTreeRootAndProve(ptrMerkleProvePrevBlock, ptrMerkleProveRefBlock, ptrCrossMerkleProve) != 0);
}

//-------------------------------------------------------
CChainId CBlock::GetBlockChainIdByHash(const uint256& hash)
{
    return hash.GetB1();
}

uint32 CBlock::GetBlockHeightByHash(const uint256& hash)
{
    return hash.GetB2();
}

uint16 CBlock::GetBlockSlotByHash(const uint256& hash)
{
    return hash.GetB3();
}

uint256 CBlock::CreateBlockHash(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, const uint256& hash)
{
    return uint256(nChainId, nHeight, nSlot, hash);
}

bool CBlock::BlockHashCompare(const uint256& a, const uint256& b) // return true: a < b, false: a >= b
{
    CChainId nChainIdA = CBlock::GetBlockChainIdByHash(a);
    CChainId nChainIdB = CBlock::GetBlockChainIdByHash(b);
    if (nChainIdA < nChainIdB)
    {
        return true;
    }
    else if (nChainIdA == nChainIdB)
    {
        uint32 nHeightA = CBlock::GetBlockHeightByHash(a);
        uint32 nHeightB = CBlock::GetBlockHeightByHash(b);
        if (nHeightA < nHeightB)
        {
            return true;
        }
        else if (nHeightA == nHeightB)
        {
            uint16 nSlotA = CBlock::GetBlockSlotByHash(a);
            uint16 nSlotB = CBlock::GetBlockSlotByHash(b);
            if (nSlotA < nSlotB)
            {
                return true;
            }
            else if (nSlotA == nSlotB && a < b)
            {
                return true;
            }
        }
    }
    return false;
}

bool CBlock::VerifyBlockMerkleProve(const uint256& hashBlock, const mtbase::MERKLE_PROVE_DATA& merkleProve, const uint256& hashVerify)
{
    return (hashBlock == uint256(GetBlockChainIdByHash(hashBlock), GetBlockHeightByHash(hashBlock), GetBlockSlotByHash(hashBlock), CMerkleTree::GetMerkleRootByProve(merkleProve, hashVerify)));
}

//-------------------------------------------------------
uint256 CBlock::CalcMerkleTreeRoot() const
{
    return CalcMerkleTreeRootAndProve(nullptr, nullptr, nullptr);
}

uint256 CBlock::CalcMerkleTreeRootAndProve(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const
{
    const uint256 hashBaseMerkleRoot = CalcBlockBaseMerkleTreeRoot(ptrMerkleProvePrevBlock, ptrMerkleProveRefBlock);
    const uint256 hashTxMerkleRoot = CalcTxMerkleTreeRoot();

    std::vector<uint256> vMerkleTree;

    vMerkleTree.push_back(hashBaseMerkleRoot);
    vMerkleTree.push_back(hashStateRoot);
    vMerkleTree.push_back(hashCrosschainMerkleRoot);
    vMerkleTree.push_back(hashTxMerkleRoot);

    const std::size_t nLeafCount = vMerkleTree.size();

    uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);

    if (ptrMerkleProvePrevBlock || ptrMerkleProveRefBlock)
    {
        SHP_MERKLE_PROVE_DATA ptrMerkleProveBase = MAKE_SHARED_MERKLE_PROVE_DATA();
        if (!ptrMerkleProveBase || !CMerkleTree::BuildMerkleProve(hashBaseMerkleRoot, vMerkleTree, nLeafCount, *ptrMerkleProveBase))
        {
            return 0;
        }
        if (ptrMerkleProvePrevBlock)
        {
            (*ptrMerkleProvePrevBlock).insert((*ptrMerkleProvePrevBlock).end(), (*ptrMerkleProveBase).begin(), (*ptrMerkleProveBase).end());
        }
        if (ptrMerkleProveRefBlock)
        {
            (*ptrMerkleProveRefBlock).insert((*ptrMerkleProveRefBlock).end(), (*ptrMerkleProveBase).begin(), (*ptrMerkleProveBase).end());
        }
    }

    if (ptrCrossMerkleProve)
    {
        if (!CMerkleTree::BuildMerkleProve(hashCrosschainMerkleRoot, vMerkleTree, nLeafCount, *ptrCrossMerkleProve))
        {
            return 0;
        }
    }
    return hashMerkleRoot;
}

uint256 CBlock::CalcBlockBaseMerkleTreeRoot(SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock) const
{
    // mtbase::CBufStream ss;
    // ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << hashCrosschainMerkleRoot << nGasLimit << nGasUsed << mapProof << txMint;
    // return uint256(GetChainId(), GetBlockHeight(), nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));

    uint256 hashBase;
    uint256 hashProof;
    uint256 hashRefBlock;
    uint256 hashBloom;

    {
        mtbase::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << nNumber << nHeight << nSlot << nGasLimit << nGasUsed;
        hashBase = crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }

    if (!mapProof.empty())
    {
        mtbase::CBufStream ss;
        ss << mapProof;
        hashProof = crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }

    hashRefBlock = GetRefBlock();

    if (!btBloomData.empty())
    {
        hashBloom = crypto::CryptoHash(btBloomData.data(), btBloomData.size());
    }

    std::vector<uint256> vMerkleTree;

    vMerkleTree.push_back(hashBase);
    vMerkleTree.push_back(hashProof);
    vMerkleTree.push_back(hashRefBlock);
    vMerkleTree.push_back(hashPrev);
    vMerkleTree.push_back(hashReceiptsRoot);
    vMerkleTree.push_back(hashBloom);

    const std::size_t nLeafCount = vMerkleTree.size();

    uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);
    if (hashMerkleRoot == 0)
    {
        return 0;
    }

    if (ptrMerkleProvePrevBlock)
    {
        if (!CMerkleTree::BuildMerkleProve(hashPrev, vMerkleTree, nLeafCount, *ptrMerkleProvePrevBlock))
        {
            return 0;
        }
    }
    if (ptrMerkleProveRefBlock)
    {
        if (!CMerkleTree::BuildMerkleProve(hashRefBlock, vMerkleTree, nLeafCount, *ptrMerkleProveRefBlock))
        {
            return 0;
        }
    }
    return hashMerkleRoot;
}

uint256 CBlock::CalcTxMerkleTreeRoot() const
{
    std::vector<uint256> vMerkleTree;
    vMerkleTree.push_back(txMint.GetHash());
    for (const CTransaction& tx : vtx)
    {
        vMerkleTree.push_back(tx.GetHash());
    }
    return CMerkleTree::BuildMerkleTree(vMerkleTree);
}

//-------------------------------------------------------
void CBlock::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    s << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << mapProve << vchSig;
}

void CBlock::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    CVarInt varTimeStamp;
    CVarInt varNumber;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit;
    bytes btGasUsed;
    s >> nVersion >> nType >> varTimeStamp >> varNumber >> nHeight >> nSlot >> hashPrev >> btMerkleRoot >> hashStateRoot >> btReceiptsRoot >> btCrosschainMerkleRoot >> btBloomData >> btGasLimit >> btGasUsed >> mapProof >> txMint >> vtx >> mapProve >> vchSig;
    nTimeStamp = varTimeStamp.nValue;
    nNumber = varNumber.nValue;
    nGasLimit.FromValidBigEndianData(btGasLimit);
    nGasUsed.FromValidBigEndianData(btGasUsed);
    hashMerkleRoot.SetBytes(btMerkleRoot);
    hashReceiptsRoot.SetBytes(btReceiptsRoot);
    hashCrosschainMerkleRoot.SetBytes(btCrosschainMerkleRoot);
}

void CBlock::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btCrosschainMerkleRoot;
    bytes btGasLimit = nGasLimit.ToValidBigEndianData();
    bytes btGasUsed = nGasUsed.ToValidBigEndianData();
    if (!hashMerkleRoot.IsNull())
    {
        btMerkleRoot = hashMerkleRoot.GetBytes();
    }
    if (!hashReceiptsRoot.IsNull())
    {
        btReceiptsRoot = hashReceiptsRoot.GetBytes();
    }
    if (!hashCrosschainMerkleRoot.IsNull())
    {
        btCrosschainMerkleRoot = hashCrosschainMerkleRoot.GetBytes();
    }
    mtbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nHeight << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btCrosschainMerkleRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << mapProve << vchSig;
    serSize = ss.GetSize();
}

///////////////////////////////////////////////
// CBlockIndex

CBlockIndex::CBlockIndex()
{
    phashBlock = nullptr;
    pOrigin = this;
    pPrev = nullptr;
    pNext = nullptr;
    nChainId = 0;
    txidMint = 0;
    nMintType = 0;
    destMint.SetNull();
    nVersion = 0;
    nType = 0;
    nTimeStamp = 0;
    nHeight = 0;
    nSlot = 0;
    nNumber = 0;
    nTxCount = 0;
    nRewardTxCount = 0;
    nUserTxCount = 0;
    nAgreement = 0;
    hashRefBlock = 0;
    hashStateRoot = 0;
    nRandBeacon = 0;
    nGasLimit = 0;
    nGasUsed = 0;
    nChainTrust = uint64(0);
    nBlockReward = 0;
    nMoneySupply = 0;
    nMoneyDestroy = 0;
    nProofAlgo = 0;
    nProofBits = 0;
    nFile = 0;
    nOffset = 0;
    nBlockCrc = 0;
}

CBlockIndex::CBlockIndex(const uint256& hashBlock, const CBlock& block, const uint32 nFileIn, const uint32 nOffsetIn, const uint32 nCrcIn)
{
    phashBlock = nullptr;
    pOrigin = this;
    pPrev = nullptr;
    pNext = nullptr;
    nChainId = 0;
    txidMint = (block.IsVacant() ? uint64(0) : block.txMint.GetHash());
    nMintType = block.txMint.GetTxType();
    destMint = block.txMint.GetToAddress();
    nVersion = block.nVersion;
    nType = block.nType;
    nTimeStamp = block.GetBlockTime();
    nHeight = block.GetBlockHeight();
    nSlot = block.GetBlockSlot();
    nNumber = block.GetBlockNumber();
    nTxCount = block.vtx.size() + 1;
    nRewardTxCount = 0;
    nUserTxCount = 0;
    nAgreement = 0;
    hashRefBlock = 0;
    hashStateRoot = block.hashStateRoot;
    nRandBeacon = 0;
    nGasLimit = block.nGasLimit;
    nGasUsed = block.nGasUsed;
    nChainTrust = uint64(0);
    nBlockReward = block.GetBlockTotalReward();
    nMoneySupply = 0;
    nMoneyDestroy = 0;
    nProofAlgo = 0;
    nProofBits = 0;
    nFile = nFileIn;
    nOffset = nOffsetIn;
    nBlockCrc = nCrcIn;

    for (auto& tx : block.vtx)
    {
        if (tx.GetTxType() == CTransaction::TX_GENESIS || tx.GetTxType() == CTransaction::TX_VOTE_REWARD)   // ToDo
        {
            nRewardTxCount++;
        }
        else if (tx.IsUserTx())
        {
            nUserTxCount++;
        }
    }

    if (IsPrimary())
    {
        if (IsProofOfWork())
        {
            CProofOfHashWork proof;
            if (block.GetHashWorkProof(proof))
            {
                nProofAlgo = proof.nAlgo;
                nProofBits = proof.nBits;
                uint256 hashTarget = (~uint256(uint64(0)) >> proof.nBits);
                nAgreement = hashTarget + proof.nNonce;
            }
        }
        else
        {
            CProofOfDelegate proof;
            if (block.GetDelegateProof(proof))
            {
                nAgreement = proof.nAgreement;
            }
        }
        hashRefBlock = hashBlock;
    }
    else
    {
        CProofOfPiggyback proof;
        if (block.GetPiggybackProof(proof) && proof.hashRefBlock != 0)
        {
            nAgreement = proof.nAgreement;
            hashRefBlock = proof.hashRefBlock;
        }
    }
}

///////////////////////////////////////////////////
// CBlockDexOrderProve

void CBlockDexOrderProve::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    s << destOrder << nChainIdOwner << nChainIdPeer << strCoinSymbolOwner << strCoinSymbolPeer << mtbase::CVarInt(nOrderNumber) << nOrderAmount.ToValidBigEndianData() << nOrderPrice.ToValidBigEndianData();
}

void CBlockDexOrderProve::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    mtbase::CVarInt varOrderNumber;
    bytes btOrderAmount;
    bytes btOrderPrice;
    s >> destOrder >> nChainIdOwner >> nChainIdPeer >> strCoinSymbolOwner >> strCoinSymbolPeer >> varOrderNumber >> btOrderAmount >> btOrderPrice;
    nOrderNumber = varOrderNumber.GetValue();
    nOrderAmount.FromValidBigEndianData(btOrderAmount);
    nOrderPrice.FromValidBigEndianData(btOrderPrice);
}

void CBlockDexOrderProve::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    mtbase::CBufStream ss;
    ss << destOrder << nChainIdOwner << nChainIdPeer << strCoinSymbolOwner << strCoinSymbolPeer << mtbase::CVarInt(nOrderNumber) << nOrderAmount.ToValidBigEndianData() << nOrderPrice.ToValidBigEndianData();
    serSize = ss.GetSize();
}

///////////////////////////////////////////////////
// CBlockCrosschainProve

void CBlockCrosschainProve::Clear()
{
    hashPrevProveBlock = 0;
    vCoinTransferProve.clear();
    mapDexOrderProve.clear();
    setCrossConfirmRecvBlock.clear();
}

bool CBlockCrosschainProve::IsNull() const
{
    return (vCoinTransferProve.empty() && mapDexOrderProve.empty() && setCrossConfirmRecvBlock.empty());
}

bool CBlockCrosschainProve::IsCrossProveNull() const
{
    return (vCoinTransferProve.empty() && mapDexOrderProve.empty());
}

uint256 CBlockCrosschainProve::GetHash() const
{
    mtbase::CBufStream ss;
    ss << *this;
    return crypto::CryptoHash(ss.GetData(), ss.GetSize());
}

void CBlockCrosschainProve::AddCoinTransferProve(const CDestination& destTransIn, const std::string& strCoinSymbolIn, const CChainId nOriChainIdIn, const CChainId nDestChainIdIn, const uint256& nTransferAmountIn)
{
    vCoinTransferProve.push_back(CBlockCoinTransferProve(destTransIn, strCoinSymbolIn, nOriChainIdIn, nDestChainIdIn, nTransferAmountIn));
}

void CBlockCrosschainProve::AddDexOrderProve(const CDestination& destOrderIn, const CChainId nChainIdOwnerIn, const CChainId nChainIdPeerIn, const std::string& strCoinSymbolOwnerIn,
                                             const std::string& strCoinSymbolPeerIn, const uint64 nOrderNumberIn, const uint256& nOrderAmountIn, const uint256& nOrderPriceIn)
{
    mapDexOrderProve[CDexOrderHeader(nChainIdOwnerIn, destOrderIn, strCoinSymbolOwnerIn, strCoinSymbolPeerIn, nOrderNumberIn)] = CBlockDexOrderProve(destOrderIn, nChainIdOwnerIn, nChainIdPeerIn, strCoinSymbolOwnerIn, strCoinSymbolPeerIn, nOrderNumberIn, nOrderAmountIn, nOrderPriceIn);
}

void CBlockCrosschainProve::AddDexOrderProve(const CBlockDexOrderProve& orderProve)
{
    mapDexOrderProve[orderProve.GetDexOrderHeader()] = orderProve;
}

void CBlockCrosschainProve::AddCrossConfirmRecvBlock(const uint256& hashRecvBlock)
{
    setCrossConfirmRecvBlock.insert(hashRecvBlock);
}

void CBlockCrosschainProve::SetPrevProveBlock(const uint256& hashBlock)
{
    hashPrevProveBlock = hashBlock;
}

const uint256& CBlockCrosschainProve::GetPrevProveBlock() const
{
    return hashPrevProveBlock;
}

const std::vector<CBlockCoinTransferProve>& CBlockCrosschainProve::GetCoinTransferProve() const
{
    return vCoinTransferProve;
}

const std::map<CDexOrderHeader, CBlockDexOrderProve>& CBlockCrosschainProve::GetDexOrderProve() const
{
    return mapDexOrderProve;
}

const std::set<uint256>& CBlockCrosschainProve::GetCrossConfirmRecvBlock() const
{
    return setCrossConfirmRecvBlock;
}

void CBlockCrosschainProve::SetProveData(const std::map<uint8, bytes>& mapProveData)
{
    for (const auto& [k, v] : mapProveData)
    {
        mtbase::CBufStream ss(v);
        switch (k)
        {
        case CP_PROVE_TYPE_COIN_TRANSFER_PROVE:
            ss >> vCoinTransferProve;
            break;
        case CP_PROVE_TYPE_DEX_ORDER_PROVE:
        {
            mtbase::CVarInt varValue;
            ss >> varValue;
            for (std::size_t i = 0; i < varValue.GetValue(); i++)
            {
                CBlockDexOrderProve orderProve;
                ss >> orderProve;
                mapDexOrderProve[orderProve.GetDexOrderHeader()] = orderProve;
            }
            break;
        }
        case CP_PROVE_TYPE_CROSS_CONFIRM_RECV_BLOCK_PROVE:
            ss >> setCrossConfirmRecvBlock;
            break;
        }
    }
}

void CBlockCrosschainProve::GetProveData(std::map<uint8, bytes>& mapProveData) const
{
    if (!vCoinTransferProve.empty())
    {
        mtbase::CBufStream ss;
        ss << vCoinTransferProve;
        mapProveData.insert(std::make_pair(CP_PROVE_TYPE_COIN_TRANSFER_PROVE, ss.GetBytes()));
    }
    if (!mapDexOrderProve.empty())
    {
        mtbase::CBufStream ss;
        ss << mtbase::CVarInt(mapDexOrderProve.size());
        for (const auto& kv : mapDexOrderProve)
        {
            ss << kv.second;
        }
        mapProveData.insert(std::make_pair(CP_PROVE_TYPE_DEX_ORDER_PROVE, ss.GetBytes()));
    }
    if (!setCrossConfirmRecvBlock.empty())
    {
        mtbase::CBufStream ss;
        ss << setCrossConfirmRecvBlock;
        mapProveData.insert(std::make_pair(CP_PROVE_TYPE_CROSS_CONFIRM_RECV_BLOCK_PROVE, ss.GetBytes()));
    }
}

//-------------------------------------------------------
void CBlockCrosschainProve::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    std::map<uint8, bytes> mapProveData;
    GetProveData(mapProveData);
    s << nProveVersion << hashPrevProveBlock << mapProveData;
}

void CBlockCrosschainProve::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    std::map<uint8, bytes> mapProveData;
    s >> nProveVersion >> hashPrevProveBlock >> mapProveData;
    SetProveData(mapProveData);
}

void CBlockCrosschainProve::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    std::map<uint8, bytes> mapProveData;
    GetProveData(mapProveData);
    mtbase::CBufStream ss;
    ss << nProveVersion << hashPrevProveBlock << mapProveData;
    serSize = ss.GetSize();
}

///////////////////////////////////////////////////
// CBlockPrevProve

void CBlockPrevProve::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        s << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        s << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    // crosschain
    if (!proveCrosschain.IsNull() && !vCrosschainMerkleProve.empty())
    {
        s << mtbase::CVarInt(vCrosschainMerkleProve.size() + 1);
        s << proveCrosschain;
        for (const auto& provePair : vCrosschainMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }
}

void CBlockPrevProve::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    mtbase::CVarInt varSize;

    // prev block
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> hashPrevBlock;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vPrevBlockMerkleProve.push_back(provePair);
        }
    }

    // crosschain
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> proveCrosschain;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vCrosschainMerkleProve.push_back(provePair);
        }
    }
}

void CBlockPrevProve::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    mtbase::CBufStream ss;

    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        ss << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    // crosschain
    if (!proveCrosschain.IsNull() && !vCrosschainMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vCrosschainMerkleProve.size() + 1);
        ss << proveCrosschain;
        for (const auto& provePair : vCrosschainMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    serSize = ss.GetSize();
}

///////////////////////////////////////////////////
// CBlockProve

void CBlockProve::Save(bytes& btData) const
{
    mtbase::CBufStream ss;
    ss << *this;
    ss.GetData(btData);
}

bool CBlockProve::Load(const bytes& btData)
{
    mtbase::CBufStream ss(btData);
    try
    {
        ss >> *this;
    }
    catch (const std::exception& e)
    {
        return false;
    }
    return true;
}

uint256 CBlockProve::GetFirstPrevBlockHash() const
{
    for (auto it = vPrevBlockCcProve.rbegin(); it != vPrevBlockCcProve.rend(); ++it)
    {
        if (it->proveCrosschain.GetPrevProveBlock() != 0)
        {
            return it->proveCrosschain.GetPrevProveBlock();
        }
    }
    return proveCrosschain.GetPrevProveBlock();
}

void CBlockProve::GetBlockHashList(std::vector<uint256>& vBlockHash) const
{
    vBlockHash.push_back(hashBlock);
    uint256 hashAtBlock = hashPrevBlock;
    for (const auto& vd : vPrevBlockCcProve)
    {
        vBlockHash.push_back(hashAtBlock);
        hashAtBlock = vd.hashPrevBlock;
    }
}

bool CBlockProve::IsCrossProveEmpty() const
{
    if (!proveCrosschain.IsCrossProveNull())
    {
        return false;
    }
    for (const CBlockPrevProve& prevProve : vPrevBlockCcProve)
    {
        if (!prevProve.proveCrosschain.IsCrossProveNull())
        {
            return false;
        }
    }
    return true;
}

void CBlockProve::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    s << hashBlock << btAggSigBitmap << btAggSigData;

    // ref block
    if (hashRefBlock != 0 && !vRefBlockMerkleProve.empty())
    {
        s << mtbase::CVarInt(vRefBlockMerkleProve.size() + 1);
        s << hashRefBlock;
        for (const auto& provePair : vRefBlockMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        s << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        s << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    // crosschain
    if (!proveCrosschain.IsNull() && !vCrosschainMerkleProve.empty())
    {
        s << mtbase::CVarInt(vCrosschainMerkleProve.size() + 1);
        s << proveCrosschain;
        for (const auto& provePair : vCrosschainMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    s << vPrevBlockCcProve;
}

void CBlockProve::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    mtbase::CVarInt varSize;

    s >> hashBlock >> btAggSigBitmap >> btAggSigData;

    // ref block
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> hashRefBlock;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vRefBlockMerkleProve.push_back(provePair);
        }
    }

    // prev block
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> hashPrevBlock;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vPrevBlockMerkleProve.push_back(provePair);
        }
    }

    // crosschain
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> proveCrosschain;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vCrosschainMerkleProve.push_back(provePair);
        }
    }

    s >> vPrevBlockCcProve;
}

void CBlockProve::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    mtbase::CBufStream ss;

    ss << hashBlock << btAggSigBitmap << btAggSigData;

    // ref block
    if (hashRefBlock != 0 && !vRefBlockMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vRefBlockMerkleProve.size() + 1);
        ss << hashRefBlock;
        for (const auto& provePair : vRefBlockMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        ss << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    // crosschain
    if (!proveCrosschain.IsNull() && !vCrosschainMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vCrosschainMerkleProve.size() + 1);
        ss << proveCrosschain;
        for (const auto& provePair : vCrosschainMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    ss << vPrevBlockCcProve;

    serSize = ss.GetSize();
}

///////////////////////////////////////////////////
// CBlockStorageProve

void CBlockStorageProve::Save(bytes& btData) const
{
    mtbase::CBufStream ss;
    ss << *this;
    ss.GetData(btData);
}

bool CBlockStorageProve::Load(const bytes& btData)
{
    mtbase::CBufStream ss(btData);
    try
    {
        ss >> *this;
    }
    catch (const std::exception& e)
    {
        return false;
    }
    return true;
}

void CBlockStorageProve::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    s << btAggSigBitmap << btAggSigData;

    // ref block
    if (hashRefBlock != 0 && !vRefBlockMerkleProve.empty())
    {
        s << mtbase::CVarInt(vRefBlockMerkleProve.size() + 1);
        s << hashRefBlock;
        for (const auto& provePair : vRefBlockMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        s << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        s << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            s << provePair;
        }
    }
    else
    {
        s << mtbase::CVarInt(0);
    }

    s << mapCrossProve;
}

void CBlockStorageProve::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    mtbase::CVarInt varSize;

    s >> btAggSigBitmap >> btAggSigData;

    // ref block
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> hashRefBlock;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vRefBlockMerkleProve.push_back(provePair);
        }
    }

    // prev block
    s >> varSize;
    if (varSize.GetValue() > 0)
    {
        s >> hashPrevBlock;
        for (std::size_t i = 0; i < varSize.GetValue() - 1; i++)
        {
            std::pair<uint8, uint256> provePair;
            s >> provePair;
            vPrevBlockMerkleProve.push_back(provePair);
        }
    }

    s >> mapCrossProve;
}

void CBlockStorageProve::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    mtbase::CBufStream ss;

    ss << btAggSigBitmap << btAggSigData;

    // ref block
    if (hashRefBlock != 0 && !vRefBlockMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vRefBlockMerkleProve.size() + 1);
        ss << hashRefBlock;
        for (const auto& provePair : vRefBlockMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    // prev block
    if (hashPrevBlock != 0 && !vPrevBlockMerkleProve.empty())
    {
        ss << mtbase::CVarInt(vPrevBlockMerkleProve.size() + 1);
        ss << hashPrevBlock;
        for (const auto& provePair : vPrevBlockMerkleProve)
        {
            ss << provePair;
        }
    }
    else
    {
        ss << mtbase::CVarInt(0);
    }

    ss << mapCrossProve;

    serSize = ss.GetSize();
}

} // namespace metabasenet

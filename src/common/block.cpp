// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "block.h"

#include "crypto.h"
#include "mtbase.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;
//using namespace dev;
//using namespace dev::eth;

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
    nSlot = 0;
    hashPrev = 0;
    hashMerkleRoot = 0;
    hashStateRoot = 0;
    hashReceiptsRoot = 0;
    nGasLimit = 0;
    nGasUsed = 0;
    mapProof.clear();
    txMint.SetNull();
    vchSig.clear();

    btBloomData.clear();
    vtx.clear();
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
    mtbase::CBufStream ss;
    ss << nVersion << nType << nTimeStamp << nNumber << nSlot << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << nGasLimit << nGasUsed << mapProof << txMint;
    return uint256(GetChainId(), GetBlockHeight(), nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));
}

std::size_t CBlock::GetTxSerializedOffset() const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
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
    mtbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btBloomData << btGasLimit << btGasUsed << mapProof;
    return ss.GetSize();
}

void CBlock::GetSerializedProofOfWorkData(std::vector<unsigned char>& vchProofOfWork) const
{
    mtbase::CBufStream ss;
    ss << nVersion << nType << nTimeStamp << nNumber << nSlot << hashPrev << mapProof;
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
    if (IsGenesis())
    {
        return 0;
    }
    else if (IsExtended())
    {
        return hashPrev.GetB2();
    }
    else
    {
        return hashPrev.GetB2() + 1;
    }
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

uint256 CBlock::CalcMerkleTreeRoot() const
{
    std::vector<uint256> vMerkleTree;
    uint256 hashBloom;
    if (!btBloomData.empty())
    {
        hashBloom = crypto::CryptoHash(btBloomData.data(), btBloomData.size());
    }
    vMerkleTree.push_back(hashBloom);
    vMerkleTree.push_back(txMint.GetHash());
    for (const CTransaction& tx : vtx)
    {
        vMerkleTree.push_back(tx.GetHash());
    }
    size_t j = 0;
    for (size_t nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (size_t i = 0; i < nSize; i += 2)
        {
            size_t i2 = std::min(i + 1, nSize - 1);
            vMerkleTree.push_back(crypto::CryptoHash(vMerkleTree[j + i], vMerkleTree[j + i2]));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? uint64(0) : vMerkleTree.back());
}

void CBlock::SetSignData(const bytes& btSigData)
{
    vchSig = btSigData;
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
    for (auto& kv : mapProof)
    {
        std::size_t nDataSize = 0;
        switch (kv.first)
        {
        case BP_FORK_PROFILE:
        {
            if (!IsOrigin())
            {
                return false;
            }
            CProfile profile;
            if (!profile.Load(kv.second))
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
            if (!proof.Load(kv.second))
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
            if (!proof.Load(kv.second))
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
            if (!proof.Load(kv.second))
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
        default:
            return false;
        }
        if (kv.second.size() != nDataSize)
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

//-------------------------------------------------------
uint32 CBlock::GetBlockChainIdByHash(const uint256& hash)
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

//-------------------------------------------------------
void CBlock::Serialize(mtbase::CStream& s, mtbase::SaveType&) const
{
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
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
    s << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << vchSig;
}

void CBlock::Serialize(mtbase::CStream& s, mtbase::LoadType&)
{
    CVarInt varTimeStamp;
    CVarInt varNumber;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
    bytes btGasLimit;
    bytes btGasUsed;
    s >> nVersion >> nType >> varTimeStamp >> varNumber >> nSlot >> hashPrev >> btMerkleRoot >> hashStateRoot >> btReceiptsRoot >> btBloomData >> btGasLimit >> btGasUsed >> mapProof >> txMint >> vtx >> vchSig;
    nTimeStamp = varTimeStamp.nValue;
    nNumber = varNumber.nValue;
    nGasLimit.FromValidBigEndianData(btGasLimit);
    nGasUsed.FromValidBigEndianData(btGasUsed);
    hashMerkleRoot.SetBytes(btMerkleRoot);
    hashReceiptsRoot.SetBytes(btReceiptsRoot);
}

void CBlock::Serialize(mtbase::CStream& s, std::size_t& serSize) const
{
    (void)s;
    bytes btMerkleRoot;
    bytes btReceiptsRoot;
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
    mtbase::CBufStream ss;
    ss << nVersion << nType << CVarInt(nTimeStamp) << CVarInt(nNumber) << nSlot << hashPrev << btMerkleRoot << hashStateRoot << btReceiptsRoot << btBloomData << btGasLimit << btGasUsed << mapProof << txMint << vtx << vchSig;
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
        if (tx.GetTxType() == CTransaction::TX_GENESIS || tx.GetTxType() == CTransaction::TX_VOTE_REWARD)
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

} // namespace metabasenet

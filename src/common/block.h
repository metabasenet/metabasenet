// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_BLOCK_H
#define COMMON_BLOCK_H

#include <stream/stream.h>
#include <vector>

#include "crc24q.h"
#include "proof.h"
#include "template/template.h"
#include "transaction.h"
#include "uint256.h"

class CBlock
{
    friend class hcode::CStream;

public:
    uint16 nVersion;
    uint16 nType;
    uint32 nTimeStamp;
    uint64 nNumber;
    uint256 hashPrev;
    uint256 hashMerkleRoot;
    uint256 hashStateRoot;
    uint256 hashReceiptsRoot;
    uint2048 nBloom;
    uint256 nGasLimit;
    uint256 nGasUsed;
    std::vector<uint8> vchProof;
    CTransaction txMint;
    std::vector<CTransaction> vtx;
    std::vector<uint8> vchSig;
    enum
    {
        BLOCK_GENESIS = 0xffff,
        BLOCK_ORIGIN = 0xff00,
        BLOCK_PRIMARY = 0x0001,
        BLOCK_SUBSIDIARY = 0x0002,
        BLOCK_EXTENDED = 0x0004,
        BLOCK_VACANT = 0x0008,
    };

public:
    CBlock()
    {
        SetNull();
    }
    void SetNull()
    {
        nVersion = 1;
        nType = 0;
        nTimeStamp = 0;
        nNumber = 0;
        hashPrev = 0;
        hashMerkleRoot = 0;
        hashStateRoot = 0;
        hashReceiptsRoot = 0;
        nBloom = 0;
        nGasLimit = 0;
        nGasUsed = 0;
        vchProof.clear();
        txMint.SetNull();
        vtx.clear();
        vchSig.clear();
    }
    bool IsNull() const
    {
        return (nType == 0 || nTimeStamp == 0 || txMint.IsNull());
    }
    bool IsGenesis() const
    {
        return (nType == BLOCK_GENESIS);
    }
    bool IsOrigin() const
    {
        return (nType >> 15);
    }
    bool IsPrimary() const
    {
        return (nType & 1);
    }
    bool IsSubsidiary() const
    {
        return (nType == BLOCK_SUBSIDIARY);
    }
    bool IsExtended() const
    {
        return (nType == BLOCK_EXTENDED);
    }
    bool IsVacant() const
    {
        return (nType == BLOCK_VACANT);
    }
    bool IsProofOfWork() const
    {
        return (txMint.nType == CTransaction::TX_WORK);
    }
    uint256 GetHash() const
    {
        hcode::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << nNumber << hashPrev << hashMerkleRoot << hashStateRoot << hashReceiptsRoot << nBloom << nGasLimit << nGasUsed << vchProof << txMint;
        uint256 hash = metabasenet::crypto::CryptoHash(ss.GetData(), ss.GetSize());
        return uint256(GetBlockHeight(), uint224(hash));
    }
    std::size_t GetTxSerializedOffset() const
    {
        return (sizeof(nVersion)
                + sizeof(nType)
                + sizeof(nTimeStamp)
                + sizeof(nNumber)
                + sizeof(hashPrev)
                + sizeof(hashMerkleRoot)
                + sizeof(hashStateRoot)
                + sizeof(hashReceiptsRoot)
                + sizeof(nBloom)
                + sizeof(nGasLimit)
                + sizeof(nGasUsed)
                + hcode::GetSerializeSize(vchProof));
    }
    void GetSerializedProofOfWorkData(std::vector<unsigned char>& vchProofOfWork) const
    {
        hcode::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << nNumber << hashPrev << vchProof;
        vchProofOfWork.assign(ss.GetData(), ss.GetData() + ss.GetSize());
    }
    uint32 GetBlockTime() const
    {
        return nTimeStamp;
    }
    uint64 GetBlockNumber() const
    {
        return nNumber;
    }
    uint32 GetBlockHeight() const
    {
        if (IsGenesis())
        {
            return 0;
        }
        else if (IsExtended())
        {
            return hashPrev.Get32(7);
        }
        else
        {
            return hashPrev.Get32(7) + 1;
        }
    }
    uint64 GetBlockBeacon(int idx = 0) const
    {
        if (vchProof.empty())
        {
            return hashPrev.Get64(idx & 3);
        }
        return 0;
    }
    bool GetBlockMint(uint256& nMintCoin) const
    {
        if (nType == BLOCK_EXTENDED || nType == BLOCK_VACANT)
        {
            nMintCoin = 0;
            return true;
        }
        bytes btTempData;
        if (txMint.GetTxData(CTransaction::DF_MINTCOIN, btTempData))
        {
            try
            {
                hcode::CBufStream ss(btTempData);
                ss >> nMintCoin;
            }
            catch (std::exception& e)
            {
                return false;
            }
            return true;
        }
        return false;
    }
    uint256 GetBlockTotalReward() const
    {
        bytes btTempData;
        if (txMint.GetTxData(CTransaction::DF_MINTREWARD, btTempData))
        {
            try
            {
                uint256 nTotalReward;
                hcode::CBufStream ss(btTempData);
                ss >> nTotalReward;
                return nTotalReward;
            }
            catch (std::exception& e)
            {
                return 0;
            }
        }
        return 0;
    }
    uint256 GetBlockMoneyDestroy() const
    {
        return 0;
    }
    uint256 BuildMerkleTree(std::vector<uint256>& vMerkleTree) const
    {
        vMerkleTree.clear();
        for (const CTransaction& tx : vtx)
            vMerkleTree.push_back(tx.GetHash());
        size_t j = 0;
        for (size_t nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            for (size_t i = 0; i < nSize; i += 2)
            {
                size_t i2 = std::min(i + 1, nSize - 1);
                vMerkleTree.push_back(metabasenet::crypto::CryptoHash(vMerkleTree[j + i], vMerkleTree[j + i2]));
            }
            j += nSize;
        }
        return (vMerkleTree.empty() ? uint64(0) : vMerkleTree.back());
    }
    uint256 CalcMerkleTreeRoot() const
    {
        std::vector<uint256> vMerkleTree;
        return BuildMerkleTree(vMerkleTree);
    }
    static uint32 GetBlockHeightByHash(const uint256& hash)
    {
        return hash.Get32(7);
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nVersion, opt);
        s.Serialize(nType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(nNumber, opt);
        s.Serialize(hashPrev, opt);
        s.Serialize(hashMerkleRoot, opt);
        s.Serialize(hashStateRoot, opt);
        s.Serialize(hashReceiptsRoot, opt);
        s.Serialize(nBloom, opt);
        s.Serialize(nGasLimit, opt);
        s.Serialize(nGasUsed, opt);
        s.Serialize(vchProof, opt);
        s.Serialize(txMint, opt);
        s.Serialize(vtx, opt);
        s.Serialize(vchSig, opt);
    }
};

class CBlockEx : public CBlock
{
    friend class hcode::CStream;

public:
    uint256 hashBlock;
    uint256 nBlockTrust;

public:
    CBlockEx() {}
    CBlockEx(const CBlock& block, const uint256& nTrustIn = uint256())
      : CBlock(block), nBlockTrust(nTrustIn)
    {
        hashBlock = block.GetHash();
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        CBlock::Serialize(s, opt);
        s.Serialize(hashBlock, opt);
        s.Serialize(nBlockTrust, opt);
    }
};

inline std::string GetBlockTypeStr(uint16 nType, uint16 nMintType)
{
    if (nType == CBlock::BLOCK_GENESIS)
        return std::string("genesis");
    if (nType == CBlock::BLOCK_ORIGIN)
        return std::string("origin");
    if (nType == CBlock::BLOCK_EXTENDED)
        return std::string("extended");
    std::string str("undefined");
    if (nType == CBlock::BLOCK_PRIMARY)
        str = "primary";
    if (nType == CBlock::BLOCK_SUBSIDIARY)
        str = "subsidiary";
    if (nType == CBlock::BLOCK_VACANT)
        str = "vacant";
    if (nMintType == CTransaction::TX_WORK)
        return (str + "-pow");
    if (nMintType == CTransaction::TX_STAKE)
        return (str + "-dpos");
    return str;
}

class CBlockIndex
{
public:
    const uint256* phashBlock;
    CBlockIndex* pOrigin;
    CBlockIndex* pPrev;
    CBlockIndex* pNext;
    uint256 txidMint;
    uint16 nMintType;
    CDestination destMint;
    uint16 nVersion;
    uint16 nType;
    uint32 nTimeStamp;
    uint32 nHeight;
    uint64 nNumber;
    uint64 nTxCount;
    uint256 hashRefBlock;
    uint256 hashStateRoot;
    uint64 nRandBeacon;
    uint256 nGasLimit;
    uint256 nGasUsed;
    uint256 nChainTrust;
    uint256 nBlockReward;
    uint256 nMintReward;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    uint8 nProofAlgo;
    uint8 nProofBits;
    uint32 nFile;
    uint32 nOffset;
    uint32 nBlockCrc;

public:
    CBlockIndex()
    {
        phashBlock = nullptr;
        pOrigin = this;
        pPrev = nullptr;
        pNext = nullptr;
        txidMint = 0;
        nMintType = 0;
        destMint.SetNull();
        nVersion = 0;
        nType = 0;
        nTimeStamp = 0;
        nHeight = 0;
        nNumber = 0;
        nTxCount = 0;
        hashRefBlock = 0;
        hashStateRoot = 0;
        nRandBeacon = 0;
        nGasLimit = 0;
        nGasUsed = 0;
        nChainTrust = uint64(0);
        nBlockReward = 0;
        nMintReward = 0;
        nMoneySupply = 0;
        nMoneyDestroy = 0;
        nProofAlgo = 0;
        nProofBits = 0;
        nFile = 0;
        nOffset = 0;
        nBlockCrc = 0;
    }
    CBlockIndex(const CBlock& block, const uint32 nFileIn, const uint32 nOffsetIn, const uint32 nCrcIn)
    {
        phashBlock = nullptr;
        pOrigin = this;
        pPrev = nullptr;
        pNext = nullptr;
        txidMint = (block.IsVacant() ? uint64(0) : block.txMint.GetHash());
        nMintType = block.txMint.nType;
        destMint = block.txMint.to;
        nVersion = block.nVersion;
        nType = block.nType;
        nTimeStamp = block.nTimeStamp;
        nHeight = block.GetBlockHeight();
        nNumber = block.GetBlockNumber();
        nTxCount = block.vtx.size() + 1;
        hashRefBlock = 0;
        if (!block.vchProof.empty()
            && (block.IsSubsidiary() || block.IsExtended() || block.IsVacant()))
        {
            CProofOfPiggyback proof;
            if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
            {
                hashRefBlock = proof.hashRefBlock;
            }
        }
        hashStateRoot = block.hashStateRoot;
        nRandBeacon = 0;
        nGasLimit = block.nGasLimit;
        nGasUsed = block.nGasUsed;
        nChainTrust = uint64(0);
        nBlockReward = block.GetBlockTotalReward();
        block.GetBlockMint(nMintReward);
        nMoneySupply = 0;
        nMoneyDestroy = 0;
        nProofAlgo = 0;
        nProofBits = 0;
        if (IsProofOfWork())
        {
            CProofOfHashWork proof;
            if (proof.Load(block.vchProof))
            {
                nProofAlgo = proof.nAlgo;
                nProofBits = proof.nBits;
            }
        }
        nFile = nFileIn;
        nOffset = nOffsetIn;
        nBlockCrc = nCrcIn;
    }
    uint256 GetBlockHash() const
    {
        return *phashBlock;
    }
    int GetBlockHeight() const
    {
        return nHeight;
    }
    uint64 GetBlockNumber() const
    {
        return nNumber;
    }
    uint64 GetTxCount() const
    {
        return nTxCount;
    }
    uint256 GetBlockGasUsed() const
    {
        return nGasUsed;
    }
    uint32 GetBlockTime() const
    {
        return nTimeStamp;
    }
    uint256 GetOriginHash() const
    {
        return pOrigin->GetBlockHash();
    }
    uint256 GetParentHash() const
    {
        return (!pOrigin->pPrev ? uint64(0) : pOrigin->pPrev->GetOriginHash());
    }
    uint256 GetPrevHash() const
    {
        return (!pPrev ? uint64(0) : pPrev->GetBlockHash());
    }
    uint256 GetRefBlock() const
    {
        return hashRefBlock;
    }
    uint256 GetStateRoot() const
    {
        return hashStateRoot;
    }
    uint256 GetBlockReward() const
    {
        return nBlockReward;
    }
    uint256 GetMintReward() const
    {
        return nMintReward;
    }
    uint256 GetMoneySupply() const
    {
        return nMoneySupply;
    }
    uint256 GetMoneyDestroy() const
    {
        return nMoneyDestroy;
    }
    bool IsOrigin() const
    {
        return (nType >> 15);
    }
    bool IsPrimary() const
    {
        return (nType & 1);
    }
    bool IsSubsidiary() const
    {
        return (nType == CBlock::BLOCK_SUBSIDIARY);
    }
    bool IsExtended() const
    {
        return (nType == CBlock::BLOCK_EXTENDED);
    }
    bool IsVacant() const
    {
        return (nType == CBlock::BLOCK_VACANT);
    }
    bool IsProofOfWork() const
    {
        return (nMintType == CTransaction::TX_WORK);
    }
    bool IsEquivalent(const CBlockIndex* pIndexCompare) const
    {
        if (pIndexCompare != nullptr)
        {
            const CBlockIndex* pIndex = this;
            while (pIndex)
            {
                if (pIndex == pIndexCompare)
                {
                    return true;
                }
                if (pIndex->nType != CBlock::BLOCK_VACANT
                    || pIndex->GetBlockHeight() <= pIndexCompare->GetBlockHeight())
                {
                    break;
                }
                pIndex = pIndex->pPrev;
            }
        }
        return false;
    }
    int GetExtendedSequence() const
    {
        int nSeq = 0;
        const CBlockIndex* pIndex = this;
        while (pIndex && pIndex->IsExtended())
        {
            nSeq++;
            pIndex = pIndex->pPrev;
        }
        return nSeq;
    }
    const std::string GetBlockType() const
    {
        return GetBlockTypeStr(nType, nMintType);
    }
    std::string ToString() const
    {
        std::ostringstream oss;
        oss << "CBlockIndex : hash=" << GetBlockHash().ToString()
            << " prev=" << (pPrev ? pPrev->GetBlockHash().ToString() : "nullptr")
            << " height=" << nHeight
            << " type=" << GetBlockType()
            << " time=" << nTimeStamp
            << " trust=" << nChainTrust.ToString();
        return oss.str();
    }
};

class CBlockOutline : public CBlockIndex
{
    friend class hcode::CStream;

public:
    uint256 hashBlock;
    uint256 hashPrev;
    uint256 hashOrigin;

public:
    CBlockOutline()
    {
        hashBlock = 0;
        hashPrev = 0;
        hashOrigin = 0;
    }
    CBlockOutline(const CBlockIndex* pIndex)
      : CBlockIndex(*pIndex)
    {
        hashBlock = pIndex->GetBlockHash();
        hashPrev = (pPrev ? pPrev->GetBlockHash() : uint64(0));
        hashOrigin = pOrigin->GetBlockHash();
    }
    uint256 GetBlockHash() const
    {
        return hashBlock;
    }
    uint32 GetCrc() const
    {
        hcode::CBufStream ss;
        ss << *this;
        return metabasenet::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
    }
    std::string ToString() const
    {
        std::ostringstream oss;
        oss << "CBlockOutline : hash=" << GetBlockHash().ToString()
            << " prev=" << hashPrev.ToString()
            << " height=" << nHeight << " file=" << nFile << " offset=" << nOffset
            << " type=" << GetBlockType();
        return oss.str();
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(hashPrev, opt);
        s.Serialize(hashOrigin, opt);
        s.Serialize(txidMint, opt);
        s.Serialize(nMintType, opt);
        s.Serialize(destMint, opt);
        s.Serialize(nVersion, opt);
        s.Serialize(nType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(nHeight, opt);
        s.Serialize(nNumber, opt);
        s.Serialize(nTxCount, opt);
        s.Serialize(hashRefBlock, opt);
        s.Serialize(hashStateRoot, opt);
        s.Serialize(nRandBeacon, opt);
        s.Serialize(nGasLimit, opt);
        s.Serialize(nGasUsed, opt);
        s.Serialize(nChainTrust, opt);
        s.Serialize(nBlockReward, opt);
        s.Serialize(nMintReward, opt);
        s.Serialize(nMoneySupply, opt);
        s.Serialize(nMoneyDestroy, opt);
        s.Serialize(nProofAlgo, opt);
        s.Serialize(nProofBits, opt);
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
        s.Serialize(nBlockCrc, opt);
    }
};

class CBlockVerify
{
    friend class hcode::CStream;

public:
    uint32 nPrevVerifyCrc;
    uint256 hashBlock;
    uint32 nFile;
    uint32 nOffset;
    uint32 nBlockCrc;
    uint32 nIndexCrc;
    uint32 nRootCrc;

public:
    CBlockVerify()
    {
        nPrevVerifyCrc = 0;
        nFile = 0;
        nOffset = 0;
        nBlockCrc = 0;
        nIndexCrc = 0;
        nRootCrc = 0;
    }
    CBlockVerify(const uint32 nPrevVerifyCrcIn, const CBlockOutline& outline, const uint32 nRootCrcIn)
    {
        nPrevVerifyCrc = nPrevVerifyCrcIn;
        hashBlock = outline.GetBlockHash();
        nFile = outline.nFile;
        nOffset = outline.nOffset;
        nBlockCrc = outline.nBlockCrc;
        nIndexCrc = outline.GetCrc();
        nRootCrc = nRootCrcIn;
    }

    uint32 GetCrc() const
    {
        hcode::CBufStream ss;
        ss << *this;
        return metabasenet::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nPrevVerifyCrc, opt);
        s.Serialize(hashBlock, opt);
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
        s.Serialize(nBlockCrc, opt);
        s.Serialize(nIndexCrc, opt);
        s.Serialize(nRootCrc, opt);
    }
};

class CBlockRoot
{
    friend class hcode::CStream;

public:
    CBlockRoot() {}

    uint32 GetRootCrc() const
    {
        hcode::CBufStream ss;
        ss << *this;
        return metabasenet::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
    }

public:
    uint256 hashStateRoot;
    uint256 hashForkContextRoot;
    uint256 hashAddressRoot;
    uint256 hashTxIndexRoot;
    uint256 hashCodeRoot;
    uint256 hashDelegateRoot;
    uint256 hashVoteRoot;
    uint256 hashVoteRedeemRoot;
    uint256 hashInviteRoot;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(hashStateRoot, opt);
        s.Serialize(hashForkContextRoot, opt);
        s.Serialize(hashAddressRoot, opt);
        s.Serialize(hashTxIndexRoot, opt);
        s.Serialize(hashCodeRoot, opt);
        s.Serialize(hashDelegateRoot, opt);
        s.Serialize(hashVoteRoot, opt);
        s.Serialize(hashVoteRedeemRoot, opt);
        s.Serialize(hashInviteRoot, opt);
    }
};

class CBlockLocator
{
    friend class hcode::CStream;

public:
    CBlockLocator() {}
    virtual ~CBlockLocator() {}

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(vBlockHash, opt);
    }

public:
    std::vector<uint256> vBlockHash;
};

class CDestState
{
    friend class hcode::CStream;

public:
    uint64 nTxNonce;
    uint256 nBalance;
    uint256 hashStorageRoot;
    uint256 hashCode;

public:
    CDestState()
      : nTxNonce(0) {}
    CDestState(const uint256& nBalanceIn)
      : nTxNonce(0), nBalance(nBalanceIn) {}

    void SetNull()
    {
        nTxNonce = 0;
        nBalance = 0;
        hashStorageRoot = 0;
        hashCode = 0;
    }
    bool IsNull() const
    {
        if (nTxNonce != 0 || nBalance != 0 || hashStorageRoot != 0 || hashCode != 0)
        {
            return false;
        }
        return true;
    }
    uint256 GetBalance() const
    {
        return nBalance;
    }
    uint64 GetNextTxNonce() const
    {
        return nTxNonce + 1;
    }
    bool VerifyTxNonce(const uint64& nTxNonceIn) const
    {
        return (nTxNonceIn == nTxNonce + 1);
    }

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(nTxNonce, opt);
        s.Serialize(nBalance, opt);
        s.Serialize(hashStorageRoot, opt);
        s.Serialize(hashCode, opt);
    }
};

class CVoteContext
{
    friend class hcode::CStream;

public:
    CVoteContext()
      : nRewardMode(0), nLastVoteHeight(0) {}

    enum
    {
        REWARD_MODE_VOTE = 0,
        REWARD_MODE_OWNER = 1
    };

public:
    CDestination destDelegate;
    CDestination destOwner;
    uint8 nRewardMode;
    uint256 nVoteAmount;
    uint32 nLastVoteHeight;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(destDelegate, opt);
        s.Serialize(destOwner, opt);
        s.Serialize(nRewardMode, opt);
        s.Serialize(nVoteAmount, opt);
        s.Serialize(nLastVoteHeight, opt);
    }
};

class CVoteRedeemContext
{
    friend class hcode::CStream;

public:
    CVoteRedeemContext()
      : nLastRedeemHeight(0) {}

public:
    CDestination destOwner;
    uint256 nRedeemAmount;
    uint32 nLastRedeemHeight;

protected:
    template <typename O>
    void Serialize(hcode::CStream& s, O& opt)
    {
        s.Serialize(destOwner, opt);
        s.Serialize(nRedeemAmount, opt);
        s.Serialize(nLastRedeemHeight, opt);
    }
};

class CReceiptMerkleTree
{
public:
    static uint256 BuildMerkleTree(const std::vector<uint256>& vReceipt)
    {
        std::vector<uint256> vMerkleTree;
        vMerkleTree = vReceipt;
        size_t j = 0;
        for (size_t nSize = vReceipt.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            for (size_t i = 0; i < nSize; i += 2)
            {
                size_t i2 = std::min(i + 1, nSize - 1);
                vMerkleTree.push_back(metabasenet::crypto::CryptoHash(vMerkleTree[j + i], vMerkleTree[j + i2]));
            }
            j += nSize;
        }
        return (vMerkleTree.empty() ? uint64(0) : vMerkleTree.back());
    }
};

class CBlockChainUpdate
{
public:
    CBlockChainUpdate()
    {
        SetNull();
    }
    CBlockChainUpdate(CBlockIndex* pIndex)
    {
        hashFork = pIndex->GetOriginHash();
        hashParent = pIndex->GetParentHash();
        nOriginHeight = pIndex->pOrigin->GetBlockHeight() - 1;
        hashPrevBlock = pIndex->GetPrevHash();
        hashLastBlock = pIndex->GetBlockHash();
        nLastBlockTime = pIndex->GetBlockTime();
        nLastBlockHeight = pIndex->GetBlockHeight();
        nLastBlockNumber = pIndex->GetBlockNumber();
        nLastMintType = pIndex->nMintType;
        nMoneySupply = pIndex->GetMoneySupply();
        nMoneyDestroy = pIndex->GetMoneyDestroy();
    }
    void SetNull()
    {
        hashFork = 0;
        nOriginHeight = -1;
        nLastBlockHeight = -1;
        nLastBlockNumber = 0;
        nLastMintType = 0;
    }
    bool IsNull() const
    {
        return (hashFork == 0);
    }

public:
    uint256 hashFork;
    uint256 hashParent;
    int nOriginHeight;
    uint256 hashPrevBlock;
    uint256 hashLastBlock;
    int64 nLastBlockTime;
    int nLastBlockHeight;
    uint32 nLastBlockNumber;
    uint16 nLastMintType;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    std::set<uint256> setTxUpdate;
    std::vector<CBlockEx> vBlockAddNew;
    std::vector<CBlockEx> vBlockRemove;
};

#endif //COMMON_BLOCK_H

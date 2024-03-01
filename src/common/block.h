// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_BLOCK_H
#define COMMON_BLOCK_H

#include <stream/stream.h>
#include <vector>

#include "crc24q.h"
#include "profile.h"
#include "proof.h"
#include "template/template.h"
#include "transaction.h"
#include "uint256.h"
#include "util.h"

namespace metabasenet
{

class CBlock
{
    friend class mtbase::CStream;

public:
    uint8 nVersion;
    uint8 nType;
    uint64 nTimeStamp;
    uint64 nNumber;
    uint16 nSlot;
    uint256 hashPrev;
    uint256 hashMerkleRoot; // Merkle: bloom hash, mint tx hash, vtx hash list
    uint256 hashStateRoot;
    uint256 hashReceiptsRoot;
    uint256 nGasLimit;
    uint256 nGasUsed;
    std::map<uint8, bytes> mapProof;
    CTransaction txMint;
    std::vector<uint8> vchSig;

    bytes btBloomData;
    std::vector<CTransaction> vtx;

    enum
    {
        BLOCK_GENESIS = 0x01,
        BLOCK_PRIMARY = 0x02,

        BLOCK_ORIGIN = 0x11,
        BLOCK_SUBSIDIARY = 0x12,
        BLOCK_EXTENDED = 0x13,
        BLOCK_VACANT = 0x14,
    };
    enum
    {
        BP_FORK_PROFILE = 0x01,
        BP_DELEGATE = 0x02,
        BP_PIGGYBACK = 0x03,
        BP_HASH_WORK = 0x04,
        BP_MINTCOIN = 0x05,
        BP_MINTREWARD = 0x06,
    };

public:
    CBlock()
    {
        SetNull();
    }

    void SetNull();
    bool IsNull() const;
    bool IsGenesis() const;
    bool IsOrigin() const;
    bool IsPrimary() const;
    bool IsSubsidiary() const;
    bool IsExtended() const;
    bool IsVacant() const;
    bool IsProofOfWork() const;
    bool IsProofEmpty() const;
    uint256 GetHash() const;
    std::size_t GetTxSerializedOffset() const;
    void GetSerializedProofOfWorkData(std::vector<unsigned char>& vchProofOfWork) const;
    uint32 GetChainId() const;
    uint64 GetBlockTime() const;
    uint64 GetBlockNumber() const;
    uint32 GetBlockSlot() const;
    uint32 GetBlockHeight() const;
    uint64 GetBlockBeacon(int idx = 0) const;
    bool GetBlockMint(uint256& nMintCoin) const;
    uint256 GetBlockTotalReward() const;
    uint256 GetBlockMoneyDestroy() const;
    void SetBlockTime(const uint64 nTime);
    uint256 CalcMerkleTreeRoot() const;
    void SetSignData(const bytes& btSigData);
    bool VerifyBlockSignature(const CDestination& destBlockSign) const;
    bool VerifyBlockProof() const;

    void AddProofData(const uint8 nProofId, const bytes& btData);
    bool GetProofData(const uint8 nProofId, bytes& btData) const;

    void AddForkProfile(const CProfile& profile);
    void AddDelegateProof(const CProofOfDelegate& proof);
    void AddPiggybackProof(const CProofOfPiggyback& proof);
    void AddHashWorkProof(const CProofOfHashWork& proof);
    void AddMintCoinProof(const uint256& nMintCoin);
    void AddMintRewardProof(const uint256& nMintReward);

    bool GetForkProfile(CProfile& profile) const;
    bool GetDelegateProof(CProofOfDelegate& proof) const;
    bool GetPiggybackProof(CProofOfPiggyback& proof) const;
    bool GetHashWorkProof(CProofOfHashWork& proof) const;
    bool GetMintCoinProof(uint256& nMintCoin) const;
    bool GetMintRewardProof(uint256& nMintReward) const;

    static uint32 GetBlockChainIdByHash(const uint256& hash);
    static uint32 GetBlockHeightByHash(const uint256& hash);
    static uint16 GetBlockSlotByHash(const uint256& hash);

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockEx : public CBlock
{
    friend class mtbase::CStream;

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
    void Serialize(mtbase::CStream& s, O& opt)
    {
        CBlock::Serialize(s, opt);
        s.Serialize(hashBlock, opt);
        s.Serialize(nBlockTrust, opt);
    }
};

inline std::string GetBlockTypeStr(uint16 nType, uint16 nMintType)
{
    switch (nType)
    {
    case CBlock::BLOCK_GENESIS:
        return std::string("genesis");
    case CBlock::BLOCK_PRIMARY:
        if (nMintType == CTransaction::TX_WORK)
            return std::string("primary-poa");
        else if (nMintType == CTransaction::TX_STAKE)
            return std::string("primary-stake");
    case CBlock::BLOCK_ORIGIN:
        return std::string("origin");
    case CBlock::BLOCK_SUBSIDIARY:
        return std::string("subsidiary");
    case CBlock::BLOCK_EXTENDED:
        return std::string("extended");
    case CBlock::BLOCK_VACANT:
        return std::string("vacant");
    }
    return std::string("undefined");
}

class CBlockRootStatus
{
    friend class mtbase::CStream;

public:
    uint16 nBlockType;
    uint64 nBlockTimeStamp;
    CDestination destMint;

public:
    CBlockRootStatus()
      : nBlockType(0), nBlockTimeStamp(0) {}
    CBlockRootStatus(const uint16 nBlockTypeIn, const uint64 nBlockTimeStampIn, const CDestination& destMintIn)
      : nBlockType(nBlockTypeIn), nBlockTimeStamp(nBlockTimeStampIn), destMint(destMintIn) {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nBlockType, opt);
        s.Serialize(nBlockTimeStamp, opt);
        s.Serialize(destMint, opt);
    }
};

class CBlockIndex
{
public:
    const uint256* phashBlock;
    CBlockIndex* pOrigin;
    CBlockIndex* pPrev;
    CBlockIndex* pNext;
    CChainId nChainId;
    uint256 txidMint;
    uint16 nMintType;
    CDestination destMint;
    uint16 nVersion;
    uint16 nType;
    uint64 nTimeStamp;
    uint32 nHeight;
    uint16 nSlot;
    uint64 nNumber;
    uint64 nTxCount;
    uint64 nRewardTxCount;
    uint64 nUserTxCount;
    uint256 nAgreement;
    uint256 hashRefBlock;
    uint256 hashStateRoot;
    uint64 nRandBeacon;
    uint256 nGasLimit;
    uint256 nGasUsed;
    uint256 nChainTrust;
    uint256 nBlockReward;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    uint8 nProofAlgo;
    uint8 nProofBits;
    uint32 nFile;
    uint32 nOffset;
    uint32 nBlockCrc;

public:
    CBlockIndex();
    CBlockIndex(const uint256& hashBlock, const CBlock& block, const uint32 nFileIn, const uint32 nOffsetIn, const uint32 nCrcIn);

    uint256 GetBlockHash() const
    {
        return *phashBlock;
    }
    int GetBlockHeight() const
    {
        return nHeight;
    }
    uint16 GetBlockSlot() const
    {
        return nSlot;
    }
    uint64 GetBlockNumber() const
    {
        return nNumber;
    }
    uint64 GetTxCount() const
    {
        return nTxCount;
    }
    uint64 GetRewardTxCount() const
    {
        return nRewardTxCount;
    }
    uint64 GetUserTxCount() const
    {
        return nUserTxCount;
    }
    uint256 GetBlockGasUsed() const
    {
        return nGasUsed;
    }
    uint64 GetBlockTime() const
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
    uint256 GetAgreement() const
    {
        return nAgreement;
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
        return (nType == CBlock::BLOCK_GENESIS || nType == CBlock::BLOCK_ORIGIN);
    }
    bool IsPrimary() const
    {
        return ((nType >> 4) == 0);
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
    friend class mtbase::CStream;

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
        mtbase::CBufStream ss;
        ss << *this;
        return crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
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
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashBlock, opt);
        s.Serialize(hashPrev, opt);
        s.Serialize(hashOrigin, opt);
        s.Serialize(nChainId, opt);
        s.Serialize(txidMint, opt);
        s.Serialize(nMintType, opt);
        s.Serialize(destMint, opt);
        s.Serialize(nVersion, opt);
        s.Serialize(nType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(nHeight, opt);
        s.Serialize(nSlot, opt);
        s.Serialize(nNumber, opt);
        s.Serialize(nTxCount, opt);
        s.Serialize(nRewardTxCount, opt);
        s.Serialize(nUserTxCount, opt);
        s.Serialize(nAgreement, opt);
        s.Serialize(hashRefBlock, opt);
        s.Serialize(hashStateRoot, opt);
        s.Serialize(nRandBeacon, opt);
        s.Serialize(nGasLimit, opt);
        s.Serialize(nGasUsed, opt);
        s.Serialize(nChainTrust, opt);
        s.Serialize(nBlockReward, opt);
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
    friend class mtbase::CStream;

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
        mtbase::CBufStream ss;
        ss << *this;
        return crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
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
    friend class mtbase::CStream;

public:
    CBlockRoot() {}

    uint32 GetRootCrc() const
    {
        mtbase::CBufStream ss;
        ss << *this;
        return crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
    }

public:
    uint256 hashStateRoot;
    uint256 hashForkContextRoot;
    uint256 hashAddressRoot;
    uint256 hashBlockNumberRoot;
    uint256 hashTxIndexRoot;
    uint256 hashCodeRoot;
    uint256 hashDelegateRoot;
    uint256 hashVoteRoot;
    uint256 hashVoteRewardRoot;

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashStateRoot, opt);
        s.Serialize(hashForkContextRoot, opt);
        s.Serialize(hashAddressRoot, opt);
        s.Serialize(hashBlockNumberRoot, opt);
        s.Serialize(hashTxIndexRoot, opt);
        s.Serialize(hashCodeRoot, opt);
        s.Serialize(hashDelegateRoot, opt);
        s.Serialize(hashVoteRoot, opt);
        s.Serialize(hashVoteRewardRoot, opt);
    }
};

class CBlockLocator
{
    friend class mtbase::CStream;

public:
    CBlockLocator() {}
    virtual ~CBlockLocator() {}

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(vBlockHash, opt);
    }

public:
    std::vector<uint256> vBlockHash;
};

class CDestState
{
    friend class mtbase::CStream;

protected:
    uint8 nType; // high 3 bit: address type, ref CDestination::PREFIX_*, low 5 bit: template type
    uint64 nTxNonce;
    uint256 nBalance;
    uint256 hashStorageRoot;
    uint256 hashCode;
    bool fDestroy;

public:
    CDestState()
      : nType(0), nTxNonce(0), fDestroy(false) {}
    CDestState(const uint256& nBalanceIn)
      : nType(0), nTxNonce(0), nBalance(nBalanceIn), fDestroy(false) {}
    CDestState(const uint8 nDestType, const uint8 nTemplateType, const uint256& nBalanceIn)
      : nTxNonce(0), nBalance(nBalanceIn), fDestroy(false)
    {
        SetType(nDestType, nTemplateType);
    }

    void SetNull()
    {
        nType = 0;
        nTxNonce = 0;
        nBalance = 0;
        hashStorageRoot = 0;
        hashCode = 0;
        fDestroy = false;
    }
    bool IsNull() const
    {
        if (nType != 0 || nTxNonce != 0 || nBalance != 0 || hashStorageRoot != 0 || hashCode != 0)
        {
            return false;
        }
        return true;
    }
    uint8 GetType() const
    {
        return nType;
    }
    void SetType(const uint8 nDestType, const uint8 nTemplateType = 0)
    {
        nType = ((nDestType << 5) | (nTemplateType & 0x1F));
    }
    uint8 GetDestType() const
    {
        return (nType >> 5);
    }
    uint8 GetTemplateType() const
    {
        return (nType & 0x1F);
    }
    bool IsPubkey() const
    {
        return (GetDestType() == CDestination::PREFIX_PUBKEY);
    }
    bool IsTemplate() const
    {
        return (GetDestType() == CDestination::PREFIX_TEMPLATE);
    }
    bool IsContract() const
    {
        return (GetDestType() == CDestination::PREFIX_CONTRACT);
    }
    void SetTxNonce(const uint64 n)
    {
        nTxNonce = n;
    }
    uint64 GetTxNonce() const
    {
        return nTxNonce;
    }
    void IncTxNonce()
    {
        nTxNonce++;
    }
    uint64 GetNextTxNonce() const
    {
        return nTxNonce + 1;
    }
    bool VerifyTxNonce(const uint64& nTxNonceIn) const
    {
        return (nTxNonceIn == nTxNonce + 1);
    }
    uint256 GetBalance() const
    {
        return nBalance;
    }
    void SetBalance(const uint256& n)
    {
        nBalance = n;
    }
    void IncBalance(const uint256& n)
    {
        nBalance += n;
    }
    void DecBalance(const uint256& n)
    {
        nBalance -= n;
    }
    uint256 GetStorageRoot() const
    {
        return hashStorageRoot;
    }
    void SetStorageRoot(const uint256& h)
    {
        hashStorageRoot = h;
    }
    uint256 GetCodeHash() const
    {
        return hashCode;
    }
    void SetCodeHash(const uint256& h)
    {
        hashCode = h;
    }
    bool IsDestroy() const
    {
        return fDestroy;
    }
    void SetDestroy(const bool f = true)
    {
        fDestroy = f;
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nType, opt);
        s.Serialize(nTxNonce, opt);
        s.Serialize(nBalance, opt);
        s.Serialize(hashStorageRoot, opt);
        s.Serialize(hashCode, opt);
        s.Serialize(fDestroy, opt);
    }
};

class CVoteContext
{
    friend class mtbase::CStream;

public:
    CVoteContext()
      : nRewardMode(0), nRewardRate(0), nFinalHeight(0) {}

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
    uint16 nRewardRate;  // base: 10000, example: 5000 is 50%
    uint32 nFinalHeight; // 0: unlimit

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(destDelegate, opt);
        s.Serialize(destOwner, opt);
        s.Serialize(nRewardMode, opt);
        s.Serialize(nVoteAmount, opt);
        s.Serialize(nRewardRate, opt);
        s.Serialize(nFinalHeight, opt);
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
                vMerkleTree.push_back(crypto::CryptoHash(vMerkleTree[j + i], vMerkleTree[j + i2]));
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
    CBlockChainUpdate(const CBlockIndex* pIndex)
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

} // namespace metabasenet

#endif //COMMON_BLOCK_H

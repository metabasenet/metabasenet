// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_BLOCK_H
#define COMMON_BLOCK_H

#include <stream/stream.h>
#include <structure/merkletree.h>
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

class CBlockProve;

class CBlock
{
    friend class mtbase::CStream;

public:
    uint8 nVersion;
    uint8 nType;
    uint64 nTimeStamp;
    uint64 nNumber; // 0-based starting from genesis block for primary fork or origin block for subsidary fork
    uint32 nHeight;
    uint16 nSlot;
    uint256 hashPrev;
    uint256 hashMerkleRoot;
    uint256 hashStateRoot;
    uint256 hashReceiptsRoot;
    uint256 hashCrosschainMerkleRoot;
    uint256 nGasLimit;
    uint256 nGasUsed;
    std::map<uint8, bytes> mapProof;
    CTransaction txMint;
    std::vector<uint8> vchSig;

    bytes btBloomData;
    std::vector<CTransaction> vtx;
    std::map<CChainId, CBlockProve> mapProve; // key: peer chainid

    enum
    {   // block type: no coinbase for extended/vacant
        BLOCK_GENESIS = 0x01,
        BLOCK_PRIMARY = 0x02,

        BLOCK_ORIGIN = 0x11,
        BLOCK_SUBSIDIARY = 0x12,
        BLOCK_EXTENDED = 0x13,
        BLOCK_VACANT = 0x14,
    };
    enum
    {   // block proof type
        BP_FORK_PROFILE = 0x01,
        BP_DELEGATE = 0x02,
        BP_PIGGYBACK = 0x03,    // BLOCK_SUBSIDIARY or BLOCK_EXTENDED or BLOCK_VACANT
        BP_HASH_WORK = 0x04,
        BP_MINTCOIN = 0x05,     // coinbase
        BP_MINTREWARD = 0x06,   // equal to or less than (coinbase + txfees)
        BP_BLOCK_VOTE_SIG = 0x07,
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
    uint256 GetRefBlock() const;
    uint64 GetBlockBeacon(int idx = 0) const;
    bool GetBlockMint(uint256& nMintCoin) const;
    uint256 GetBlockTotalReward() const;
    uint256 GetBlockMoneyDestroy() const;
    void SetBlockTime(const uint64 nTime);
    void SetSignData(const bytes& btSigData);
    void UpdateMerkleRoot();
    bool VerifyBlockHeight() const;
    bool VerifyBlockMerkleTreeRoot() const;
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
    void AddBlockVoteSig(const CBlockVoteSig& proof);

    bool GetForkProfile(CProfile& profile) const;
    bool GetDelegateProof(CProofOfDelegate& proof) const;
    bool GetPiggybackProof(CProofOfPiggyback& proof) const;
    bool GetHashWorkProof(CProofOfHashWork& proof) const;
    bool GetMintCoinProof(uint256& nMintCoin) const;
    bool GetMintRewardProof(uint256& nMintReward) const;
    bool GetBlockVoteSig(CBlockVoteSig& proof) const;

    bool GetMerkleProve(mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, mtbase::SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const;

    static CChainId GetBlockChainIdByHash(const uint256& hash);
    static uint32 GetBlockHeightByHash(const uint256& hash);
    static uint16 GetBlockSlotByHash(const uint256& hash);
    static uint256 CreateBlockHash(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, const uint256& hash);

    static bool BlockHashCompare(const uint256& a, const uint256& b); // return true: a < b, false: a >= b
    static bool VerifyBlockMerkleProve(const uint256& hashBlock, const mtbase::MERKLE_PROVE_DATA& merkleProve, const uint256& hashVerify);

protected:
    uint256 CalcMerkleTreeRoot() const;
    uint256 CalcMerkleTreeRootAndProve(mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock, mtbase::SHP_MERKLE_PROVE_DATA ptrCrossMerkleProve) const;
    uint256 CalcBlockBaseMerkleTreeRoot(mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProvePrevBlock, mtbase::SHP_MERKLE_PROVE_DATA ptrMerkleProveRefBlock) const;
    uint256 CalcTxMerkleTreeRoot() const;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

struct CustomBlockHashCompare
{
    bool operator()(const uint256& a, const uint256& b) const
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
            else if (nSlotA == nSlotB)
            {
                CChainId nChainIdA = CBlock::GetBlockChainIdByHash(a);
                CChainId nChainIdB = CBlock::GetBlockChainIdByHash(b);
                if (nChainIdA < nChainIdB)
                {
                    return true;
                }
                else if (nChainIdA == nChainIdB && a < b)
                {
                    return true;
                }
            }
        }
        return false;
    }
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
    uint64 nRewardTxCount;  // ToDo
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
    uint32 GetBlockHeight() const
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
    uint256 hashDexOrderRoot;

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
        s.Serialize(hashDexOrderRoot, opt);
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

class CDestState    // for account modal (related to UTXO)
{
    friend class mtbase::CStream;

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

protected:
    uint8 nType; // high 3-bits: address type, CDestination::PREFIX_PUBKEY/PREFIX_TEMPLATE/PREFIX_CONTRACT, low 5-bits: template type
    uint64 nTxNonce;
    uint256 nBalance;
    uint256 hashStorageRoot;
    uint256 hashCode;
    bool fDestroy;
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
    uint16 nRewardRate;  // base: 10000, example: 5000 is 50%   ToDo: to be removed
    uint32 nFinalHeight; // 0: unlimit   ToDo: to be removed

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

class CBlockChainUpdate
{
public:
    CBlockChainUpdate()
    {
        SetNull();
    }
    CBlockChainUpdate(const CBlockIndex* pIndex, const std::vector<CTransactionReceipt>& vBlockTxReceiptsIn)
    {
        SetNull();

        hashFork = pIndex->GetOriginHash();
        hashParent = pIndex->GetParentHash();
        nOriginHeight = pIndex->pOrigin->GetBlockHeight() - 1;
        hashPrevBlock = pIndex->GetPrevHash();
        hashLastBlock = pIndex->GetBlockHash();
        nLastBlockTime = pIndex->GetBlockTime();
        nLastBlockHeight = pIndex->GetBlockHeight();
        nLastBlockNumber = pIndex->GetBlockNumber();
        nLastMintType = pIndex->nMintType;
        nLastChainTrust = pIndex->nChainTrust;
        nMoneySupply = pIndex->GetMoneySupply();
        nMoneyDestroy = pIndex->GetMoneyDestroy();
        vBlockTxReceipts = vBlockTxReceiptsIn;
    }
    void SetNull()
    {
        hashFork = 0;
        hashParent = 0;
        nOriginHeight = -1;
        hashPrevBlock = 0;
        hashLastBlock = 0;
        nLastBlockTime = 0;
        nLastBlockHeight = -1;
        nLastBlockNumber = 0;
        nLastMintType = 0;
        nLastChainTrust = 0;
        nMoneySupply = 0;
        nMoneyDestroy = 0;
        setTxUpdate.clear();
        vBlockAddNew.clear();
        vBlockRemove.clear();
        mapBlockProve.clear();
        vBlockTxReceipts.clear();
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
    uint256 nLastChainTrust;
    uint256 nMoneySupply;
    uint256 nMoneyDestroy;
    std::set<uint256> setTxUpdate;
    std::vector<CBlockEx> vBlockAddNew;
    std::vector<CBlockEx> vBlockRemove;
    std::map<CChainId, CBlockProve> mapBlockProve;
    std::vector<CTransactionReceipt> vBlockTxReceipts;
};

class CBlockCoinTransferProve
{
    friend class mtbase::CStream;

public:
    CBlockCoinTransferProve()
      : nOriChainId(0), nDestChainId(0) {}
    CBlockCoinTransferProve(const CDestination& destTransferIn, const std::string& strCoinSymbolIn, const CChainId nOriChainIdIn, const CChainId nDestChainIdIn, const uint256& nTransferAmountIn)
      : destTransfer(destTransferIn), strCoinSymbol(strCoinSymbolIn), nOriChainId(nOriChainIdIn), nDestChainId(nDestChainIdIn), nTransferAmount(nTransferAmountIn) {}

public:
    CDestination destTransfer;
    std::string strCoinSymbol;
    CChainId nOriChainId;
    CChainId nDestChainId;
    uint256 nTransferAmount;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const
    {
        s << destTransfer << strCoinSymbol << nOriChainId << nDestChainId << nTransferAmount.ToValidBigEndianData();
    }
    void Serialize(mtbase::CStream& s, mtbase::LoadType&)
    {
        bytes btTransferAmount;
        s >> destTransfer >> strCoinSymbol >> nOriChainId >> nDestChainId >> btTransferAmount;
        nTransferAmount.FromValidBigEndianData(btTransferAmount);
    }
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const
    {
        (void)s;
        mtbase::CBufStream ss;
        ss << destTransfer << strCoinSymbol << nOriChainId << nDestChainId << nTransferAmount.ToValidBigEndianData();
        serSize = ss.GetSize();
    }
};

class CBlockDexOrderProve
{
    friend class mtbase::CStream;

public:
    CBlockDexOrderProve()
      : nChainIdOwner(0), nChainIdPeer(0), nOrderNumber(0) {}
    CBlockDexOrderProve(const CDestination& destOrderIn, const CChainId nChainIdOwnerIn, const CChainId nChainIdPeerIn, const std::string& strCoinSymbolOwnerIn, const std::string& strCoinSymbolPeerIn,
                        const uint64 nOrderNumberIn, const uint256& nOrderAmountIn, const uint256& nOrderPriceIn)
      : destOrder(destOrderIn), nChainIdOwner(nChainIdOwnerIn), nChainIdPeer(nChainIdPeerIn), strCoinSymbolOwner(strCoinSymbolOwnerIn), strCoinSymbolPeer(strCoinSymbolPeerIn),
        nOrderNumber(nOrderNumberIn), nOrderAmount(nOrderAmountIn), nOrderPrice(nOrderPriceIn) {}

    CDexOrderHeader GetDexOrderHeader() const
    {
        return CDexOrderHeader(nChainIdOwner, destOrder, strCoinSymbolOwner, strCoinSymbolPeer, nOrderNumber);
    }

public:
    CDestination destOrder;
    CChainId nChainIdOwner;
    CChainId nChainIdPeer;
    std::string strCoinSymbolOwner;
    std::string strCoinSymbolPeer;
    uint64 nOrderNumber;
    uint256 nOrderAmount;
    uint256 nOrderPrice;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockCrosschainProve
{
    friend class mtbase::CStream;

public:
    CBlockCrosschainProve(const uint256& hashPrevProveBlockIn = uint256())
      : nProveVersion(CP_PROVE_VERSION_1), hashPrevProveBlock(hashPrevProveBlockIn) {}

    void Clear();
    bool IsNull() const;
    bool IsCrossProveNull() const;
    uint256 GetHash() const;

    void AddCoinTransferProve(const CDestination& destTransIn, const std::string& strCoinSymbolIn, const CChainId nOriChainIdIn, const CChainId nDestChainIdIn, const uint256& nTransferAmountIn);
    void AddDexOrderProve(const CDestination& destOrderIn, const CChainId nChainIdOwnerIn, const CChainId nChainIdPeerIn, const std::string& strCoinSymbolOwnerIn,
                          const std::string& strCoinSymbolPeerIn, const uint64 nOrderNumberIn, const uint256& nOrderAmountIn, const uint256& nOrderPriceIn);
    void AddDexOrderProve(const CBlockDexOrderProve& orderProve);
    void AddCrossConfirmRecvBlock(const uint256& hashRecvBlock);

    void SetPrevProveBlock(const uint256& hashBlock);
    const uint256& GetPrevProveBlock() const;
    const std::vector<CBlockCoinTransferProve>& GetCoinTransferProve() const;
    const std::map<CDexOrderHeader, CBlockDexOrderProve>& GetDexOrderProve() const;
    const std::set<uint256>& GetCrossConfirmRecvBlock() const;

    void SetProveData(const std::map<uint8, bytes>& mapProveData);
    void GetProveData(std::map<uint8, bytes>& mapProveData) const;

public:
    enum
    {
        // v1
        CP_PROVE_TYPE_COIN_TRANSFER_PROVE = 1,
        CP_PROVE_TYPE_DEX_ORDER_PROVE = 2,
        CP_PROVE_TYPE_CROSS_CONFIRM_RECV_BLOCK_PROVE = 3,
    };

    enum
    {
        CP_PROVE_VERSION_1 = 1
    };

protected:
    uint8 nProveVersion;
    uint256 hashPrevProveBlock;

    std::vector<CBlockCoinTransferProve> vCoinTransferProve;
    std::map<CDexOrderHeader, CBlockDexOrderProve> mapDexOrderProve;
    std::set<uint256> setCrossConfirmRecvBlock;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockPrevProve
{
    friend class mtbase::CStream;

public:
    CBlockPrevProve() {}
    CBlockPrevProve(const uint256& hashPrevBlockIn, const mtbase::MERKLE_PROVE_DATA& vPrevBlockMerkleProveIn,
                    const CBlockCrosschainProve& proveCrosschainIn, const mtbase::MERKLE_PROVE_DATA& vCrosschainMerkleProveIn)
      : hashPrevBlock(hashPrevBlockIn), vPrevBlockMerkleProve(vPrevBlockMerkleProveIn),
        proveCrosschain(proveCrosschainIn), vCrosschainMerkleProve(vCrosschainMerkleProveIn) {}

public:
    uint256 hashPrevBlock;
    mtbase::MERKLE_PROVE_DATA vPrevBlockMerkleProve;

    CBlockCrosschainProve proveCrosschain;
    mtbase::MERKLE_PROVE_DATA vCrosschainMerkleProve;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockProve
{
    friend class mtbase::CStream;

public:
    CBlockProve() {}

    void Save(bytes& btData) const;
    bool Load(const bytes& btData);

    uint256 GetFirstPrevBlockHash() const;
    void GetBlockHashList(std::vector<uint256>& vBlockHash) const;
    bool IsCrossProveEmpty() const;

public:
    uint256 hashBlock;
    bytes btAggSigBitmap;
    bytes btAggSigData;

    uint256 hashRefBlock;
    mtbase::MERKLE_PROVE_DATA vRefBlockMerkleProve;

    uint256 hashPrevBlock;
    mtbase::MERKLE_PROVE_DATA vPrevBlockMerkleProve;

    CBlockCrosschainProve proveCrosschain;
    mtbase::MERKLE_PROVE_DATA vCrosschainMerkleProve;

    std::vector<CBlockPrevProve> vPrevBlockCcProve;

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockStorageProve
{
    friend class mtbase::CStream;

public:
    CBlockStorageProve() {}

    void SetNull()
    {
        btAggSigBitmap.clear();
        btAggSigData.clear();

        hashRefBlock = 0;
        vRefBlockMerkleProve.clear();

        hashPrevBlock = 0;
        vPrevBlockMerkleProve.clear();

        mapCrossProve.clear();
    }

    void Save(bytes& btData) const;
    bool Load(const bytes& btData);

public:
    bytes btAggSigBitmap;
    bytes btAggSigData;

    uint256 hashRefBlock;
    mtbase::MERKLE_PROVE_DATA vRefBlockMerkleProve;

    uint256 hashPrevBlock;
    mtbase::MERKLE_PROVE_DATA vPrevBlockMerkleProve;

    std::map<CChainId, std::pair<CBlockCrosschainProve, mtbase::MERKLE_PROVE_DATA>> mapCrossProve; // key is peer chainid

protected:
    void Serialize(mtbase::CStream& s, mtbase::SaveType&) const;
    void Serialize(mtbase::CStream& s, mtbase::LoadType&);
    void Serialize(mtbase::CStream& s, std::size_t& serSize) const;
};

class CBlockBroadcastProve
{
public:
    CBlockBroadcastProve() {}
    CBlockBroadcastProve(const uint256& hashBlockIn, const std::map<CChainId, CBlockProve>& mapBlockProveIn)
      : hashBlock(hashBlockIn), mapBlockProve(mapBlockProveIn) {}

public:
    uint256 hashBlock;
    std::map<CChainId, CBlockProve> mapBlockProve;
};

} // namespace metabasenet

#endif // COMMON_BLOCK_H

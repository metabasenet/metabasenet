// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_FORKCONTEXT_H
#define COMMON_FORKCONTEXT_H

#include <stream/stream.h>
#include <string>

#include "destination.h"
#include "profile.h"
#include "uint256.h"

namespace metabasenet
{

class CForkContext
{
    friend class mtbase::CStream;

public:
    uint8 nType;
    std::string strName;
    std::string strSymbol;
    CChainId nChainId;
    uint256 hashFork;
    uint256 hashParent;
    uint256 hashJoint; // fork orgin block is prev block
    uint256 txidEmbedded;
    uint16 nVersion;
    uint256 nAmount;
    uint256 nMintReward;
    uint256 nMinTxFee;
    uint32 nHalveCycle;
    int nJointHeight;
    CDestination destOwner;

public:
    CForkContext()
    {
        SetNull();
    }
    CForkContext(const uint256& hashForkIn, const uint256& hashJointIn, const uint256& txidEmbeddedIn, const CProfile& profile)
    {
        nType = profile.nType;
        hashFork = hashForkIn;
        hashJoint = hashJointIn;
        txidEmbedded = txidEmbeddedIn;
        strName = profile.strName;
        strSymbol = profile.strSymbol;
        nChainId = profile.nChainId;
        hashParent = profile.hashParent;
        nVersion = profile.nVersion;
        nAmount = profile.nAmount;
        nMintReward = profile.nMintReward;
        nMinTxFee = profile.nMinTxFee;
        nHalveCycle = profile.nHalveCycle;
        destOwner = profile.destOwner;
        nJointHeight = profile.nJointHeight;
    }
    virtual ~CForkContext() = default;
    virtual void SetNull()
    {
        nType = 0;
        nChainId = 0;
        hashFork = 0;
        hashParent = 0;
        hashJoint = 0;
        txidEmbedded = 0;
        nVersion = 1;
        nAmount = 0;
        nMintReward = 0;
        nMinTxFee = 0;
        nHalveCycle = 0;
        nJointHeight = -1;
        strName.clear();
        strSymbol.clear();
        destOwner.SetNull();
    }
    bool IsNull() const
    {
        return (nType == 0);
    }
    bool IsMainFork() const
    {
        return (nType == CProfile::PROFILE_FORK_TYPE_MAIN);
    }
    bool IsClonemapFork() const
    {
        return (nType == CProfile::PROFILE_FORK_TYPE_CLONEMAP);
    }
    bool IsUserFork() const
    {
        return (nType == CProfile::PROFILE_FORK_TYPE_USER);
    }
    const CProfile GetProfile() const
    {
        CProfile profile;
        profile.nType = nType;
        profile.strName = strName;
        profile.strSymbol = strSymbol;
        profile.nChainId = nChainId;
        profile.hashParent = hashParent;
        profile.nVersion = nVersion;
        profile.nAmount = nAmount;
        profile.nMintReward = nMintReward;
        profile.nMinTxFee = nMinTxFee;
        profile.nHalveCycle = nHalveCycle;
        profile.destOwner = destOwner;
        profile.nJointHeight = nJointHeight;
        return profile;
    }

    friend bool operator==(const CForkContext& a, const CForkContext& b)
    {
        return (a.nType == b.nType
                && a.strName == b.strName
                && a.strSymbol == b.strSymbol
                && a.nChainId == b.nChainId
                && a.hashFork == b.hashFork
                && a.hashParent == b.hashParent
                && a.hashJoint == b.hashJoint
                && a.txidEmbedded == b.txidEmbedded
                && a.nVersion == b.nVersion
                && a.nAmount == b.nAmount
                && a.nMintReward == b.nMintReward
                && a.nMinTxFee == b.nMinTxFee
                && a.nHalveCycle == b.nHalveCycle
                && a.nJointHeight == b.nJointHeight
                && a.destOwner == b.destOwner);
    }
    friend bool operator!=(const CForkContext& a, const CForkContext& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(nType, opt);
        s.Serialize(strName, opt);
        s.Serialize(strSymbol, opt);
        s.Serialize(nChainId, opt);
        s.Serialize(hashFork, opt);
        s.Serialize(hashParent, opt);
        s.Serialize(hashJoint, opt);
        s.Serialize(txidEmbedded, opt);
        s.Serialize(nVersion, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nMintReward, opt);
        s.Serialize(nMinTxFee, opt);
        s.Serialize(nHalveCycle, opt);
        s.Serialize(nJointHeight, opt);
        s.Serialize(destOwner, opt);
    }
};

class CValidForkId
{
    friend class mtbase::CStream;

public:
    CValidForkId() {}
    CValidForkId(const uint256& hashRefFdBlockIn, const std::map<uint256, int>& mapForkIdIn)
    {
        hashRefFdBlock = hashRefFdBlockIn;
        mapForkId.clear();
        mapForkId.insert(mapForkIdIn.begin(), mapForkIdIn.end());
    }
    void Clear()
    {
        hashRefFdBlock = 0;
        mapForkId.clear();
    }
    int GetCreatedHeight(const uint256& hashFork)
    {
        const auto it = mapForkId.find(hashFork);
        if (it != mapForkId.end())
        {
            return it->second;
        }
        return -1;
    }

public:
    uint256 hashRefFdBlock;
    std::map<uint256, int> mapForkId; // key: forkid, value: fork created height
                                      // When hashRefFdBlock == 0, it is the total quantity, otherwise it is the increment

protected:
    template <typename O>
    void Serialize(mtbase::CStream& s, O& opt)
    {
        s.Serialize(hashRefFdBlock, opt);
        s.Serialize(mapForkId, opt);
    }
};

} // namespace metabasenet

#endif //COMMON_FORKCONTEXT_H

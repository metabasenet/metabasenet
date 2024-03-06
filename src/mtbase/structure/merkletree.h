// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MTBASE_STRUCTURE_MERKLETREE_H
#define MTBASE_STRUCTURE_MERKLETREE_H

#include "crypto.h"

using namespace metabasenet::crypto;

namespace mtbase
{

using MERKLE_PROVE_DATA = std::vector<std::pair<uint8, uint256>>;
using SHP_MERKLE_PROVE_DATA = std::shared_ptr<MERKLE_PROVE_DATA>;
#define MAKE_SHARED_MERKLE_PROVE_DATA std::make_shared<MERKLE_PROVE_DATA>

class CMerkleTree
{
public:
    enum
    {
        PD_LEFT = 0,
        PD_RIGHT = 1
    };

    static uint256 BuildMerkleTree(std::vector<uint256>& vMerkleTree)
    {
        std::size_t nLayerHeadIndex = 0;
        for (std::size_t nSize = vMerkleTree.size(); nSize > 1; nSize = (nSize + 1) / 2)
        {
            for (std::size_t i = 0; i < nSize; i += 2)
            {
                std::size_t i2 = std::min(i + 1, nSize - 1);
                vMerkleTree.push_back(CryptoHash(vMerkleTree[nLayerHeadIndex + i], vMerkleTree[nLayerHeadIndex + i2]));
            }
            nLayerHeadIndex += nSize;
        }
        return (vMerkleTree.empty() ? uint64(0) : vMerkleTree.back());
    }
    static uint256 CalcMerkleTreeRoot(const std::vector<uint256>& vLeafHash)
    {
        std::vector<uint256> vMerkleTree = vLeafHash;
        return BuildMerkleTree(vMerkleTree);
    }
    static bool BuildMerkleProve(const std::size_t nLeafIndex, const std::vector<uint256>& vMerkleTree, const std::size_t nLeafCount, MERKLE_PROVE_DATA& vMerkleProve)
    {
        if (nLeafIndex >= nLeafCount)
        {
            return false;
        }
        std::size_t nLayerHeadIndex = 0;
        std::size_t nIndex = nLeafIndex;
        for (std::size_t nSize = nLeafCount; nSize > 1; nSize = (nSize + 1) / 2)
        {
            if (nLayerHeadIndex + nSize > vMerkleTree.size())
            {
                return false;
            }
            if ((nIndex % 2) == 0)
            {
                if (nIndex >= nSize - 1)
                {
                    vMerkleProve.push_back(std::make_pair(PD_RIGHT, vMerkleTree[nLayerHeadIndex + nIndex]));
                }
                else
                {
                    vMerkleProve.push_back(std::make_pair(PD_RIGHT, vMerkleTree[nLayerHeadIndex + nIndex + 1]));
                }
            }
            else
            {
                vMerkleProve.push_back(std::make_pair(PD_LEFT, vMerkleTree[nLayerHeadIndex + nIndex - 1]));
            }
            nIndex /= 2;
            nLayerHeadIndex += nSize;
        }
        return true;
    }
    static bool BuildMerkleProve(const uint256& hashLeaf, const std::vector<uint256>& vMerkleTree, const std::size_t nLeafCount, MERKLE_PROVE_DATA& vMerkleProve)
    {
        if (vMerkleTree.size() < nLeafCount)
        {
            return false;
        }
        std::size_t nLeafIndex = nLeafCount;
        for (std::size_t i = 0; i < nLeafCount; i++)
        {
            if (vMerkleTree[i] == hashLeaf)
            {
                nLeafIndex = i;
                break;
            }
        }
        if (nLeafIndex == nLeafCount)
        {
            return false;
        }
        return BuildMerkleProve(nLeafIndex, vMerkleTree, nLeafCount, vMerkleProve);
    }
    static uint256 GetMerkleRootByProve(const MERKLE_PROVE_DATA& vMerkleProve, const uint256& hashVerify)
    {
        uint256 hashNode = hashVerify;
        for (auto& vd : vMerkleProve)
        {
            if (vd.first == PD_LEFT)
            {
                hashNode = CryptoHash(vd.second, hashNode);
            }
            else
            {
                hashNode = CryptoHash(hashNode, vd.second);
            }
        }
        return hashNode;
    }
    static bool VerifyMerkleProve(const uint256& hashMerkleRoot, const MERKLE_PROVE_DATA& vMerkleProve, const uint256& hashVerify)
    {
        return (hashMerkleRoot == GetMerkleRootByProve(vMerkleProve, hashVerify));
    }
};

} // namespace mtbase

#endif // MTBASE_STRUCTURE_MERKLETREE_H

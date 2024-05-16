// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "structure/merkletree.h"

#include <boost/test/unit_test.hpp>
#include <sodium.h>

#include "crypto.h"
#include "test_big.h"
#include "util.h"

using namespace metabasenet;
using namespace metabasenet::crypto;
using namespace mtbase;
using namespace std;

BOOST_FIXTURE_TEST_SUITE(merkletree_tests, BasicUtfSetup)

//./build-release/test/test_big --log_level=all --run_test=merkletree_tests/merkletreetest

BOOST_AUTO_TEST_CASE(merkletreetest)
{
    auto funcBuildMerkle = [](const size_t nHashCount) {
        std::vector<uint256> vMerkleTree;
        for (size_t i = 0; i < nHashCount; i++)
        {
            uint256 h(i + 1);
            vMerkleTree.push_back(crypto::CryptoHash(h.begin(), h.size()));
        }
        uint256 hashMerkleRoot = CMerkleTree::BuildMerkleTree(vMerkleTree);
        BOOST_CHECK(hashMerkleRoot != 0);
        // for (auto& hash : vMerkleTree)
        // {
        //     std::cout << "tree: " << hash.ToString() << std::endl;
        // }
        // std::cout << "merkle root: " << hashMerkleRoot.ToString() << std::endl;
        auto funcVerifyProve = [&](const size_t nIndex) {
            //std::cout << "index: " << nIndex << ", hash: " << vMerkleTree[nIndex].ToString() << std::endl;
            std::vector<std::pair<uint8, uint256>> vMerkleProve;
            BOOST_CHECK(CMerkleTree::BuildMerkleProve(nIndex, vMerkleTree, nHashCount, vMerkleProve));
            // for (auto& vd : vMerkleProve)
            // {
            //     std::cout << "prove: " << (vd.first == CMerkleTree::PD_LEFT ? "left" : "right") << " : " << vd.second.ToString() << std::endl;
            // }
            BOOST_CHECK(CMerkleTree::VerifyMerkleProve(hashMerkleRoot, vMerkleProve, vMerkleTree[nIndex]));
        };
        for (size_t i = 0; i < nHashCount; i++)
        {
            funcVerifyProve(i);
        }
    };
    for (size_t nHashCount = 1; nHashCount <= 12; nHashCount++)
    {
        funcBuildMerkle(nHashCount);
    }
}

BOOST_AUTO_TEST_SUITE_END()

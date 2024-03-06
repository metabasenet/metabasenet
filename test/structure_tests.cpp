// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <set>
#include <vector>

#include "destination.h"
#include "structure/tree.h"
#include "test_big.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet;

BOOST_FIXTURE_TEST_SUITE(structure_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(tree)
{

    // sort
    // a21:     1782a5jv06egd6pb2gjgp2p664tgej6n4gmj79e1hbvgfgy3t006wvwzt
    // b2:      1w0k188jwq5aenm6sfj6fdx9f3d96k20k71612ddz81qrx6syr4bnp5m9
    // a111:    1bvaag2t23ybjmasvjyxnzetja0awe5d1pyw361ea3jmkfdqt5greqvfq
    // B:       1fpt2z9nyh0a5999zrmabg6ppsbx78wypqapm29fsasx993z11crp6zm7
    // a2:      1ab1sjh07cz7xpb0xdkpwykfm2v91cvf2j1fza0gj07d2jktdnrwtyd57
    // b1:      1rampdvtmzmxfzr3crbzyz265hbr9a8y4660zgpbw6r7qt9hdy535zed7
    // b3:      196wx05mee1zavws828vfcap72tebtskw094tp5sztymcy30y7n9varfa
    // A:       1632srrskscs1d809y3x5ttf50f0gabf86xjz2s6aetc9h9ewwhm58dj3
    // a221:    1ytysehvcj13ka9r1qhh9gken1evjdp9cn00cz0bhqjqgzwc6sdmse548
    // a22:     1c7s095dcvzdsedrkpj6y5kjysv5sz3083xkahvyk7ry3tag4ddyydbv4
    // a222:    16aaahq32cncamxbmvedjy5jqccc2hcpk7rc0h2zqcmmrnm1g2213t2k3
    // a3:      1vz7k0748t840bg3wgem4mhf9pyvptf1z2zqht6bc2g9yn6cmke8h4dwe
    // C:       1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm
    // a11:     1pmj5p47zhqepwa9vfezkecxkerckhrks31pan5fh24vs78s6cbkrqnxp
    // b4:      19k8zjwdntjp8avk41c8aek0jxrs1fgyad5q9f1gd1q2fdd0hafmm549d
    // a1:      1f1nj5gjgrcz45g317s1y4tk18bbm89jdtzd41m9s0t14tp2ngkz4cg0x
    CDestination A("1632srrskscs1d809y3x5ttf50f0gabf86xjz2s6aetc9h9ewwhm58dj3");
    CDestination a1("1f1nj5gjgrcz45g317s1y4tk18bbm89jdtzd41m9s0t14tp2ngkz4cg0x");
    CDestination a11("1pmj5p47zhqepwa9vfezkecxkerckhrks31pan5fh24vs78s6cbkrqnxp");
    CDestination a111("1bvaag2t23ybjmasvjyxnzetja0awe5d1pyw361ea3jmkfdqt5greqvfq");
    CDestination a2("1ab1sjh07cz7xpb0xdkpwykfm2v91cvf2j1fza0gj07d2jktdnrwtyd57");
    CDestination a21("1782a5jv06egd6pb2gjgp2p664tgej6n4gmj79e1hbvgfgy3t006wvwzt");
    CDestination a22("1c7s095dcvzdsedrkpj6y5kjysv5sz3083xkahvyk7ry3tag4ddyydbv4");
    CDestination a221("1ytysehvcj13ka9r1qhh9gken1evjdp9cn00cz0bhqjqgzwc6sdmse548");
    CDestination a222("16aaahq32cncamxbmvedjy5jqccc2hcpk7rc0h2zqcmmrnm1g2213t2k3");
    CDestination a3("1vz7k0748t840bg3wgem4mhf9pyvptf1z2zqht6bc2g9yn6cmke8h4dwe");
    CDestination B("1fpt2z9nyh0a5999zrmabg6ppsbx78wypqapm29fsasx993z11crp6zm7");
    CDestination b1("1rampdvtmzmxfzr3crbzyz265hbr9a8y4660zgpbw6r7qt9hdy535zed7");
    CDestination b2("1w0k188jwq5aenm6sfj6fdx9f3d96k20k71612ddz81qrx6syr4bnp5m9");
    CDestination b3("196wx05mee1zavws828vfcap72tebtskw094tp5sztymcy30y7n9varfa");
    CDestination b4("19k8zjwdntjp8avk41c8aek0jxrs1fgyad5q9f1gd1q2fdd0hafmm549d");
    CDestination C("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

    CForest<CDestination, CDestination> relation;
    BOOST_CHECK(relation.Insert(a1, A, A));
    BOOST_CHECK(relation.mapRoot.size() == 1
                && (relation.mapRoot.begin()->second->spRoot->key == A));
    BOOST_CHECK(relation.Insert(a11, a1, a1));
    BOOST_CHECK(relation.Insert(a111, a11, a11));
    BOOST_CHECK(relation.Insert(a221, a22, a22));
    BOOST_CHECK(relation.mapRoot.size() == 2
                && (relation.mapRoot.begin()->second->spRoot->key == A)
                && ((++relation.mapRoot.begin())->second->spRoot->key == a22));
    BOOST_CHECK(relation.Insert(a222, a22, a22));
    BOOST_CHECK(relation.Insert(a21, a2, a2));
    BOOST_CHECK(relation.mapRoot.size() == 3
                && (relation.mapRoot.begin()->second->spRoot->key == a2)
                && ((++relation.mapRoot.begin())->second->spRoot->key == A)
                && (relation.mapRoot.rbegin()->second->spRoot->key == a22));
    BOOST_CHECK(relation.Insert(a22, a2, a2));
    BOOST_CHECK(relation.mapRoot.size() == 2
                && (relation.mapRoot.begin()->second->spRoot->key == a2)
                && (relation.mapRoot.rbegin()->second->spRoot->key == A));
    BOOST_CHECK(relation.Insert(a2, A, A));
    BOOST_CHECK(relation.mapRoot.size() == 1
                && (relation.mapRoot.begin()->second->spRoot->key == A));
    BOOST_CHECK(relation.Insert(a3, A, A));
    BOOST_CHECK(relation.Insert(b1, B, B));
    BOOST_CHECK(relation.mapRoot.size() == 2
                && (relation.mapRoot.begin()->second->spRoot->key == B)
                && (relation.mapRoot.rbegin()->second->spRoot->key == A));
    BOOST_CHECK(relation.Insert(b2, B, B));
    BOOST_CHECK(relation.Insert(b3, B, B));
    BOOST_CHECK(relation.Insert(b4, B, B));

    BOOST_CHECK(relation.mapNode.size() == 15);

    // test remove
    relation.RemoveRelation(a2);
    BOOST_CHECK(relation.mapNode.size() == 15);
    BOOST_CHECK(relation.mapRoot.size() == 3
                && (relation.mapRoot.begin()->second->spRoot->key == B)
                && ((++relation.mapRoot.begin())->second->spRoot->key == a2)
                && (relation.mapRoot.rbegin()->second->spRoot->key == A));

    // test post-order traversal
    {
        vector<CDestination> v;
        set<decltype(relation)::NodePtr> s;
        // B
        s.insert(relation.GetRelation(b1));
        s.insert(relation.GetRelation(b2));
        s.insert(relation.GetRelation(b3));
        s.insert(relation.GetRelation(b4));
        for (auto& x : s)
        {
            v.push_back(x->key);
        }
        v.push_back(B);

        // a2
        s.clear();
        s.insert(relation.GetRelation(a21));
        s.insert(relation.GetRelation(a22));
        for (auto& x : s)
        {
            if (x->key == a21)
            {
                v.push_back(a21);
            }
            else if (x->key == a22)
            {
                if (relation.GetRelation(a221) < relation.GetRelation(a222))
                {
                    v.push_back(a221);
                    v.push_back(a222);
                }
                else
                {
                    v.push_back(a222);
                    v.push_back(a221);
                }
                v.push_back(a22);
            }
        }
        v.push_back(a2);

        // A
        s.clear();
        s.insert(relation.GetRelation(a1));
        s.insert(relation.GetRelation(a2));
        s.insert(relation.GetRelation(a3));
        for (auto& x : s)
        {
            if (x->key == a1)
            {
                v.push_back(a111);
                v.push_back(a11);
                v.push_back(a1);
            }
            else if (x->key == a3)
            {
                v.push_back(a3);
            }
        }
        v.push_back(A);

        BOOST_CHECK(v.size() == 15);

        int index = 0;
        relation.PostorderTraversal([&](decltype(relation)::NodePtr pNode) {
            BOOST_CHECK(pNode->key == v[index]);
            return pNode->key == v[index++];
        });
    }

    // test copy
    CForest<CDestination, CDestination> relation2;
    relation2 = relation.Copy<CDestination>();
    BOOST_CHECK(relation2.mapNode.size() == 15);
    BOOST_CHECK(relation2.mapRoot.size() == 3
                && (relation2.mapRoot.begin()->second->spRoot->key == B)
                && ((++relation2.mapRoot.begin())->second->spRoot->key == a2)
                && (relation2.mapRoot.rbegin()->second->spRoot->key == A));

    BOOST_CHECK(!relation2.GetRelation(A)->spParent.lock());
    BOOST_CHECK(relation2.GetRelation(a1)->spParent.lock()->key == A);
    BOOST_CHECK(relation2.GetRelation(a3)->spParent.lock()->key == A);
    BOOST_CHECK(relation2.GetRelation(a11)->spParent.lock()->key == a1);
    BOOST_CHECK(relation2.GetRelation(a111)->spParent.lock()->key == a11);
    BOOST_CHECK(!relation2.GetRelation(a2)->spParent.lock());
    BOOST_CHECK(relation2.GetRelation(a21)->spParent.lock()->key == a2);
    BOOST_CHECK(relation2.GetRelation(a22)->spParent.lock()->key == a2);
    BOOST_CHECK(relation2.GetRelation(a221)->spParent.lock()->key == a22);
    BOOST_CHECK(relation2.GetRelation(a222)->spParent.lock()->key == a22);
    BOOST_CHECK(!relation2.GetRelation(B)->spParent.lock());
    BOOST_CHECK(relation2.GetRelation(b1)->spParent.lock()->key == B);
    BOOST_CHECK(relation2.GetRelation(b2)->spParent.lock()->key == B);
    BOOST_CHECK(relation2.GetRelation(b3)->spParent.lock()->key == B);
    BOOST_CHECK(relation2.GetRelation(b4)->spParent.lock()->key == B);
}

BOOST_AUTO_TEST_SUITE_END()
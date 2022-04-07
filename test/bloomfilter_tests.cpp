// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloomfilter/bloomfilter.h"

#include <boost/test/unit_test.hpp>

#include "test_big.h"

using namespace std;
using namespace hnbase;

BOOST_FIXTURE_TEST_SUITE(bloomfilter_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(basetest)
{
    CBloomFilter<256> bf;
    char buf[] = "1234567";
    bf.Add((unsigned char*)buf, strlen(buf));
    BOOST_CHECK(bf.Test((unsigned char*)buf, strlen(buf)));
    buf[0] = 'a';
    BOOST_CHECK(!bf.Test((unsigned char*)buf, strlen(buf)));

    CBloomFilter<256> nf;
    nf.Add((unsigned char*)buf, strlen(buf));
    BOOST_CHECK(nf.Test((unsigned char*)buf, strlen(buf)));
    std::vector<unsigned char> vBfData;
    nf.GetData(vBfData);
    CBloomFilter<256> gf;
    gf.SetData(vBfData);
    BOOST_CHECK(gf.Test((unsigned char*)buf, strlen(buf)));
}

BOOST_AUTO_TEST_CASE(stresstest)
{
    CBloomFilter<0x1000000> bf;
    const int64_t nTestCount = 1000000;
    for (int64_t i = 0; i < nTestCount; i++)
    {
        bf.Add((unsigned char*)&i, sizeof(i));
    }
    int64_t nErrCount = 0;
    for (int64_t i = 0; i < nTestCount; i++)
    {
        if (!bf.Test((unsigned char*)&i, sizeof(i)))
        {
            nErrCount++;
        }
    }
    BOOST_CHECK(nErrCount == 0);
    printf("nErrCount=%ld\n", nErrCount);
    nErrCount = 0;
    for (int64_t i = nTestCount; i < nTestCount * 2; i++)
    {
        if (bf.Test((unsigned char*)&i, sizeof(i)))
        {
            nErrCount++;
        }
    }
    printf("nErrCount=%ld\n", nErrCount);
}

BOOST_AUTO_TEST_SUITE_END()

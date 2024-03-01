// Copyright (c) 2021-2023 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloomfilter/bloomfilter.h"

#include <boost/test/unit_test.hpp>

#include "crypto.h"
#include "test_big.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet::crypto;

//./build-release/test/test_big --log_level=all --run_test=bloomfilter_tests/basetest
//./build-release/test/test_big --log_level=all --run_test=bloomfilter_tests/stresstest
//./build-release/test/test_big --log_level=all --run_test=bloomfilter_tests/stresstestbytes

BOOST_FIXTURE_TEST_SUITE(bloomfilter_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(basetest)
{
    CNhBloomFilter bf(256);
    char buf[] = "1234567";
    bf.Add((unsigned char*)buf, strlen(buf));
    BOOST_CHECK(bf.Test((unsigned char*)buf, strlen(buf)));
    buf[0] = 'a';
    BOOST_CHECK(!bf.Test((unsigned char*)buf, strlen(buf)));

    CNhBloomFilter nf(256);
    nf.Add((unsigned char*)buf, strlen(buf));
    BOOST_CHECK(nf.Test((unsigned char*)buf, strlen(buf)));
    std::vector<unsigned char> vBfData;
    nf.GetData(vBfData);
    CNhBloomFilter gf(256);
    gf.SetData(vBfData);
    BOOST_CHECK(gf.Test((unsigned char*)buf, strlen(buf)));
}

BOOST_AUTO_TEST_CASE(stresstest)
{
    const int64_t nTestCount = 1000000;
    CNhBloomFilter bf(nTestCount * 4);
    for (int64_t i = 0; i < nTestCount; i++)
    {
        uint256 hash = CryptoHash(&i, sizeof(i));
        bf.Add((unsigned char*)hash.begin(), hash.size());
    }
    int64_t nErrCount = 0;
    for (int64_t i = 0; i < nTestCount; i++)
    {
        uint256 hash = CryptoHash(&i, sizeof(i));
        if (!bf.Test((unsigned char*)hash.begin(), hash.size()))
        {
            nErrCount++;
        }
    }
    BOOST_CHECK(nErrCount == 0);
    printf("nErrCount=%ld\n", nErrCount);
    nErrCount = 0;
    for (int64_t i = nTestCount; i < nTestCount * 2; i++)
    {
        uint256 hash = CryptoHash(&i, sizeof(i));
        if (bf.Test((unsigned char*)hash.begin(), hash.size()))
        {
            nErrCount++;
        }
    }
    printf("nErrCount=%ld\n", nErrCount);
}

BOOST_AUTO_TEST_CASE(stresstestbytes)
{
    const int64_t nTestCount = 1000000;
    bytes btBmData;

    {
        CNhBloomFilter bf(nTestCount * 4);
        for (int64_t i = 0; i < nTestCount; i++)
        {
            uint256 hash = CryptoHash(&i, sizeof(i));
            bf.Add((unsigned char*)hash.begin(), hash.size());
        }
        bf.GetData(btBmData);
    }

    {
        CNhBloomFilter bf(btBmData);
        int64_t nErrCount = 0;
        for (int64_t i = 0; i < nTestCount; i++)
        {
            uint256 hash = CryptoHash(&i, sizeof(i));
            if (!bf.Test((unsigned char*)hash.begin(), hash.size()))
            {
                nErrCount++;
            }
        }
        BOOST_CHECK(nErrCount == 0);
        printf("nErrCount=%ld\n", nErrCount);
    }

    {
        CNhBloomFilter bf(btBmData);
        int64_t nErrCount = 0;
        for (int64_t i = nTestCount; i < nTestCount * 2; i++)
        {
            uint256 hash = CryptoHash(&i, sizeof(i));
            if (bf.Test((unsigned char*)hash.begin(), hash.size()))
            {
                nErrCount++;
            }
        }
        printf("nErrCount=%ld\n", nErrCount);
    }
}

BOOST_AUTO_TEST_SUITE_END()

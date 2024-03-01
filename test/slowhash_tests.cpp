// Copyright (c) 2021-2023 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <map>
#include <string>

#include "crypto.h"
#include "test_big.h"
#include "uint256.h"
#include "util.h"

using namespace mtbase;
using namespace metabasenet;
using namespace metabasenet::crypto;

BOOST_FIXTURE_TEST_SUITE(slowhash_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(slowhash)
{
#if !defined NO_AES && (defined(__x86_64__) || (defined(_MSC_VER) && defined(_WIN64)))
    //x86/x64 with optimised sse2 instructions
    BOOST_TEST_MESSAGE("Hashing with sse2 native instructions running on X86");

#elif !defined NO_AES && (defined(__arm__) || defined(__aarch64__))
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRYPTO)
    //arm64 with optimised neon instructions
    BOOST_TEST_MESSAGE("Hashing with neon native instructions running on ARM64 WITH CRYPTO FEATURE");

#else
    //arm64 without optimised neon instructions
    BOOST_TEST_MESSAGE("Hashing without neon native instructions running on ARM64 WITHOUT CRYPTO FEATURE");

#endif
#else
    //fallback implementation with any optimization
    BOOST_TEST_MESSAGE("Hashing with FALLBACK IMPLEMENTATION");

#endif
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2021-2023 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto.h"

#include <boost/test/unit_test.hpp>
#include <sodium.h>

#include "crypto.h"
#include "curve25519/curve25519.h"
#include "test_big.h"
#include "util.h"

using namespace metabasenet;
using namespace metabasenet::crypto;
using namespace curve25519;
using namespace mtbase;
using namespace std;

BOOST_FIXTURE_TEST_SUITE(crypto_tests, BasicUtfSetup)

//./build-release/test/test_big --log_level=all --run_test=crypto_tests/makekey

BOOST_AUTO_TEST_CASE(makekey)
{
    CCryptoKey key;
    CryptoMakeNewKey(key);
    std::cout << "secret: " << key.secret.GetHex() << ", pubkey: " << key.pubkey.GetHex() << std::endl;

    CCryptoKey key2;
    uint512 pk = CryptoImportKey(key2, key.secret);
    std::cout << "secret: " << key2.secret.GetHex() << ", pubkey: " << key2.pubkey.GetHex() << ", pk: " << pk.GetHex() << std::endl;

    uint256 hash = CryptoHash(key2.secret.begin(), key2.secret.size());
    printf("hash: %s\n", hash.GetHex().c_str());

    bytes btSigData;
    CryptoSign(key, hash, btSigData);
    std::cout << "sig data: " << ToHexString(btSigData) << std::endl;

    if (CryptoVerify(key.pubkey, hash, btSigData))
    {
        printf("CryptoVerify OK\n");
    }
    else
    {
        printf("CryptoVerify fail\n");
    }
}

// BOOST_AUTO_TEST_CASE(multisign)
// {
//     srand(time(0));

//     int64_t signCount = 0, signTime = 0, verifyTime = 0;
//     int count = 5;
//     for (int i = 0; i < count; i++)
//     {
//         uint32_t nKey = CryptoGetRand32() % 256 + 1;
//         uint32_t nPartKey = CryptoGetRand32() % nKey + 1;

//         CCryptoKey* keys = new CCryptoKey[nKey];
//         std::set<uint256> setPubKey;
//         for (int i = 0; i < nKey; i++)
//         {
//             CryptoMakeNewKey(keys[i]);
//             setPubKey.insert(keys[i].pubkey);
//         }

//         uint256 msg;
//         CryptoGetRand256(msg);

//         std::vector<uint8_t> vchSig;
//         for (int i = 0; i < nPartKey; i++)
//         {
//             boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::universal_time();
//             BOOST_CHECK(CryptoMultiSign(setPubKey, keys[i], msg.begin(), msg.size(), vchSig));
//             boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
//             signTime += (t1 - t0).ticks();
//             ++signCount;
//         }

//         std::set<uint256> setPartKey;
//         boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::universal_time();
//         BOOST_CHECK(CryptoMultiVerify(setPubKey, msg.begin(), msg.size(), vchSig, setPartKey) && (setPartKey.size() == nPartKey));
//         boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
//         verifyTime += (t1 - t0).ticks();
//     }

//     std::cout << "multisign count : " << count << ", key count: " << signCount << std::endl;
//     std::cout << "multisign sign count : " << signCount << "; average time : " << signTime / signCount << "us." << std::endl;
//     std::cout << "multisign verify count : " << count << "; time per count : " << verifyTime / count << "us.; time per key: " << verifyTime / signCount << "us." << std::endl;
// }

// BOOST_AUTO_TEST_CASE(multisign_defect)
// {
//     srand(time(0));

//     int64_t signCount = 0, signTime1 = 0, verifyTime1 = 0, signTime2 = 0, verifyTime2 = 0;
//     int count = 5;
//     for (int i = 0; i < count; i++)
//     {
//         uint32_t nKey = CryptoGetRand32() % 256 + 1;
//         uint32_t nPartKey = CryptoGetRand32() % nKey + 1;

//         CCryptoKey* keys = new CCryptoKey[nKey];
//         std::set<uint256> setPubKey;
//         for (int i = 0; i < nKey; i++)
//         {
//             CryptoMakeNewKey(keys[i]);
//             setPubKey.insert(keys[i].pubkey);
//         }

//         uint256 msg;
//         CryptoGetRand256(msg);
//         uint256 seed;
//         CryptoGetRand256(seed);

//         std::vector<uint8_t> vchSig;
//         for (int i = 0; i < nPartKey; i++)
//         {
//             boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::universal_time();
//             BOOST_CHECK(CryptoMultiSignDefect(setPubKey, keys[i], seed.begin(), seed.size(), msg.begin(), msg.size(), vchSig));
//             boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
//             signTime1 += (t1 - t0).ticks();
//             ++signCount;
//         }

//         {
//             boost::posix_time::ptime t0 = boost::posix_time::microsec_clock::universal_time();
//             std::set<uint256> setPartKey;
//             BOOST_CHECK(CryptoMultiVerifyDefect(setPubKey, seed.begin(), seed.size(), msg.begin(), msg.size(), vchSig, setPartKey) && (setPartKey.size() == nPartKey));
//             boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::universal_time();
//             verifyTime1 += (t1 - t0).ticks();
//         }
//     }

//     std::cout << "multisign count : " << count << ", key count: " << signCount << std::endl;
//     std::cout << "multisign sign1 count : " << signCount << "; average time : " << signTime1 / signCount << "us." << std::endl;
//     std::cout << "multisign verify1 count : " << count << "; time per count : " << verifyTime1 / count << "us.; time per key: " << verifyTime1 / signCount << "us." << std::endl;
//     std::cout << "multisign sign2 count : " << signCount << "; average time : " << signTime2 / signCount << "us." << std::endl;
//     std::cout << "multisign verify2 count : " << count << "; time per count : " << verifyTime2 / count << "us.; time per key: " << verifyTime2 / signCount << "us." << std::endl;
// }

BOOST_AUTO_TEST_SUITE_END()

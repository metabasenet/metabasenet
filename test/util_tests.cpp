// Copyright (c) 2021-2023 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

#include <boost/test/unit_test.hpp>

#include "forkcontext.h"
#include "param.h"
#include "profile.h"
#include "test_big.h"

using namespace mtbase;
using namespace metabasenet;

BOOST_FIXTURE_TEST_SUITE(util_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(util)
{
    double a = -0.1, b = 0.1;
    BOOST_CHECK(!IsDoubleEqual(a, b));

    a = 1.23456, b = 1.23455;
    BOOST_CHECK(!IsDoubleEqual(a, b));

    a = -1, b = -1.0;
    BOOST_CHECK(IsDoubleEqual(a, b));

    a = -1, b = -1;
    BOOST_CHECK(IsDoubleEqual(a, b));

    a = 0.1234567, b = 0.1234567;
    BOOST_CHECK(IsDoubleEqual(a, b));
}

BOOST_AUTO_TEST_CASE(common_profile)
{
    CProfile profile;
    profile.strName = "NHS Test";
    profile.strSymbol = "NHSA";
    profile.nChainId = 1;
    profile.nVersion = 1;
    profile.nMinTxFee = 100;
    profile.nMintReward = 1000;
    profile.nAmount = 100000;

    std::vector<uint8> vchProfile;
    BOOST_CHECK(profile.Save(vchProfile));

    CProfile profileLoad;
    try
    {
        BOOST_CHECK(profileLoad.Load(vchProfile));
    }
    catch (const std::exception& e)
    {
        BOOST_FAIL(e.what());
    }

    BOOST_CHECK(profileLoad.strName == profile.strName);
    BOOST_CHECK(profileLoad.strSymbol == profile.strSymbol);
    BOOST_CHECK(profileLoad.nChainId == profile.nChainId);
    BOOST_CHECK(profileLoad.nVersion == profile.nVersion);
    BOOST_CHECK(profileLoad.nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(profileLoad.nMintReward == profile.nMintReward);
    BOOST_CHECK(profileLoad.nAmount == profile.nAmount);

    CForkContext forkContextWrite(uint256(), uint256(), uint256(), profile);
    CBufStream ss;
    ss << forkContextWrite;
    CForkContext forkContextRead;
    ss >> forkContextRead;

    BOOST_CHECK(forkContextRead.GetProfile().strName == profile.strName);
    BOOST_CHECK(forkContextRead.GetProfile().strSymbol == profile.strSymbol);
    BOOST_CHECK(forkContextRead.GetProfile().nVersion == profile.nVersion);
    BOOST_CHECK(forkContextRead.GetProfile().nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(forkContextRead.GetProfile().nMintReward == profile.nMintReward);
    BOOST_CHECK(forkContextRead.GetProfile().nAmount == profile.nAmount);
}

BOOST_AUTO_TEST_CASE(token_big_float_test)
{
    uint256 n;

    BOOST_CHECK(!TokenBigFloatToCoin(std::string("   0012300.0.076000"), n));
    BOOST_CHECK(!TokenBigFloatToCoin(std::string("  h 0012300.0.076000"), n));
    BOOST_CHECK(!TokenBigFloatToCoin(std::string("001@3w23000.076000"), n));
    BOOST_CHECK(!TokenBigFloatToCoin(std::string("0012300.0 076 000 "), n));
    BOOST_CHECK(TokenBigFloatToCoin(std::string("   0012300.0076000 "), n));
    BOOST_CHECK(TokenBigFloatToCoin(std::string("100000000"), n));
    BOOST_CHECK(TokenBigFloatToCoin(std::string("0000100000000"), n));
    BOOST_CHECK(TokenBigFloatToCoin(std::string(".000001"), n));
    BOOST_CHECK(TokenBigFloatToCoin(std::string("98776."), n));

    BOOST_CHECK(TokenBigFloatToCoin(std::string(""), n));
    BOOST_CHECK(n.Get64() == 0);

    BOOST_CHECK(TokenBigFloatToCoin(std::string("   "), n));
    BOOST_CHECK(n.Get64() == 0);

    BOOST_CHECK(TokenBigFloatToCoin(std::string("."), n));
    BOOST_CHECK(n.Get64() == 0);

    BOOST_CHECK(TokenBigFloatToCoin(std::string("00012300.0076000"), n));
    BOOST_CHECK(CoinToTokenBigFloat(n) == std::string("12300.0076"));

    BOOST_CHECK(TokenBigFloatToCoin(std::string("12300.0098076"), n));
    BOOST_CHECK(CoinToTokenBigFloat(n) == std::string("12300.0098076"));

    BOOST_CHECK(TokenBigFloatToCoin(std::string("123456789012345678901234567890.1234567891"), n));
    BOOST_CHECK(CoinToTokenBigFloat(n) == std::string("123456789012345678901234567890.1234567891"));

    BOOST_CHECK(TokenBigFloatToCoin(std::string("123.023000"), n));
    BOOST_CHECK(CoinToTokenBigFloat(n) == std::string("123.023"));
    std::string str1 = CoinToTokenBigFloat(n);
    std::cout << "n: " << str1 << std::endl;

    BOOST_CHECK(TokenBigFloatToCoin(std::string("123"), n));
    str1 = CoinToTokenBigFloat(n);
    std::cout << "n: " << str1 << std::endl;
}

BOOST_AUTO_TEST_CASE(reverse_hex_string_test)
{
    BOOST_CHECK(ReverseHexString("0x11223344") == std::string("0x44332211"));
    BOOST_CHECK(ReverseHexString("0x1122334") == std::string(""));

    BOOST_CHECK(ReverseHexNumericString("0x9") == std::string("0x09"));
    BOOST_CHECK(ReverseHexNumericString("0x1234") == std::string("0x3412"));
    BOOST_CHECK(ReverseHexNumericString("0x123") == std::string("0x2301"));
}

BOOST_AUTO_TEST_SUITE_END()

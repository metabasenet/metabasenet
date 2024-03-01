// Copyright (c) 2021-2023 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uint256.h"

#include <boost/test/unit_test.hpp>

#include "crypto.h"
#include "test_big.h"
#include "util.h"
//#include <stdint.h>
//#include <sstream>
//#include <iomanip>
//#include <limits>
//#include <cmath>
//#include <string>
//#include <stdio.h>

using namespace mtbase;
using namespace metabasenet::crypto;

BOOST_FIXTURE_TEST_SUITE(uint256_tests, BasicUtfSetup)

const unsigned char R1Array[] = "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
                                "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
uint256 R1L = uint256(std::vector<unsigned char>(R1Array, R1Array + 32));
uint160 R1S = uint160(std::vector<unsigned char>(R1Array, R1Array + 20));

const unsigned char R2Array[] = "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
                                "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
uint256 R2L = uint256(std::vector<unsigned char>(R2Array, R2Array + 32));
uint160 R2S = uint160(std::vector<unsigned char>(R2Array, R2Array + 20));

const unsigned char ZeroArray[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
uint256 ZeroL = uint256(std::vector<unsigned char>(ZeroArray, ZeroArray + 32));
uint160 ZeroS = uint160(std::vector<unsigned char>(ZeroArray, ZeroArray + 20));

const unsigned char OneArray[] = "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                                 "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
uint256 OneL = uint256(std::vector<unsigned char>(OneArray, OneArray + 32));
uint160 OneS = uint160(std::vector<unsigned char>(OneArray, OneArray + 20));

const unsigned char MaxArray[] = "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
                                 "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
uint256 MaxL = uint256(std::vector<unsigned char>(MaxArray, MaxArray + 32));
uint160 MaxS = uint160(std::vector<unsigned char>(MaxArray, MaxArray + 20));

std::string ArrayToString(const unsigned char A[], unsigned int width)
{
    std::stringstream Stream;
    Stream << std::hex;
    for (unsigned int i = 0; i < width; ++i)
    {
        Stream << std::setw(2) << std::setfill('0') << (unsigned int)A[width - i - 1];
    }
    return Stream.str();
}

inline uint160 uint160S(const char* str)
{
    uint160 rv;
    rv.SetHex(str);
    return rv;
}
inline uint160 uint160S(const std::string& str)
{
    uint160 rv;
    rv.SetHex(str);
    return rv;
}

BOOST_AUTO_TEST_CASE(basics) // constructors, equality, inequality
{
    BOOST_CHECK(1 == 0 + 1);
    // constructor uint256(vector<char>):
    BOOST_CHECK(R1L.ToString() == ArrayToString(R1Array, 32));
    BOOST_CHECK(R1S.ToString() == ArrayToString(R1Array, 20));
    BOOST_CHECK(R2L.ToString() == ArrayToString(R2Array, 32));
    BOOST_CHECK(R2S.ToString() == ArrayToString(R2Array, 20));
    BOOST_CHECK(ZeroL.ToString() == ArrayToString(ZeroArray, 32));
    BOOST_CHECK(ZeroS.ToString() == ArrayToString(ZeroArray, 20));
    BOOST_CHECK(OneL.ToString() == ArrayToString(OneArray, 32));
    BOOST_CHECK(OneS.ToString() == ArrayToString(OneArray, 20));
    BOOST_CHECK(MaxL.ToString() == ArrayToString(MaxArray, 32));
    BOOST_CHECK(MaxS.ToString() == ArrayToString(MaxArray, 20));
    BOOST_CHECK(OneL.ToString() != ArrayToString(ZeroArray, 32));
    BOOST_CHECK(OneS.ToString() != ArrayToString(ZeroArray, 20));

    // == and !=
    BOOST_CHECK(R1L != R2L && R1S != R2S);
    BOOST_CHECK(ZeroL != OneL && ZeroS != OneS);
    BOOST_CHECK(OneL != ZeroL && OneS != ZeroS);
    BOOST_CHECK(MaxL != ZeroL && MaxS != ZeroS);

    // String Constructor and Copy Constructor
    BOOST_CHECK(uint256("0x" + R1L.ToString()) == R1L);
    BOOST_CHECK(uint256("0x" + R2L.ToString()) == R2L);
    BOOST_CHECK(uint256("0x" + ZeroL.ToString()) == ZeroL);
    BOOST_CHECK(uint256("0x" + OneL.ToString()) == OneL);
    BOOST_CHECK(uint256("0x" + MaxL.ToString()) == MaxL);
    BOOST_CHECK(uint256(R1L.ToString()) == R1L);
    BOOST_CHECK(uint256("   0x" + R1L.ToString() + "   ") == R1L);
    BOOST_CHECK(uint256("") == ZeroL);
    BOOST_CHECK(R1L == uint256(R1ArrayHex));
    BOOST_CHECK(uint256(R1L) == R1L);
    BOOST_CHECK(uint256(ZeroL) == ZeroL);
    BOOST_CHECK(uint256(OneL) == OneL);

    BOOST_CHECK(uint160S("0x" + R1S.ToString()) == R1S);
    BOOST_CHECK(uint160S("0x" + R2S.ToString()) == R2S);
    BOOST_CHECK(uint160S("0x" + ZeroS.ToString()) == ZeroS);
    BOOST_CHECK(uint160S("0x" + OneS.ToString()) == OneS);
    BOOST_CHECK(uint160S("0x" + MaxS.ToString()) == MaxS);
    BOOST_CHECK(uint160S(R1S.ToString()) == R1S);
    BOOST_CHECK(uint160S("   0x" + R1S.ToString() + "   ") == R1S);
    BOOST_CHECK(uint160S("") == ZeroS);
    BOOST_CHECK(R1S == uint160S(R1ArrayHex));

    BOOST_CHECK(uint160(R1S) == R1S);
    BOOST_CHECK(uint160(ZeroS) == ZeroS);
    BOOST_CHECK(uint160(OneS) == OneS);

    uint256 a;
    a.SetHex("1234567890123456789012345678901234567890123456789012345678901234");
    uint256 b = 2;
    uint256 c = a * b;
    BOOST_CHECK((a * b) == (a + a));
    BOOST_CHECK((a * 2) == (a + a));
    BOOST_CHECK((c / b) == a);
    BOOST_CHECK((c / 2) == a);

    uint256 a1 = 3;
    uint256 b1 = 2;
    uint256 c1 = a1 / b1;
    BOOST_CHECK(c1 == 1);
}

BOOST_AUTO_TEST_CASE(comparison) // <= >= < >
{
    uint256 LastL;
    for (int i = 255; i >= 255; --i) //i >= 0
    {
        uint256 TmpL;
        *(TmpL.begin() + (i >> 3)) |= 1 << (7 - (i & 7));
        BOOST_CHECK_MESSAGE(LastL < TmpL, TmpL.GetHex() + "\"vs.[@" + std::to_string(i) + "]\"" + LastL.GetHex());
        LastL = TmpL;
    }

    BOOST_CHECK(ZeroL < R1L);
    BOOST_CHECK(R1L < R2L);
    BOOST_CHECK(ZeroL < OneL);
    BOOST_CHECK(OneL < MaxL);
    BOOST_CHECK(R1L < MaxL);
    BOOST_CHECK(R2L < MaxL);

    uint160 LastS;
    for (int i = 159; i >= 159; --i) //i >= 0
    {
        uint160 TmpS;
        *(TmpS.begin() + (i >> 3)) |= 1 << (7 - (i & 7));
        BOOST_CHECK(LastS < TmpS);
        LastS = TmpS;
    }
    BOOST_CHECK(ZeroS < R1S);
    BOOST_CHECK(R2S < R1S);
    BOOST_CHECK(ZeroS < OneS);
    BOOST_CHECK(OneS < MaxS);
    BOOST_CHECK(R1S < MaxS);
    BOOST_CHECK(R2S < MaxS);
}

BOOST_AUTO_TEST_CASE(methods) // GetHex SetHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Deserialize
{
    BOOST_CHECK(R1L.GetHex() == R1L.ToString());
    BOOST_CHECK(R2L.GetHex() == R2L.ToString());
    BOOST_CHECK(OneL.GetHex() == OneL.ToString());
    BOOST_CHECK(MaxL.GetHex() == MaxL.ToString());
    uint256 TmpL(R1L);
    BOOST_CHECK(TmpL == R1L);
    TmpL.SetHex(R2L.ToString());
    BOOST_CHECK(TmpL == R2L);
    TmpL.SetHex(ZeroL.ToString());
    BOOST_CHECK(TmpL == uint256(uint64(0)));

    TmpL.SetHex(R1L.ToString());
    BOOST_CHECK(memcmp(R1L.begin(), R1Array, 32) == 0);
    BOOST_CHECK(memcmp(TmpL.begin(), R1Array, 32) == 0);
    BOOST_CHECK(memcmp(R2L.begin(), R2Array, 32) == 0);
    BOOST_CHECK(memcmp(ZeroL.begin(), ZeroArray, 32) == 0);
    BOOST_CHECK(memcmp(OneL.begin(), OneArray, 32) == 0);
    BOOST_CHECK(R1L.size() == sizeof(R1L));
    BOOST_CHECK(sizeof(R1L) == 32);
    BOOST_CHECK(R1L.size() == 32);
    BOOST_CHECK(R2L.size() == 32);
    BOOST_CHECK(ZeroL.size() == 32);
    BOOST_CHECK(MaxL.size() == 32);
    BOOST_CHECK(R1L.begin() + 32 == R1L.end());
    BOOST_CHECK(R2L.begin() + 32 == R2L.end());
    BOOST_CHECK(OneL.begin() + 32 == OneL.end());
    BOOST_CHECK(MaxL.begin() + 32 == MaxL.end());
    BOOST_CHECK(TmpL.begin() + 32 == TmpL.end());

    mtbase::CBufStream ss;
    ss << R1L;
    BOOST_CHECK(ss.GetData() == std::string(R1Array, R1Array + 32));
    ss >> TmpL;
    BOOST_CHECK(R1L == TmpL);
    ss.Clear();
    ss << ZeroL;
    BOOST_CHECK(std::string(ss.GetData(), ss.GetSize()) == std::string(ZeroArray, ZeroArray + 32));
    ss >> TmpL;
    BOOST_CHECK(ZeroL == TmpL);
    ss.Clear();
    ss << MaxL;
    BOOST_CHECK(ss.GetData() == std::string(MaxArray, MaxArray + 32));
    ss >> TmpL;
    BOOST_CHECK(MaxL == TmpL);
    ss.Clear();

    BOOST_CHECK(R1S.GetHex() == R1S.ToString());
    BOOST_CHECK(R2S.GetHex() == R2S.ToString());
    BOOST_CHECK(OneS.GetHex() == OneS.ToString());
    BOOST_CHECK(MaxS.GetHex() == MaxS.ToString());
    uint160 TmpS(R1S);
    BOOST_CHECK(TmpS == R1S);
    TmpS.SetHex(R2S.ToString());
    BOOST_CHECK(TmpS == R2S);
    TmpS.SetHex(ZeroS.ToString());
    BOOST_CHECK(TmpS == uint160());

    TmpS.SetHex(R1S.ToString());
    BOOST_CHECK(memcmp(R1S.begin(), R1Array, 20) == 0);
    BOOST_CHECK(memcmp(TmpS.begin(), R1Array, 20) == 0);
    BOOST_CHECK(memcmp(R2S.begin(), R2Array, 20) == 0);
    BOOST_CHECK(memcmp(ZeroS.begin(), ZeroArray, 20) == 0);
    BOOST_CHECK(memcmp(OneS.begin(), OneArray, 20) == 0);
    BOOST_CHECK(R1S.size() == sizeof(R1S));
    BOOST_CHECK(sizeof(R1S) == 20);
    BOOST_CHECK(R1S.size() == 20);
    BOOST_CHECK(R2S.size() == 20);
    BOOST_CHECK(ZeroS.size() == 20);
    BOOST_CHECK(MaxS.size() == 20);
    BOOST_CHECK(R1S.begin() + 20 == R1S.end());
    BOOST_CHECK(R2S.begin() + 20 == R2S.end());
    BOOST_CHECK(OneS.begin() + 20 == OneS.end());
    BOOST_CHECK(MaxS.begin() + 20 == MaxS.end());
    BOOST_CHECK(TmpS.begin() + 20 == TmpS.end());

    ss << R1S;
    BOOST_CHECK(ss.GetData() == std::string(R1Array, R1Array + 20));
    ss >> TmpS;
    BOOST_CHECK(R1S == TmpS);
    ss.Clear();
    ss << ZeroS;
    BOOST_CHECK(std::string(ss.GetData(), ss.GetSize()) == std::string(ZeroArray, ZeroArray + 20));
    ss >> TmpS;
    BOOST_CHECK(ZeroS == TmpS);
    ss.Clear();
    ss << MaxS;
    BOOST_CHECK(ss.GetData() == std::string(MaxArray, MaxArray + 20));
    ss >> TmpS;
    BOOST_CHECK(MaxS == TmpS);
    ss.Clear();
}

BOOST_AUTO_TEST_CASE(uint256id)
{
    uint256 tid = uint256(uint32(0x12345678), uint224(0x123456789abc));
    for (int i = 0; i < 8; i++)
    {
        printf("%8.8x ", tid.Get32(i));
    }
    printf("\n");
    printf("%s\n", tid.GetHex().c_str());

    uint256 nid = uint256(uint32(0xaabbccdd), uint256(0x123456789abc));
    for (int i = 0; i < 8; i++)
    {
        printf("%8.8x ", nid.Get32(i));
    }
    printf("\n");
    printf("%s\n", nid.GetHex().c_str());

    uint256 tid1 = uint256(uint32(0xa), tid);
    uint256 tid2 = uint256(uint32(0xb), tid);
    uint256 tid3 = uint256(uint32(0xc), tid);
    std::map<uint256, uint256> mapId;
    mapId.insert(std::make_pair(tid1, tid1));
    mapId.insert(std::make_pair(tid2, tid2));
    mapId.insert(std::make_pair(tid3, tid3));
    for (const auto& kv : mapId)
    {
        printf("%s\n", kv.first.GetHex().c_str());
    }

    mtbase::CBufStream ss;
    ss << nid;
    bytes btNid;
    ss.GetData(btNid);
    std::cout << mtbase::ToHexString(btNid) << std::endl;

    try
    {
        mtbase::CBufStream mm(btNid);
        uint256 cnid;
        mm >> cnid;
        std::cout << cnid.GetHex() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
}

BOOST_AUTO_TEST_CASE(bigendiantest)
{
    uint256 n(0x11223344);
    printf("n: %s\n", n.ToString().c_str());
    printf("big endian: %s\n", ToHexString(n.ToBigEndian()).c_str());

    bytes btValidBe = n.ToValidBigEndianData();
    printf("valid big endian: %s\n", ToHexString(btValidBe).c_str());

    uint256 u;
    u.FromValidBigEndianData(btValidBe);
    printf("big endian: %s\n", u.ToString().c_str());
}

BOOST_AUTO_TEST_CASE(sethashtest)
{
    {
        uint256 hash = 0;
        hash.Set32(0, BSwap32(0x1234));
        hash.Set32(1, BSwap32(0x5678));
        hash.Set32(2, ((hash.Get32(2) & 0xFFFF0000) | (uint32)BSwap16(0xABCD)));
        printf("%s\n", ToHexString(hash.begin(), hash.size()).c_str());
        printf("%s\n", hash.ToString().c_str());
        mtbase::CBufStream ss;
        ss << hash;
        printf("%s\n", ToHexString((uint8*)ss.GetData(), ss.GetSize()).c_str());
    }

    {
        std::map<uint256, uint32> mapIndex;
        for (uint32 i = 400000; i <= 400000 + 100; i++)
        {
            uint256 n1 = i;
            uint256 h2 = CryptoHash(n1.begin(), n1.size());
            mapIndex.insert(std::make_pair(uint256(71204, i, 0, h2), i));
        }
        for (auto& kv : mapIndex)
        {
            printf("%d:0x%2.2x: %s\n", kv.second, kv.second, kv.first.ToString().c_str());
        }
        uint256 h(0x12345678);
        printf("%s\n", h.ToString().c_str());
        printf("%s\n", h.GetValueHex().c_str());
    }

    {
        uint256 n1 = 1;
        uint256 h2 = CryptoHash(n1.begin(), n1.size());
        uint256 k(0x1234, 0x5678, 0xABCD, h2);
        printf("k: %s\n", ToHexString(k.begin(), k.size()).c_str());
        printf("k: %s\n", k.ToString().c_str());
        mtbase::CBufStream ss;
        ss << k;
        printf("k: %s\n", ToHexString((uint8*)ss.GetData(), ss.GetSize()).c_str());
    }
}

/////////////////////////////////////////////////
using bigint = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<>>;

static const uint8 c_rlpMaxLengthBytes = 8;
static const uint8 c_rlpDataImmLenStart = 0x80;
static const uint8 c_rlpListStart = 0xc0;

static const uint8 c_rlpDataImmLenCount = c_rlpListStart - c_rlpDataImmLenStart - c_rlpMaxLengthBytes;
static const uint8 c_rlpDataIndLenZero = c_rlpDataImmLenStart + c_rlpDataImmLenCount - 1;
static const uint8 c_rlpListImmLenCount = 256 - c_rlpListStart - c_rlpMaxLengthBytes;
static const uint8 c_rlpListIndLenZero = c_rlpListStart + c_rlpListImmLenCount - 1;

/// Determine bytes required to encode the given integer value. @returns 0 if @a _i is zero.
template <class T>
inline unsigned bytesRequired(T _i)
{
    static_assert(std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "only unsigned types or bigint supported"); //bigint does not carry sign bit on shift
    unsigned i = 0;
    for (; _i != 0; ++i, _i >>= 8)
    {
    }
    return i;
}

/// Push an integer as a raw big-endian byte-stream.
template <class _T>
void pushInt(bytes& m_out, _T _i, size_t _br)
{
    m_out.resize(m_out.size() + _br);
    uint8* b = &m_out.back();
    for (; _i; _i >>= 8)
    {
        *(b--) = (uint8)(_i & 0xff);
    }
}

bool append_bigint(bytes& m_out, bigint _i)
{
    if (!_i)
    {
        m_out.push_back(c_rlpDataImmLenStart);
    }
    else if (_i < c_rlpDataImmLenStart)
    {
        m_out.push_back((uint8)_i);
    }
    else
    {
        unsigned br = bytesRequired(_i);
        if (br < c_rlpDataImmLenCount)
        {
            m_out.push_back((uint8)(br + c_rlpDataImmLenStart));
        }
        else
        {
            auto brbr = bytesRequired(br);
            if (c_rlpDataIndLenZero + brbr > 0xff)
            {
                //printf("Number too large for RLP\n");
                return false;
            }
            m_out.push_back((uint8)(c_rlpDataIndLenZero + brbr));
            pushInt(m_out, br, brbr);
        }
        pushInt(m_out, _i, br);
    }
    return true;
}

BOOST_AUTO_TEST_CASE(biginttest)
{
    {
        bytes out;
        append_bigint(out, 0x12);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0xA1);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x1234);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x123456);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x12345678);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x1234567890);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x1234567890ABL);
        printf("out: %s\n", ToHexString(out).c_str());
    }
    {
        bytes out;
        append_bigint(out, 0x1234567890ABCDLL);
        printf("out: %s\n", ToHexString(out).c_str());
    }
}

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "triedb.h"

#include <boost/test/unit_test.hpp>

#include "base_tests.h"
#include "block.h"
#include "destination.h"
#include "test_big.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet;
using namespace metabasenet::storage;
using namespace boost::filesystem;

//./build-release/test/test_big --log_level=all --run_test=triedb_tests/basetest
//./build-release/test/test_big --log_level=all --run_test=triedb_tests/shorttest
//./build-release/test/test_big --log_level=all --run_test=triedb_tests/stresstest

BOOST_FIXTURE_TEST_SUITE(triedb_tests, BasicUtfSetup)

class CListTrieDBWalker : public CTrieDBWalker
{
public:
    CListTrieDBWalker(bytesmap& mapKvIn)
      : mapKv(mapKvIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
    {
        mapKv[btKey] = btValue;
        return true;
    }

public:
    bytesmap& mapKv;
};

class CListOrderTrieDBWalker : public CTrieDBWalker
{
public:
    CListOrderTrieDBWalker(const uint64 nGetCountIn, std::vector<std::pair<bytes, bytes>>& vKvIn)
      : nGetCount(nGetCountIn), vKv(vKvIn) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
    {
        vKv.push_back(std::make_pair(btKey, btValue));
        if (nGetCount > 0 && vKv.size() >= nGetCount)
        {
            fWalkOver = true;
        }
        return true;
    }

public:
    const uint64 nGetCount;
    std::vector<std::pair<bytes, bytes>>& vKv;
};

bytes GetBytes(const string& str)
{
    return bytes(str.begin(), str.end());
}

BOOST_AUTO_TEST_CASE(basetest)
{
    cout << GetLocalTime() << "  triedb base test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    uint256 hashPrevRoot;
    uint256 hashNewRoot;

    {
        bytesmap mapKv;

        mapKv.insert(make_pair(GetBytes("key001"), GetBytes("value0000001")));
        mapKv.insert(make_pair(GetBytes("key002"), GetBytes("value0000002")));
        mapKv.insert(make_pair(GetBytes("key003"), GetBytes("value0000003")));

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;
    }

    {
        bytes key, value;
        string strKey("key001");
        key.assign(strKey.begin(), strKey.end());
        BOOST_CHECK(db.Retrieve(hashNewRoot, key, value));
        string strValue;
        strValue.assign(value.begin(), value.end());
        printf("Retrieve: key: %s, value: [%ld] %s\n", strKey.c_str(), value.size(), strValue.c_str());
    }

    {
        bytesmap mapRetKv;
        CListTrieDBWalker walker(mapRetKv);
        BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker));

        for (const auto& kv : mapRetKv)
        {
            string strKey(kv.first.begin(), kv.first.end());
            string strValue(kv.second.begin(), kv.second.end());
            printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
        }
    }

    printf("=================================================================\n");

    {
        bytesmap mapKv;

        mapKv.insert(make_pair(GetBytes("3478"), GetBytes("3478")));
        mapKv.insert(make_pair(GetBytes("3401"), GetBytes("3401")));
        mapKv.insert(make_pair(GetBytes("3478001"), GetBytes("3478001")));
        mapKv.insert(make_pair(GetBytes("92098"), GetBytes("92098")));
        mapKv.insert(make_pair(GetBytes("3"), GetBytes("3")));
        mapKv.insert(make_pair(GetBytes("key003"), GetBytes("qqqq-value0000003")));
        if (!mapKv.insert(make_pair(GetBytes("key002"), bytes())).second)
        {
            printf("insert key002 fail\n");
        }

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;
    }

    {
        bytesmap mapRetKv;
        CListTrieDBWalker walker(mapRetKv);
        BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker));

        for (const auto& kv : mapRetKv)
        {
            string strKey(kv.first.begin(), kv.first.end());
            string strValue(kv.second.begin(), kv.second.end());
            printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
        }
    }

    //---------------------------------------------------
    db.Clear();
    db.Deinitialize();
}

BOOST_AUTO_TEST_CASE(shorttest)
{
    cout << GetLocalTime() << "  triedb short test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    //---------------------------------------------------
    uint256 hashPrevRoot;
    uint256 hashNewRoot;
    bytesmap mapKvTotal;

    {
        bytesmap mapKv;
        mapKv.insert(make_pair(GetBytes("a35"), GetBytes("a35")));

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;

        bytesmap mapRetKv;
        CListTrieDBWalker walker(mapRetKv);
        BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker));

        for (const auto& kv : mapRetKv)
        {
            string strKey(kv.first.begin(), kv.first.end());
            string strValue(kv.second.begin(), kv.second.end());
            printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
        }
    }

    printf("=================================================================\n");

    {
        bytesmap mapKv;
        mapKv.insert(make_pair(GetBytes("a4"), GetBytes("a4")));
        mapKv.insert(make_pair(GetBytes("a47"), GetBytes("a47")));

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));

        bytesmap mapRetKv;
        CListTrieDBWalker walker(mapRetKv);
        BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker));

        for (const auto& kv : mapRetKv)
        {
            string strKey(kv.first.begin(), kv.first.end());
            string strValue(kv.second.begin(), kv.second.end());
            printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
        }
    }

    //---------------------------------------------------
    db.Clear();
    db.Deinitialize();
}

BOOST_AUTO_TEST_CASE(stresstest)
{
    cout << GetLocalTime() << "  triedb stress test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    //---------------------------------------------------
    uint256 hashPrevRoot;
    uint256 hashNewRoot;
    int nSingleCount = 100000;
    bytesmap mapKvTotal;
    std::map<uint256, bytesmap> mapRootKv;

    for (int n = 0; n < 10; n++)
    {
        bytesmap mapKv;

        for (int i = n * nSingleCount; i < (n + 1) * nSingleCount; i++)
        {
            int64 t = GetTimeMillis() + rand();
            t <<= 32;
            t += i;
            uint256 hash = crypto::CryptoSHA256(((uint8*)&t), sizeof(t));

            bytes key, value;
            key.assign(hash.begin(), hash.end());

            //char buf[52] = { 0 };
            //sprintf(buf, "v%d", i);
            //memset(buf, 'A', sizeof(buf) - 1);
            //value.assign(buf, buf + strlen(buf));
            value.assign(hash.begin(), hash.end());

            mapKv.insert(make_pair(key, value));
            mapKvTotal.insert(make_pair(key, value));
        }
        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));

        printf("AddNewTrie success, count: %ld\n", mapKv.size());

        mapRootKv.insert(make_pair(hashNewRoot, mapKv));

        int64 nSuccess = 0, nFail = 0;
        for (const auto& kv : mapKvTotal)
        {
            bytes value;
            if (!db.Retrieve(hashNewRoot, kv.first, value))
            {
                string strValue;
                strValue.assign(kv.second.begin(), kv.second.end());
                printf("Retrieve fail, value: %s\n", strValue.c_str());
                nFail++;
                continue;
            }
            if (kv.second != value)
            {
                string strSrcValue;
                strSrcValue.assign(kv.second.begin(), kv.second.end());
                string strDbValue;
                strDbValue.assign(value.begin(), value.end());
                printf("Value error, src value: %s, db value: %s\n", strSrcValue.c_str(), strDbValue.c_str());
                nFail++;
                continue;
            }
            nSuccess++;
        }
        printf("Verify success: %ld, fail: %ld\n", nSuccess, nFail);

        hashPrevRoot = hashNewRoot;
        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        if (hashPrevRoot != hashNewRoot)
        {
            printf("Repeat add root error, prev root: %s, new root: %s\n", hashPrevRoot.GetHex().c_str(), hashNewRoot.GetHex().c_str());
        }
        hashPrevRoot = hashNewRoot;
    }

    for (auto& vd : mapRootKv)
    {
        int64 nSuccess = 0, nFail = 0;
        for (const auto& kv : vd.second)
        {
            bytes value;
            if (!db.Retrieve(vd.first, kv.first, value))
            {
                string strValue;
                strValue.assign(kv.second.begin(), kv.second.end());
                printf("Retrieve fail, value: %s\n", strValue.c_str());
                nFail++;
                continue;
            }
            if (kv.second != value)
            {
                string strSrcValue;
                strSrcValue.assign(kv.second.begin(), kv.second.end());
                string strDbValue;
                strDbValue.assign(value.begin(), value.end());
                printf("Value error, src value: %s, db value: %s\n", strSrcValue.c_str(), strDbValue.c_str());
                nFail++;
                continue;
            }
            nSuccess++;
        }
        printf("Verify prev key, success: %ld, fail: %ld, root: %s\n", nSuccess, nFail, vd.first.GetHex().c_str());
    }

    //---------------------------------------------------
    db.Clear();
    db.Deinitialize();
}

BOOST_AUTO_TEST_CASE(walkthroughtest)
{
    cout << GetLocalTime() << "  triedb walk through test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    //---------------------------------------------------
    uint256 hashPrevRoot;
    uint256 hashNewRoot;

    {
        bytesmap mapKv;
        mapKv.insert(make_pair(GetBytes("000001"), GetBytes("000001")));
        mapKv.insert(make_pair(GetBytes("000002"), GetBytes("000002")));
        mapKv.insert(make_pair(GetBytes("000003"), GetBytes("000003")));

        mapKv.insert(make_pair(GetBytes("000011"), GetBytes("000011")));
        mapKv.insert(make_pair(GetBytes("000012"), GetBytes("000012")));
        mapKv.insert(make_pair(GetBytes("000013"), GetBytes("000013")));

        mapKv.insert(make_pair(GetBytes("002010"), GetBytes("002010")));
        mapKv.insert(make_pair(GetBytes("002011"), GetBytes("002011")));
        mapKv.insert(make_pair(GetBytes("002012"), GetBytes("002012")));
        mapKv.insert(make_pair(GetBytes("002013"), GetBytes("002013")));
        mapKv.insert(make_pair(GetBytes("002014"), GetBytes("002014")));
        mapKv.insert(make_pair(GetBytes("002015"), GetBytes("002015")));

        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));

        {
            bytes btKeyPrefix = GetBytes("002");
            bytes btBeginKeyTail;
            bool fReverse = true;
            uint64 nGetCount = 0;

            std::vector<std::pair<bytes, bytes>> vKv;
            CListOrderTrieDBWalker walker(nGetCount, vKv);
            BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker, btKeyPrefix, btBeginKeyTail, fReverse));

            for (const auto& vd : vKv)
            {
                string strKey(vd.first.begin(), vd.first.end());
                string strValue(vd.second.begin(), vd.second.end());
                printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
            }
        }

        printf("=========================================================\n");
        {
            bytes btKeyPrefix = GetBytes("002");
            bytes btBeginKeyTail = GetBytes("014");
            bool fReverse = true;
            uint64 nGetCount = 3;

            std::vector<std::pair<bytes, bytes>> vKv;
            CListOrderTrieDBWalker walker(nGetCount, vKv);
            BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker, btKeyPrefix, btBeginKeyTail, fReverse));

            for (const auto& vd : vKv)
            {
                string strKey(vd.first.begin(), vd.first.end());
                string strValue(vd.second.begin(), vd.second.end());
                printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
            }
        }

        printf("=========================================================\n");
        {
            bytes btKeyPrefix;
            bytes btBeginKeyTail = GetBytes("000012");
            bool fReverse = false;
            uint64 nGetCount = 0;

            std::vector<std::pair<bytes, bytes>> vKv;
            CListOrderTrieDBWalker walker(nGetCount, vKv);
            BOOST_CHECK(db.WalkThroughTrie(hashNewRoot, walker, btKeyPrefix, btBeginKeyTail, fReverse));

            for (const auto& vd : vKv)
            {
                string strKey(vd.first.begin(), vd.first.end());
                string strValue(vd.second.begin(), vd.second.end());
                printf("WalkThroughTrie: key: %s, value: %s\n", strKey.c_str(), strValue.c_str());
            }
        }
    }

    db.Deinitialize();
}

/////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(stresstest2)
{
    cout << GetLocalTime() << "  triedb stress test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    //---------------------------------------------------
    uint256 hashPrevRoot;
    int nTotalCount = 10000 * 100;
    int nSingleCount = 10;
    bytesmap mapKvTotal;
    std::map<uint256, bytesmap> mapRootKv;
    uint32 nTimestamp = (uint32)GetTime();

    for (int n = 0; n < nTotalCount / nSingleCount; n++)
    {
        bytesmap mapKv;
        for (int i = n * nSingleCount; i < (n + 1) * nSingleCount; i++)
        {
            int64 t = GetTimeMillis() + rand();
            t <<= 32;
            t += i;
            uint256 hash = crypto::CryptoSHA256(((uint8*)&t), sizeof(t));

            uint256 txid = uint256(nTimestamp, hash);

            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            //ssKey << std::string("txid") << BSwap32(nTimestamp) << hash;
            ssKey << std::string("txid") << txid;
            ssKey.GetData(btKey);
            if (i % 3 == 2)
                nTimestamp++;

            ssValue << hash;
            ssValue.GetData(btValue);

            mapKv.insert(make_pair(btKey, btValue));
        }

        uint256 hashNewRoot;
        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;

        printf("AddNewTrie success, count: %ld, total: %d\n", mapKv.size(), (n + 1) * nSingleCount);
    }

    //---------------------------------------------------
    db.Deinitialize();
}

BOOST_AUTO_TEST_CASE(stresstest3)
{
    cout << GetLocalTime() << "  triedb stress test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    //---------------------------------------------------
    uint256 hashPrevRoot;
    int64 nTotalCount = 10000 * 1000;
    int64 nSingleCount = 2;
    bytesmap mapKvTotal;
    std::map<uint256, bytesmap> mapRootKv;
    uint32 nTimestamp = (uint32)GetTime() / 10;
    uint32 nFile = 0, nOffset = 0;

    for (int64 n = 0; n < nTotalCount / nSingleCount; n++)
    {
        bytesmap mapKv;
        for (int64 i = n * nSingleCount; i < (n + 1) * nSingleCount; i++)
        {
            mtbase::CBufStream ssKey, ssValue;
            bytes btKey, btValue;

            ssKey << std::string("txid") << BSwap32(nTimestamp);
            ssKey.GetData(btKey);

            nTimestamp++;

            ssValue << nFile << nOffset;
            ssValue.GetData(btValue);

            if (++nOffset >= 100000)
            {
                nFile++;
                nOffset = 0;
            }

            mapKv.insert(make_pair(btKey, btValue));
        }

        uint256 hashNewRoot;
        BOOST_CHECK(db.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot));
        hashPrevRoot = hashNewRoot;

        printf("AddNewTrie success, count: %ld, total: %ld\n", mapKv.size(), (n + 1) * nSingleCount);
    }

    //---------------------------------------------------
    db.Deinitialize();
}

BOOST_AUTO_TEST_CASE(stattest)
{
    cout << GetLocalTime() << "  triedb stat node count test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    bytesmap mapRetKv;
    CListTrieDBWalker walker(mapRetKv);
    BOOST_CHECK(db.WalkThroughAll(walker));
    printf("Node count: %ld\n", mapRetKv.size());

    db.Deinitialize();
}

class CStatTrieDBWalker : public CTrieDBWalker
{
public:
    CStatTrieDBWalker()
      : nNodeCount(0), nDataSize(0), nMaxKeySize(0), nMaxValueSize(0) {}

    bool Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver) override
    {
        ++nNodeCount;
        nDataSize += btKey.size();
        nDataSize += btValue.size();

        if (btKey.size() > nMaxKeySize)
        {
            nMaxKeySize = btKey.size();
        }
        if (btValue.size() > nMaxValueSize)
        {
            nMaxValueSize = btValue.size();
        }
        return true;
    }

public:
    uint64 nNodeCount;
    uint64 nDataSize;
    uint64 nMaxKeySize;
    uint64 nMaxValueSize;
};

BOOST_AUTO_TEST_CASE(stattrienode)
{
    cout << GetLocalTime() << "  triedb stat trie node count test.........." << endl;

    std::string fullpath = GetOutPath("triedb_tests");
    cout << "fullpath: " << fullpath << endl;

    CTrieDB db;
    BOOST_CHECK(db.Initialize(boost::filesystem::path(fullpath)));

    CStatTrieDBWalker walker;
    BOOST_CHECK(db.WalkThroughAll(walker));

    printf("Node count: %ld, data size: %ld, nMaxKeySize: %ld, nMaxValueSize: %ld\n",
           walker.nNodeCount, walker.nDataSize,
           walker.nMaxKeySize, walker.nMaxValueSize);

    db.Deinitialize();
}

BOOST_AUTO_TEST_SUITE_END()

//./build/test/test_big --log_level=all --run_test=triedb_tests/basetest
//./build/test/test_big --log_level=all --run_test=triedb_tests/shorttest
//./build/test/test_big --log_level=all --run_test=triedb_tests/stresstest2
//./build/test/test_big --log_level=all --run_test=triedb_tests/stattrienode

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include "consblockvote.h"
#include "crypto.h"
#include "test_big.h"

using namespace std;
using namespace mtbase;
using namespace metabasenet;
using namespace consensus::consblockvote;

//./build-release/test/test_big --log_level=all --run_test=blockvote_tests/basetest

BOOST_FIXTURE_TEST_SUITE(blockvote_tests, BasicUtfSetup)

const vector<pair<string, string>> vNodeKey = {
    { "0x41d6d98568cc8058bc7048b384c40f765adb7b974b98b6986c8105bb1f666641", "0xb16382574785b810ff8fa7af6262a63173eaeab75aa75023776d8afcfe5c002db41f0b2d7efddf3d313f91e71f4fc9b2" },
    { "0x24922e21bb29bfe0f50830df6188b30e3751347dc88fe1a461dd1380f3735022", "0x8a9e5824ceef8051545c008b599c994ab09e981593c144aa2e9a820772a6ec8e7b87b1bff199c347184c4930bb30403f" },
    { "0x5468d445251dacf195c1301362494cecd67b387a594b7389d91a333720f10967", "0x87422c0bf990539467f96e2840d3e3aebe778e6330b4017d06c0dda74821c18f3d942e2dad2835fa9d656807e6aa14f0" },
    { "0x57e1521eaab6a8e2dc936e36adab51b34eac95efda51647aad8310f5ba43b2c4", "0xa890b5e43d17806041bee15ab2f7a64f6cff49c8af672eed46fce0d30032790f1c3e3f550291252093fe28e0df0e6553" },
    { "0x4a3a244d1263a1275f0423eb1fdeac97ab8bf9da6d8bcb036b764931933fcd7e", "0xad0d37dee0c1301d6613c5a3c0bdd36067805fdb96b067487a38e17d9580884f99402273dbbdfb0f585845f1b4cdc1bb" },
    // { "0x5d7c45e419c195ee862dc12395a6426336302c4d3a1e469f2d0c67cb2d5429fc", "0xa51d944e5772399ad2f6de19803dc50b4f79455b23017d6ddae09672c69613fb5cfdbcceb3ee0a13469a42e7ab058135" },
    // { "0x2372913ca787ae49d87c96a3a5311713ee8d6fdb309534e33842c39b44e0ad25", "0x892ce52dff507e7308445d5bb592a1683a360f292510c168c96c701268e0c85c4764fb956b3b520c79e3d0c5f2f6fbdd" },
    // { "0x4ce80935d3a1505ced54e08819d44fa25c513a32075afd02ade9a43635e69d59", "0xab795666fb2e403e19bdb922423592bd0002356afb2d9f096efb4b64fe780a729184ce561dbb66fdde65d1eff5ccebe0" },
    // { "0x3c1c87f14fcf2585540efb2b76fc1e5d227b0164c94069f0455b919c48aa7e0f", "0xb41fb90bf9aa2cc0b192bfd7d92911d88f99b9f0da2a3651a90c54de0abf9983a4187d66885d1f693f91669d58e105d3" },
    // { "0x5ab7fd8f0559a3e99dbbfcee0e315e204f27308fc143cb3de5310e1da9a4dfad", "0x988efcc325a1f3bea9fc2a562e1633d06698cbbb85cba12bdc0193e6a0956d718389f76aa25f5adb1ba265d214ac5dd1" }
};

const uint8 nTunnelId = 1;
const int64 nEpochDuration = 10 * 1000;

class CNetNode;
using SHP_NET_NODE = shared_ptr<CNetNode>;

class CNetNode
{
public:
    CNetNode(const size_t nNodeIndexIn)
      : nNodeIndex(nNodeIndexIn)
    {
        nLocalNetId = ((nNodeIndexIn + 1) << 16) + (GetTime() & 0xFFFF);
        nCreateLocalConnId = ((nNodeIndexIn + 1) << 24) + (GetTime() & 0xFFFFFF);
    }

    bool Init()
    {
        
        ptrBlockVote = shared_ptr<CConsBlockVote>(new CConsBlockVote(nTunnelId, nEpochDuration,
                                                                     boost::bind(&CNetNode::SendNetData, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3),
                                                                     boost::bind(&CNetNode::GetVoteBlockCandidatePubkey, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3, boost::placeholders::_4, boost::placeholders::_5, boost::placeholders::_6),
                                                                     boost::bind(&CNetNode::AddBlockLocalVoteSignFlag, this, boost::placeholders::_1),
                                                                     boost::bind(&CNetNode::CommitBlockVoteResult, this, boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)));
        if (ptrBlockVote == nullptr)
        {
            return false;
        }
        BOOST_CHECK(ptrBlockVote->AddConsKey(uint256(vNodeKey[nNodeIndex].first), uint384(vNodeKey[nNodeIndex].second)));
        
        return true;
    }
    uint64 GetLocalNetId() const
    {
        return nLocalNetId;
    }
    void AddNetNode(const SHP_NET_NODE ptrNetNode)
    {
        mapNetConnect[nCreateLocalConnId] = ptrNetNode;
        mapPeerNodeNetId[ptrNetNode->GetLocalNetId()] = nCreateLocalConnId;
        BOOST_CHECK(ptrBlockVote->AddNetNode(nCreateLocalConnId));
        nCreateLocalConnId++;
    }
    bool SendNetData(const uint64 nNetConnId, const uint8 nTunnelId, const bytes& btData)
    {
        auto it = mapNetConnect.find(nNetConnId);
        if (it != mapNetConnect.end())
        {
            it->second->RecvNetData(nLocalNetId, btData);
            return true;
        }
        return false;
    }
    bool GetVoteBlockCandidatePubkey(const uint256& hashBlock, uint32& nBlockHeight, int64& nBlockTime, vector<uint384>& vPubkey, bytes& btAggBitmap, bytes& btAggSig)
    {
        return false;
    }
    bool AddBlockLocalVoteSignFlag(const uint256& hashBlock)
    {
        return true;
    }
    bool CommitBlockVoteResult(const uint256& hashBlock, const bytes& btBitmap, const bytes& btAggSig)
    {
        CBitmap bmVoteBitmap;
        bmVoteBitmap.ImportBytes(btBitmap);
        printf("Commit Block Vote Result: vote bitmap: %s, agg sig: %s, block: [%d] %s\n",
               bmVoteBitmap.GetBitmapString().c_str(), ToHexString(btAggSig).c_str(),
               CBlock::GetBlockHeightByHash(hashBlock), hashBlock.GetHex().c_str());
        return true;
    }
    void RecvNetData(const uint64 nPeerNetId, const bytes& btData)
    {
        qSendNetData.push(make_pair(nPeerNetId, btData));
    }
    void AddBlock(const uint256& hashBlock, const uint32 nBlockEpoch, const int64 nVoteBeginTime, const vector<uint384>& vPubkey)
    {
        BOOST_CHECK(ptrBlockVote->AddCandidatePubkey(hashBlock, nBlockEpoch, nVoteBeginTime, vPubkey));
    }
    void OnSendData()
    {
        while (!qSendNetData.empty())
        {
            auto& sendData = qSendNetData.front();
            auto it = mapPeerNodeNetId.find(sendData.first);
            if (it != mapPeerNodeNetId.end())
            {
                ptrBlockVote->OnEventNetData(it->second, sendData.second);
            }
            qSendNetData.pop();
        }
    }
    void OnTimer()
    {
        ptrBlockVote->OnTimer();
    }
    bool GetBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig)
    {
        return ptrBlockVote->GetBlockVoteResult(hashBlock, btBitmap, btAggSig);
    }

public:
    const size_t nNodeIndex;
    uint64 nLocalNetId;
    uint64 nCreateLocalConnId;
    map<uint64, SHP_NET_NODE> mapNetConnect;
    map<uint64, uint64> mapPeerNodeNetId; // key: peer netid, value: connect id
    shared_ptr<CConsBlockVote> ptrBlockVote;
    queue<pair<uint64, bytes>> qSendNetData;
};

BOOST_AUTO_TEST_CASE(basetest)
{
    //NointLogOut();

    vector<SHP_NET_NODE> vNetNode;

    vNetNode.reserve(vNodeKey.size());
    for (size_t i = 0; i < vNodeKey.size(); i++)
    {
        SHP_NET_NODE ptrNetNode = shared_ptr<CNetNode>(new CNetNode(i));
        BOOST_CHECK(ptrNetNode->Init());
        vNetNode.push_back(ptrNetNode);
    }

    for (size_t i = 0; i < vNetNode.size(); i++)
    {
        for (size_t j = 0; j < vNetNode.size(); j++)
        {
            if (j != i)
            {
                vNetNode[i]->AddNetNode(vNetNode[j]);
            }
        }
    }

    vector<uint384> vPubkey;
    for (auto& vd : vNodeKey)
    {
        vPubkey.push_back(uint384(vd.second));
    }

    uint256 hashBlock;
    int64 nPrevAddBlockTime = 0;
    uint32 nBlockHeight = 1;
    bool fShowVoteResult = true;

    while (true)
    {
        for (size_t i = 0; i < vNodeKey.size(); i++)
        {
            vNetNode[i]->OnTimer();
        }

        int64 nCurTime = GetTimeMillis();
        if (nCurTime - nPrevAddBlockTime >= nEpochDuration)
        {
            nPrevAddBlockTime = nCurTime;

            mtbase::CBufStream ss;
            ss << nBlockHeight << nCurTime;
            uint256 hash = crypto::CryptoHash(ss.GetData(), ss.GetSize());
            hashBlock = uint256(BSwap32(nBlockHeight), uint224(hash));

            for (size_t i = 0; i < vNodeKey.size(); i++)
            {
                vNetNode[i]->AddBlock(hashBlock, nBlockHeight, nCurTime, vPubkey);
            }
            nBlockHeight++;
            fShowVoteResult = false;
        }

        for (size_t i = 0; i < vNodeKey.size(); i++)
        {
            vNetNode[i]->OnSendData();
        }

        usleep(1000 * 100);
    }
}

BOOST_AUTO_TEST_CASE(makekeytest)
{
    for (int i = 0; i < 10; i++)
    {
        CCryptoBlsKey key;
        BOOST_CHECK(CryptoBlsMakeNewKey(key));
        printf("secret: %s, pubkey: %s\n", key.secret.GetHex().c_str(), key.pubkey.GetHex().c_str());
    }
}

BOOST_AUTO_TEST_CASE(verifykeytest)
{
    for (size_t i = 0; i < vNodeKey.size(); i++)
    {
        uint256 privkey(vNodeKey[i].first);
        uint384 pubkey;
        BOOST_CHECK(CryptoBlsGetPubkey(privkey, pubkey));
        BOOST_CHECK(pubkey.GetHex() == vNodeKey[i].second);
    }
}

BOOST_AUTO_TEST_CASE(blockhashtest)
{
    auto funcGetHash = [](const uint32 nHeight, const uint16 nSlot, const uint32 nNonce = 0) -> uint256 {
        mtbase::CBufStream ss;
        ss << nHeight << nSlot << nNonce;
        return uint256(201, nHeight, nSlot, crypto::CryptoHash(ss.GetData(), ss.GetSize()));
    };

    std::map<uint256, std::pair<uint32, uint16>> mapBlockNon;
    std::map<uint256, std::pair<uint32, uint16>, CustomBlockHashCompare> mapBlockCus;
    for (uint32 i = 0; i < 10; i++)
    {
        for (uint32 j = 0; j < 3; j++)
        {
            uint256 hashBlock = funcGetHash(i, (i + 1) % 3 + j);
            mapBlockNon.insert(std::make_pair(hashBlock, std::make_pair(i, (i + 1) % 3 + j)));
            mapBlockCus.insert(std::make_pair(hashBlock, std::make_pair(i, (i + 1) % 3 + j)));
        }

        if (i % 2 == 1)
        {
            uint256 hashBlock = funcGetHash(i, (i + 1) % 3, 56789);
            mapBlockNon.insert(std::make_pair(hashBlock, std::make_pair(i, (i + 1) % 3)));
            mapBlockCus.insert(std::make_pair(hashBlock, std::make_pair(i, (i + 1) % 3)));
        }
    }

    printf("block non count: %lu\n", mapBlockNon.size());
    for (auto& kv : mapBlockNon)
    {
        printf("height: %u, slot: %u, block: %s\n", CBlock::GetBlockHeightByHash(kv.first), CBlock::GetBlockSlotByHash(kv.first), kv.first.ToString().c_str());
    }

    printf("block cus count: %lu\n", mapBlockCus.size());
    for (auto& kv : mapBlockCus)
    {
        printf("custom height: %u, slot: %u, block: %s\n", CBlock::GetBlockHeightByHash(kv.first), CBlock::GetBlockSlotByHash(kv.first), kv.first.ToString().c_str());
    }
}

BOOST_AUTO_TEST_SUITE_END()

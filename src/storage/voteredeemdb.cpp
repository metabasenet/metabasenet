// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "voteredeemdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"
#include "param.h"

using namespace std;
using namespace hcode;

namespace metabasenet
{
namespace storage
{

#define DB_REDEEM_KEY_NAME_VOTE_DEST string("vdest")
#define DB_REDEEM_KEY_NAME_TRIEROOT string("trieroot")
#define DB_REDEEM_KEY_NAME_PREVROOT string("prevroot")

//////////////////////////////
// CVoteRedeemDB

bool CVoteRedeemDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData / "voteredeem"))
    {
        return false;
    }
    return true;
}

void CVoteRedeemDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

void CVoteRedeemDB::Clear()
{
    dbTrie.Clear();
}

bool CVoteRedeemDB::AddBlockVoteRedeem(const uint256& hashPrev, const uint256& hashBlock, const std::map<CDestination, CVoteRedeemContext>& mapBlockVoteRedeem, uint256& hashVoteRedeemRoot)
{
    uint256 hashPrevRoot;
    if (hashPrev != 0)
    {
        if (!ReadTrieRoot(hashPrev, hashPrevRoot))
        {
            StdLog("CVoteRedeemDB", "Add block vote redeem: Read trie root fail, prev: %s", hashPrev.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    for (const auto& kv : mapBlockVoteRedeem)
    {
        hcode::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_REDEEM_KEY_NAME_VOTE_DEST << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashVoteRedeemRoot))
    {
        StdLog("CVoteRedeemDB", "Add block vote redeem: Add trie fail, prev root: %s, prev block: %s",
               hashPrevRoot.GetHex().c_str(), hashPrev.GetHex().c_str());
        return false;
    }

    if (!WriteTrieRoot(hashBlock, hashVoteRedeemRoot))
    {
        StdLog("CVoteRedeemDB", "Add block vote redeem: Write block root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CVoteRedeemDB::RetrieveDestVoteRedeemContext(const uint256& hashBlock, const CDestination& destVote, CVoteRedeemContext& ctxtVoteRedeem)
{
    if (hashBlock == 0)
    {
        return true;
    }

    uint256 hashTrieRoot;
    if (!ReadTrieRoot(hashBlock, hashTrieRoot))
    {
        StdLog("CVoteRedeemDB", "Retrieve dest vote redeem context: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_REDEEM_KEY_NAME_VOTE_DEST << destVote;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashTrieRoot, btKey, btValue))
    {
        StdLog("CVoteRedeemDB", "Retrieve dest vote redeem context: Retrieve kv trie, root: %s, block: %s", hashTrieRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }

    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> ctxtVoteRedeem;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CVoteRedeemDB::VerifyVoteRedeem(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(hashBlock, hashRoot))
    {
        StdLog("CVoteRedeemDB", "Verify vote redeem: Read trie root fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            StdLog("CVoteRedeemDB", "Verify vote redeem: Check trie node fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        if (!ReadTrieRoot(hashPrevBlock, hashPrevRoot))
        {
            StdLog("CVoteRedeemDB", "Verify vote redeem: Read prev trie root fail, prev block: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        StdLog("CVoteRedeemDB", "Verify vote redeem: Get prev root fail, root: %s, block: %s", hashRoot.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        StdLog("CVoteRedeemDB", "Verify vote redeem: Verify error, hashRootPrevDb: %s, hashPrevRoot: %s, hashBlockLocalDb: %s, block: %s",
               hashRootPrevDb.GetHex().c_str(), hashPrevRoot.GetHex().c_str(),
               hashBlockLocalDb.GetHex().c_str(), hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

///////////////////////////////////
bool CVoteRedeemDB::WriteTrieRoot(const uint256& hashBlock, const uint256& hashTrieRoot)
{
    CBufStream ssKey;
    ssKey << DB_REDEEM_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ssKey.GetData(), ssKey.GetSize());

    CBufStream ssValue;
    ssValue << hashTrieRoot;
    bytes btValue;
    ssValue.GetData(btValue);
    if (!dbTrie.SetValueNode(hashKey, btValue))
    {
        return false;
    }
    return true;
}

bool CVoteRedeemDB::ReadTrieRoot(const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    CBufStream ssKey;
    ssKey << DB_REDEEM_KEY_NAME_TRIEROOT << hashBlock;
    uint256 hashKey = metabasenet::crypto::CryptoHash(ssKey.GetData(), ssKey.GetSize());

    bytes btValue;
    if (!dbTrie.GetValueNode(hashKey, btValue))
    {
        return false;
    }
    CBufStream ssValue(btValue);
    ssValue >> hashTrieRoot;
    return true;
}

void CVoteRedeemDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << DB_REDEEM_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CVoteRedeemDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hcode::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << DB_REDEEM_KEY_NAME_PREVROOT;
    ssKey.GetData(btKey);
    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> hashPrevRoot >> hashBlock;
    }
    catch (std::exception& e)
    {
        hcode::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

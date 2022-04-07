// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegatedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace hnbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CListDelegateVoteTrieDBWalker

bool CListDelegateVoteTrieDBWalker::Walk(const bytes& btKey, const bytes& btValue, const uint32 nDepth, bool& fWalkOver)
{
    if (btKey.size() == 0 || btValue.size() == 0)
    {
        StdError("CListDelegateVoteTrieDBWalker", "btKey.size() = %ld, btValue.size() = %ld", btKey.size(), btValue.size());
        return false;
    }

    try
    {
        hnbase::CBufStream ssKey;
        ssKey.Write((char*)(btKey.data()), btKey.size());

        string strName;
        ssKey >> strName;
        if (strName == string("vote"))
        {
            hnbase::CBufStream ssValue;
            CDestination dest;
            uint256 nVote;
            ssValue.Write((char*)(btValue.data()), btValue.size());
            ssKey >> dest;
            ssValue >> nVote;
            if (nVote != 0)
            {
                mapVote.insert(std::make_pair(dest, nVote));
            }
        }
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

//////////////////////////////
// CDelegateDB

bool CDelegateDB::Initialize(const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = (pathData / "delegateenroll").string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!Open(engine))
    {
        delete engine;
        return false;
    }

    if (!dbVoteTrie.Initialize(pathData / "delegatevote"))
    {
        Deinitialize();
        return false;
    }
    return true;
}

void CDelegateDB::Deinitialize()
{
    cacheDelegate.Clear();
    dbVoteTrie.Deinitialize();
    Close();
}

void CDelegateDB::Clear()
{
    cacheDelegate.Clear();
    dbVoteTrie.RemoveAll();
    RemoveAll();
}

bool CDelegateDB::AddNewDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote,
                                 const std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTx, uint256& hashDelegateRoot)
{
    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        CDelegateContext ctxtPrevDelegate;
        if (!Retrieve(hashPrevBlock, ctxtPrevDelegate))
        {
            return false;
        }
        hashPrevRoot = ctxtPrevDelegate.hashVoteTrieRoot;
    }

    CDelegateContext ctxtDelegate;
    ctxtDelegate.mapEnrollTx = mapEnrollTx;

    if (!AddDelegateVote(hashPrevRoot, hashBlock, mapVote, ctxtDelegate.hashVoteTrieRoot))
    {
        return false;
    }

    if (!Write(hashBlock, ctxtDelegate))
    {
        return false;
    }
    cacheDelegate.AddNew(hashBlock, ctxtDelegate);

    hashDelegateRoot = ctxtDelegate.hashVoteTrieRoot;
    return true;
}

bool CDelegateDB::Remove(const uint256& hashBlock)
{
    cacheDelegate.Remove(hashBlock);
    return Erase(hashBlock);
}

bool CDelegateDB::RetrieveDestDelegateVote(const uint256& hashBlock, const CDestination& dest, uint256& nVote)
{
    if (hashBlock == 0)
    {
        nVote = 0;
        return true;
    }
    CDelegateContext ctxtDelegate;
    if (!Retrieve(hashBlock, ctxtDelegate))
    {
        StdLog("CDelegateDB", "Retrieve Dest Delegate Vote: Retrieve delegate fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return GetDestVote(ctxtDelegate.hashVoteTrieRoot, dest, nVote);
}

bool CDelegateDB::RetrieveDelegatedVote(const uint256& hashBlock, map<CDestination, uint256>& mapVote)
{
    if (hashBlock == 0)
    {
        return true;
    }

    CDelegateContext ctxtDelegate;
    if (!Retrieve(hashBlock, ctxtDelegate))
    {
        return false;
    }

    CListDelegateVoteTrieDBWalker walker(mapVote);
    if (!dbVoteTrie.WalkThroughTrie(ctxtDelegate.hashVoteTrieRoot, walker))
    {
        return false;
    }
    return true;
}

bool CDelegateDB::RetrieveDelegatedEnrollTx(const uint256& hashBlock, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos)
{
    if (hashBlock == 0)
    {
        return true;
    }

    CDelegateContext ctxtDelegate;
    if (!Retrieve(hashBlock, ctxtDelegate))
    {
        return false;
    }
    mapEnrollTxPos = ctxtDelegate.mapEnrollTx;
    return true;
}

bool CDelegateDB::RetrieveEnrollTx(int height, const vector<uint256>& vBlockRange,
                                   map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    for (const uint256& hash : boost::adaptors::reverse(vBlockRange))
    {
        if (hash == 0)
        {
            continue;
        }

        CDelegateContext ctxtDelegate;
        if (!Retrieve(hash, ctxtDelegate))
        {
            return false;
        }

        map<int, map<CDestination, CDiskPos>>::iterator it = ctxtDelegate.mapEnrollTx.find(height);
        if (it != ctxtDelegate.mapEnrollTx.end())
        {
            mapEnrollTxPos.insert((*it).second.begin(), (*it).second.end());
        }
    }
    return true;
}

bool CDelegateDB::VerifyDelegate(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CDelegateContext ctxtDelegate;
    if (!Retrieve(hashBlock, ctxtDelegate))
    {
        return false;
    }
    hashRoot = ctxtDelegate.hashVoteTrieRoot;

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbVoteTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashPrevBlock != 0)
    {
        CDelegateContext ctxtPrevDelegate;
        if (!Retrieve(hashPrevBlock, ctxtPrevDelegate))
        {
            return false;
        }
        hashPrevRoot = ctxtPrevDelegate.hashVoteTrieRoot;
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        return false;
    }
    return true;
}

/////////////////////////////////////////
bool CDelegateDB::Retrieve(const uint256& hashBlock, CDelegateContext& ctxtDelegate)
{
    if (cacheDelegate.Retrieve(hashBlock, ctxtDelegate))
    {
        return true;
    }
    if (!Read(hashBlock, ctxtDelegate))
    {
        return false;
    }
    cacheDelegate.AddNew(hashBlock, ctxtDelegate);
    return true;
}

bool CDelegateDB::GetDestVote(const uint256& hashTrieRoot, const CDestination& dest, uint256& nVote)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << string("vote") << dest;
    ssKey.GetData(btKey);
    if (!dbVoteTrie.Retrieve(hashTrieRoot, btKey, btValue))
    {
        StdLog("CDelegateDB", "Get Dest Vote: Retrieve trie fail, dest: %s, hashTrieRoot: %s",
               dest.ToString().c_str(), hashTrieRoot.GetHex().c_str());
        return false;
    }
    try
    {
        ssValue.Write((char*)(btValue.data()), btValue.size());
        ssValue >> nVote;
    }
    catch (std::exception& e)
    {
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CDelegateDB::AddDelegateVote(const uint256& hashPrevRoot, const uint256& hashBlock, const std::map<CDestination, uint256>& mapVote, uint256& hashNewRoot)
{
    bytesmap mapKv;
    for (const auto& kv : mapVote)
    {
        hnbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << string("vote") << kv.first;
        ssKey.GetData(btKey);

        ssValue << kv.second;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(hashPrevRoot, hashBlock, mapKv);

    if (!dbVoteTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        return false;
    }
    return true;
}

void CDelegateDB::AddPrevRoot(const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    hnbase::CBufStream ssKey;
    bytes btKey, btValue;

    ssKey << string("prevroot");
    ssKey.GetData(btKey);

    hnbase::CBufStream ssValue;
    ssValue << hashPrevRoot << hashBlock;
    btValue.assign(ssValue.GetData(), ssValue.GetData() + ssValue.GetSize());

    mapKv.insert(make_pair(btKey, btValue));
}

bool CDelegateDB::GetPrevRoot(const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    hnbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << string("prevroot");
    ssKey.GetData(btKey);
    if (!dbVoteTrie.Retrieve(hashRoot, btKey, btValue))
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
        hnbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

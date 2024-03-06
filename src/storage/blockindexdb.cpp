// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockindexdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

const string DB_BLOCKINDEX_KEY_ID_TRIEROOT("trieroot");
const string DB_BLOCKINDEX_KEY_ID_PREVROOT("prevroot");

const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX = 0x10;
const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER = 0x20;
const uint8 DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT = 0x30;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_INDEX | 0x01;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER | 0x01;

const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x01;
const uint8 DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x02;
const uint8 DB_BLOCKINDEX_KEY_TYPE_BLOCK_LOCAL_SIGN_FLAG = DB_BLOCKINDEX_ROOT_TYPE_BLOCK_VOTE_RESULT | 0x03;

//////////////////////////////
// CBlockIndexDB

CBlockIndexDB::CBlockIndexDB()
{
}

CBlockIndexDB::~CBlockIndexDB()
{
    dbTrie.Deinitialize();
}

bool CBlockIndexDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!dbTrie.Initialize(pathData / "blockindex"))
    {
        return false;
    }
    return true;
}

void CBlockIndexDB::Deinitialize()
{
    dbTrie.Deinitialize();
}

bool CBlockIndexDB::Clear()
{
    dbTrie.RemoveAll();
    return true;
}

//-----------------------------------------------------
// block index

bool CBlockIndexDB::AddNewBlockIndex(const CBlockOutline& outline)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << outline.GetBlockHash();
    ssValue << outline;
    if (!dbTrie.WriteExtKv(ssKey, ssValue))
    {
        StdLog("CBlockIndexDB", "Add new block index: Write ext kv fail, block: %s", outline.GetBlockHash().GetHex().c_str());
        return false;
    }
    return true;
}

bool CBlockIndexDB::RemoveBlockIndex(const uint256& hashBlock)
{
    CBufStream ssKey;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << hashBlock;
    return dbTrie.RemoveExtKv(ssKey);
}

bool CBlockIndexDB::RetrieveBlockIndex(const uint256& hashBlock, CBlockOutline& outline)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> outline;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::WalkThroughBlockIndex(CBlockDBWalker& walker)
{
    auto funcWalker = [&](CBufStream& ssKey, CBufStream& ssValue) -> bool {
        try
        {
            CBlockOutline outline;
            ssValue >> outline;
            return walker.Walk(outline);
        }
        catch (std::exception& e)
        {
            mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    };

    CBufStream ssKeyBegin, ssKeyPrefix;
    ssKeyBegin << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX;
    ssKeyPrefix << DB_BLOCKINDEX_KEY_TYPE_BLOCK_INDEX;

    return dbTrie.WalkThroughExtKv(ssKeyBegin, ssKeyPrefix, funcWalker);
}

//-----------------------------------------------------
// block number

bool CBlockIndexDB::AddBlockNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashPrevBlock, const uint64 nBlockNumber, const uint256& hashBlock, uint256& hashNewRoot)
{
    uint256 hashPrevRoot;
    if (hashPrevBlock != 0 && hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashPrevBlock, hashPrevRoot))
        {
            StdLog("CBlockIndexDB", "Add block number: Read trie root fail, hashPrevBlock: %s", hashPrevBlock.GetHex().c_str());
            return false;
        }
    }

    bytesmap mapKv;
    {
        mtbase::CBufStream ssKey, ssValue;
        bytes btKey, btValue;

        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER << nChainId << BSwap64(nBlockNumber);
        ssKey.GetData(btKey);

        ssValue << hashBlock;
        ssValue.GetData(btValue);

        mapKv.insert(make_pair(btKey, btValue));
    }
    AddPrevRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashPrevRoot, hashBlock, mapKv);

    if (!dbTrie.AddNewTrie(hashPrevRoot, mapKv, hashNewRoot))
    {
        StdLog("CBlockIndexDB", "Add block number: Add new trie fail, hashPrevBlock: %s, nBlockNumber: %lu",
               hashPrevBlock.GetHex().c_str(), nBlockNumber);
        return false;
    }

    if (!WriteTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashBlock, hashNewRoot))
    {
        StdLog("CBlockIndexDB", "Add block number: Write trie root fail, hashPrevBlock: %s, nBlockNumber: %lu",
               hashPrevBlock.GetHex().c_str(), nBlockNumber);
        return false;
    }
    return true;
}

bool CBlockIndexDB::RetrieveBlockHashByNumber(const uint256& hashFork, const uint32 nChainId, const uint256& hashLastBlock, const uint64 nBlockNumber, uint256& hashBlock)
{
    uint256 hashRoot;
    if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashLastBlock, hashRoot))
    {
        StdLog("CBlockIndexDB", "Retrieve block hash by number: Read trie root fail, last block: %s", hashLastBlock.GetHex().c_str());
        return false;
    }

    mtbase::CBufStream ssKey;
    bytes btKey, btValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_NUMBER << nChainId << BSwap64(nBlockNumber);
    ssKey.GetData(btKey);

    if (!dbTrie.Retrieve(hashRoot, btKey, btValue))
    {
        StdLog("CBlockIndexDB", "Retrieve block hash by number: Trie retrieve kv fail, root: %s, chainid: %u, number: %lu, last block: %s",
               hashRoot.GetHex().c_str(), nChainId, nBlockNumber, hashLastBlock.GetHex().c_str());
        return false;
    }

    try
    {
        mtbase::CBufStream ssValue(btValue);
        ssValue >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::VerifyBlockNumberContext(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashBlock, hashRoot))
    {
        return false;
    }

    if (fVerifyAllNode)
    {
        std::map<uint256, CTrieValue> mapCacheNode;
        if (!dbTrie.CheckTrieNode(hashRoot, mapCacheNode))
        {
            return false;
        }
    }

    uint256 hashPrevRoot;
    if (hashBlock != hashFork)
    {
        if (!ReadTrieRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashPrevBlock, hashPrevRoot))
        {
            return false;
        }
    }

    uint256 hashRootPrevDb;
    uint256 hashBlockLocalDb;
    if (!GetPrevRoot(DB_BLOCKINDEX_ROOT_TYPE_BLOCK_NUMBER, hashRoot, hashRootPrevDb, hashBlockLocalDb))
    {
        return false;
    }
    if (hashRootPrevDb != hashPrevRoot || hashBlockLocalDb != hashBlock)
    {
        return false;
    }
    return true;
}

///////////////////////////////////
// block vote result

bool CBlockIndexDB::AddBlockVoteResult(const uint256& hashBlock, const bool fLongChain, const bytes& btBitmap, const bytes& btAggSig, const bool fAtChain, const uint256& hashAtBlock)
{
    CWriteLock wlock(rwAccess);

    {
        CBufStream ssKey, ssValue;
        ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
        ssValue << btBitmap << btAggSig << fAtChain << hashAtBlock;
        if (!dbTrie.WriteExtKv(ssKey, ssValue))
        {
            StdLog("CBlockIndexDB", "Add block vote result: Write ext kv fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    if (fLongChain)
    {
        CBlockOutline outline;
        if (!RetrieveBlockIndex(hashBlock, outline))
        {
            StdLog("CBlockIndexDB", "Add block vote result: Retrieve block index fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }

        uint256 hashLastBlock;
        uint64 nLastNumber;
        bool fLastAtChain = false;
        {
            CBufStream ssKey, ssValue;
            ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << outline.hashOrigin;
            if (dbTrie.ReadExtKv(ssKey, ssValue))
            {
                try
                {
                    bytes btTempBitmap;
                    bytes btTempAggSig;
                    uint256 hashLastAtBlock;
                    ssValue >> hashLastBlock >> nLastNumber >> btTempBitmap >> btTempAggSig >> fLastAtChain >> hashLastAtBlock;
                }
                catch (std::exception& e)
                {
                    mtbase::StdError(__PRETTY_FUNCTION__, e.what());
                    hashLastBlock = 0;
                }
            }
        }

        bool fWriteLast = false;
        do
        {
            if (hashLastBlock.IsNull())
            {
                fWriteLast = true;
                break;
            }
            if (!hashLastBlock.IsNull() && hashLastBlock == hashBlock && !fLastAtChain && fAtChain)
            {
                fWriteLast = true;
                break;
            }
            if (CBlock::GetBlockHeightByHash(hashLastBlock) <= CBlock::GetBlockHeightByHash(hashBlock) && nLastNumber < outline.GetBlockNumber())
            {
                fWriteLast = true;
                break;
            }
        } while (0);

        if (fWriteLast)
        {
            CBufStream ssKey, ssValue;
            ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << outline.hashOrigin;
            ssValue << hashBlock << outline.GetBlockNumber() << btBitmap << btAggSig << fAtChain << hashAtBlock;
            if (!dbTrie.WriteExtKv(ssKey, ssValue))
            {
                StdLog("CBlockIndexDB", "Add block vote result: Write ext kv fail, block: %s", hashBlock.GetHex().c_str());
                return false;
            }
        }
    }
    return true;
}

bool CBlockIndexDB::RemoveBlockVoteResult(const uint256& hashBlock)
{
    CWriteLock wlock(rwAccess);
    CBufStream ssKey;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
    return dbTrie.RemoveExtKv(ssKey);
}

bool CBlockIndexDB::RetrieveBlockVoteResult(const uint256& hashBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    CReadLock rlock(rwAccess);
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_VOTE_RESULT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> btBitmap >> btAggSig >> fAtChain >> hashAtBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::GetLastBlockVoteResult(const uint256& hashFork, uint256& hashLastBlock, bytes& btBitmap, bytes& btAggSig, bool& fAtChain, uint256& hashAtBlock)
{
    CReadLock rlock(rwAccess);
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << hashFork;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        uint64 nLastNumber = 0;
        ssValue >> hashLastBlock >> nLastNumber >> btBitmap >> btAggSig >> fAtChain >> hashAtBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::GetLastConfirmBlock(const uint256& hashFork, uint256& hashLastConfirmBlock)
{
    CReadLock rlock(rwAccess);
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_LAST_BLOCK_VOTE << hashFork;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> hashLastConfirmBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::AddBlockLocalVoteSignFlag(const uint256& hashBlock)
{
    CWriteLock wlock(rwAccess);

    uint256 hashBlockDb;
    if (GetBlockLocalSignFlagDb(CBlock::GetBlockChainIdByHash(hashBlock), CBlock::GetBlockHeightByHash(hashBlock), CBlock::GetBlockSlotByHash(hashBlock), hashBlockDb))
    {
        return hashBlockDb == hashBlock;
    }
    return AddBlockLocalSignFlagDb(hashBlock);
}

bool CBlockIndexDB::GetBlockLocalSignFlag(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock)
{
    CReadLock rlock(rwAccess);
    return GetBlockLocalSignFlagDb(nChainId, nHeight, nSlot, hashBlock);
}

///////////////////////////////////
bool CBlockIndexDB::WriteTrieRoot(const uint8 nRootType, const uint256& hashBlock, const uint256& hashTrieRoot)
{
    mtbase::CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_TRIEROOT << hashBlock;
    ssValue << hashTrieRoot;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CBlockIndexDB::ReadTrieRoot(const uint8 nRootType, const uint256& hashBlock, uint256& hashTrieRoot)
{
    if (hashBlock == 0)
    {
        hashTrieRoot = 0;
        return true;
    }

    mtbase::CBufStream ssKey, ssValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_TRIEROOT << hashBlock;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }

    try
    {
        ssValue >> hashTrieRoot;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

void CBlockIndexDB::AddPrevRoot(const uint8 nRootType, const uint256& hashPrevRoot, const uint256& hashBlock, bytesmap& mapKv)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;

    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_PREVROOT;
    ssKey.GetData(btKey);

    ssValue << hashPrevRoot << hashBlock;
    ssValue.GetData(btValue);

    mapKv.insert(make_pair(btKey, btValue));
}

bool CBlockIndexDB::GetPrevRoot(const uint8 nRootType, const uint256& hashRoot, uint256& hashPrevRoot, uint256& hashBlock)
{
    mtbase::CBufStream ssKey, ssValue;
    bytes btKey, btValue;
    ssKey << nRootType << DB_BLOCKINDEX_KEY_ID_PREVROOT;
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
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockIndexDB::AddBlockLocalSignFlagDb(const uint256& hashBlock)
{
    const CChainId nChainId = CBlock::GetBlockChainIdByHash(hashBlock);
    const uint32 nHeight = CBlock::GetBlockHeightByHash(hashBlock);
    const uint16 nSlot = CBlock::GetBlockSlotByHash(hashBlock);

    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_LOCAL_SIGN_FLAG << nChainId << nHeight << nSlot;
    ssValue << hashBlock;
    return dbTrie.WriteExtKv(ssKey, ssValue);
}

bool CBlockIndexDB::GetBlockLocalSignFlagDb(const CChainId nChainId, const uint32 nHeight, const uint16 nSlot, uint256& hashBlock)
{
    CBufStream ssKey, ssValue;
    ssKey << DB_BLOCKINDEX_KEY_TYPE_BLOCK_LOCAL_SIGN_FLAG << nChainId << nHeight << nSlot;
    if (!dbTrie.ReadExtKv(ssKey, ssValue))
    {
        return false;
    }
    try
    {
        ssValue >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

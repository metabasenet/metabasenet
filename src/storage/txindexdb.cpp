// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txindexdb.h"

#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

const uint8 DB_TXINDEX_KEY_NAME_TXINDEX = 0x01;
const uint8 DB_TXINDEX_KEY_NAME_TXRECEIPT = 0x02;
const uint8 DB_TXINDEX_KEY_NAME_TXPOS = 0x03;

//////////////////////////////
// CForkTxIndexDB

CForkTxIndexDB::CForkTxIndexDB()
{
}

CForkTxIndexDB::~CForkTxIndexDB()
{
}

bool CForkTxIndexDB::Initialize(const uint256& hashForkIn, const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = pathData.string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);
    if (!Open(engine))
    {
        StdLog("CForkTxIndexDB", "Open db fail");
        delete engine;
        return false;
    }
    hashFork = hashForkIn;
    return true;
}

void CForkTxIndexDB::Deinitialize()
{
    Close();
}

bool CForkTxIndexDB::AddBlockTxIndexReceipt(const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts)
{
    CWriteLock wlock(rwAccess);
    if (!TxnBegin())
    {
        return false;
    }

    for (const auto& kv : mapBlockTxIndex)
    {
        mtbase::CBufStream ss;
        ss << hashBlock << kv.first;
        uint256 hash = crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());

        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXINDEX << hash;
        ssValue << kv.second;

        if (!Write(ssKey, ssValue))
        {
            TxnAbort();
            return false;
        }
    }

    for (const auto& receipt : vTxReceipts)
    {
        mtbase::CBufStream ss;
        ss << hashBlock << receipt.txid;
        uint256 hash = crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());

        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXRECEIPT << hash;
        ssValue << receipt;

        if (!Write(ssKey, ssValue))
        {
            TxnAbort();
            return false;
        }
    }

    if (!TxnCommit())
    {
        TxnAbort();
        return false;
    }
    return true;
}

bool CForkTxIndexDB::UpdateBlockLongChain(const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx)
{
    CWriteLock wlock(rwAccess);
    if (!TxnBegin())
    {
        return false;
    }

    for (const auto& txid : vRemoveTx)
    {
        mtbase::CBufStream ssKey;
        ssKey << DB_TXINDEX_KEY_NAME_TXPOS << txid;
        if (!Erase(ssKey))
        {
            TxnAbort();
            return false;
        }
    }

    for (const auto& kv : mapNewTx)
    {
        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXPOS << kv.first;
        ssValue << kv.second;
        if (!Write(ssKey, ssValue))
        {
            TxnAbort();
            return false;
        }
    }

    if (!TxnCommit())
    {
        TxnAbort();
        return false;
    }
    return true;
}

bool CForkTxIndexDB::RetrieveTxIndex(const uint256& txid, CTxIndex& txIndex)
{
    CReadLock rlock(rwAccess);
    uint256 hashBlock;
    try
    {
        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXPOS << txid;
        if (!Read(ssKey, ssValue))
        {
            return false;
        }
        ssValue >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    try
    {
        mtbase::CBufStream ss;
        ss << hashBlock << txid;
        uint256 hash = crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());

        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXINDEX << hash;
        if (!Read(ssKey, ssValue))
        {
            StdLog("CForkTxIndexDB", "Retrieve Tx Index: Read tx id fail, txid: %s, block: %s", txid.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }
        ssValue >> txIndex;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkTxIndexDB::RetrieveTxReceipt(const uint256& txid, CTransactionReceipt& txReceipt)
{
    CReadLock rlock(rwAccess);
    uint256 hashBlock;
    try
    {
        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXPOS << txid;
        if (!Read(ssKey, ssValue))
        {
            return false;
        }
        ssValue >> hashBlock;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    try
    {
        mtbase::CBufStream ss;
        ss << hashBlock << txid;
        uint256 hash = crypto::CryptoKeccakHash(ss.GetData(), ss.GetSize());

        mtbase::CBufStream ssKey, ssValue;
        ssKey << DB_TXINDEX_KEY_NAME_TXRECEIPT << hash;
        if (!Read(ssKey, ssValue))
        {
            return false;
        }
        ssValue >> txReceipt;
    }
    catch (std::exception& e)
    {
        mtbase::StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkTxIndexDB::VerifyTxIndex(const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    return true;
}

//////////////////////////////
// CTxIndexDB

bool CTxIndexDB::Initialize(const boost::filesystem::path& pathData)
{
    pathTxIndex = pathData / "txindex";

    if (!boost::filesystem::exists(pathTxIndex))
    {
        boost::filesystem::create_directories(pathTxIndex);
    }

    if (!boost::filesystem::is_directory(pathTxIndex))
    {
        return false;
    }
    return true;
}

void CTxIndexDB::Deinitialize()
{
    CWriteLock wlock(rwAccess);
    mapTxIndexDB.clear();
}

bool CTxIndexDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return (mapTxIndexDB.find(hashFork) != mapTxIndexDB.end());
}

bool CTxIndexDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return true;
    }

    auto spTxIndex(std::make_shared<CForkTxIndexDB>());
    if (spTxIndex == nullptr)
    {
        return false;
    }
    if (!spTxIndex->Initialize(hashFork, pathTxIndex / hashFork.GetHex()))
    {
        return false;
    }
    mapTxIndexDB.insert(make_pair(hashFork, spTxIndex));
    return true;
}

void CTxIndexDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        it->second->RemoveAll();
        mapTxIndexDB.erase(it);
    }

    boost::filesystem::path forkPath = pathTxIndex / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CTxIndexDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CTxIndexDB::Clear()
{
    CWriteLock wlock(rwAccess);

    auto it = mapTxIndexDB.begin();
    while (it != mapTxIndexDB.end())
    {
        it->second->RemoveAll();
        mapTxIndexDB.erase(it++);
    }
}

bool CTxIndexDB::AddBlockTxIndexReceipt(const uint256& hashFork, const uint256& hashBlock, const std::map<uint256, CTxIndex>& mapBlockTxIndex, const std::vector<CTransactionReceipt>& vTxReceipts)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->AddBlockTxIndexReceipt(hashBlock, mapBlockTxIndex, vTxReceipts);
    }
    return false;
}

bool CTxIndexDB::UpdateBlockLongChain(const uint256& hashFork, const std::vector<uint256>& vRemoveTx, const std::map<uint256, uint256>& mapNewTx)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->UpdateBlockLongChain(vRemoveTx, mapNewTx);
    }
    return false;
}

bool CTxIndexDB::RetrieveTxIndex(const uint256& hashFork, const uint256& txid, CTxIndex& txIndex)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->RetrieveTxIndex(txid, txIndex);
    }
    return false;
}

bool CTxIndexDB::RetrieveTxReceipt(const uint256& hashFork, const uint256& txid, CTransactionReceipt& txReceipt)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->RetrieveTxReceipt(txid, txReceipt);
    }
    return false;
}

bool CTxIndexDB::VerifyTxIndex(const uint256& hashFork, const uint256& hashPrevBlock, const uint256& hashBlock, uint256& hashRoot, const bool fVerifyAllNode)
{
    CReadLock rlock(rwAccess);

    auto it = mapTxIndexDB.find(hashFork);
    if (it != mapTxIndexDB.end())
    {
        return it->second->VerifyTxIndex(hashPrevBlock, hashBlock, hashRoot, fVerifyAllNode);
    }
    return false;
}

} // namespace storage
} // namespace metabasenet

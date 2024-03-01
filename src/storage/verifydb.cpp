// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "verifydb.h"

#include "leveldbeng.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{
namespace storage
{

//////////////////////////////
// CVerifyDB

bool CVerifyDB::Initialize(const boost::filesystem::path& pathData)
{
    if (!tsVerify.Initialize(pathData / "fdb", "verify"))
    {
        StdLog("CVerifyDB", "Initialize: Initialize fail");
        return false;
    }
    if (!LoadVerifyDB())
    {
        StdLog("CVerifyDB", "Initialize: Load verify DB fail");
        return false;
    }
    return true;
}

void CVerifyDB::Deinitialize()
{
    tsVerify.Deinitialize();
    Clear();
}

void CVerifyDB::Clear()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);
    mapVerifyIndex.clear();
    vVerifyIndex.clear();
}

bool CVerifyDB::AddBlockVerify(const CBlockOutline& outline, const uint32 nRootCrc)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);

    auto it = mapVerifyIndex.find(outline.GetBlockHash());
    if (it == mapVerifyIndex.end())
    {
        uint32 nPrevVerifyCrc = 0;
        if (vVerifyIndex.size() > 0)
        {
            nPrevVerifyCrc = vVerifyIndex[vVerifyIndex.size() - 1].GetCrc();
        }

        CBlockVerify verifyBlock(nPrevVerifyCrc, outline, nRootCrc);

        uint32 nFile, nOffset, nCrc;
        if (!tsVerify.Write(verifyBlock, nFile, nOffset, nCrc))
        {
            StdError("CVerifyDB", "Add Block Verify: Write fail, block: %s", outline.GetBlockHash().GetHex().c_str());
            return false;
        }

        vVerifyIndex.push_back(verifyBlock);
        mapVerifyIndex.insert(make_pair(outline.GetBlockHash(), vVerifyIndex.size() - 1));
    }
    else
    {
        if (it->second >= vVerifyIndex.size())
        {
            StdError("CVerifyDB", "Add Block Verify: Index error, index: %ld, size: %ld, block: %s",
                     it->second, vVerifyIndex.size(), outline.GetBlockHash().GetHex().c_str());
            return false;
        }
        if (vVerifyIndex[it->second].nIndexCrc != outline.GetCrc())
        {
            StdError("CVerifyDB", "Add Block Verify: Index crc error, index crc: 0x%8.8x, outline crc: 0x%8.8x, block: %s",
                     vVerifyIndex[it->second].nIndexCrc, outline.GetCrc(), outline.GetBlockHash().GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CVerifyDB::RetrieveBlockVerify(const uint256& hashBlock, CBlockVerify& verifyBlock)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    auto it = mapVerifyIndex.find(hashBlock);
    if (it != mapVerifyIndex.end() && it->second < vVerifyIndex.size())
    {
        verifyBlock = vVerifyIndex[it->second];
        return true;
    }
    return false;
}

std::size_t CVerifyDB::GetBlockVerifyCount()
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    return vVerifyIndex.size();
}

bool CVerifyDB::GetBlockVerify(const std::size_t pos, CBlockVerify& verifyBlock)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    if (pos >= vVerifyIndex.size())
    {
        return false;
    }
    verifyBlock = vVerifyIndex[pos];
    return true;
}

////////////////////////////////////////
bool CVerifyDB::LoadVerifyDB()
{
    class CLoadVerifyDBWalker : public storage::CTSWalker<CBlockVerify>
    {
    public:
        CLoadVerifyDBWalker(std::map<uint256, std::size_t>& mapVerifyIndexIn, std::vector<CBlockVerify>& vVerifyIndexIn)
          : mapVerifyIndex(mapVerifyIndexIn), vVerifyIndex(vVerifyIndexIn) {}

        bool Walk(const CBlockVerify& t, uint32 nFile, uint32 nOffset) override
        {
            if (mapVerifyIndex.count(t.hashBlock) == 0)
            {
                if (vVerifyIndex.size() > 0)
                {
                    if (t.nPrevVerifyCrc != vVerifyIndex[vVerifyIndex.size() - 1].GetCrc())
                    {
                        StdLog("CVerifyDB", "Load Verify DB: Crc error, block: %s", t.hashBlock.GetHex().c_str());
                        return false;
                    }
                }

                vVerifyIndex.push_back(t);
                mapVerifyIndex.insert(make_pair(t.hashBlock, vVerifyIndex.size() - 1));
            }
            return true;
        }

    protected:
        std::map<uint256, std::size_t>& mapVerifyIndex;
        std::vector<CBlockVerify>& vVerifyIndex;
    };

    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);

    CLoadVerifyDBWalker walker(mapVerifyIndex, vVerifyIndex);
    uint32 nLastFile, nLastPos;
    if (!tsVerify.WalkThrough(walker, nLastFile, nLastPos, false))
    {
        StdLog("CVerifyDB", "LoadVerifyDB: Walk through fail, repair file, nLastFile: %d, nLastPos: %d", nLastFile, nLastPos);
        if (!tsVerify.RepairFile(nLastFile, nLastPos))
        {
            StdError("CVerifyDB", "LoadVerifyDB: Repair file fail, nLastFile: %d, nLastPos: %d", nLastFile, nLastPos);
            return false;
        }
    }
    return true;
}

} // namespace storage
} // namespace metabasenet

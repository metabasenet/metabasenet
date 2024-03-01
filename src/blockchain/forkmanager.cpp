// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "forkmanager.h"

#include <boost/range/adaptor/reversed.hpp>

#include "template/fork.h"

using namespace std;
using namespace mtbase;

namespace metabasenet
{

//////////////////////////////
// CForkManager

CForkManager::CForkManager()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    fAllowAnyFork = false;
}

CForkManager::~CForkManager()
{
}

bool CForkManager::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    return true;
}

void CForkManager::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
}

bool CForkManager::HandleInvoke()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);

    fAllowAnyFork = ForkConfig()->fAllowAnyFork;
    if (!fAllowAnyFork)
    {
        setForkAllowed.insert(pCoreProtocol->GetGenesisBlockHash());
        for (const string& strFork : ForkConfig()->vFork)
        {
            uint256 hashFork(strFork);
            if (hashFork != 0)
            {
                setForkAllowed.insert(hashFork);
            }
        }
        for (const string& strFork : ForkConfig()->vGroup)
        {
            uint256 hashFork(strFork);
            if (hashFork != 0)
            {
                setGroupAllowed.insert(hashFork);
            }
        }
    }
    for (const string& strFork : ForkConfig()->vExcludeFork)
    {
        uint256 hashFork(strFork);
        if (hashFork != 0 && hashFork != pCoreProtocol->GetGenesisBlockHash())
        {
            setForkExcluded.insert(hashFork);
        }
    }

    return true;
}

void CForkManager::HandleHalt()
{
    setForkAllowed.clear();
    setGroupAllowed.clear();
    setForkExcluded.clear();
    fAllowAnyFork = false;
}

bool CForkManager::GetActiveFork(std::vector<uint256>& vActive)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!pBlockChain->ListForkContext(mapForkCtxt))
    {
        return false;
    }
    for (const auto& kv : mapForkCtxt)
    {
        const uint256& hashFork = kv.first;
        if (IsAllowedFork(hashFork))
        {
            vActive.push_back(hashFork);
            setCurActiveFork.insert(hashFork);
        }
    }
    return true;
}

bool CForkManager::IsAllowed(const uint256& hashFork) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwAccess);
    return IsAllowedFork(hashFork);
}

void CForkManager::ForkUpdate(const CBlockChainUpdate& update, vector<uint256>& vActive, vector<uint256>& vDeactive)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwAccess);

    std::map<uint256, CForkContext> mapForkCtxt;
    if (!pBlockChain->ListForkContext(mapForkCtxt))
    {
        return;
    }
    std::multimap<uint256, uint256> mapForkParent;
    std::multimap<uint256, uint256> mapForkJoint;
    for (const auto& kv : mapForkCtxt)
    {
        mapForkParent.insert(make_pair(kv.second.hashParent, kv.first));
        mapForkJoint.insert(make_pair(kv.second.hashJoint, kv.first));
    }

    if (mapForkParent.count(update.hashFork) > 0)
    {
        for (const CBlockEx& block : boost::adaptors::reverse(update.vBlockAddNew))
        {
            if (!block.IsExtended() && !block.IsVacant())
            {
                const uint256 hashBlock = block.GetHash();
                auto it = mapForkJoint.lower_bound(hashBlock);
                while (it != mapForkJoint.upper_bound(hashBlock))
                {
                    const uint256& hashFork = it->second;
                    if (IsAllowedFork(hashFork))
                    {
                        vActive.push_back(hashFork);
                    }
                    ++it;
                }
            }
        }
    }

    if (update.hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        for (const CBlockEx& block : boost::adaptors::reverse(update.vBlockAddNew))
        {
            for (size_t i = 0; i < block.vtx.size(); i++)
            {
                const CTransaction& tx = block.vtx[i];

                CAddressContext ctxAddress;
                if (!pBlockChain->GetTxToAddressContext(update.hashFork, block.hashPrev, tx, ctxAddress))
                {
                    continue;
                }

                if (ctxAddress.IsTemplate() && ctxAddress.GetTemplateType() == TEMPLATE_FORK)
                {
                    bytes btTempData;
                    if (!tx.GetTxData(CTransaction::DF_FORKDATA, btTempData))
                    {
                        continue;
                    }
                    CBlock blockOrigin;
                    try
                    {
                        CBufStream ss(btTempData);
                        ss >> blockOrigin;
                    }
                    catch (std::exception& e)
                    {
                        continue;
                    }
                    uint256 hashNewFork = blockOrigin.GetHash();
                    if (mapForkCtxt.count(hashNewFork) > 0)
                    {
                        if (IsAllowedFork(hashNewFork))
                        {
                            vActive.push_back(hashNewFork);
                        }
                    }
                }
            }
        }

        auto it = setCurActiveFork.begin();
        while (it != setCurActiveFork.end())
        {
            auto mt = mapForkCtxt.find(*it);
            if (mt == mapForkCtxt.end())
            {
                vDeactive.push_back(*it);
                setCurActiveFork.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        for (const auto& kv : mapForkCtxt)
        {
            if (setCurActiveFork.count(kv.first) == 0 && IsAllowedFork(kv.first))
            {
                setCurActiveFork.insert(kv.first);
            }
        }
    }
}

//-----------------------------------------------------------------------------------------------
bool CForkManager::IsAllowedFork(const uint256& hashFork) const
{
    if (setForkExcluded.count(hashFork))
    {
        return false;
    }
    if (fAllowAnyFork || setForkAllowed.count(hashFork) || setGroupAllowed.count(hashFork))
    {
        return true;
    }
    if (!setGroupAllowed.empty())
    {
        uint256 hashParent;
        CForkContext ctxtParent;
        if (pBlockChain->GetForkContext(hashFork, ctxtParent))
        {
            hashParent = ctxtParent.hashParent;
        }
        else
        {
            return false;
        }
        if (setGroupAllowed.count(hashParent))
        {
            return true;
        }
        uint256 hash = hashParent;
        while (hash != 0 && pBlockChain->GetForkContext(hash, ctxtParent))
        {
            if (setGroupAllowed.count(ctxtParent.hashParent))
            {
                return true;
            }
            hash = ctxtParent.hashParent;
        }
    }
    return false;
}

} // namespace metabasenet

// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_FORKMANAGER_H
#define METABASENET_FORKMANAGER_H

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <stack>

#include "base.h"
#include "peernet.h"

namespace metabasenet
{

class CForkManager : public IForkManager
{
public:
    CForkManager();
    ~CForkManager();
    bool GetActiveFork(std::vector<uint256>& vActive) override;
    bool IsAllowed(const uint256& hashFork) const override;
    void ForkUpdate(const CBlockChainUpdate& update, std::vector<uint256>& vActive, std::vector<uint256>& vDeactive) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool IsAllowedFork(const uint256& hashFork) const;

protected:
    mutable boost::shared_mutex rwAccess;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;

    bool fAllowAnyFork;
    std::set<uint256> setForkAllowed;
    std::set<uint256> setGroupAllowed;
    std::set<uint256> setForkExcluded;

    std::set<uint256> setCurActiveFork;
};

} // namespace metabasenet

#endif //METABASENET_FORKMANAGER_H

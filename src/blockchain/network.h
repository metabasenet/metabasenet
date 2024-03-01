// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_NETWORK_H
#define METABASENET_NETWORK_H

#include "base.h"
#include "config.h"
#include "peernet.h"

namespace metabasenet
{

class CNetwork : public network::CBbPeerNet
{
public:
    CNetwork();
    ~CNetwork();
    bool CheckPeerVersion(uint32 nVersionIn, uint64 nServiceIn, const std::string& subVersionIn) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    const CNetworkConfig* NetworkConfig()
    {
        return dynamic_cast<const CNetworkConfig*>(mtbase::IBase::Config());
    }
    const CStorageConfig* StorageConfig()
    {
        return dynamic_cast<const CStorageConfig*>(mtbase::IBase::Config());
    }

protected:
    ICoreProtocol* pCoreProtocol;
};

} // namespace metabasenet

#endif //METABASENET_NETWORK_H

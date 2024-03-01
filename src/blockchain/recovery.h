// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_RECOVERY_H
#define METABASENET_RECOVERY_H

#include "base.h"

namespace metabasenet
{

class CRecovery : public IRecovery
{
public:
    CRecovery();
    ~CRecovery();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;

protected:
    IDispatcher* pDispatcher;
};

} // namespace metabasenet
#endif // METABASENET_RECOVERY_H

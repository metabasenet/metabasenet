// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_ENTRY_H
#define METABASENET_ENTRY_H

#include <boost/filesystem.hpp>
#include <mtbase.h>

#include "config.h"

namespace metabasenet
{

class CBbEntry : public mtbase::CEntry
{
public:
    static CBbEntry& GetInstance();

public:
    ~CBbEntry();
    bool Initialize(int argc, char* argv[]);
    bool Run() override;
    void Exit();

protected:
    bool InitializeModules(const EModeType& mode);
    bool AttachModule(mtbase::IBase* pBase);

    bool GetRPCHostConfig(std::vector<mtbase::CHttpHostConfig>& vHostCfg);

    void PurgeStorage();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    mtbase::CLog log;
    mtbase::CDocker docker;

private:
    CBbEntry();
};

} // namespace metabasenet

#endif //METABASENET_ENTRY_H

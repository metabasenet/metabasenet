// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_ENTRY_H
#define METABASENET_ENTRY_H

#include <boost/filesystem.hpp>
#include <hnbase.h>

#include "config.h"

namespace metabasenet
{

class CBbEntry : public hnbase::CEntry
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
    bool AttachModule(hnbase::IBase* pBase);

    hnbase::CHttpHostConfig GetRPCHostConfig();

    void PurgeStorage();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    hnbase::CLog log;
    hnbase::CDocker docker;

private:
    CBbEntry();
};

} // namespace metabasenet

#endif //METABASENET_ENTRY_H

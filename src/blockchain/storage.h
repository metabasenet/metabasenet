// Copyright (c) 2021-2022 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef METABASENET_STORAGE_H
#define METABASENET_STORAGE_H

#include <boost/filesystem.hpp>
#include <hcode.h>

#include "config.h"

namespace metabasenet
{

class CEntry : public hcode::CEntry
{
public:
    CEntry();
    ~CEntry();
    bool Initialize(int argc, char* argv[]);
    bool Run();
    void Exit();

protected:
    bool InitializeService();
    bool InitializeClient();
    //    hcode::CHttpHostConfig GetRPCHostConfig();
    //    hcode::CHttpHostConfig GetWebUIHostConfig();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    hcode::CLog log;
    hcode::CDocker docker;
};

} // namespace metabasenet

#endif //METABASENET_STORAGE_H

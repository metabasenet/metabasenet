// Copyright (c) 2022-2024 The MetabaseNet developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TEST_BIG_BASE_TESTS_H
#define TEST_BIG_BASE_TESTS_H

#include <boost/filesystem.hpp>
#include <string>

#include "util.h"

inline void InitLogDir()
{
    std::string fullpath = boost::filesystem::initial_path<boost::filesystem::path>().string() + std::string("/.testdata/logs");
    if (!mtbase::InitLog(fullpath))
    {
        printf("Init log fail\n");
    }
}

inline std::string GetOutPath(const std::string& strName)
{
    std::string fullpath = boost::filesystem::initial_path<boost::filesystem::path>().string() + std::string("/.testdata/") + strName;
    if (!boost::filesystem::exists(fullpath))
    {
        boost::filesystem::create_directories(fullpath);
    }
    return fullpath;
}

#endif

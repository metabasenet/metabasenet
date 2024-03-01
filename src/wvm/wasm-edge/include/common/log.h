
// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/log.h - Logging system --------------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the linkage of logging system.
///
//===----------------------------------------------------------------------===//
#pragma once
#include <ostream>

#include "util.h"

enum wsm_level
{
    DEBUG,
    ERROR
};

#define LOG(LEVEL) WasmLog(LEVEL)

class WasmLog
{
private:
    enum wsm_level wl;
    std::ostringstream os;

public:
    WasmLog(wsm_level wl_)
      : wl(wl_){};

    template <typename T>
    WasmLog& operator<<(const T& msg)
    {
        os << msg;
        return *this;
    };

    ~WasmLog();
};
# ------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2022-2024 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(sources
    wasmhost.cpp wasmhost.h
    wasmrun.cpp wasmrun.h
)

add_library(mwvm ${sources})

include_directories(../../mtbase ../../crypto ../../common ../../wvm/utils/evmc/include ../vface)

target_link_libraries(mwvm
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_TIMER_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    mtbase
    crypto
    common
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
    aleth-interpreter
)

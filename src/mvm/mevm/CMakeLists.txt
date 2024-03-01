# ------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2022-2024 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(sources
    evmexec.cpp evmexec.h
    evmhost.cpp evmhost.h
    precompiled.cpp precompiled.h
    mutil.cpp mutil.h
)

add_library(mevm ${sources})

include_directories(../../mtbase ../../crypto ../../common ../../meth/evmc/include ../vface)

target_link_libraries(mevm
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_TIMER_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    mtbase
    crypto
    common
    snappy
    devcrypto
    aleth-interpreter
)

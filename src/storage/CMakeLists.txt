# ------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2022-2024 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(sources
    timeseries.cpp timeseries.h
    blockdb.cpp blockdb.h
    blockbase.cpp blockbase.h
    blockindexdb.cpp blockindexdb.h
    walletdb.cpp walletdb.h
    txpooldata.cpp txpooldata.h
    forkdb.cpp forkdb.h
    purger.cpp purger.h
    leveldbeng.cpp leveldbeng.h
    txindexdb.cpp txindexdb.h
    ctsdb.cpp ctsdb.h
    delegatevotesave.cpp delegatevotesave.h
    triedb.cpp triedb.h
    votedb.cpp votedb.h
    statedb.cpp statedb.h
    addressdb.cpp addressdb.h
    addresstxinfodb.cpp addresstxinfodb.h
    hdexdb.cpp hdexdb.h
    matchdex.cpp matchdex.h
    verifydb.cpp verifydb.h
    contractdb.cpp contractdb.h
    addressblacklistdb.cpp addressblacklistdb.h
    cfgmintmingasprice.cpp cfgmintmingasprice.h
)

include_directories(../mtbase ../crypto ../common ../leveldb/include ../snappy ../delegate ../mpvss ../mvm)

add_library(storage ${sources})

target_link_libraries(storage
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
    leveldb
    snappy
    delegate
    mpvss
    #ssvmCommon
    #ssvmEVMCUtilEVMCLoader
    #ssvm-evmc
    mevm
    #mwvm
)

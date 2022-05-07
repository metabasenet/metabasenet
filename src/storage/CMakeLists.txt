#------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2021-2022 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#------------------------------------------------------------------------------

set(sources
    timeseries.cpp      timeseries.h
    blockdb.cpp         blockdb.h
    blockbase.cpp       blockbase.h
    blockindexdb.cpp    blockindexdb.h
    walletdb.cpp        walletdb.h
    txpooldata.cpp      txpooldata.h
    delegatedb.cpp      delegatedb.h
    forkdb.cpp          forkdb.h
    invitedb.cpp        invitedb.h
    purger.cpp          purger.h
    leveldbeng.cpp      leveldbeng.h
    txindexdb.cpp       txindexdb.h
    ctsdb.cpp           ctsdb.h
    delegatevotesave.cpp delegatevotesave.h
    triedb.cpp          triedb.h
    votedb.cpp          votedb.h
    voterewarddb.cpp    voterewarddb.h
    statedb.cpp         statedb.h
    addressdb.cpp       addressdb.h
    addresstxinfodb.cpp addresstxinfodb.h
    codedb.cpp          codedb.h
    verifydb.cpp        verifydb.h
    wasmdb.cpp          wasmdb.h
    wasmhost.cpp        wasmhost.h
    wasmrun.cpp         wasmrun.h
    wasmbase.h
)

add_library(storage ${sources})

include_directories(../hcode ../crypto ../common ../leveldb/include ../snappy ../delegate ../mpvss ../wvm/utils/evmc/include)

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
    hcode
    crypto
    common
    leveldb    
    snappy
    delegate
    mpvss
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
)

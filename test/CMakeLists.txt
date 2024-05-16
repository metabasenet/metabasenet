# ------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2022-2024 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/blockchain)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src/jsonrpc ../src/jsonrpc)
#include_directories(../src/wvm/wasm-edge/include)

set(sources
    base_tests.h
    test_big_main.cpp
    test_big.h test_big.cpp
    uint256_tests.cpp
    rpc_tests.cpp
    version_tests.cpp
    mpvss_tests.cpp
    ipv6_tests.cpp
    crypto_tests.cpp
    delegate_tests.cpp
    eth_test.cpp
    hdexdb_tests.cpp
    storage_tests.cpp
    structure_tests.cpp
    util_tests.cpp
    slowhash_tests.cpp
    triedb_tests.cpp
    bloomfilter_tests.cpp
    blockvote_tests.cpp
    merkletree_tests.cpp
    evmc/example_host.cpp
)

add_executable(test_big ${sources})

#include_directories(../src/blockchain ../src/mtbase ../src/crypto ../src/common ../src/consensus ../src/storage ../src/network ../src/mpvss ../src/delegate ../src/wvm/utils ../src/meth ../src/mvm/mevm ../src/mvm/mwvm ../src/mvm/vface)
include_directories(../src/blockchain ../src/mtbase ../src/crypto ../src/common ../src/consensus ../src/storage ../src/network ../src/mpvss ../src/delegate ../src/meth ../src/mvm/mevm  ../src/mvm/vface)

target_link_libraries(test_big
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    delegate
    crypto
    common
    blockchain
    mtbase
    storage
    devcore
    devcrypto
    ethcore
    #devcommon
    evm
    #ssvmCommon
    #ssvmEVMCUtilEVMCLoader
    #ssvm-evmc
    mevm
    #mwvm
    vface
    consensus
    ${Boost_LOG_LIBRARY}
)

add_executable(test_ctsdb test_big_main.cpp test_big.h test_big.cpp ctsdb_test.cpp)

target_link_libraries(test_ctsdb
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    mpvss
    common
    crypto
    storage
    blockchain
)

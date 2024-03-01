# ------------------------------------------------------------------------------
# CMake file for MetabaseNet
#
# Copyright (c) 2022-2024 The MetabaseNet developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# ------------------------------------------------------------------------------

set(curve25519
    curve25519/base25519.cpp curve25519/base25519.h
    curve25519/fp25519.cpp curve25519/fp25519.h
    curve25519/sc25519.cpp curve25519/sc25519.h
    curve25519/ed25519.cpp curve25519/ed25519.h
    curve25519/curve25519.h
)

set(sources
    uint256.h
    crc24q.cpp crc24q.h
    base32.cpp base32.h
    crypto.cpp crypto.h
    key.cpp key.h
    keystore.cpp keystore.h
    ${curve25519}
)

if(ARM_CRYPTO)
    set(CMAKE_C_FLAGS "-march=armv8-a+crypto")
endif()

add_library(crypto ${sources})

include_directories(crypto ../mtbase ../common ../meth ../wvm/utils ./ ${sodium_INCLUDE_DIR})
link_directories()

if(MSVC)
    set(Crypto_WIN32_LIB "Crypt32.lib")
endif()

target_link_libraries(crypto
    ${Crypto_WIN32_LIB}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${sodium_LIBRARY_RELEASE}
    mtbase
    common
    ssvmCommon
    ssvmEVMCUtilEVMCLoader
    ssvm-evmc
    devcore
    devcrypto
    devcommon
)

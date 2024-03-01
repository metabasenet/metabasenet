# file(GLOB SOURCES "*.cpp")
# file(GLOB HEADERS "*.h")

set(sources

    # AES.cpp AES.h
    Blake2.cpp Blake2.h
    Common.cpp Common.h

    # CryptoPP.cpp CryptoPP.h
    Exceptions.h
    Hash.cpp Hash.h
    LibSnark.cpp LibSnark.h
)

add_library(devcrypto ${sources})

target_include_directories(devcrypto PRIVATE ../)

# target_link_libraries(devcrypto PUBLIC devcore Secp256k1 PRIVATE libff::ff cryptopp-static libscrypt::scrypt)
target_link_libraries(devcrypto PUBLIC devcore Secp256k1 PRIVATE libff::ff)

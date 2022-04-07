#!/bin/bash

origin_path=$(dirname "$0"); pwd
cd "$(dirname "$0")" || exit 1

# cmake
flagdebug='-DCMAKE_BUILD_TYPE=Release'
flagarm64crypto='-DARM_CRYPTO=off'
while [ -n "$1" ]
do
    if [ "$1" = "debug" ]; then
        flagdebug="-DCMAKE_BUILD_TYPE=Debug"
    fi
    if [ "$1" = "arm64crypto" ]; then
        flagarm64crypto="-DARM_CRYPTO=on"
    fi
    shift
done

# create build directory and go to build
if [ $flagdebug == "-DCMAKE_BUILD_TYPE=Release" ]; then
if [ ! -d "build-release/" ]; then
    mkdir build-release
fi
cd build-release || exit 1
else
if [ ! -d "build-debug/" ]; then
    mkdir build-debug
fi
cd build-debug || exit 1
fi

#if ! cmake .. -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang $flagdebug $flagarm64crypto;
if ! cmake .. $flagdebug $flagarm64crypto;
then
    echo "cmake failed"
    cd "$origin_path" || exit 1
fi 

# make & install
os=$(uname)
if [ "$os" == "Darwin" ]; then
    cores=$(sysctl -n hw.logicalcpu)
    if [ "${cores}" == "" ]; then
        cores=1
    fi
    echo "make install -j${cores}"

    if ! make install -j${cores};
    then
        echo "make failed on Mac"
        exit 1 
    fi
else
    cores=$(nproc --all)
    if [ "${cores}" == "" ]; then
        cores=1
    fi
    echo "make -j${cores}"

    if ! make -j${cores};
    then
        echo "make failed on Linux"
        exit 1
    fi 

    echo "sudo make install"
    sudo make install
fi

cd "$origin_path" || exit 1

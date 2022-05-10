cd build

cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=/home/shang/wasm/ssvm-evmc/build/_deps/ssvm-src .. 

#cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=_deps/ssvm-src .. 

cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=_deps/ssvm-src .. 
cmake -DCMAKE_C_COMPILER=clang ../llvm  -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_CXX_COMPILER=clang++ 
make ssvm-evmc ssvmEVMCTest -j8

#SSVM_CORE_PATH set(SSVM_CORE_PATH "_deps/ssvm-src")

# Generate assembly language and generate files with s suffix
#./soll --action=EmitAssembly ../0-0-3.wasm
# Syntax structure of generating AST
#./soll --action=ASTDump ../0-0-3.wasm
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  .. 


#### Tool
Open Ethereum development tool
https://remix.ethereum.org/

download: ganache-2.4.0-linux-x86_64.AppImage
address: https://www.trufflesuite.com/ganache
This tool is the development simulation node of Ethereum

# Call,CallCode,DelegateCall,StaticCall,你分得清吗？
# https://blog.csdn.net/TurkeyCock/article/details/83826531

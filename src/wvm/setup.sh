cd build

cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=/home/shang/wasm/ssvm-evmc/build/_deps/ssvm-src .. 

#cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=_deps/ssvm-src .. 

cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_TOOLS=OFF -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  -DSSVM_CORE_PATH=_deps/ssvm-src .. 
cmake -DCMAKE_C_COMPILER=clang ../llvm  -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_CXX_COMPILER=clang++ 
make ssvm-evmc ssvmEVMCTest -j8

#SSVM_CORE_PATH set(SSVM_CORE_PATH "_deps/ssvm-src")

# 生成汇编语言，生成s后缀的文件
#./soll --action=EmitAssembly ../0-0-3.wasm
# 生成ast的语法结构
#./soll --action=ASTDump ../0-0-3.wasm
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++  -DCMAKE_C_COMPILER=clang  .. 


#### 工具
打开以太坊开发工具
https://remix.ethereum.org/

下载ganache-2.4.0-linux-x86_64.AppImage
地址:https://www.trufflesuite.com/ganache
该工具是以太坊的开发模拟节点

# Call,CallCode,DelegateCall,StaticCall,你分得清吗？
# https://blog.csdn.net/TurkeyCock/article/details/83826531

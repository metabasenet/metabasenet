# SPDX-License-Identifier: Apache-2.0

include_directories(../wasm-edge/include)
include_directories(../wasm-edge/thirdparty)
add_library(ssvmEVMCHostModuleEEI
  eeienv.cpp
  eeifunc.cpp
  eeimodule.cpp
)

target_link_libraries(ssvmEVMCHostModuleEEI
  PRIVATE
  ssvmEVMCUtilKeccak
)

target_include_directories(ssvmEVMCHostModuleEEI
  PUBLIC
  ../include
  ../utils/keccak
  ../utils/evmc/include
  ../../crypto
  ../../mtbase
)

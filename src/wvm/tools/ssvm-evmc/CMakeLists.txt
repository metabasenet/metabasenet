# SPDX-License-Identifier: Apache-2.0

add_library(ssvm-evmc
  evmccreate.cpp
)


target_link_libraries(ssvm-evmc
  PRIVATE
  ssvmVM
  ssvmEVMCHostModuleEEI
)

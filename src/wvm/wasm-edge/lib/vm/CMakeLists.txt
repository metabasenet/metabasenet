# SPDX-License-Identifier: Apache-2.0

add_library(ssvmVM
  vm.cpp
)

target_link_libraries(ssvmVM
  PUBLIC
  ssvmCommon
  ssvmAST
  ssvmLoader
  ssvmValidator
  ssvmInterpreter
  ssvmHostModuleWasi
  ssvmHostModuleSSVMProcess
)


# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleWasi
  wasienv.cpp
  wasifunc.cpp
  wasimodule.cpp
)


target_link_libraries(ssvmHostModuleWasi
  PUBLIC
  Threads::Threads
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_link_libraries(ssvmHostModuleWasi
    PUBLIC
    rt
  )
endif()

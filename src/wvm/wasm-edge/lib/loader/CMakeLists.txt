# SPDX-License-Identifier: Apache-2.0

add_library(ssvmLoaderFileMgr
  filemgr.cpp
  ldmgr.cpp
  shared_library.cpp
)

target_link_libraries(ssvmLoaderFileMgr
  PUBLIC
  dl
  std::filesystem
)

target_link_libraries(ssvmLoaderFileMgr
  PRIVATE
  ssvmCommon
)


add_library(ssvmLoader
  loader.cpp
)

target_link_libraries(ssvmLoader
  PRIVATE
  ssvmAST
  ssvmLoaderFileMgr
  ssvmCommon
  PUBLIC
  std::filesystem
)

include_directories(include)
include_directories(../../mtbase)
include_directories(thirdparty)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(Filesystem REQUIRED Final Experimental)
find_package(Threads REQUIRED)

add_subdirectory(include)
add_subdirectory(lib)

set(sources
    util.h
)

add_library(devcommon ${sources})

target_include_directories(devcommon PRIVATE ../)

target_link_libraries(devcommon devcrypto devcore)

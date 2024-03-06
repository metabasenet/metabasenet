
set(sources
    conscommon.h
    consblockvote.cpp consblockvote.h
)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    msgproto/msgblockvote.proto)

add_library(consensus ${sources} ${PROTO_SRCS} ${PROTO_HDRS})

include_directories(./ ../mtbase ../crypto ../common)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Protobuf_INCLUDE_DIRS})

target_link_libraries(consensus
    mtbase
    crypto
    common
    ${Protobuf_LIBRARIES}
)

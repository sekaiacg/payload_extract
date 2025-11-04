set(TARGET payload)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/payload")

set(TARGET_CFLAGS)

set(libpayload_function_list
    "ftruncate"
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    list(APPEND libpayload_function_list
        "fallocate"
        "fallocate64"
        "ftruncate64"
        "lseek64"
        "pread64"
        "pwrite64"
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND libpayload_function_list "ftruncate64")
endif ()

check_fun(libpayload_function_list)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/libpayload_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

file(GLOB PAYLOAD_COMMON_SRCS "${TARGET_SRC_DIR}/common/*.cpp")
file(GLOB PAYLOAD_DECOMPRESS_SRCS "${TARGET_SRC_DIR}/decompress/*.cpp")
file(GLOB PAYLOAD_CPP_SRCS "${TARGET_SRC_DIR}/*.cpp")
file(GLOB PAYLOAD_CC_SRCS "${TARGET_SRC_DIR}/*.cc")
set(PAYLOAD_SRCS
    ${PAYLOAD_CPP_SRCS}
    ${PAYLOAD_CC_SRCS}
    ${PAYLOAD_COMMON_SRCS}
    ${PAYLOAD_DECOMPRESS_SRCS}
)

add_library(${TARGET} STATIC ${PAYLOAD_SRCS})
target_precompile_headers(${TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/config.h")
target_include_directories(${TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/src/lib/xz/src/liblzma/api")
target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(COMMON_LINK_LIBS bz2_static bspatch_static liblzma libzstd protobuf-cpp-full)
target_link_libraries(${TARGET} PUBLIC ${COMMON_LINK_LIBS})

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

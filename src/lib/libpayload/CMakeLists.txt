set(TARGET payload)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(TARGET_CFLAGS "-Wno-user-defined-warnings")

set(libpayload_function_list "lseek64")
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    list(APPEND libpayload_function_list
        "fallocate"
        "fallocate64"
        "ftruncate64"
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

file(GLOB PAYLOAD_SRCS "${TARGET_SRC_DIR}/*.cpp")
file(GLOB PROTOBUF_SRCS "${TARGET_SRC_DIR}/*.cc")
set(TARGET_SRCS ${PAYLOAD_SRCS} ${PROTOBUF_SRCS})

add_library(${TARGET} STATIC ${TARGET_SRCS})
target_precompile_headers(${TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/config.h")
target_include_directories(${TARGET} PUBLIC ${TARGET_SRC_DIR})
target_include_directories(${TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/src/lib/xz/src/liblzma/api")

set(COMMON_LINK_LIBS bz2_static cpr::cpr liblzma libzstd protobuf-cpp-full)
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    list(APPEND COMMON_LINK_LIBS mbedtls)
endif ()
target_link_libraries(${TARGET} PUBLIC ${COMMON_LINK_LIBS})

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

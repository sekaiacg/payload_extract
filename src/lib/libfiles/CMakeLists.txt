set(TARGET files_static)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/files")

set(TARGET_CFLAGS)

set(libfiles_function_list
    "ftruncate"
)

if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    list(APPEND libfiles_function_list
        "fallocate"
        "fallocate64"
        "ftruncate64"
        "lseek64"
        "pread64"
        "pwrite64"
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND libfiles_function_list "ftruncate64")
endif ()

check_fun(libfiles_function_list)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/libfiles_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libfiles_config.h"
)

file(GLOB FILES_COMMON_SRCS "${TARGET_SRC_DIR}/common/*.cpp")
file(GLOB FILES_CPP_SRCS "${TARGET_SRC_DIR}/*.cpp")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    file(GLOB FILES_MMAN_SRCS "${TARGET_SRC_DIR}/mman/*.c")
endif ()

set(FILES_SRCS
    ${FILES_COMMON_SRCS}
    ${FILES_MMAN_SRCS}
    ${FILES_CPP_SRCS}
)

add_library(${TARGET} STATIC ${FILES_SRCS})
target_precompile_headers(${TARGET} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/libfiles_config.h")
target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

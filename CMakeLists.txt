cmake_minimum_required(VERSION 3.26...4.2.0)
include(cmake/check.cmake)
project(payload_extract LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

option(LIB_USE_MBEDTLS "Using MbedTLS with SHA256 or TSL. default: ON" ON)
option(ENABLE_HTTP_CPR "Enable cpr HTTP download implementation. default: ON" ON)
option(ENABLE_FULL_LTO "Enable full lto. default: OFF" OFF)
option(BUILD_PAYLOAD_EXTRACT "Whether to compile the payload_extract? default: ON" ON)

# File options
add_definitions(
    "-D_FILE_OFFSET_BITS=64"
    "-D_LARGEFILE_SOURCE"
    "-D_LARGEFILE64_SOURCE"
)

# Compile flags
set(GLOBAL_CFLAGS
    "-Wformat"
    "-Werror=format-security"
    "-no-canonical-prefixes"
    "-faddrsig"
    "-fdata-sections"
    "-ffunction-sections"
    "-fvisibility=hidden"
    "-fvisibility-inlines-hidden"
    "-fmessage-length=0"
    "-fstack-protector-strong"
    "-Wno-unused-result"
    "-fno-unwind-tables"
    "-fno-asynchronous-unwind-tables"
)

set(GLOBAL_CFLAGS_RELEASE
    "-O3"
    "-D_FORTIFY_SOURCE=2"
)

# Linker flags
set(GLOBAL_LDFLAGS
    "-fstack-protector-strong"
    "-Qunused-arguments"
)

set(GLOBAL_LDFLAGS_RELEASE)

message(STATUS "[payload_extract] Build type is ${CMAKE_BUILD_TYPE}")

if (LIB_USE_MBEDTLS)
    list(APPEND GLOBAL_CFLAGS "-DLIB_USE_MBEDTLS")
endif ()

# OS link flags
if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    list(APPEND GLOBAL_CFLAGS "-fno-semantic-interposition")
    list(APPEND GLOBAL_LDFLAGS "-Wl,--no-undefined" "-Wl,--build-id=md5")
    list(APPEND GLOBAL_LDFLAGS_RELEASE "-Wl,-s,-x,-w,--gc-sections")
    if (CMAKE_SYSTEM_NAME STREQUAL "Android")
        list(APPEND GLOBAL_CFLAGS "-mllvm" "-polly")
    endif ()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND GLOBAL_LDFLAGS_RELEASE "-Wl,-icf=safe,-s,--gc-sections")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND GLOBAL_LDFLAGS_RELEASE "-Wl,-dead_strip,-S,-x")
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND GLOBAL_CFLAGS "-static")
    list(APPEND GLOBAL_LDFLAGS "-static")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (ENABLE_FULL_LTO)
        if (NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
            list(APPEND GLOBAL_LDFLAGS "-fuse-ld=lld")
            list(APPEND GLOBAL_LDFLAGS_RELEASE "-Wl,-icf=safe,--lto-O3")
        endif ()
        list(APPEND GLOBAL_CFLAGS "-flto=full")
        list(APPEND GLOBAL_LDFLAGS "-flto=full")
        message(STATUS "[payload_extract] Use full-lto")
    endif ()
    list(APPEND GLOBAL_CFLAGS ${GLOBAL_CFLAGS_RELEASE})
    list(APPEND GLOBAL_LDFLAGS ${GLOBAL_LDFLAGS_RELEASE})
    add_definitions("-DNDEBUG" "-UDEBUG")
elseif (CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    list(APPEND GLOBAL_CFLAGS "-O0" "-g" "-DDEBUG")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "i686|x86_64")
    list(APPEND GLOBAL_CFLAGS "-msse4.2" "-msha")
else (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    list(APPEND GLOBAL_CFLAGS "-march=armv8-a+crypto")
endif ()

add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${GLOBAL_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${GLOBAL_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:ASM>:${GLOBAL_CFLAGS}>"
)
add_link_options(
    "$<$<LINK_LANGUAGE:C>:${GLOBAL_LDFLAGS}>"
    "$<$<LINK_LANGUAGE:CXX>:${GLOBAL_LDFLAGS}>"
    "$<$<LINK_LANGUAGE:ASM>:${GLOBAL_LDFLAGS}>"
)

add_subdirectory(src)

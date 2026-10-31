# WolfSSL

execute_process(COMMAND readlink -f "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl/CMakeLists.txt" OUTPUT_VARIABLE PATH_TARGET_FILE)
string(REGEX REPLACE "\n$" "" PATH_TARGET_FILE "${PATH_TARGET_FILE}")
execute_process(COMMAND patch -N -s --no-backup-if-mismatch -r "/dev/null"
    "${PATH_TARGET_FILE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/patch/libwolfssl_CMakeLists.patch"
)
set(WOLFSSL_EXAMPLES OFF)
set(WOLFSSL_INSTALL OFF)
set(WOLFSSL_OPENSSLEXTRA ON)
set(WOLFSSL_ALT_CERT_CHAINS ON)
set(WOLFSSL_SP
    "asm" "ec256" "ec384" "ec521" "ec1024"
    "256" "384" "521" "1024" "2048" "3072" "4096"
)
set(WOLFSSL_SP_MATH ON)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(WOLFSSL_INTEL_ASM ON)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(WOLFSSL_ARMASM "yes")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "loongarch64")
    list(REMOVE_ITEM WOLFSSL_SP "asm")
endif ()
if (NOT CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    set(WOLFSSL_CRYPT_ONLY ON)
endif ()
add_subdirectory("wolfssl")
set(WOLFSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wolfssl" CACHE INTERNAL "WolfSSL Include Dir")
set(WOLFSSL_LIBRARY "wolfssl" CACHE INTERNAL "WolfSSL Library")
set(WOLFSSL_LIBRARIES "wolfssl" CACHE INTERNAL "WolfSSL Libraries")
set(WolfSSL_FOUND TRUE CACHE INTERNAL "")
target_compile_options(wolfssl PRIVATE "-Wno-user-defined-warnings")
if (NOT TARGET WolfSSL::wolfssl)
    add_library(WolfSSL::wolfssl ALIAS wolfssl)
endif ()

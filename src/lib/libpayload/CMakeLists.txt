set(TARGET payload)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/payload")

set(TARGET_CFLAGS)

file(GLOB PAYLOAD_COMMON_SRCS "${TARGET_SRC_DIR}/common/*.cpp")
file(GLOB PAYLOAD_DECOMPRESS_SRCS "${TARGET_SRC_DIR}/decompress/*.cpp")
file(GLOB PAYLOAD_VERIFY_SRCS "${TARGET_SRC_DIR}/verify/*.cpp")
file(GLOB PAYLOAD_CC_SRCS "${TARGET_SRC_DIR}/*.cc")
file(GLOB PAYLOAD_CPP_SRCS "${TARGET_SRC_DIR}/*.cpp")
if (ENABLE_HTTP_CPR)
    file(GLOB PAYLOAD_HTTP_SRCS "${TARGET_SRC_DIR}/httpDownloadImpl/*.cpp")
endif ()
set(PAYLOAD_SRCS
    ${PAYLOAD_COMMON_SRCS}
    ${PAYLOAD_DECOMPRESS_SRCS}
    ${PAYLOAD_VERIFY_SRCS}
    ${PAYLOAD_HTTP_SRCS}
    ${PAYLOAD_CC_SRCS}
    ${PAYLOAD_CPP_SRCS}
)

add_library(${TARGET} STATIC ${PAYLOAD_SRCS})
target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${TARGET}
    PRIVATE
    ${TARGET_SRC_DIR}
    "${PROJECT_SOURCE_DIR}/src/lib/xz/src/liblzma/api"
)

set(COMMON_LINK_LIBS files_static fec_rs_static bz2_static bspatch_static liblzma libzstd protobuf-cpp-full)
if (ENABLE_HTTP_CPR)
    target_compile_definitions(${TARGET} PRIVATE "-DENABLE_HTTP_CPR")
    list(APPEND COMMON_LINK_LIBS cpr::cpr)
    if (LIB_USE_MBEDTLS)
        list(APPEND COMMON_LINK_LIBS MbedTLS::mbedtls MbedTLS::mbedcrypto MbedTLS::mbedx509)
    elseif (LIB_USE_OPENSSL)
        list(APPEND COMMON_LINK_LIBS OpenSSL::SSL OpenSSL::Crypto)
    endif ()
endif ()

if (LIB_USE_MBEDTLS)
    list(APPEND COMMON_LINK_LIBS MbedTLS::mbedcrypto MbedTLS::mbedx509)
else (LIB_USE_OPENSSL)
    target_compile_definitions(${TARGET} PRIVATE "-DLIB_USE_OPENSSL")
    list(APPEND COMMON_LINK_LIBS OpenSSL::Crypto)
endif ()
target_link_libraries(${TARGET} PUBLIC ${COMMON_LINK_LIBS})

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

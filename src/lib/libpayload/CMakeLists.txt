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

set(COMMON_LINK_LIBS
    files_static
    fec_rs_static
    bz2_static
    bspatch_static
    liblzma libzstd
    protobuf-cpp-full
)

if (USE_LIB_WOLFSSL)
    list(APPEND COMMON_LINK_LIBS WolfSSL::wolfssl)
    target_compile_definitions(${TARGET} PRIVATE "LIB_USE_WOLFSSL")
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(${TARGET} PRIVATE "LIB_USE_COMM_CRYPTO")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND COMMON_LINK_LIBS bcrypt)
    target_compile_definitions(${TARGET} PRIVATE "LIB_USE_BCRYPT" "_WIN32_WINNT=0x0A00")
endif ()
if (ENABLE_HTTP_CPR)
    list(APPEND COMMON_LINK_LIBS cpr::cpr)
    target_compile_definitions(${TARGET} PRIVATE "ENABLE_HTTP_CPR")
endif ()
target_link_libraries(${TARGET} PUBLIC ${COMMON_LINK_LIBS})

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

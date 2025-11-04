set(TARGET payload_extract)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(TARGET_CFLAGS
    "-Wno-deprecated-declarations"
)

#file(GLOB EXTRACT_SRCS "*.cpp")

set(TARGET_SRCS
    "${TARGET_SRC_DIR}/ExtractOperation.cpp"
    "${TARGET_SRC_DIR}/main.cpp"
)

if (ENABLE_HTTP_CPR)
    list(APPEND TARGET_SRCS "${TARGET_SRC_DIR}/CprHttpDownload.cpp")
endif ()

add_executable(${TARGET} ${TARGET_SRCS})
target_include_directories(${TARGET} PRIVATE "include")

set(COMMON_LINK_LIBS payload)
if (ENABLE_HTTP_CPR)
    target_compile_definitions(${TARGET} PRIVATE "-DENABLE_HTTP_CPR")
    list(APPEND COMMON_LINK_LIBS cpr::cpr libcurl)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
        list(APPEND COMMON_LINK_LIBS mbedtls)
    endif ()
endif ()
target_link_libraries(${TARGET} PUBLIC ${COMMON_LINK_LIBS})
target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)

if (DEFINED PAYLOAD_EXTRACT_VERSION)
    target_compile_definitions(${TARGET} PRIVATE "-DPAYLOAD_EXTRACT_VERSION=\"${PAYLOAD_EXTRACT_VERSION}\"")
    MESSAGE(STATUS "[payload_extract] version is ${PAYLOAD_EXTRACT_VERSION}")
else ()
    MESSAGE(WARNING "[payload_extract] version is v0.0.0")
endif ()

set(ENV{TZ} UTF-8)
execute_process(
    COMMAND date "+%y%m%d%H%M"
    OUTPUT_VARIABLE PAYLOAD_EXTRACT_BUILD_TIME
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
target_compile_definitions(${TARGET} PRIVATE "-DPAYLOAD_EXTRACT_BUILD_TIME=\"-${PAYLOAD_EXTRACT_BUILD_TIME}\"")
MESSAGE(STATUS "[payload_extract] build time is ${PAYLOAD_EXTRACT_BUILD_TIME}")

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_STRIP} -w "$<TARGET_FILE:${TARGET}>"
    )
endif ()

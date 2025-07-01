set(TARGET payload_extract)

set(TARGET_CFLAGS
    "-Wno-deprecated-declarations"
)

file(GLOB EXTRACT_SRCS "*.cpp")
set(TARGET_SRCS ${EXTRACT_SRCS})

add_executable(${TARGET} ${TARGET_SRCS})
target_include_directories(${TARGET} PRIVATE "include")

target_link_libraries(${TARGET} PRIVATE payload)
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

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_STRIP} -w "$<TARGET_FILE:${TARGET}>"
    )
endif ()

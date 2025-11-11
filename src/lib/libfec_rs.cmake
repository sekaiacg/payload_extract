set(TARGET fec_rs_static)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fec_rs")

set(TARGET_CFLAGS
    "-Wall"
    "-Werror"
    "-O3"
)

set(FEC_RS_SRCS
    "encode_rs_char.c"
    "decode_rs_char.c"
    "init_rs_char.c"
)
list(TRANSFORM FEC_RS_SRCS PREPEND "${TARGET_SRC_DIR}/")

add_library(${TARGET} STATIC ${FEC_RS_SRCS})
target_include_directories(${TARGET} PUBLIC ${TARGET_SRC_DIR})

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)
target_link_options(${TARGET} PRIVATE
    "$<$<LINK_LANGUAGE:C>:${TARGET_LDFLAGS}>"
    "$<$<LINK_LANGUAGE:CXX>:${TARGET_LDFLAGS}>"
)

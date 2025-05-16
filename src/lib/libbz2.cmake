set(TARGET bz2_static)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bzip2")
set(TARGET_CFLAGS
    "-O3"
    "-DUSE_MMAP"
    "-Werror"
    "-Wno-unused-parameter"
)

set(BZ_SRCS
    "blocksort.c"
    "bzlib.c"
    "compress.c"
    "crctable.c"
    "decompress.c"
    "huffman.c"
    "randtable.c"
)
list(TRANSFORM BZ_SRCS PREPEND "${TARGET_SRC_DIR}/")

add_library(${TARGET} STATIC ${BZ_SRCS})
target_link_directories(${TARGET} PRIVATE ${TARGET_SRC_DIR})
target_include_directories(${TARGET}
    PUBLIC $<BUILD_INTERFACE:${TARGET_SRC_DIR}>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)
target_link_options(${TARGET} PRIVATE
    "$<$<LINK_LANGUAGE:C>:${TARGET_LDFLAGS}>"
    "$<$<LINK_LANGUAGE:CXX>:${TARGET_LDFLAGS}>"
)

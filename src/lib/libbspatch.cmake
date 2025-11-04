set(TARGET bspatch_static)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bsdiff")

set(TARGET_CFLAGS
    "-D_FILE_OFFSET_BITS=64"
    "-Wall"
    "-Werror"
    "-Wextra"
    "-Wno-unused-parameter"
)

set(BSPATCH_SRCS
    "brotli_decompressor.cc"
    "bspatch.cc"
    "bz2_decompressor.cc"
    "buffer_file.cc"
    "decompressor_interface.cc"
    "extents.cc"
    "extents_file.cc"
    "file.cc"
    "logging.cc"
    "memory_file.cc"
    "patch_reader.cc"
    "sink_file.cc"
    "utils.cc"
)
list(TRANSFORM BSPATCH_SRCS PREPEND "${TARGET_SRC_DIR}/")

execute_process(COMMAND readlink -f "${TARGET_SRC_DIR}/file.cc" OUTPUT_VARIABLE PATH_TARGET_FILE)
string(REGEX REPLACE "\n$" "" PATH_TARGET_FILE "${PATH_TARGET_FILE}")
execute_process(COMMAND patch -N -f -s --no-backup-if-mismatch -r "/dev/null"
    "${PATH_TARGET_FILE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/patch/libbspatch_file.cc.patch"
)

add_library(${TARGET} STATIC ${BSPATCH_SRCS})
target_link_directories(${TARGET} PRIVATE "${TARGET_SRC_DIR}/include")
target_include_directories(${TARGET}
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC $<BUILD_INTERFACE:${TARGET_SRC_DIR}/include>
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${TARGET} PUBLIC brotlidec bz2_static)

target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${TARGET_CFLAGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${TARGET_CFLAGS}>"
)
target_link_options(${TARGET} PRIVATE
    "$<$<LINK_LANGUAGE:C>:${TARGET_LDFLAGS}>"
    "$<$<LINK_LANGUAGE:CXX>:${TARGET_LDFLAGS}>"
)

set(TARGET_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protobuf")

set(libprotobuf_cflags_defaults
    "-Wall"
    "-Werror"
    "-Wno-missing-field-initializers"
    "-Wno-unused-function"
    "-Wno-unused-parameter"
    "-Wno-error=user-defined-warnings"
    "-Wno-deprecated-declarations"
    "-Wno-deprecated-enum-enum-conversion"
    "-Wno-deprecated-pragma"
    "-Wno-invalid-noreturn"
    "-Wno-macro-redefined"
    "-U__ANDROID__"
)

set(libprotobuf_cpp_lite_srcs
    "src/google/protobuf/any_lite.cc"
    "src/google/protobuf/arena.cc"
    "src/google/protobuf/arenastring.cc"
    "src/google/protobuf/arenaz_sampler.cc"
    "src/google/protobuf/extension_set.cc"
    "src/google/protobuf/generated_enum_util.cc"
    "src/google/protobuf/generated_message_tctable_lite.cc"
    "src/google/protobuf/generated_message_util.cc"
    "src/google/protobuf/implicit_weak_message.cc"
    "src/google/protobuf/inlined_string_field.cc"
    "src/google/protobuf/io/coded_stream.cc"
    "src/google/protobuf/io/io_win32.cc"
    "src/google/protobuf/io/strtod.cc"
    "src/google/protobuf/io/zero_copy_stream.cc"
    "src/google/protobuf/io/zero_copy_stream_impl.cc"
    "src/google/protobuf/io/zero_copy_stream_impl_lite.cc"
    "src/google/protobuf/map.cc"
    "src/google/protobuf/message_lite.cc"
    "src/google/protobuf/parse_context.cc"
    "src/google/protobuf/repeated_field.cc"
    "src/google/protobuf/repeated_ptr_field.cc"
    "src/google/protobuf/stubs/bytestream.cc"
    "src/google/protobuf/stubs/common.cc"
    "src/google/protobuf/stubs/int128.cc"
    "src/google/protobuf/stubs/status.cc"
    "src/google/protobuf/stubs/statusor.cc"
    "src/google/protobuf/stubs/stringpiece.cc"
    "src/google/protobuf/stubs/stringprintf.cc"
    "src/google/protobuf/stubs/structurally_valid.cc"
    "src/google/protobuf/stubs/strutil.cc"
    "src/google/protobuf/stubs/time.cc"
    "src/google/protobuf/wire_format_lite.cc"
)
list(TRANSFORM libprotobuf_cpp_lite_srcs PREPEND "${TARGET_SRC_DIR}/")

set(libprotobuf_cpp_full_common_srcs
    "src/google/protobuf/any.cc"
    "src/google/protobuf/any.pb.cc"
    "src/google/protobuf/api.pb.cc"
    "src/google/protobuf/compiler/importer.cc"
    "src/google/protobuf/compiler/parser.cc"
    "src/google/protobuf/descriptor.cc"
    "src/google/protobuf/descriptor.pb.cc"
    "src/google/protobuf/descriptor_database.cc"
    "src/google/protobuf/duration.pb.cc"
    "src/google/protobuf/dynamic_message.cc"
    "src/google/protobuf/empty.pb.cc"
    "src/google/protobuf/extension_set_heavy.cc"
    "src/google/protobuf/field_mask.pb.cc"
    "src/google/protobuf/generated_message_bases.cc"
    "src/google/protobuf/generated_message_reflection.cc"
    "src/google/protobuf/generated_message_tctable_full.cc"
    "src/google/protobuf/io/gzip_stream.cc"
    "src/google/protobuf/io/printer.cc"
    "src/google/protobuf/io/tokenizer.cc"
    "src/google/protobuf/map_field.cc"
    "src/google/protobuf/message.cc"
    "src/google/protobuf/reflection_ops.cc"
    "src/google/protobuf/service.cc"
    "src/google/protobuf/source_context.pb.cc"
    "src/google/protobuf/struct.pb.cc"
    "src/google/protobuf/stubs/substitute.cc"
    "src/google/protobuf/text_format.cc"
    "src/google/protobuf/timestamp.pb.cc"
    "src/google/protobuf/type.pb.cc"
    "src/google/protobuf/unknown_field_set.cc"
    "src/google/protobuf/util/delimited_message_util.cc"
    "src/google/protobuf/util/field_comparator.cc"
    "src/google/protobuf/util/field_mask_util.cc"
    "src/google/protobuf/util/internal/datapiece.cc"
    "src/google/protobuf/util/internal/default_value_objectwriter.cc"
    "src/google/protobuf/util/internal/error_listener.cc"
    "src/google/protobuf/util/internal/field_mask_utility.cc"
    "src/google/protobuf/util/internal/json_escaping.cc"
    "src/google/protobuf/util/internal/json_objectwriter.cc"
    "src/google/protobuf/util/internal/json_stream_parser.cc"
    "src/google/protobuf/util/internal/object_writer.cc"
    "src/google/protobuf/util/internal/proto_writer.cc"
    "src/google/protobuf/util/internal/protostream_objectsource.cc"
    "src/google/protobuf/util/internal/protostream_objectwriter.cc"
    "src/google/protobuf/util/internal/type_info.cc"
    "src/google/protobuf/util/internal/utility.cc"
    "src/google/protobuf/util/json_util.cc"
    "src/google/protobuf/util/message_differencer.cc"
    "src/google/protobuf/util/time_util.cc"
    "src/google/protobuf/util/type_resolver_util.cc"
    "src/google/protobuf/wire_format.cc"
    "src/google/protobuf/wrappers.pb.cc"
)
list(TRANSFORM libprotobuf_cpp_full_common_srcs PREPEND "${TARGET_SRC_DIR}/")

set(libprotobuf_headers "${TARGET_SRC_DIR}/src")
set(libprotobuf_config "${TARGET_SRC_DIR}/config")

###################################################libprotobuf_cpp_lite###########################################################
set(TARGET protobuf-cpp-lite)
add_library(${TARGET} STATIC ${libprotobuf_cpp_lite_srcs})
target_include_directories(${TARGET} PUBLIC "${libprotobuf_headers}")
target_include_directories(${TARGET} PRIVATE "${libprotobuf_config}")
target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${libprotobuf_cflags_defaults}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${libprotobuf_cflags_defaults}>"
)
###################################################################################################################################

###################################################libprotobuf_cpp_full############################################################
set(TARGET protobuf-cpp-full)
set(libprotobuf_cflags_full ${libprotobuf_cflags_defaults} "-DHAVE_ZLIB=0")
set(libprotobuf_cpp_full_srcs ${libprotobuf_cpp_lite_srcs} ${libprotobuf_cpp_full_common_srcs})
add_library(${TARGET} STATIC ${libprotobuf_cpp_full_srcs})
target_include_directories(${TARGET} PUBLIC ${libprotobuf_headers})
target_include_directories(${TARGET} PRIVATE "${libprotobuf_config}")
target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${libprotobuf_cflags_full}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${libprotobuf_cflags_full}>"
)
###################################################################################################################################

#######################################################libproto####################################################################
set(TARGET protoc)
set(libprotoc_cflags ${libprotobuf_cflags_full} "-Wno-unused-private-field")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND libprotoc_cflags "-UWIN32_LEAN_AND_MEAN")
endif ()

set(libprotoc_common_srcs
    "src/google/protobuf/compiler/code_generator.cc"
    "src/google/protobuf/compiler/command_line_interface.cc"
    "src/google/protobuf/compiler/cpp/enum.cc"
    "src/google/protobuf/compiler/cpp/enum_field.cc"
    "src/google/protobuf/compiler/cpp/extension.cc"
    "src/google/protobuf/compiler/cpp/field.cc"
    "src/google/protobuf/compiler/cpp/file.cc"
    "src/google/protobuf/compiler/cpp/generator.cc"
    "src/google/protobuf/compiler/cpp/helpers.cc"
    "src/google/protobuf/compiler/cpp/map_field.cc"
    "src/google/protobuf/compiler/cpp/message.cc"
    "src/google/protobuf/compiler/cpp/message_field.cc"
    "src/google/protobuf/compiler/cpp/padding_optimizer.cc"
    "src/google/protobuf/compiler/cpp/parse_function_generator.cc"
    "src/google/protobuf/compiler/cpp/primitive_field.cc"
    "src/google/protobuf/compiler/cpp/service.cc"
    "src/google/protobuf/compiler/cpp/string_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_doc_comment.cc"
    "src/google/protobuf/compiler/csharp/csharp_enum.cc"
    "src/google/protobuf/compiler/csharp/csharp_enum_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_field_base.cc"
    "src/google/protobuf/compiler/csharp/csharp_generator.cc"
    "src/google/protobuf/compiler/csharp/csharp_helpers.cc"
    "src/google/protobuf/compiler/csharp/csharp_map_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_message.cc"
    "src/google/protobuf/compiler/csharp/csharp_message_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_primitive_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_reflection_class.cc"
    "src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc"
    "src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc"
    "src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc"
    "src/google/protobuf/compiler/java/context.cc"
    "src/google/protobuf/compiler/java/doc_comment.cc"
    "src/google/protobuf/compiler/java/enum.cc"
    "src/google/protobuf/compiler/java/enum_field.cc"
    "src/google/protobuf/compiler/java/enum_field_lite.cc"
    "src/google/protobuf/compiler/java/enum_lite.cc"
    "src/google/protobuf/compiler/java/extension.cc"
    "src/google/protobuf/compiler/java/extension_lite.cc"
    "src/google/protobuf/compiler/java/field.cc"
    "src/google/protobuf/compiler/java/file.cc"
    "src/google/protobuf/compiler/java/generator.cc"
    "src/google/protobuf/compiler/java/generator_factory.cc"
    "src/google/protobuf/compiler/java/helpers.cc"
    "src/google/protobuf/compiler/java/kotlin_generator.cc"
    "src/google/protobuf/compiler/java/map_field.cc"
    "src/google/protobuf/compiler/java/map_field_lite.cc"
    "src/google/protobuf/compiler/java/message.cc"
    "src/google/protobuf/compiler/java/message_builder.cc"
    "src/google/protobuf/compiler/java/message_builder_lite.cc"
    "src/google/protobuf/compiler/java/message_field.cc"
    "src/google/protobuf/compiler/java/message_field_lite.cc"
    "src/google/protobuf/compiler/java/message_lite.cc"
    "src/google/protobuf/compiler/java/name_resolver.cc"
    "src/google/protobuf/compiler/java/primitive_field.cc"
    "src/google/protobuf/compiler/java/primitive_field_lite.cc"
    "src/google/protobuf/compiler/java/service.cc"
    "src/google/protobuf/compiler/java/shared_code_generator.cc"
    "src/google/protobuf/compiler/java/string_field.cc"
    "src/google/protobuf/compiler/java/string_field_lite.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_enum.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_enum_field.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_extension.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_field.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_file.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_generator.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_helpers.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_map_field.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_message.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_message_field.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_oneof.cc"
    "src/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc"
    "src/google/protobuf/compiler/php/php_generator.cc"
    "src/google/protobuf/compiler/plugin.cc"
    "src/google/protobuf/compiler/plugin.pb.cc"
    "src/google/protobuf/compiler/python/generator.cc"
    "src/google/protobuf/compiler/python/helpers.cc"
    "src/google/protobuf/compiler/python/pyi_generator.cc"
    "src/google/protobuf/compiler/ruby/ruby_generator.cc"
    "src/google/protobuf/compiler/subprocess.cc"
    "src/google/protobuf/compiler/zip_writer.cc"
)
list(TRANSFORM libprotoc_common_srcs PREPEND "${TARGET_SRC_DIR}/")

set(libprotoc_srcs ${libprotobuf_cpp_full_srcs} ${libprotoc_common_srcs})
add_library(${TARGET} STATIC ${libprotoc_srcs})
target_include_directories(${TARGET} PUBLIC ${libprotobuf_headers})
target_include_directories(${TARGET} PRIVATE "${libprotobuf_config}")
target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${libprotoc_cflags}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${libprotoc_cflags}>"
)
###################################################################################################################################

########################################################aprotoc####################################################################
set(TARGET aprotoc)
file(GLOB aprotoc_srcs "${TARGET_SRC_DIR}/src/google/protobuf/compiler/main.cc")
add_executable(${TARGET} ${aprotoc_srcs})
target_include_directories(${TARGET} PRIVATE ${libprotobuf_headers})
target_include_directories(${TARGET} PRIVATE "${libprotobuf_config}")
target_link_libraries(${TARGET} PRIVATE protoc)
target_compile_options(${TARGET} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${libprotoc_cflags}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${libprotoc_cflags}>"
)
###################################################################################################################################

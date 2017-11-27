cmake_minimum_required (VERSION 3.8)

project (libuhs CXX)

set (version 1.0.0-alpha)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

# lib/libuhs.a
set (json_includes ${PROJECT_SOURCE_DIR}/contrib/jsoncpp/json)
add_library (json OBJECT contrib/jsoncpp/jsoncpp.cpp)
target_include_directories (json PUBLIC ${json_includes})

set (uhs_sources
	src/codec.cc
	src/crc.cc
	src/document.cc
	src/element.cc
	src/error.cc
	src/node.cc
	src/parser.cc
	src/pipe.cc
	src/strings.cc
	src/token.cc
	src/tokenizer.cc
	src/traits.cc
	src/writer.cc)

add_library (uhs STATIC $<TARGET_OBJECTS:json> ${uhs_sources})
target_include_directories (uhs PUBLIC ${json_includes} ${PROJECT_SOURCE_DIR}/include)
target_compile_options (uhs PRIVATE -Wall -Wextra -O0 -g -fstandalone-debug)
add_definitions (-DUHS_VERSION="${version}")

# bin/uhs
add_executable (uhs-bin src/cmd/uhs/main.cc)
set_target_properties (uhs-bin PROPERTIES OUTPUT_NAME uhs)
target_link_libraries (uhs-bin PUBLIC uhs)

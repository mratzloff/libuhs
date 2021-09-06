cmake_minimum_required (VERSION 3.8)

project (libuhs CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
list (APPEND CMAKE_MODULE_PATH vendor)

set (argh_includes vendor/argh)
set (json_includes vendor/jsoncpp/json)
set (pugixml_includes vendor/pugixml)
set (tinyformat_includes vendor/tinyformat)
set (uhs_includes include CMakeFiles/CMakeTmp)

set (uhs_sources
	src/codec.cpp
	src/crc.cpp
	src/document.cpp
	src/element.cpp
	src/error.cpp
	src/node.cpp
	src/parser.cpp
	src/pipe.cpp
	src/strings.cpp
	src/token.cpp
	src/tokenizer.cpp
	src/traits.cpp
	src/writer/writer.cpp
	src/writer/html.cpp
	src/writer/json.cpp
	src/writer/tree.cpp
	src/writer/uhs.cpp)

set (uhs_bin_sources src/cmd/uhs/main.cpp)

# js
add_custom_target(js
	COMMAND tsc
	BYPRODUCTS CMakeFiles/CMakeTmp/uhs.js)

# js_includes
add_custom_target(js_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_js_h.cmake
	BYPRODUCTS include/js.h)
add_dependencies (js_includes js)

# json.o
add_library (json OBJECT vendor/jsoncpp/jsoncpp.cpp)
target_include_directories (json PUBLIC ${json_includes})

# pugixml.o
add_library (pugixml OBJECT vendor/pugixml/pugixml.cpp)
target_include_directories (pugixml PUBLIC ${pugixml_includes})

# lib/libuhs.a
add_library (uhs STATIC ${uhs_sources})
add_dependencies (uhs js_includes)
target_link_libraries(uhs json pugixml)
target_include_directories (uhs
	PUBLIC ${json_includes}
		   ${pugixml_includes}
	       ${tinyformat_includes}
	       ${uhs_includes})
target_compile_options (uhs PRIVATE -g -Wall -Wextra)

# bin/uhs
add_executable (uhs_bin src/cmd/uhs/main.cpp)
add_dependencies (uhs_bin js_includes)
set_target_properties (uhs_bin PROPERTIES OUTPUT_NAME uhs)
target_include_directories (uhs_bin PUBLIC ${argh_includes})
target_link_libraries (uhs_bin PUBLIC uhs)

# clang-format
add_custom_target (format
	COMMAND clang-format -style=file -i -verbose
		${uhs_includes}/uhs.h
		${uhs_sources}
		${uhs_bin_sources})

# clang --analyze
add_custom_target (lint
	COMMAND clang
		--analyze
		-Xanalyzer
		-analyzer-output=text
		-I${argh_includes}
		-I${json_includes}
		-I${pugixml_includes}
		-I${tinyformat_includes}
		-I${uhs_includes}
		-std=c++20
		${uhs_sources} ${uhs_bin_sources})
add_dependencies (lint format)

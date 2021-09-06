cmake_minimum_required (VERSION 3.8)

project (libuhs CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY cpp/lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
list (APPEND CMAKE_MODULE_PATH cpp/vendor)

set (argh_includes cpp/vendor/argh)
set (json_includes cpp/vendor/jsoncpp/json)
set (pugixml_includes cpp/vendor/pugixml)
set (tinyformat_includes cpp/vendor/tinyformat)
set (uhs_includes cpp/include)

set (uhs_sources
	cpp/src/codec.cpp
	cpp/src/crc.cpp
	cpp/src/document.cpp
	cpp/src/element.cpp
	cpp/src/error.cpp
	cpp/src/node.cpp
	cpp/src/parser.cpp
	cpp/src/pipe.cpp
	cpp/src/strings.cpp
	cpp/src/token.cpp
	cpp/src/tokenizer.cpp
	cpp/src/traits.cpp
	cpp/src/writer/writer.cpp
	cpp/src/writer/html.cpp
	cpp/src/writer/json.cpp
	cpp/src/writer/tree.cpp
	cpp/src/writer/uhs.cpp)

set (uhs_bin_sources cpp/src/cmd/uhs/main.cpp)

# js
add_custom_target(js
	COMMAND tsc
	BYPRODUCTS ts/build/uhs.js)

# js_includes
add_custom_target(js_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_js_h.cmake
	BYPRODUCTS cpp/include/js.h)
add_dependencies (js_includes js)

# json.o
add_library (json OBJECT cpp/vendor/jsoncpp/jsoncpp.cpp)
target_include_directories (json PUBLIC ${json_includes})

# pugixml.o
add_library (pugixml OBJECT cpp/vendor/pugixml/pugixml.cpp)
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
add_executable (uhs_bin cpp/src/cmd/uhs/main.cpp)
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

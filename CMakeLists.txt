cmake_minimum_required (VERSION 3.8)

project (libuhs CXX)
set (version 1.0.0-alpha)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
list (APPEND CMAKE_MODULE_PATH contrib)

set (argh_includes contrib/argh)
set (json_includes contrib/jsoncpp/json)
set (pugixml_includes contrib/pugixml)
set (tinyformat_includes contrib/tinyformat)
set (uhs_includes include)

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
	src/writer.cpp)

set (uhs_bin_sources src/cmd/uhs/main.cpp)

# json.o
add_library (json OBJECT contrib/jsoncpp/jsoncpp.cpp)
target_include_directories (json PUBLIC ${json_includes})

# pugixml.o
add_library (pugixml OBJECT contrib/pugixml/pugixml.cpp)
target_include_directories (pugixml PUBLIC ${pugixml_includes})

# lib/libuhs.a
add_library (uhs STATIC ${uhs_sources})
target_link_libraries(uhs json pugixml)
target_include_directories (uhs
	PUBLIC ${json_includes}
		   ${pugixml_includes}
	       ${tinyformat_includes}
	       ${uhs_includes})
target_compile_options (uhs PRIVATE -g -Wall -Wextra)
add_definitions (-DUHS_VERSION="${version}")

# bin/uhs
add_executable (uhs-bin src/cmd/uhs/main.cpp)
set_target_properties (uhs-bin PROPERTIES OUTPUT_NAME uhs)
target_include_directories (uhs-bin PUBLIC ${argh_includes})
target_link_libraries (uhs-bin PUBLIC uhs)

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
		-DUHS_VERSION="${version}"
		${uhs_sources} ${uhs_bin_sources})
add_dependencies (lint format)

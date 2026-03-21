cmake_minimum_required (VERSION 4.0)
cmake_policy (SET CMP0135 NEW)

project (libuhs CXX C)

include (ExternalProject)
include (FetchContent)

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_C_STANDARD 23)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
list (APPEND CMAKE_MODULE_PATH cpp/vendor)

# Fetch dependencies
FetchContent_Declare (argh_dep
	GIT_REPOSITORY https://github.com/adishavit/argh
	GIT_TAG c3f0d8c8a6dacb00df626b409248a34e3bcd15f5)
FetchContent_MakeAvailable (argh_dep)

FetchContent_Declare (catch2_dep
	GIT_REPOSITORY https://github.com/catchorg/Catch2
	GIT_TAG v3.13.0)
FetchContent_MakeAvailable (catch2_dep)

FetchContent_Declare (hopscotch_map_dep
	GIT_REPOSITORY https://github.com/Tessil/hopscotch-map
	GIT_TAG 799f43600f9ca65a3045decbc7571b5522fa21b7)
FetchContent_MakeAvailable (hopscotch_map_dep)

FetchContent_Declare (httplib_dep
	GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
	GIT_TAG d5409ab541ecc6f4f3df7558a829c5dc522ecf7e)
FetchContent_MakeAvailable (httplib_dep)

set (jsoncpp_prefix ${FETCHCONTENT_BASE_DIR}/jsoncpp_dep)

ExternalProject_Add (jsoncpp_dep
	GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp
	GIT_TAG 1.9.6
	PREFIX ${jsoncpp_prefix}
	CMAKE_CACHE_ARGS
		-DBUILD_SHARED_LIBS:BOOL=OFF
		-DBUILD_OBJECT_LIBS:BOOL=OFF
		-DJSONCPP_WITH_CMAKE_PACKAGE:BOOL=OFF
		-DJSONCPP_WITH_POST_BUILD_UNITTEST:BOOL=OFF
		-DJSONCPP_WITH_PKGCONFIG_SUPPORT:BOOL=OFF
		-DJSONCPP_WITH_TESTS:BOOL=OFF
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
	LOG_UPDATE ON
	LOG_CONFIGURE ON
	LOG_BUILD ON
	LOG_TEST ON
	LOG_MERGED_STDOUTERR ON
	LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Get_Property (jsoncpp_dep SOURCE_DIR BINARY_DIR)
set (jsoncpp_dep_SOURCE_DIR ${SOURCE_DIR})
set (jsoncpp_dep_BINARY_DIR ${BINARY_DIR})

FetchContent_Declare (zlib_dep
	GIT_REPOSITORY https://github.com/madler/zlib
	GIT_TAG 5a82f71ed1dfc0bec044d9702463dbdf84ea3b71)
FetchContent_MakeAvailable (zlib_dep)

set (puff_dep_SOURCE_DIR ${zlib_dep_SOURCE_DIR}/contrib/puff)

FetchContent_GetProperties (zlib_dep)
if (zlib_dep_POPULATED)
	add_library (puff OBJECT ${puff_dep_SOURCE_DIR}/puff.c)
endif ()

FetchContent_Declare (pugixml_dep
	GIT_REPOSITORY https://github.com/zeux/pugixml
	GIT_TAG 5a1892b321a38997e3e359069aea3fa35d49a5c6)
FetchContent_MakeAvailable (pugixml_dep)

set (tinyformat_prefix ${FETCHCONTENT_BASE_DIR}/tinyformat_dep)

ExternalProject_Add (tinyformat_dep
	GIT_REPOSITORY https://github.com/mratzloff/tinyformat
	GIT_TAG 9f6032683b2fb900a3de456f39a61da684704e8e
	PREFIX ${tinyformat_prefix}
	CMAKE_CACHE_ARGS -DBUILD_TESTS:BOOL=OFF
	LOG_DOWNLOAD ON
	LOG_UPDATE ON
	LOG_PATCH ON
	LOG_CONFIGURE ON
	LOG_INSTALL ON
	LOG_TEST ON
	LOG_MERGED_STDOUTERR ON
	LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Get_Property (tinyformat_dep SOURCE_DIR)
set (tinyformat_dep_SOURCE_DIR ${SOURCE_DIR})

FetchContent_Declare (tinyutf8_dep
	GIT_REPOSITORY https://github.com/mratzloff/tiny-utf8
	GIT_TAG e8ebed5b54f2ae80598228013f4044fd8bf8ce62)
FetchContent_MakeAvailable (tinyutf8_dep)

# Define dependency path variables
set (argh_includes ${argh_dep_SOURCE_DIR})
set (catch2_build_includes ${catch2_dep_BINARY_DIR}/generated-includes)
set (catch2_includes ${catch2_dep_SOURCE_DIR}/src)
set (hopscotch_map_includes ${hopscotch_map_dep_SOURCE_DIR}/include)
set (httplib_includes ${httplib_dep_SOURCE_DIR})
set (jsoncpp_includes ${jsoncpp_dep_SOURCE_DIR}/include)
set (jsoncpp_lib ${jsoncpp_dep_BINARY_DIR}/lib)
set (puff_includes ${puff_dep_SOURCE_DIR})
set (pugixml_includes ${pugixml_dep_SOURCE_DIR}/src)
set (tinyformat_includes ${tinyformat_dep_SOURCE_DIR})
set (tinyutf8_includes ${tinyutf8_dep_SOURCE_DIR}/include)

# Define project path variables
set (bin node_modules/.bin)
set (uhs_build_includes build/include)
set (uhs_includes include)

set (uhs_sources
	cpp/src/bridge.cpp
	cpp/src/codec.cpp
	cpp/src/crc.cpp
	cpp/src/document.cpp
	cpp/src/downloader.cpp
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
	cpp/src/writer/html/table.cpp
	cpp/src/writer/json.cpp
	cpp/src/writer/tree.cpp
	cpp/src/writer/uhs.cpp
	cpp/src/uhs.cpp
	cpp/src/zip.cpp)

set (uhs_bin_sources cpp/src/cmd/uhs/main.cpp)

set (uhs_test_sources
	cpp/test/test_codec.cpp
	cpp/test/test_crc.cpp
	cpp/test/test_document.cpp
	cpp/test/test_element.cpp
	cpp/test/test_error.cpp
	cpp/test/test_node.cpp
	cpp/test/test_pipe.cpp
	cpp/test/test_strings.cpp
	cpp/test/test_text_format.cpp
	cpp/test/test_token.cpp
	cpp/test/test_tokenizer.cpp
	cpp/test/test_traits.cpp
	cpp/test/test_writer_html.cpp
	cpp/test/test_writer_html_table.cpp
	cpp/test/test_writer_json.cpp
	cpp/test/test_writer_tree.cpp
	cpp/test/test_writer_uhs.cpp
	cpp/test/test_zip.cpp)

# Create build directories
file (MAKE_DIRECTORY build/assets)
file (MAKE_DIRECTORY build/include)

# yarn
add_custom_target (yarn COMMAND yarn)

# css_minified
add_custom_target (css_minified
	COMMAND ${bin}/minify cpp/assets/css/uhs.css > build/assets/uhs.min.css
	BYPRODUCTS build/assets/uhs.min.css)
add_dependencies (css_minified yarn)

# css_includes
add_custom_target (css_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_css_h.cmake
	BYPRODUCTS ${uhs_build_includes}/css.h)
add_dependencies (css_includes css_minified)

# js
add_custom_target (js
	COMMAND ${bin}/tsc
	BYPRODUCTS
		build/ts/constants.js
		build/ts/history.js
		build/ts/search.js
		build/ts/viewport.js)
add_dependencies (js yarn)

# js_combined
add_custom_target (js_combined
	COMMAND ${bin}/ncc build ts/src/index.ts --out build/ts --no-source-map-register --quiet
	BYPRODUCTS build/ts/index.js)
add_dependencies (js_combined js)

# js_minified
add_custom_target (js_minified
	COMMAND ${bin}/minify build/ts/index.js > build/ts/index.min.js
	BYPRODUCTS build/ts/index.min.js)
add_dependencies (js_minified js_combined)

# js_includes
add_custom_target (js_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_js_h.cmake
	BYPRODUCTS ${uhs_build_includes}/js.h)
add_dependencies (js_includes js_minified)

# lib/libuhs.a
add_library (uhs STATIC ${uhs_sources})
add_dependencies (uhs css_includes js_includes)
link_directories (
	${jsoncpp_lib}
	${CMAKE_SOURCE_DIR}/lib)
target_link_libraries (uhs
	PUBLIC
		jsoncpp
		puff
		pugixml)
target_include_directories (uhs
	PUBLIC
		${hopscotch_map_includes}
		${httplib_includes}
		${jsoncpp_includes}
		${puff_includes}
		${pugixml_includes}
		${tinyformat_includes}
		${tinyutf8_includes}
		${uhs_build_includes}
		${uhs_includes})
target_compile_options (uhs PRIVATE -g -O3 -Wall -Wextra)

# bin/uhs
add_executable (uhs_bin cpp/src/cmd/uhs/main.cpp)
add_dependencies (uhs_bin css_includes js_includes)
set_target_properties (uhs_bin
	PROPERTIES
		LINKER_LANGUAGE CXX
		OUTPUT_NAME uhs)
target_include_directories (uhs_bin PRIVATE ${argh_includes})
target_link_libraries (uhs_bin PRIVATE uhs)

# bin/uhsc
add_executable (uhsc_bin c/src/cmd/uhsc/main.c)
set_target_properties (uhsc_bin
	PROPERTIES
		LINKER_LANGUAGE C
		OUTPUT_NAME uhsc)
target_link_libraries (uhsc_bin PRIVATE uhs)

# make clean-all
add_custom_target (clean-all
	COMMAND rm -f *.cmake
	COMMAND rm -rf build
	COMMAND rm -rf _deps
	COMMAND rm -f CMakeCache.txt
	COMMAND rm -rf CMakeFiles
	COMMAND rm -rf lib
	COMMAND rm -f Makefile
	COMMAND rm -rf node_modules)

# make format
add_custom_target (format
	COMMAND clang-format -style=file -i -verbose
		${uhs_includes}/uhs.h
		${uhs_includes}/uhs_bridge.h
		${uhs_sources}
		${uhs_bin_sources}
		${uhs_test_sources})

# make lint
add_custom_target (lint
	COMMAND clang
		--analyze
		# -Xanalyzer -analyzer-display-progress
		-Xclang -analyzer-output=text
		-I${argh_includes}
		-I${catch2_build_includes}
		-I${catch2_includes}
		-I${hopscotch_map_includes}
		-I${httplib_includes}
		-I${jsoncpp_includes}
		-I${puff_includes}
		-I${pugixml_includes}
		-I${tinyformat_includes}
		-I${tinyutf8_includes}
		-I${uhs_build_includes}
		-I${uhs_includes}
		-std=c++23
		${uhs_sources}
		${uhs_bin_sources}
		${uhs_test_sources})
add_dependencies (lint css_includes js_includes format)

# make test
add_executable (uhs_test EXCLUDE_FROM_ALL ${uhs_test_sources})
target_include_directories (uhs_test PRIVATE ${catch2_build_includes} ${catch2_includes})
target_link_libraries (uhs_test PRIVATE uhs Catch2::Catch2WithMain)

add_custom_target (test
	COMMAND uhs_test
		--invisibles
		--warn NoAssertions
		--warn UnmatchedTestSpec
		--warn InfiniteGenerators
	DEPENDS uhs_test)

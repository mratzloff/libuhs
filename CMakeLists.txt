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
	GIT_REPOSITORY https://github.com/mratzloff/argh
	GIT_TAG 06ad8d13ca0ae7384a01b48246b952d638eb85cd)
FetchContent_MakeAvailable (argh_dep)

FetchContent_Declare (catch2_dep
	GIT_REPOSITORY https://github.com/catchorg/Catch2
	GIT_TAG v3.13.0)
FetchContent_MakeAvailable (catch2_dep)

FetchContent_Declare (hopscotch_map_dep
	GIT_REPOSITORY https://github.com/Tessil/hopscotch-map
	GIT_TAG v2.4.0)
FetchContent_MakeAvailable (hopscotch_map_dep)

FetchContent_Declare (httplib_dep
	GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
	GIT_TAG v0.31.0)
FetchContent_MakeAvailable (httplib_dep)

set (external_project_platform_args)
if (CMAKE_MSVC_RUNTIME_LIBRARY)
	list (APPEND external_project_platform_args
		-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${CMAKE_MSVC_RUNTIME_LIBRARY})
endif ()
if (CMAKE_OSX_DEPLOYMENT_TARGET)
	list (APPEND external_project_platform_args
		-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif ()
if (CMAKE_TOOLCHAIN_FILE)
	list (APPEND external_project_platform_args
		-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE})
endif ()

set (jsoncpp_prefix ${FETCHCONTENT_BASE_DIR}/jsoncpp_dep)

set (jsoncpp_cache_args
	-DBUILD_OBJECT_LIBS:BOOL=OFF
	-DBUILD_SHARED_LIBS:BOOL=OFF
	-DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
	-DJSONCPP_WITH_CMAKE_PACKAGE:BOOL=OFF
	-DJSONCPP_WITH_PKGCONFIG_SUPPORT:BOOL=OFF
	-DJSONCPP_WITH_POST_BUILD_UNITTEST:BOOL=OFF
	-DJSONCPP_WITH_TESTS:BOOL=OFF
	${external_project_platform_args})

ExternalProject_Add (jsoncpp_dep
	GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp
	GIT_TAG 1.9.7
	PREFIX ${jsoncpp_prefix}
	CMAKE_CACHE_ARGS ${jsoncpp_cache_args}
	INSTALL_COMMAND ""
	LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Get_Property (jsoncpp_dep SOURCE_DIR BINARY_DIR)
set (jsoncpp_dep_SOURCE_DIR ${SOURCE_DIR})
set (jsoncpp_dep_BINARY_DIR ${BINARY_DIR})

set (ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set (ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare (mbedtls_dep
	GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls
	GIT_TAG v3.6.5)
FetchContent_MakeAvailable (mbedtls_dep)

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

set (tinyformat_cache_args
	-DBUILD_TESTS:BOOL=OFF
	${external_project_platform_args})

ExternalProject_Add (tinyformat_dep
	GIT_REPOSITORY https://github.com/mratzloff/tinyformat
	GIT_TAG 9f6032683b2fb900a3de456f39a61da684704e8e
	PREFIX ${tinyformat_prefix}
	CMAKE_CACHE_ARGS ${tinyformat_cache_args}
	LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Get_Property (tinyformat_dep SOURCE_DIR)
set (tinyformat_dep_SOURCE_DIR ${SOURCE_DIR})

FetchContent_Declare (tinyutf8_dep
	GIT_REPOSITORY https://github.com/mratzloff/tiny-utf8
	GIT_TAG v4.5.0)
FetchContent_MakeAvailable (tinyutf8_dep)

# Define dependency path variables
set (argh_includes ${argh_dep_SOURCE_DIR})
set (catch2_build_includes ${catch2_dep_BINARY_DIR}/generated-includes)
set (catch2_includes ${catch2_dep_SOURCE_DIR}/src)
set (hopscotch_map_includes ${hopscotch_map_dep_SOURCE_DIR}/include)
set (httplib_includes ${httplib_dep_SOURCE_DIR})
set (jsoncpp_includes ${jsoncpp_dep_SOURCE_DIR}/include)
set (jsoncpp_lib ${jsoncpp_dep_BINARY_DIR}/lib)
set (mbedtls_includes ${mbedtls_dep_SOURCE_DIR}/include)
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
	cpp/src/error/http_error.cpp
	cpp/src/error/parse_error.cpp
	cpp/src/node_iterator.cpp
	cpp/src/node.cpp
	cpp/src/node/break_node.cpp
	cpp/src/node/container_node.cpp
	cpp/src/node/group_node.cpp
	cpp/src/node/text_node.cpp
	cpp/src/parser.cpp
	cpp/src/pipe.cpp
	cpp/src/regex.cpp
	cpp/src/strings.cpp
	cpp/src/token.cpp
	cpp/src/tokenizer.cpp
	cpp/src/traits/attributes.cpp
	cpp/src/traits/body.cpp
	cpp/src/traits/inlined.cpp
	cpp/src/traits/title.cpp
	cpp/src/traits/visibility.cpp
	cpp/src/uhs.cpp
	cpp/src/writer.cpp
	cpp/src/writer/html_writer.cpp
	cpp/src/writer/html_writer/table.cpp
	cpp/src/writer/json_writer.cpp
	cpp/src/writer/tree_writer.cpp
	cpp/src/writer/uhs_writer.cpp
	cpp/src/zip.cpp)

set (uhs_bin_sources cpp/src/cmd/uhs/main.cpp)

set (uhs_test_sources
	cpp/test/integration/builders.cpp
	cpp/test/integration/helpers.cpp
	cpp/test/integration/test_formatting.cpp
	cpp/test/integration/test_media.cpp
	cpp/test/integration/test_metadata.cpp
	cpp/test/integration/test_structure.cpp
	cpp/test/integration/test_version.cpp
	cpp/test/unit/error/test_parse_error.cpp
	cpp/test/unit/node/test_break_node.cpp
	cpp/test/unit/node/test_group_node.cpp
	cpp/test/unit/node/test_text_node.cpp
	cpp/test/unit/test_bridge.cpp
	cpp/test/unit/test_codec.cpp
	cpp/test/unit/test_crc.cpp
	cpp/test/unit/test_document.cpp
	cpp/test/unit/test_downloader.cpp
	cpp/test/unit/test_element.cpp
	cpp/test/unit/test_error.cpp
	cpp/test/unit/test_node.cpp
	cpp/test/unit/test_node_iterator.cpp
	cpp/test/unit/test_parser.cpp
	cpp/test/unit/test_pipe.cpp
	cpp/test/unit/test_strings.cpp
	cpp/test/unit/test_text_format.cpp
	cpp/test/unit/test_token.cpp
	cpp/test/unit/test_tokenizer.cpp
	cpp/test/unit/test_uhs.cpp
	cpp/test/unit/test_zip.cpp
	cpp/test/unit/traits/test_attributes.cpp
	cpp/test/unit/traits/test_body.cpp
	cpp/test/unit/traits/test_inlined.cpp
	cpp/test/unit/traits/test_title.cpp
	cpp/test/unit/traits/test_visibility.cpp
	cpp/test/unit/writer/html_writer/test_table.cpp
	cpp/test/unit/writer/test_html_writer.cpp
	cpp/test/unit/writer/test_json_writer.cpp
	cpp/test/unit/writer/test_tree_writer.cpp
	cpp/test/unit/writer/test_uhs_writer.cpp)

# Create build directories
file (MAKE_DIRECTORY build/assets)
file (MAKE_DIRECTORY build/include)

# yarn
add_custom_target (yarn COMMAND yarn)

# css_minified
add_custom_target (css_minified
	COMMAND ${bin}/lightningcss --minify assets/css/uhs.css -o build/assets/uhs.min.css
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
add_dependencies (uhs css_includes js_includes jsoncpp_dep tinyformat_dep)
link_directories (
	${jsoncpp_lib}
	${CMAKE_SOURCE_DIR}/lib)
target_compile_definitions (uhs PRIVATE CPPHTTPLIB_MBEDTLS_SUPPORT)
target_link_libraries (uhs
	PUBLIC
		jsoncpp
		MbedTLS::mbedcrypto
		MbedTLS::mbedtls
		MbedTLS::mbedx509
		puff
		pugixml)
if (APPLE)
	target_compile_definitions (uhs PRIVATE CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)
	target_link_libraries (uhs PUBLIC
		"-framework CoreFoundation"
		"-framework Security")
elseif (WIN32)
	target_link_libraries (uhs PUBLIC crypt32)
endif ()
target_include_directories (uhs
	PUBLIC
		${hopscotch_map_includes}
		${httplib_includes}
		${jsoncpp_includes}
		${mbedtls_includes}
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
add_custom_target (format_cpp
	COMMAND clang-format -style=file -i -verbose
		${uhs_includes}/uhs.h
		${uhs_includes}/uhs_bridge.h
		${uhs_sources}
		${uhs_bin_sources}
		${uhs_test_sources})

add_custom_target (format_css
	COMMAND yarn prettier --write assets/css/uhs.css)

add_custom_target (format_ts
	COMMAND yarn prettier --write ts/src/**/*.ts ts/test/**/*.ts
	COMMAND yarn eslint --fix ts/src/ ts/test/)

add_custom_target (format)
add_dependencies (format format_css format_ts format_cpp)

# make lint
set (lint_defines -DCPPHTTPLIB_MBEDTLS_SUPPORT)
if (APPLE)
	list (APPEND lint_defines -DCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)
endif ()

add_custom_target (lint_cpp
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
		-I${mbedtls_includes}
		-I${puff_includes}
		-I${pugixml_includes}
		-I${tinyformat_includes}
		-I${tinyutf8_includes}
		-I${uhs_build_includes}
		-I${uhs_includes}
		${lint_defines}
		-std=c++23
		${uhs_sources}
		${uhs_bin_sources}
		${uhs_test_sources})
add_dependencies (lint_cpp css_includes js_includes)

add_custom_target (lint_css
	COMMAND yarn stylelint assets/css/uhs.css)

add_custom_target (lint_ts
	COMMAND yarn eslint ts/src/ ts/test/)

add_custom_target (lint)
add_dependencies (lint format lint_cpp lint_css lint_ts)

# make test
add_executable (uhs_test EXCLUDE_FROM_ALL ${uhs_test_sources})
target_include_directories (uhs_test PRIVATE ${catch2_build_includes} ${catch2_includes})
target_link_libraries (uhs_test PRIVATE uhs Catch2::Catch2WithMain)

add_custom_target (test_cpp
	COMMAND uhs_test
		--invisibles
		--warn NoAssertions
		--warn UnmatchedTestSpec
		--warn InfiniteGenerators
	DEPENDS uhs_test)

add_custom_target (test_ts
	COMMAND yarn vitest run)

add_custom_target (test)
add_dependencies (test test_cpp test_ts)

# make integrations
add_executable (uhs_integrations EXCLUDE_FROM_ALL
	cpp/test/integration/builders.cpp
	cpp/test/integration/helpers.cpp
	cpp/test/cmd/uhs_integrations/main.cpp)
target_link_libraries (uhs_integrations PRIVATE uhs)

add_custom_target (integrations
	COMMAND uhs_integrations
	DEPENDS uhs_integrations)

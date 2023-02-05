cmake_minimum_required (VERSION 3.8)

project (libuhs CXX C)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
list (APPEND CMAKE_MODULE_PATH cpp/vendor)

set (argh_includes cpp/vendor/argh)
set (bin node_modules/.bin)
set (httplib_includes cpp/vendor/httplib)
set (json_includes cpp/vendor/jsoncpp/json)
set (puff_includes cpp/vendor/puff)
set (pugixml_includes cpp/vendor/pugixml)
set (tinyformat_includes cpp/vendor/tinyformat)
set (tinyutf8_includes cpp/vendor/tinyutf8)
set (tsl_includes cpp/vendor/tsl)
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
	cpp/src/writer/json.cpp
	cpp/src/writer/tree.cpp
	cpp/src/writer/uhs.cpp
	cpp/src/uhs.cpp
	cpp/src/zip.cpp)

set (uhs_bin_sources cpp/src/cmd/uhs/main.cpp)

file (MAKE_DIRECTORY build/assets)
file (MAKE_DIRECTORY build/include)

# css_minified
add_custom_target(css_minified
	COMMAND ${bin}/minify cpp/assets/css/uhs.css > build/assets/uhs.min.css
	BYPRODUCTS build/assets/uhs.min.css)

# css_includes
add_custom_target(css_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_css_h.cmake
	BYPRODUCTS ${uhs_build_includes}/css.h)
add_dependencies (css_includes css_minified)

# js
add_custom_target(js
	COMMAND ${bin}/tsc
	BYPRODUCTS build/ts/constants.js
	           build/ts/history.js
	           build/ts/search.js
	           build/ts/viewport.js)

# js_combined
add_custom_target(js_combined
	COMMAND ${bin}/ncc build ts/src/index.ts --out build/ts --no-source-map-register --quiet
	BYPRODUCTS build/ts/index.js)
add_dependencies (js_combined js)

# js_minified
add_custom_target(js_minified
	COMMAND ${bin}/minify build/ts/index.js > build/ts/index.min.js
	BYPRODUCTS build/ts/index.min.js)
add_dependencies (js_minified js_combined)

# js_includes
add_custom_target(js_includes
	COMMAND ${CMAKE_COMMAND} -P cmake/create_js_h.cmake
	BYPRODUCTS ${uhs_build_includes}/js.h)
add_dependencies (js_includes js_minified)

# json.o
add_library (json OBJECT cpp/vendor/jsoncpp/jsoncpp.cpp)
target_include_directories (json PUBLIC ${json_includes})

# puff.o
add_library (puff OBJECT cpp/vendor/puff/puff.c)
target_include_directories (puff PUBLIC ${puff_includes})

# pugixml.o
add_library (pugixml OBJECT cpp/vendor/pugixml/pugixml.cpp)
target_include_directories (pugixml PUBLIC ${pugixml_includes})

# lib/libuhs.a
add_library (uhs STATIC ${uhs_sources})
add_dependencies (uhs css_includes js_includes)
target_link_libraries(uhs json pugixml puff)
target_include_directories (uhs
	PUBLIC ${json_includes}
	       ${httplib_includes}
	       ${puff_includes}
	       ${pugixml_includes}
	       ${tinyformat_includes}
	       ${tinyutf8_includes}
	       ${tsl_includes}
		   ${uhs_build_includes}
	       ${uhs_includes})
target_compile_options (uhs PRIVATE -g -O3 -Wall -Wextra)

# bin/uhs
add_executable (uhs_bin cpp/src/cmd/uhs/main.cpp)
add_dependencies (uhs_bin css_includes js_includes)
set_target_properties (uhs_bin
	PROPERTIES LINKER_LANGUAGE CXX
               OUTPUT_NAME uhs)
target_include_directories (uhs_bin PUBLIC ${argh_includes})
target_link_libraries (uhs_bin PUBLIC uhs)

# bin/uhsc
add_executable (uhsc_bin c/src/cmd/uhsc/main.c)
set_target_properties (uhsc_bin
	PROPERTIES LINKER_LANGUAGE C
               OUTPUT_NAME uhsc)
target_link_libraries (uhsc_bin PUBLIC uhs)

# make clean-all
add_custom_target (clean-all
	COMMAND rm -rf build
	COMMAND rm -rf CMakeFiles
	COMMAND rm -rf lib)

# make format
add_custom_target (format
	COMMAND clang-format -style=file -i -verbose
		${uhs_includes}/uhs.h
		${uhs_includes}/uhs_bridge.h
		${uhs_sources}
		${uhs_bin_sources})

# make lint
add_custom_target (lint
	COMMAND clang
		--analyze
		-Xanalyzer
		-analyzer-output=text
		-I${argh_includes}
		-I${httplib_includes}
		-I${json_includes}
		-I${puff_includes}
		-I${pugixml_includes}
		-I${tinyformat_includes}
		-I${tinyutf8_includes}
		-I${tsl_includes}
		-I${uhs_includes}
		-std=c++20
		${uhs_sources}
		${uhs_bin_sources})
add_dependencies (lint format)

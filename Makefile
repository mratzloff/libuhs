FLAGS_CLANG = -O0 -stdlib=libc++ -Wall -Wextra
FLAGS_CLANG_DEBUG = -g -fstandalone-debug
FLAGS_DEFAULT = $(FLAGS_CLANG) $(FLAGS_CLANG_DEBUG)

library = lib/libuhs.a
# library-sources = $(wildcard src/*.cc)
library-sources = src/scanner.cc src/token_queue.cc src/token.cc
library-objects = $(library-sources:src/%.cc=obj/%.o)

command = bin/uhs
command-sources = $(wildcard src/cmd/uhs/*.cc)
command-objects = $(command-sources:src/%.cc=obj/%.o)

.PHONY: all
all: obj $(library) # $(command)

.PHONY: clean
clean:
	rm -rf bin/* lib/*
	rm -rf obj
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

obj/%.o: src/%.cc
	$(CXX) -std=c++1y $(FLAGS_DEFAULT) -Iinclude -o $@ -c $<

$(library): $(library-objects)
	ar rcs $@ $(library-objects)

$(command): $(command-objects)

obj:
	mkdir -p obj

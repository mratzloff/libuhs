FLAGS_CLANG = -O0 -stdlib=libc++ -std=c++1z -Wall -Wextra
FLAGS_CLANG_DEBUG = -g -fstandalone-debug
FLAGS_DEFAULT = $(FLAGS_CLANG) $(FLAGS_CLANG_DEBUG)
FLAGS_CONTRIB = -O0 -stdlib=libc++ -std=c++1z
CXXFLAGS ?= $(FLAGS_DEFAULT)
LDFLAGS ?= -Llib -luhs

jsoncpp = obj/jsoncpp.o
jsoncpp-sources = contrib/jsoncpp/jsoncpp.cpp
jsoncpp-objects = obj/jsoncpp.o

library = lib/libuhs.a
library-sources = $(wildcard src/*.cc)
library-objects = $(library-sources:src/%.cc=obj/%.o)

command = bin/uhs
command-sources = $(wildcard src/cmd/uhs/*.cc)
command-objects = $(command-sources:src/%.cc=obj/%.o)

.PHONY: all
all: obj $(jsoncpp) $(library) $(command)

.PHONY: clean
clean:
	rm -rf bin/* lib/*
	rm -rf obj
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find . -name "*.dSYM" -print`

.PHONY: valgrind
valgrind: all
	valgrind --verbose --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes $(command)

$(jsoncpp): $(jsoncpp-sources)
	$(CXX) $(FLAGS_CONTRIB) -Icontrib/jsoncpp/json -c $< -o $@

obj/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -Icontrib/jsoncpp/json -Iinclude -c $< -o $@

$(library): $(jsoncpp-objects) $(library-objects)
	ar rcs $@ $(library-objects)

$(command): $(jsoncpp-objects) $(command-objects)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@ $(LDFLAGS)

obj:
	mkdir -p obj/cmd/uhs

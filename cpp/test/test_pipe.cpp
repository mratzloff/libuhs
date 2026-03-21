#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Pipe reads all data from stream", "[pipe]") {
	std::string input = "hello world";
	std::istringstream stream(input);
	Pipe pipe(stream);

	std::string collected;
	pipe.addHandler([&](char const* buffer, std::streamsize length) {
		collected.append(buffer, length);
	});

	pipe.read();

	REQUIRE(collected == input);
}

TEST_CASE("Pipe reads data in chunks", "[pipe]") {
	// Create a string larger than Pipe::ReadLength (1024) to force chunking
	std::string input(3000, 'x');
	std::istringstream stream(input);
	Pipe pipe(stream);

	int callCount = 0;
	std::string collected;
	pipe.addHandler([&](char const* buffer, std::streamsize length) {
		collected.append(buffer, length);
		++callCount;
	});

	pipe.read();

	REQUIRE(collected == input);
	REQUIRE(callCount > 1);
}

TEST_CASE("Pipe invokes multiple handlers", "[pipe]") {
	std::string input = "test";
	std::istringstream stream(input);
	Pipe pipe(stream);

	std::string collected1;
	std::string collected2;
	pipe.addHandler([&](char const* buffer, std::streamsize length) {
		collected1.append(buffer, length);
	});
	pipe.addHandler([&](char const* buffer, std::streamsize length) {
		collected2.append(buffer, length);
	});

	pipe.read();

	REQUIRE(collected1 == input);
	REQUIRE(collected2 == input);
}

TEST_CASE("Pipe captures handler exceptions", "[pipe]") {
	std::string input = "data";
	std::istringstream stream(input);
	Pipe pipe(stream);

	pipe.addHandler([](char const*, std::streamsize) {
		throw Error("handler error");
	});

	pipe.read();

	REQUIRE(pipe.error() != nullptr);
}

TEST_CASE("Pipe::eof after full read", "[pipe]") {
	std::string input = "short";
	std::istringstream stream(input);
	Pipe pipe(stream);

	pipe.addHandler([](char const*, std::streamsize) {});
	pipe.read();

	REQUIRE(pipe.eof());
}

TEST_CASE("Pipe handles empty stream", "[pipe]") {
	std::istringstream stream("");
	Pipe pipe(stream);

	std::string collected;
	pipe.addHandler([&](char const* buffer, std::streamsize length) {
		collected.append(buffer, length);
	});

	pipe.read();

	REQUIRE(collected.empty());
}

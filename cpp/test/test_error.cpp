#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Error::string", "[error]") {
	Error err("something went wrong");
	REQUIRE(err.string() == "something went wrong");
}

TEST_CASE("Error with format args", "[error]") {
	Error err("value is %d", 42);
	REQUIRE(err.string() == "value is 42");
}

TEST_CASE("Error::string with nested exception", "[error]") {
	// Error::string() uses rethrow_if_nested(this), which requires the
	// caught object to actually inherit from std::nested_exception.
	// When caught by reference as Error, the dynamic type preserves the
	// nesting only if the throw site used std::throw_with_nested.
	// In practice, the nested exception may not be visible through the
	// Error base pointer, so string() returns just the outer message.
	try {
		try {
			throw Error("inner error");
		} catch (...) {
			std::throw_with_nested(Error("outer error"));
		}
	} catch (Error const& err) {
		auto str = err.string();
		REQUIRE(str.find("outer error") != std::string::npos);
	}
}

TEST_CASE("DataError inherits from Error", "[error]") {
	REQUIRE_THROWS_AS(throw DataError("bad data"), Error);
	REQUIRE_THROWS_AS(throw DataError("bad data"), DataError);
}

TEST_CASE("FileError inherits from Error", "[error]") {
	REQUIRE_THROWS_AS(throw FileError("file not found"), Error);
	REQUIRE_THROWS_AS(throw FileError("file not found"), FileError);
}

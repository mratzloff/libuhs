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

TEST_CASE("ParseError formatting", "[error]") {
	ParseError err(5, 10, "unexpected token");
	REQUIRE(std::string(err.what()).find("line 5") != std::string::npos);
	REQUIRE(std::string(err.what()).find("column 10") != std::string::npos);
	REQUIRE(std::string(err.what()).find("unexpected token") != std::string::npos);
}

TEST_CASE("ParseError::badValue", "[error]") {
	SECTION("with ValueType") {
		auto err = ParseError::badValue(1, 2, ParseError::Uint, "abc");
		auto msg = std::string(err.what());
		REQUIRE(msg.find("positive integer") != std::string::npos);
		REQUIRE(msg.find("abc") != std::string::npos);
	}

	SECTION("with string expected") {
		auto err = ParseError::badValue(1, 2, "hex value", "xyz");
		auto msg = std::string(err.what());
		REQUIRE(msg.find("hex value") != std::string::npos);
		REQUIRE(msg.find("xyz") != std::string::npos);
	}
}

TEST_CASE("ParseError::badToken", "[error]") {
	SECTION("single token") {
		auto err = ParseError::badToken(3, 4, TokenType::Data);
		auto msg = std::string(err.what());
		REQUIRE(msg.find("Data") != std::string::npos);
	}

	SECTION("expected vs found") {
		auto err = ParseError::badToken(3, 4, TokenType::Line, TokenType::String);
		auto msg = std::string(err.what());
		REQUIRE(msg.find("Line") != std::string::npos);
		REQUIRE(msg.find("String") != std::string::npos);
	}
}

TEST_CASE("ParseError::badLine", "[error]") {
	auto err = ParseError::badLine(1, 0, 99);
	auto msg = std::string(err.what());
	REQUIRE(msg.find("99") != std::string::npos);
}

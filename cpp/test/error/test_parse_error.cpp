#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("ParseError formatting", "[error][parse_error]") {
	ParseError err(5, 10, "unexpected token");
	REQUIRE(std::string(err.what()).find("line 5") != std::string::npos);
	REQUIRE(std::string(err.what()).find("column 10") != std::string::npos);
	REQUIRE(std::string(err.what()).find("unexpected token") != std::string::npos);
}

TEST_CASE("ParseError::badValue", "[error][parse_error]") {
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

TEST_CASE("ParseError::badToken", "[error][parse_error]") {
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

TEST_CASE("ParseError::badLine", "[error][parse_error]") {
	auto err = ParseError::badLine(1, 0, 99);
	auto msg = std::string(err.what());
	REQUIRE(msg.find("99") != std::string::npos);
}

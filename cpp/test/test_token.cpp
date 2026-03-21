#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Token::typeString static", "[token]") {
	REQUIRE(Token::typeString(TokenType::CRC) == "CRC");
	REQUIRE(Token::typeString(TokenType::Data) == "Data");
	REQUIRE(Token::typeString(TokenType::Ident) == "Ident");
	REQUIRE(Token::typeString(TokenType::Line) == "Line");
	REQUIRE(Token::typeString(TokenType::Length) == "Length");
	REQUIRE(Token::typeString(TokenType::String) == "String");
	REQUIRE(Token::typeString(TokenType::Signature) == "Signature");
	REQUIRE(Token::typeString(TokenType::HeaderSep) == "HeaderSep");
	REQUIRE(Token::typeString(TokenType::TextFormat) == "TextFormat");
}

TEST_CASE("Token properties", "[token]") {
	Token token(TokenType::String, 100, 5, 10, "hello");

	REQUIRE(token.type() == TokenType::String);
	REQUIRE(token.offset() == 100);
	REQUIRE(token.line() == 5);
	REQUIRE(token.column() == 10);
	REQUIRE(token.value() == "hello");
	REQUIRE(token.typeString() == "String");
}

TEST_CASE("Token::string representation", "[token]") {
	SECTION("string token includes value in brackets") {
		Token token(TokenType::String, 0, 1, 0, "test value");
		auto str = token.string();
		REQUIRE(str.find("String") != std::string::npos);
		REQUIRE(str.find("[test value]") != std::string::npos);
		REQUIRE(str.front() == '(');
		REQUIRE(str.back() == ')');
	}

	SECTION("ident token includes value in brackets") {
		Token token(TokenType::Ident, 0, 1, 0, "hint");
		auto str = token.string();
		REQUIRE(str.find("Ident") != std::string::npos);
		REQUIRE(str.find("[hint]") != std::string::npos);
	}

	SECTION("line token includes value in brackets") {
		Token token(TokenType::Line, 0, 1, 0, "42");
		auto str = token.string();
		REQUIRE(str.find("Line") != std::string::npos);
		REQUIRE(str.find("[42]") != std::string::npos);
	}

	SECTION("data token shows hex") {
		Token token(TokenType::Data, 0, 1, 0, "AB");
		auto str = token.string();
		REQUIRE(str.find("Data") != std::string::npos);
		REQUIRE(str.find("[41 42]") != std::string::npos);
	}

	SECTION("signature token has no special value") {
		Token token(TokenType::Signature, 0, 1, 0, "");
		auto str = token.string();
		REQUIRE(str.find("Signature") != std::string::npos);
	}
}

TEST_CASE("Token::string includes position", "[token]") {
	Token token(TokenType::String, 50, 3, 7, "val");
	auto str = token.string();
	// Format: (TypeName line:column:offset [value])
	REQUIRE(str.find("3:7:50") != std::string::npos);
}

TEST_CASE("Token default construction", "[token]") {
	Token token(TokenType::FileEnd);
	REQUIRE(token.type() == TokenType::FileEnd);
	REQUIRE(token.line() == 0);
	REQUIRE(token.column() == 0);
	REQUIRE(token.offset() == 0);
	REQUIRE(token.value().empty());
}

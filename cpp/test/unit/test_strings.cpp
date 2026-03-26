#include <catch2/catch_test_macros.hpp>

#include "uhs/error/error.h"
#include "uhs/strings.h"

namespace UHS {

TEST_CASE("Strings::beginsWithAttachedPunctuation", "[strings]") {
	REQUIRE(Strings::beginsWithAttachedPunctuation("'s"));
	REQUIRE(Strings::beginsWithAttachedPunctuation("\"hello"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(")end"));
	REQUIRE(Strings::beginsWithAttachedPunctuation("?what"));
	REQUIRE(Strings::beginsWithAttachedPunctuation("!bang"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(".dot"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(",comma"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(";semi"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(":colon"));
	REQUIRE(Strings::beginsWithAttachedPunctuation(" space"));
	REQUIRE_FALSE(Strings::beginsWithAttachedPunctuation("hello"));
	REQUIRE_FALSE(Strings::beginsWithAttachedPunctuation("123"));
}

TEST_CASE("Strings::endsWithAttachedPunctuation", "[strings]") {
	REQUIRE(Strings::endsWithAttachedPunctuation("it'"));
	REQUIRE(Strings::endsWithAttachedPunctuation("said\""));
	REQUIRE(Strings::endsWithAttachedPunctuation("begin("));
	REQUIRE(Strings::endsWithAttachedPunctuation("trailing "));
	REQUIRE_FALSE(Strings::endsWithAttachedPunctuation("hello"));
	REQUIRE_FALSE(Strings::endsWithAttachedPunctuation("end."));
}

TEST_CASE("Strings::endsWithFinalPunctuation", "[strings]") {
	REQUIRE(Strings::endsWithFinalPunctuation("end."));
	REQUIRE(Strings::endsWithFinalPunctuation("what?"));
	REQUIRE(Strings::endsWithFinalPunctuation("wow!"));
	REQUIRE_FALSE(Strings::endsWithFinalPunctuation("hello"));
	REQUIRE_FALSE(Strings::endsWithFinalPunctuation("trail,"));
}

TEST_CASE("Strings::chomp", "[strings]") {
	std::string s = "hello\n";
	Strings::chomp(s, '\n');
	REQUIRE(s == "hello");

	std::string s2 = "hello";
	Strings::chomp(s2, '\n');
	REQUIRE(s2 == "hello");

	std::string s3 = "path/";
	Strings::chomp(s3, '/');
	REQUIRE(s3 == "path");
}

TEST_CASE("Strings::hex", "[strings]") {
	SECTION("single char") {
		REQUIRE(Strings::hex('A') == "41");
		REQUIRE(Strings::hex('\0') == "00");
		REQUIRE(Strings::hex('\xFF') == "FF");
		REQUIRE(Strings::hex(' ') == "20");
	}

	SECTION("string") {
		REQUIRE(Strings::hex("AB") == "41 42");
		REQUIRE(Strings::hex("") == "");
		REQUIRE(Strings::hex("A") == "41");
		REQUIRE(Strings::hex(std::string(1, '\x00')) == "00");
	}
}

TEST_CASE("Strings::isInt", "[strings]") {
	REQUIRE(Strings::isInt("0"));
	REQUIRE(Strings::isInt("123"));
	REQUIRE(Strings::isInt("9999999"));
	REQUIRE_FALSE(Strings::isInt(""));
	REQUIRE_FALSE(Strings::isInt("abc"));
	REQUIRE_FALSE(Strings::isInt("12a3"));
	REQUIRE_FALSE(Strings::isInt("-1"));
	REQUIRE_FALSE(Strings::isInt("1.5"));
}

TEST_CASE("Strings::isPrintable", "[strings]") {
	REQUIRE(Strings::isPrintable(' '));
	REQUIRE(Strings::isPrintable('A'));
	REQUIRE(Strings::isPrintable('~'));
	REQUIRE(Strings::isPrintable(0x7F));
	REQUIRE_FALSE(Strings::isPrintable(0x19));
	REQUIRE_FALSE(Strings::isPrintable(0x80));
}

TEST_CASE("Strings::join", "[strings]") {
	REQUIRE(Strings::join({"a", "b", "c"}, ", ") == "a, b, c");
	REQUIRE(Strings::join({"only"}, ", ") == "only");
	REQUIRE(Strings::join({}, ", ") == "");
	REQUIRE(Strings::join({"x", "y"}, "") == "xy");
	REQUIRE(Strings::join({"a", "b"}, "---") == "a---b");
}

TEST_CASE("Strings::ltrim", "[strings]") {
	REQUIRE(Strings::ltrim("   hello", ' ') == "hello");
	REQUIRE(Strings::ltrim("hello", ' ') == "hello");
	REQUIRE(Strings::ltrim("   ", ' ') == "   ");
	REQUIRE(Strings::ltrim("xxhello", 'x') == "hello");
}

TEST_CASE("Strings::rtrim", "[strings]") {
	REQUIRE(Strings::rtrim("hello   ", ' ') == "hello");
	REQUIRE(Strings::rtrim("hello", ' ') == "hello");
	REQUIRE(Strings::rtrim("   ", ' ') == "   ");
	REQUIRE(Strings::rtrim("helloxx", 'x') == "hello");
}

TEST_CASE("Strings::trim", "[strings]") {
	REQUIRE(Strings::trim("  hello  ", ' ') == "hello");
	REQUIRE(Strings::trim("hello", ' ') == "hello");
	REQUIRE(Strings::trim("xxhelloxx", 'x') == "hello");
	REQUIRE(Strings::trim("", ' ') == "");
}

TEST_CASE("Strings::split with string separator", "[strings]") {
	SECTION("basic split") {
		auto result = Strings::split("a,b,c", ",");
		REQUIRE(result.size() == 3);
		REQUIRE(result[0] == "a");
		REQUIRE(result[1] == "b");
		REQUIRE(result[2] == "c");
	}

	SECTION("with limit") {
		// n is the number of splits, so n=2 produces 2 items then stops
		auto result = Strings::split("a,b,c,d", ",", 2);
		REQUIRE(result.size() == 2);
		REQUIRE(result[0] == "a");
		REQUIRE(result[1] == "b");
	}

	SECTION("no match") {
		auto result = Strings::split("hello", ",");
		REQUIRE(result.size() == 1);
		REQUIRE(result[0] == "hello");
	}

	SECTION("empty string") {
		auto result = Strings::split("", ",");
		REQUIRE(result.empty());
	}

	SECTION("multi-char separator") {
		auto result = Strings::split("a::b::c", "::");
		REQUIRE(result.size() == 3);
		REQUIRE(result[0] == "a");
		REQUIRE(result[1] == "b");
		REQUIRE(result[2] == "c");
	}
}

TEST_CASE("Strings::toBase64", "[strings]") {
	REQUIRE(Strings::toBase64("f") == "Zg==");
	REQUIRE(Strings::toBase64("fo") == "Zm8=");
	REQUIRE(Strings::toBase64("foo") == "Zm9v");
	REQUIRE(Strings::toBase64("foob") == "Zm9vYg==");
	REQUIRE(Strings::toBase64("fooba") == "Zm9vYmE=");
	REQUIRE(Strings::toBase64("foobar") == "Zm9vYmFy");
}

TEST_CASE("Strings::toInt", "[strings]") {
	REQUIRE(Strings::toInt("0") == 0);
	REQUIRE(Strings::toInt("42") == 42);
	REQUIRE(Strings::toInt("-7") == -7);
	REQUIRE_THROWS_AS(Strings::toInt("abc"), Error);
	REQUIRE_THROWS_AS(Strings::toInt("12abc"), Error);
	REQUIRE_THROWS_AS(Strings::toInt(""), Error);
}

TEST_CASE("Strings::toLower", "[strings]") {
	std::string s = "Hello World";
	Strings::toLower(s);
	REQUIRE(s == "hello world");

	std::string s2 = "ALLCAPS";
	Strings::toLower(s2);
	REQUIRE(s2 == "allcaps");

	std::string s3 = "already";
	Strings::toLower(s3);
	REQUIRE(s3 == "already");
}

TEST_CASE("Strings::wrap", "[strings]") {
	SECTION("no wrapping needed") {
		REQUIRE(Strings::wrap("short", "\n", 80) == "short");
	}

	SECTION("wraps at word boundary") {
		auto result = Strings::wrap("hello world", "\n", 6);
		REQUIRE(result == "hello\nworld");
	}

	SECTION("preserves embedded newlines") {
		auto result = Strings::wrap("line1\nline2", "\n", 80);
		REQUIRE(result == "line1\nline2");
	}

	SECTION("hard cut when no space found") {
		auto result = Strings::wrap("abcdefghij", "\n", 5);
		REQUIRE(result == "abcde\nfghij");
	}
}

TEST_CASE("Strings::wrap handles edge cases", "[strings]") {
	SECTION("empty string") {
		auto result = Strings::wrap("", "\n", 80);
		REQUIRE(result == "");
	}

	SECTION("single character") {
		auto result = Strings::wrap("x", "\n", 80);
		REQUIRE(result == "x");
	}

	SECTION("string exactly at width") {
		auto result = Strings::wrap("12345", "\n", 5);
		REQUIRE(result == "12345");
	}

	SECTION("string two past width with no spaces") {
		auto result = Strings::wrap("1234567", "\n", 5);
		REQUIRE(result == "12345\n67");
	}

	SECTION("with numLines variant") {
		int numLines = 0;
		auto result = Strings::wrap("hello world", "\n", 6, numLines);
		REQUIRE(result == "hello\nworld");
		REQUIRE(numLines == 2);
	}

	SECTION("with numLines and empty string") {
		int numLines = 0;
		auto result = Strings::wrap("", "\n", 80, numLines);
		REQUIRE(result == "");
		REQUIRE(numLines == 1);
	}

	SECTION("with prefix") {
		int numLines = 0;
		auto result = Strings::wrap("hello world foo", "\n", 12, numLines, "> ");
		REQUIRE(result.find("> ") != std::string::npos);
	}
}

TEST_CASE("Strings::split bounds", "[strings]") {
	SECTION("single character string") {
		auto result = Strings::split("a", ",");
		REQUIRE(result.size() == 1);
		REQUIRE(result[0] == "a");
	}

	SECTION("separator only") {
		auto result = Strings::split(",", ",");
		REQUIRE(result.size() == 2);
	}

	SECTION("trailing separator") {
		auto result = Strings::split("a,", ",");
		REQUIRE(result.size() == 2);
		REQUIRE(result[0] == "a");
		REQUIRE(result[1] == "");
	}
}

TEST_CASE("Strings::ltrim and rtrim edge cases", "[strings]") {
	SECTION("ltrim empty string") {
		REQUIRE(Strings::ltrim("", ' ') == "");
	}

	SECTION("rtrim empty string") {
		REQUIRE(Strings::rtrim("", ' ') == "");
	}

	SECTION("trim single char that matches") {
		REQUIRE(Strings::trim("x", 'x') == "");
	}
}

TEST_CASE("Strings::hex single null byte", "[strings]") {
	REQUIRE(Strings::hex(std::string(1, '\0')) == "00");
}

} // namespace UHS

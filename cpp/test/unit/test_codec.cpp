#include <catch2/catch_test_macros.hpp>

#include "uhs/codec.h"
#include "uhs/strings.h"

namespace UHS {

TEST_CASE("Codec::encode88a and decode88a roundtrip", "[codec]") {
	Codec codec;

	SECTION("printable ASCII roundtrips") {
		std::string original = "Hello, World!";
		auto encoded = codec.encode88a(original);
		REQUIRE(encoded != original);
		auto decoded = codec.decode88a(encoded);
		REQUIRE(decoded == original);
	}

	SECTION("single character") {
		std::string original = "A";
		auto decoded = codec.decode88a(codec.encode88a(original));
		REQUIRE(decoded == original);
	}

	SECTION("all printable chars") {
		std::string original;
		for (int c = Strings::AsciiStart; c <= Strings::AsciiEnd; ++c) {
			original += static_cast<char>(c);
		}
		auto decoded = codec.decode88a(codec.encode88a(original));
		REQUIRE(decoded == original);
	}
}

TEST_CASE("Codec::encode96a and decode96a roundtrip", "[codec]") {
	Codec codec;
	auto key = codec.createKey("testkey");

	SECTION("text element roundtrip") {
		std::string original = "Hello, World!";
		auto encoded = codec.encode96a(original, key, true);
		REQUIRE(encoded != original);
		auto decoded = codec.decode96a(encoded, key, true);
		REQUIRE(decoded == original);
	}

	SECTION("non-text element roundtrip") {
		std::string original = "Some data here";
		auto encoded = codec.encode96a(original, key, false);
		auto decoded = codec.decode96a(encoded, key, false);
		REQUIRE(decoded == original);
	}

	SECTION("different keys produce different encodings") {
		auto key2 = codec.createKey("otherkey");
		std::string original = "test";
		auto encoded1 = codec.encode96a(original, key, true);
		auto encoded2 = codec.encode96a(original, key2, true);
		REQUIRE(encoded1 != encoded2);
	}
}

TEST_CASE("Codec::decodeSpecialChars", "[codec]") {
	Codec codec;

	SECTION("no special chars") {
		REQUIRE(codec.decodeSpecialChars("hello") == "hello");
	}

	SECTION("copyright symbol") {
		REQUIRE(codec.decodeSpecialChars("#a+(C)#a-") == "©");
	}

	SECTION("accented character") {
		REQUIRE(codec.decodeSpecialChars("#a+e'#a-") == "é");
	}

	SECTION("mixed text and special chars") {
		REQUIRE(codec.decodeSpecialChars("caf#a+e'#a-") == "café");
	}
}

TEST_CASE("Codec::encodeSpecialChars", "[codec]") {
	Codec codec;

	SECTION("no special chars") {
		REQUIRE(codec.encodeSpecialChars("hello") == "hello");
	}

	SECTION("copyright symbol") {
		REQUIRE(codec.encodeSpecialChars("©") == "#a+(C)#a-");
	}

	SECTION("roundtrip") {
		std::string original = "café";
		auto encoded = codec.encodeSpecialChars(original);
		auto decoded = codec.decodeSpecialChars(encoded);
		REQUIRE(decoded == original);
	}
}

TEST_CASE("Codec::toPrintable wraps to printable range", "[codec]") {
	Codec codec;

	// Verify encode/decode stays within the printable range
	auto key = codec.createKey("key");
	for (int c = Strings::AsciiStart; c <= Strings::AsciiEnd; ++c) {
		std::string original(1, static_cast<char>(c));
		auto encoded = codec.encode96a(original, key, true);
		REQUIRE(encoded.length() == 1);
		REQUIRE(Strings::isPrintable(encoded[0]));
	}
}

} // namespace UHS

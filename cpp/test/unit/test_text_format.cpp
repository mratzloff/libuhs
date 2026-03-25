#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"

namespace UHS {

TEST_CASE("TextFormat bitwise operators", "[text_format]") {
	SECTION("OR combines flags") {
		auto combined = TextFormat::Monospace | TextFormat::Overflow;
		REQUIRE(hasFormat(combined, TextFormat::Monospace));
		REQUIRE(hasFormat(combined, TextFormat::Overflow));
		REQUIRE_FALSE(hasFormat(combined, TextFormat::Hyperlink));
	}

	SECTION("AND masks flags") {
		auto combined = TextFormat::Monospace | TextFormat::Overflow;
		auto masked = combined & TextFormat::Monospace;
		REQUIRE(hasFormat(masked, TextFormat::Monospace));
		REQUIRE_FALSE(hasFormat(masked, TextFormat::Overflow));
	}

	SECTION("XOR toggles flags") {
		auto format = TextFormat::Monospace | TextFormat::Overflow;
		auto toggled = format ^ TextFormat::Overflow;
		REQUIRE(hasFormat(toggled, TextFormat::Monospace));
		REQUIRE_FALSE(hasFormat(toggled, TextFormat::Overflow));
	}

	SECTION("NOT inverts flags") {
		auto inverted = ~TextFormat::None;
		REQUIRE(hasFormat(inverted, TextFormat::Monospace));
		REQUIRE(hasFormat(inverted, TextFormat::Overflow));
		REQUIRE(hasFormat(inverted, TextFormat::Hyperlink));
	}
}

TEST_CASE("withFormat adds flag", "[text_format]") {
	auto format = TextFormat::None;
	format = withFormat(format, TextFormat::Monospace);
	REQUIRE(hasFormat(format, TextFormat::Monospace));
	REQUIRE_FALSE(hasFormat(format, TextFormat::Overflow));

	format = withFormat(format, TextFormat::Overflow);
	REQUIRE(hasFormat(format, TextFormat::Monospace));
	REQUIRE(hasFormat(format, TextFormat::Overflow));
}

TEST_CASE("withoutFormat removes flag", "[text_format]") {
	auto format = TextFormat::Monospace | TextFormat::Overflow;
	format = withoutFormat(format, TextFormat::Overflow);
	REQUIRE(hasFormat(format, TextFormat::Monospace));
	REQUIRE_FALSE(hasFormat(format, TextFormat::Overflow));
}

TEST_CASE("hasFormat checks flag presence", "[text_format]") {
	REQUIRE(hasFormat(TextFormat::Monospace, TextFormat::Monospace));
	REQUIRE_FALSE(hasFormat(TextFormat::Monospace, TextFormat::Overflow));
	REQUIRE(hasFormat(TextFormat::None, TextFormat::None));

	auto combined = TextFormat::Monospace | TextFormat::Overflow;
	REQUIRE(hasFormat(combined, TextFormat::Monospace));
	REQUIRE(hasFormat(combined, TextFormat::Overflow));
	REQUIRE(hasFormat(combined, TextFormat::Monospace | TextFormat::Overflow));
	REQUIRE_FALSE(hasFormat(combined, TextFormat::Hyperlink));
}

TEST_CASE("compound assignment operators", "[text_format]") {
	SECTION("|= adds flags") {
		auto format = TextFormat::None;
		format |= TextFormat::Monospace;
		REQUIRE(hasFormat(format, TextFormat::Monospace));
	}

	SECTION("&= masks flags") {
		auto format = TextFormat::Monospace | TextFormat::Overflow;
		format &= TextFormat::Monospace;
		REQUIRE(hasFormat(format, TextFormat::Monospace));
		REQUIRE_FALSE(hasFormat(format, TextFormat::Overflow));
	}

	SECTION("^= toggles flags") {
		auto format = TextFormat::Monospace;
		format ^= TextFormat::Monospace;
		REQUIRE(format == TextFormat::None);
	}
}

} // namespace UHS

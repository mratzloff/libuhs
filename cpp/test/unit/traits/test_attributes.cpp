#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/element.h"

namespace UHS {

TEST_CASE("Traits::Attributes", "[traits][attributes]") {
	auto element = Element::create(ElementType::Info, 1);

	SECTION("attr returns nullopt for missing key") {
		REQUIRE_FALSE(element->attr("missing").has_value());
	}

	SECTION("attr stores and retrieves string value") {
		element->attr("key", "value");
		auto result = element->attr("key");
		REQUIRE(result.has_value());
		REQUIRE(result.value() == "value");
	}

	SECTION("attr stores int as string") {
		element->attr("count", 42);
		auto result = element->attr("count");
		REQUIRE(result.has_value());
		REQUIRE(result.value() == "42");
	}

	SECTION("attr overwrites existing value") {
		element->attr("key", "first");
		element->attr("key", "second");
		REQUIRE(element->attr("key").value() == "second");
	}

	SECTION("attrs returns map of all attributes") {
		element->attr("a", "1");
		element->attr("b", "2");
		auto const& attrs = element->attrs();
		REQUIRE(attrs.size() == 2);
		REQUIRE(attrs.at("a") == "1");
		REQUIRE(attrs.at("b") == "2");
	}
}

} // namespace UHS

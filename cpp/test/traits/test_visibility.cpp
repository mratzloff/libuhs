#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

namespace UHS {

TEST_CASE("Traits::Visibility", "[traits][visibility]") {
	auto element = Element::create(ElementType::Hint, 1);

	SECTION("default visibility is All") {
		REQUIRE(element->visibility() == VisibilityType::All);
	}

	SECTION("visibility setter and getter") {
		element->visibility(VisibilityType::RegisteredOnly);
		REQUIRE(element->visibility() == VisibilityType::RegisteredOnly);
	}

	SECTION("visibilityString") {
		element->visibility(VisibilityType::All);
		REQUIRE(element->visibilityString() == "all");

		element->visibility(VisibilityType::None);
		REQUIRE(element->visibilityString() == "none");

		element->visibility(VisibilityType::RegisteredOnly);
		REQUIRE(element->visibilityString() == "registered");

		element->visibility(VisibilityType::UnregisteredOnly);
		REQUIRE(element->visibilityString() == "unregistered");
	}
}

} // namespace UHS

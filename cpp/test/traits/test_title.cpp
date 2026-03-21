#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Traits::Title", "[traits][title]") {
	auto element = Element::create(ElementType::Subject, 1);

	SECTION("default title is empty") {
		REQUIRE(element->title().empty());
	}

	SECTION("title setter and getter") {
		element->title("My Title");
		REQUIRE(element->title() == "My Title");
	}
}

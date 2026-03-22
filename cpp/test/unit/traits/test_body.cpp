#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

namespace UHS {

TEST_CASE("Traits::Body", "[traits][body]") {
	auto textNode = TextNode::create("initial");

	SECTION("body getter returns value") {
		REQUIRE(textNode->body() == "initial");
	}

	SECTION("body setter updates value") {
		textNode->body("updated");
		REQUIRE(textNode->body() == "updated");
	}

	SECTION("body from int") {
		auto element = Element::create(ElementType::Text, 1);
		element->body(123);
		REQUIRE(element->body() == "123");
	}
}

} // namespace UHS

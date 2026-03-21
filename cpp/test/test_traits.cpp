#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Traits::Attributes", "[traits]") {
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

TEST_CASE("Traits::Body", "[traits]") {
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

TEST_CASE("Traits::Inlined", "[traits]") {
	auto element = Element::create(ElementType::Text, 1);

	REQUIRE_FALSE(element->inlined());
	element->inlined(true);
	REQUIRE(element->inlined());
	element->inlined(false);
	REQUIRE_FALSE(element->inlined());
}

TEST_CASE("Traits::Title", "[traits]") {
	auto element = Element::create(ElementType::Subject, 1);

	SECTION("default title is empty") {
		REQUIRE(element->title().empty());
	}

	SECTION("title setter and getter") {
		element->title("My Title");
		REQUIRE(element->title() == "My Title");
	}
}

TEST_CASE("Traits::Visibility", "[traits]") {
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

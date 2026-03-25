#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/element.h"
#include "uhs/error/data_error.h"
#include "uhs/node/text_node.h"

namespace UHS {

TEST_CASE("Element::typeString and elementType mapping", "[element]") {
	REQUIRE(Element::typeString(ElementType::Hint) == "hint");
	REQUIRE(Element::typeString(ElementType::Text) == "text");
	REQUIRE(Element::typeString(ElementType::Subject) == "subject");
	REQUIRE(Element::typeString(ElementType::Comment) == "comment");
	REQUIRE(Element::typeString(ElementType::Credit) == "credit");
	REQUIRE(Element::typeString(ElementType::Gifa) == "gifa");
	REQUIRE(Element::typeString(ElementType::Hyperpng) == "hyperpng");
	REQUIRE(Element::typeString(ElementType::Incentive) == "incentive");
	REQUIRE(Element::typeString(ElementType::Info) == "info");
	REQUIRE(Element::typeString(ElementType::Link) == "link");
	REQUIRE(Element::typeString(ElementType::Nesthint) == "nesthint");
	REQUIRE(Element::typeString(ElementType::Overlay) == "overlay");
	REQUIRE(Element::typeString(ElementType::Sound) == "sound");
	REQUIRE(Element::typeString(ElementType::Version) == "version");
	REQUIRE(Element::typeString(ElementType::Blank) == "blank");
	REQUIRE(Element::typeString(ElementType::Unknown) == "unknown");
}

TEST_CASE("Element::elementType from string", "[element]") {
	REQUIRE(Element::elementType("hint") == ElementType::Hint);
	REQUIRE(Element::elementType("text") == ElementType::Text);
	REQUIRE(Element::elementType("subject") == ElementType::Subject);
	REQUIRE(Element::elementType("nonexistent") == ElementType::Unknown);
}

TEST_CASE("Element::create", "[element]") {
	auto element = Element::create(ElementType::Hint, 42);
	REQUIRE(element->elementType() == ElementType::Hint);
	REQUIRE(element->id() == 42);
	REQUIRE(element->elementTypeString() == "hint");
	REQUIRE(element->isElement());
}

TEST_CASE("Element::isMedia", "[element]") {
	REQUIRE(Element::create(ElementType::Gifa)->isMedia());
	REQUIRE(Element::create(ElementType::Hyperpng)->isMedia());
	REQUIRE(Element::create(ElementType::Overlay)->isMedia());
	REQUIRE(Element::create(ElementType::Sound)->isMedia());

	REQUIRE_FALSE(Element::create(ElementType::Hint)->isMedia());
	REQUIRE_FALSE(Element::create(ElementType::Text)->isMedia());
	REQUIRE_FALSE(Element::create(ElementType::Subject)->isMedia());
}

TEST_CASE("Element::mediaExt", "[element]") {
	REQUIRE(Element::create(ElementType::Gifa)->mediaExt() == "gif");
	REQUIRE(Element::create(ElementType::Hyperpng)->mediaExt() == "png");
	REQUIRE(Element::create(ElementType::Overlay)->mediaExt() == "png");
	REQUIRE(Element::create(ElementType::Sound)->mediaExt() == "wav");

	REQUIRE_THROWS_AS(Element::create(ElementType::Hint)->mediaExt(), DataError);
}

TEST_CASE("Element traits", "[element]") {
	auto element = Element::create(ElementType::Text, 1);

	SECTION("title") {
		element->title("My Title");
		REQUIRE(element->title() == "My Title");
	}

	SECTION("body") {
		element->body("content here");
		REQUIRE(element->body() == "content here");
	}

	SECTION("body from int") {
		element->body(42);
		REQUIRE(element->body() == "42");
	}

	SECTION("inlined") {
		REQUIRE_FALSE(element->inlined());
		element->inlined(true);
		REQUIRE(element->inlined());
	}

	SECTION("visibility") {
		REQUIRE(element->visibility() == VisibilityType::All);
		element->visibility(VisibilityType::RegisteredOnly);
		REQUIRE(element->visibility() == VisibilityType::RegisteredOnly);
	}

	SECTION("attributes") {
		element->attr("key1", "value1");
		element->attr("key2", 99);

		auto val1 = element->attr("key1");
		REQUIRE(val1.has_value());
		REQUIRE(val1.value() == "value1");

		auto val2 = element->attr("key2");
		REQUIRE(val2.has_value());
		REQUIRE(val2.value() == "99");

		auto missing = element->attr("nonexistent");
		REQUIRE_FALSE(missing.has_value());
	}
}

TEST_CASE("Element::clone produces independent copy", "[element]") {
	auto original = Element::create(ElementType::Hint, 10);
	original->title("Original Title");
	original->body("Original Body");

	auto child = TextNode::create("child text");
	original->appendChild(child);

	auto cloned = original->clone();
	REQUIRE(cloned->elementType() == ElementType::Hint);
	REQUIRE(cloned->id() == 10);
	REQUIRE(cloned->title() == "Original Title");
	REQUIRE(cloned->body() == "Original Body");
	REQUIRE(cloned->numChildren() == 1);
	REQUIRE_FALSE(cloned->hasParent());

	cloned->title("Changed");
	REQUIRE(original->title() == "Original Title");
}

} // namespace UHS

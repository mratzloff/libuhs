#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("TextNode format operations", "[text_node]") {
	auto textNode = TextNode::create("hello");
	REQUIRE(textNode->format() == TextFormat::None);

	textNode->addFormat(TextFormat::Monospace);
	REQUIRE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE_FALSE(textNode->hasFormat(TextFormat::Overflow));

	textNode->addFormat(TextFormat::Overflow);
	REQUIRE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE(textNode->hasFormat(TextFormat::Overflow));

	textNode->removeFormat(TextFormat::Monospace);
	REQUIRE_FALSE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE(textNode->hasFormat(TextFormat::Overflow));
}

TEST_CASE("TextNode::string returns body", "[text_node]") {
	auto textNode = TextNode::create("hello world");
	REQUIRE(textNode->string() == "hello world");
}

TEST_CASE("TextNode::clone produces independent copy", "[text_node]") {
	auto original = TextNode::create("hello", TextFormat::Monospace);
	auto cloned = original->clone();

	REQUIRE(cloned->string() == "hello");
	REQUIRE(cloned->hasFormat(TextFormat::Monospace));
	REQUIRE_FALSE(cloned->hasParent());

	cloned->body("changed");
	REQUIRE(original->string() == "hello");
}

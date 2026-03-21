#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("GroupNode", "[group_node]") {
	auto groupNode = GroupNode::create(5, 10);
	REQUIRE(groupNode->isGroup());
	REQUIRE(groupNode->line() == 5);
	REQUIRE(groupNode->length() == 10);

	auto cloned = groupNode->clone();
	REQUIRE(cloned->isGroup());
	REQUIRE(cloned->line() == 5);
	REQUIRE_FALSE(cloned->hasParent());
}

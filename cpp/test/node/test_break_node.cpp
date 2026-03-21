#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("BreakNode", "[break_node]") {
	auto breakNode = BreakNode::create();
	REQUIRE(breakNode->isBreak());
	REQUIRE(breakNode->nodeType() == NodeType::Break);

	auto cloned = breakNode->clone();
	REQUIRE(cloned->isBreak());
}

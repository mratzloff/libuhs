#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/node/break_node.h"

namespace UHS {

TEST_CASE("BreakNode", "[break_node]") {
	auto breakNode = BreakNode::create();
	REQUIRE(breakNode->isBreak());
	REQUIRE(breakNode->nodeType() == NodeType::Break);

	auto cloned = breakNode->clone();
	REQUIRE(cloned->isBreak());
}

} // namespace UHS

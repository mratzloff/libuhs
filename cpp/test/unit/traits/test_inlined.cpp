#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/element.h"

namespace UHS {

TEST_CASE("Traits::Inlined", "[traits][inlined]") {
	auto element = Element::create(ElementType::Text, 1);

	REQUIRE_FALSE(element->inlined());
	element->inlined(true);
	REQUIRE(element->inlined());
	element->inlined(false);
	REQUIRE_FALSE(element->inlined());
}

} // namespace UHS

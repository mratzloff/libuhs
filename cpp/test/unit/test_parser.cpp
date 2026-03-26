#include <catch2/catch_test_macros.hpp>

#include "uhs/logger.h"
#include "uhs/options.h"
#include "uhs/parser.h"

namespace UHS {

TEST_CASE("Parser construction does not alter localtime behavior", "[parser]") {
	auto const now = std::time(nullptr);

	std::tm before{};
	localtime_r(&now, &before);

	Logger logger{LogLevel::None};
	Options options;
	Parser parser{logger, options};

	std::tm after{};
	localtime_r(&now, &after);

	REQUIRE(before.tm_hour == after.tm_hour);
	REQUIRE(before.tm_min == after.tm_min);
	REQUIRE(before.tm_sec == after.tm_sec);
}

} // namespace UHS

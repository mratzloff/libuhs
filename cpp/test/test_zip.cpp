#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Zip::isZip detects valid ZIP signature", "[zip]") {
	// ZIP local file header signature: PK\x03\x04 = 0x04034b50 (little-endian)
	std::string zipData(30, '\0');
	zipData[0] = 'P';
	zipData[1] = 'K';
	zipData[2] = '\x03';
	zipData[3] = '\x04';

	Zip zip(zipData);
	REQUIRE(zip.isZip());
}

TEST_CASE("Zip::isZip rejects non-ZIP data", "[zip]") {
	std::string notZip = "This is not a ZIP file at all!";
	Zip zip(notZip);
	REQUIRE_FALSE(zip.isZip());
}

TEST_CASE("Zip::isZip rejects empty data", "[zip]") {
	std::string empty(4, '\0');
	Zip zip(empty);
	REQUIRE_FALSE(zip.isZip());
}

TEST_CASE("Zip::unzip throws for non-ZIP data", "[zip]") {
	std::string notZip = "Not a ZIP file";
	Zip zip(notZip);
	REQUIRE_THROWS_AS(zip.unzip("/tmp"), FileError);
}

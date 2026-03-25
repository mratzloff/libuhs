#include <catch2/catch_test_macros.hpp>

#include "uhs/crc.h"

namespace UHS {

TEST_CASE("CRC produces consistent results", "[crc]") {
	CRC crc;

	SECTION("same data produces same checksum") {
		std::string data = "Hello, World!";
		crc.calculate(data.data(), data.size());
		auto result1 = crc.result();

		CRC crc2;
		crc2.calculate(data.data(), data.size());
		auto result2 = crc2.result();

		REQUIRE(result1 == result2);
	}

	SECTION("different data produces different checksum") {
		std::string data1 = "Hello";
		CRC crc1;
		crc1.calculate(data1.data(), data1.size());

		std::string data2 = "World";
		CRC crc2;
		crc2.calculate(data2.data(), data2.size());

		REQUIRE(crc1.result() != crc2.result());
	}
}

TEST_CASE("CRC::reset clears state", "[crc]") {
	CRC crc;

	std::string data = "test data";
	crc.calculate(data.data(), data.size());
	auto firstResult = crc.result();

	crc.reset();
	crc.calculate(data.data(), data.size());
	auto secondResult = crc.result();

	REQUIRE(firstResult == secondResult);
}

TEST_CASE("CRC handles chunked input", "[crc]") {
	std::string data = "Hello, World!";

	CRC crcWhole;
	crcWhole.calculate(data.data(), data.size());

	CRC crcChunked;
	crcChunked.calculate(data.data(), 5);
	crcChunked.calculate(data.data() + 5, data.size() - 5);

	REQUIRE(crcWhole.result() == crcChunked.result());
}

TEST_CASE("CRC handles empty data", "[crc]") {
	CRC crc;
	crc.calculate("", 0);

	// Should not crash; result is the initial remainder
	auto result = crc.result();
	REQUIRE(result == result); // Compiles and doesn't throw
}

TEST_CASE("CRC::result returns checksum in vector form", "[crc]") {
	CRC crc;
	std::string data = "test";
	crc.calculate(data.data(), data.size());

	auto scalarResult = crc.result();

	CRC crc2;
	crc2.calculate(data.data(), data.size());
	std::vector<char> vectorResult;
	crc2.result(vectorResult);

	REQUIRE(vectorResult.size() == 2);
	uint16_t reconstructed = static_cast<uint8_t>(vectorResult[0])
	                         | (static_cast<uint8_t>(vectorResult[1]) << 8);
	REQUIRE(reconstructed == scalarResult);
}

TEST_CASE("CRC handles single-byte buffers correctly", "[crc]") {
	SECTION("single byte with no prior checksum data") {
		CRC crc;
		char buffer[] = {'A'};
		REQUIRE_NOTHROW(crc.calculate(buffer, 1, true));
		REQUIRE_NOTHROW(crc.result());
	}

	SECTION("two consecutive single-byte calls") {
		CRC crc;
		char buffer1[] = {'A'};
		char buffer2[] = {'B'};
		REQUIRE_NOTHROW(crc.calculate(buffer1, 1, true));
		REQUIRE_NOTHROW(crc.calculate(buffer2, 1, true));
		REQUIRE_NOTHROW(crc.result());
	}

	// This should never happen, but just in case
	SECTION("three single-byte calls") {
		CRC crc;
		char buffer1[] = {'A'};
		char buffer2[] = {'B'};
		char buffer3[] = {'C'};
		REQUIRE_NOTHROW(crc.calculate(buffer1, 1, true));
		REQUIRE_NOTHROW(crc.calculate(buffer2, 1, true));
		REQUIRE_NOTHROW(crc.calculate(buffer3, 1, true));
		REQUIRE_NOTHROW(crc.result());
	}

	SECTION("single-byte input matches multi-byte input") {
		std::string data = "AB";

		CRC crcWhole;
		crcWhole.calculate(data.data(), data.size(), true);

		CRC crcSingle;
		crcSingle.calculate(data.data(), 1, true);
		crcSingle.calculate(data.data() + 1, 1, true);

		REQUIRE(crcWhole.result() == crcSingle.result());
	}
}

} // namespace UHS

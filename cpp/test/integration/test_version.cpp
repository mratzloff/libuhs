#include <catch2/catch_test_macros.hpp>

#include "builders.h"

namespace UHS {

TEST_CASE("88a round-trip preserves structure", "[integration][version]") {
	auto document = build88aDocument();

	auto uhsOutput = writeUHS(document);
	REQUIRE(uhsOutput.substr(0, 3) == "UHS");

	auto parsed = roundTrip(document);
	REQUIRE(parsed->title() == "Wobbling Parchment Finder");
	REQUIRE(parsed->version() == VersionType::Version88a);
	REQUIRE(parsed->numChildren() == 1);

	auto subject = asElement(parsed->firstChild());
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Crumpled Envelope Dilemma");
	REQUIRE(subject->numChildren() == 1);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "Where is the speckled lantern?");
	REQUIRE(firstTextBody(hint) == "Tilt the crooked sundial leftward.");

	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("91a round-trip preserves structure", "[integration][version]") {
	auto document = build91aDocument();
	auto parsed = roundTrip(document);
	REQUIRE(parsed->title() == "Glinting Thimble Archives");
	REQUIRE(parsed->version() == VersionType::Version91a);

	REQUIRE(parsed->numChildren() == 4); // 88a header + subject + version + info

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Glinting Thimble Archives");
	REQUIRE(subject->numChildren() == 1);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "How do I open the brass valve?");
	REQUIRE(firstTextBody(hint) == "The amber flask rests beneath the mossy threshold.");

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"91a\"") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("95a round-trip preserves structure", "[integration][version]") {
	auto document = build95aDocument();
	auto parsed = roundTrip(document);
	REQUIRE(parsed->title() == "Muddled Keystone Gazette");
	REQUIRE(parsed->version() == VersionType::Version95a);
	REQUIRE(parsed->numChildren() == 4);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Muddled Keystone Gazette");

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->title() == "What triggers the revolving shelf?");
	REQUIRE(firstTextBody(hint) == "Pull the frayed cord beside the stone basin.");

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"95a\"") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("96a round-trip preserves structure", "[integration][version]") {
	auto document = build96aDocument();
	auto parsed = roundTrip(document);
	REQUIRE(parsed->title() == "Crimpled Bellows Manual");
	REQUIRE(parsed->version() == VersionType::Version96a);
	REQUIRE(parsed->numChildren() == 4);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Crimpled Bellows Manual");

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->title() == "Where does the tilted mirror point?");
	REQUIRE(firstTextBody(hint) == "Examine the chipped gargoyle near the fountain.");

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"96a\"") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

} // namespace UHS

#include <catch2/catch_test_macros.hpp>

#include "builders.h"

namespace UHS {

TEST_CASE("Gifa element round-trips", "[integration][media]") {
	auto document = buildGifaDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 2);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "Where is the secret passage?");
	REQUIRE(firstTextBody(hint) == "Watch the animation for the moving wall.");

	auto gifa = asElement(hint->nextSibling());
	REQUIRE(gifa->elementType() == ElementType::Gifa);
	REQUIRE(gifa->title() == "Wall movement animation");
	REQUIRE(!gifa->body().empty());

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Hyperpng simple round-trips", "[integration][media]") {
	auto document = buildHyperpngSimpleDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 2);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "Where is the flooded corridor?");
	REQUIRE(firstTextBody(hint) == "Consult the diagram below.");

	auto hyperpng = asElement(hint->nextSibling());
	REQUIRE(hyperpng->elementType() == ElementType::Hyperpng);
	REQUIRE(hyperpng->title() == "Corridor layout diagram");
	REQUIRE(!hyperpng->body().empty());
	REQUIRE(hyperpng->numChildren() == 0);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Hyperpng with link child round-trips", "[integration][media]") {
	auto document = buildHyperpngLinkDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 3);

	auto hyperpng = asElement(child(subject, 1));
	REQUIRE(hyperpng->elementType() == ElementType::Hyperpng);
	REQUIRE(hyperpng->title() == "Star chart overview");
	REQUIRE(!hyperpng->body().empty());
	REQUIRE(hyperpng->numChildren() == 1);

	auto link = asElement(hyperpng->firstChild());
	REQUIRE(link->elementType() == ElementType::Link);
	REQUIRE(link->title() == "View constellation details");
	REQUIRE(!link->body().empty());
	REQUIRE(link->attr("region-top-left-x") == "100");
	REQUIRE(link->attr("region-top-left-y") == "50");
	REQUIRE(link->attr("region-bottom-right-x") == "300");
	REQUIRE(link->attr("region-bottom-right-y") == "250");

	auto targetHint = asElement(child(subject, 2));
	REQUIRE(targetHint->elementType() == ElementType::Hint);
	REQUIRE(targetHint->title() == "Constellation details");

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Hyperpng with overlay children round-trips", "[integration][media]") {
	auto document = buildHyperpngOverlayDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 2);

	auto hyperpng = asElement(child(subject, 1));
	REQUIRE(hyperpng->elementType() == ElementType::Hyperpng);
	REQUIRE(hyperpng->title() == "Mosaic overview");
	REQUIRE(!hyperpng->body().empty());
	REQUIRE(hyperpng->numChildren() == 2);

	auto overlay1 = asElement(hyperpng->firstChild());
	REQUIRE(overlay1->elementType() == ElementType::Overlay);
	REQUIRE(overlay1->title() == "Upper left quadrant");
	REQUIRE(!overlay1->body().empty());
	REQUIRE(overlay1->attr("region-top-left-x") == "200");
	REQUIRE(overlay1->attr("region-top-left-y") == "300");
	REQUIRE(overlay1->attr("region-bottom-right-x") == "400");
	REQUIRE(overlay1->attr("region-bottom-right-y") == "500");

	auto overlay2 = asElement(overlay1->nextSibling());
	REQUIRE(overlay2->elementType() == ElementType::Overlay);
	REQUIRE(overlay2->title() == "Lower right quadrant");
	REQUIRE(!overlay2->body().empty());
	REQUIRE(overlay2->attr("region-top-left-x") == "400");
	REQUIRE(overlay2->attr("region-top-left-y") == "100");
	REQUIRE(overlay2->attr("region-bottom-right-x") == "600");
	REQUIRE(overlay2->attr("region-bottom-right-y") == "300");

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Sound element round-trips", "[integration][media]") {
	auto document = buildSoundDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 2);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "What sound opens the lock?");
	REQUIRE(firstTextBody(hint) == "Listen carefully to the chime.");

	auto sound = asElement(hint->nextSibling());
	REQUIRE(sound->elementType() == ElementType::Sound);
	REQUIRE(sound->title() == "Lock chime recording");
	REQUIRE(!sound->body().empty());

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

} // namespace UHS

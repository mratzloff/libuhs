#include <catch2/catch_test_macros.hpp>

#include "builders.h"

namespace UHS {

TEST_CASE(
    "Mixed plain and monospace in hint group round-trips", "[integration][formatting]") {
	auto document = buildMixedMonospaceHintDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "What is the combination?");

	auto group = hint->firstChild();
	REQUIRE(group->isGroup());
	REQUIRE(group->numChildren() == 3);

	auto plainNode1 = asTextNode(group->firstChild());
	REQUIRE(plainNode1->format() == TextFormat::None);
	REQUIRE(plainNode1->body().find("Rotate the outer ring") != std::string::npos);

	auto monoNode = asTextNode(plainNode1->nextSibling());
	REQUIRE(monoNode->format() == TextFormat::Monospace);
	REQUIRE(monoNode->body() == "VII-III-IX");

	auto plainNode2 = asTextNode(monoNode->nextSibling());
	REQUIRE(plainNode2->format() == TextFormat::None);
	REQUIRE(plainNode2->body().find("center button") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE(
    "Monospace plus overflow combined text round-trips", "[integration][formatting]") {
	auto document = buildMonospaceOverflowTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);

	auto textGroup = text->firstChild();
	auto textNode = asTextNode(textGroup->firstChild());
	auto combined = TextFormat::Monospace | TextFormat::Overflow;
	REQUIRE(textNode->format() == combined);
	REQUIRE(textNode->body().find("VALVE") != std::string::npos);
	REQUIRE(textNode->body().find("Alpha") != std::string::npos);

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"monospace\" : true") != std::string::npos);
	REQUIRE(json.find("\"overflow\" : true") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Monospace text element round-trips", "[integration][formatting]") {
	auto document = buildMonospaceTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);
	REQUIRE(text->title() == "Sprocket alignment table");

	auto textGroup = text->firstChild();
	REQUIRE(textGroup->isGroup());
	auto textNode = asTextNode(textGroup->firstChild());
	REQUIRE(textNode->format() == TextFormat::Monospace);
	REQUIRE(textNode->body().find("SLOT") != std::string::npos);
	REQUIRE(textNode->body().find("A1") != std::string::npos);

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"monospace\" : true") != std::string::npos);

	auto html = writeHTML(parsed);
	REQUIRE(html.find("monospace") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Overflow text element round-trips", "[integration][formatting]") {
	auto document = buildOverflowTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);

	auto textGroup = text->firstChild();
	auto textNode = asTextNode(textGroup->firstChild());
	REQUIRE(textNode->format() == TextFormat::Overflow);
	REQUIRE(textNode->body().find("mossy canal") != std::string::npos);

	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"overflow\" : true") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Plain text element round-trips", "[integration][formatting]") {
	auto document = buildPlainTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 2);

	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);
	REQUIRE(text->title() == "Parchment scrawl");

	auto textGroup = text->firstChild();
	REQUIRE(textGroup->isGroup());
	auto textNode = asTextNode(textGroup->firstChild());
	REQUIRE(textNode->body() == "The crooked lantern sways above the cobblestones.");
	REQUIRE(textNode->format() == TextFormat::None);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE(
    "Plain to hyperlink to plain text element round-trips", "[integration][formatting]") {
	auto document = buildPlainHyperlinkPlainTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);
	REQUIRE(text->title() == "Contact details");

	auto textGroup = text->firstChild();
	REQUIRE(textGroup->numChildren() == 3);

	auto plainNode1 = asTextNode(textGroup->firstChild());
	REQUIRE(plainNode1->format() == TextFormat::None);
	REQUIRE(plainNode1->body().find("Send inquiries") != std::string::npos);

	auto linkNode = asTextNode(plainNode1->nextSibling());
	REQUIRE(linkNode->format() == TextFormat::Hyperlink);
	REQUIRE(linkNode->body() == "quill@thornberry.net");

	auto plainNode2 = asTextNode(linkNode->nextSibling());
	REQUIRE(plainNode2->format() == TextFormat::None);
	REQUIRE(plainNode2->body().find("further details") != std::string::npos);

	auto html = writeHTML(parsed);
	REQUIRE(html.find("quill@thornberry.net") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE(
    "Plain to monospace to plain text element round-trips", "[integration][formatting]") {
	auto document = buildPlainMonospacePlainTextDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto text = asElement(child(subject, 1));
	REQUIRE(text->elementType() == ElementType::Text);
	REQUIRE(text->title() == "Console instructions");

	// Text element should have a group with 3 text nodes: plain, monospace, plain
	auto textGroup = text->firstChild();
	REQUIRE(textGroup->isGroup());
	REQUIRE(textGroup->numChildren() == 3);

	auto plainNode1 = asTextNode(textGroup->firstChild());
	REQUIRE(plainNode1->format() == TextFormat::None);
	REQUIRE(plainNode1->body().find("brass handle") != std::string::npos);

	auto monoNode = asTextNode(plainNode1->nextSibling());
	REQUIRE(monoNode->format() == TextFormat::Monospace);
	REQUIRE(monoNode->body() == "ACTIVATE PUMP 7");

	auto plainNode2 = asTextNode(monoNode->nextSibling());
	REQUIRE(plainNode2->format() == TextFormat::None);
	REQUIRE(plainNode2->body().find("release the handle") != std::string::npos);

	// Text element encoding is non-deterministic, so re-parse and compare structure
	auto reparsed = roundTrip(parsed);
	auto text2 = asElement(child(firstSubject(reparsed), 1));
	auto group2 = text2->firstChild();
	REQUIRE(group2->numChildren() == 3);
	REQUIRE(asTextNode(group2->firstChild())->format() == TextFormat::None);
}

} // namespace UHS

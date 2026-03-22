#include <catch2/catch_test_macros.hpp>

#include "builders.h"

namespace UHS {

TEST_CASE("Comment element round-trips", "[integration][metadata]") {
	auto document = buildCommentDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 3); // credit, comment, credit

	auto credit1 = asElement(subject->firstChild());
	REQUIRE(credit1->elementType() == ElementType::Credit);
	REQUIRE(credit1->title() == "Who compiled this file?");
	REQUIRE(credit1->body().find("Fern Kettlewick") != std::string::npos);

	auto comment = asElement(credit1->nextSibling());
	REQUIRE(comment->elementType() == ElementType::Comment);
	REQUIRE(comment->title() == "How do I write my own UHS file?");
	REQUIRE(comment->body().find("Brass Lantern Works") != std::string::npos);

	auto credit2 = asElement(comment->nextSibling());
	REQUIRE(credit2->elementType() == ElementType::Credit);
	REQUIRE(credit2->title() == "Who designed Smudged Quarto?");
	REQUIRE(credit2->body().find("Oaken Bridge Press") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Credit element round-trips", "[integration][metadata]") {
	auto document = buildCreditDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 4); // hint + 3 credits

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "Where do I find the broadside?");
	REQUIRE(firstTextBody(hint) == "Pinned to the corkboard in the foyer.");

	auto credit1 = asElement(hint->nextSibling());
	REQUIRE(credit1->elementType() == ElementType::Credit);
	REQUIRE(credit1->title() == "Who wrote this file?");
	REQUIRE(credit1->body().find("Fern Kettlewick") != std::string::npos);

	auto credit2 = asElement(credit1->nextSibling());
	REQUIRE(credit2->elementType() == ElementType::Credit);
	REQUIRE(credit2->title() == "Who designed the UHS?");
	REQUIRE(credit2->body().find("Strautman") != std::string::npos);

	auto credit3 = asElement(credit2->nextSibling());
	REQUIRE(credit3->elementType() == ElementType::Credit);
	REQUIRE(credit3->title() == "Who wrote Curled Broadside?");
	REQUIRE(credit3->body().find("Oaken Bridge Press") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Document attributes round-trip", "[integration][metadata]") {
	auto document = buildDocumentAttributesDocument();
	auto parsed = roundTrip(document);

	REQUIRE(parsed->attr("author") == "Quill Thornberry");
	REQUIRE(parsed->attr("publisher") == "Brass Lantern Works");
	REQUIRE(parsed->attr("copyright").has_value());
	REQUIRE(parsed->attr("copyright")->find("Thornberry estate") != std::string::npos);
	REQUIRE(parsed->attr("timestamp").has_value());
	REQUIRE(parsed->attr("notice").has_value());
	REQUIRE(parsed->attr("notice")->find("Bramblewood") != std::string::npos);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "Where do I begin?");
	REQUIRE(firstTextBody(hint) == "Start with the index on the back cover.");

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Incentive element round-trips with preserve", "[integration][metadata]") {
	auto document = buildIncentiveDocument();

	Options preserveOptions;
	preserveOptions.preserve = true;

	auto parsed = roundTrip(document, preserveOptions);

	REQUIRE(parsed->title() == "Tattered Reward Ledger");

	auto subject = firstSubject(parsed);
	REQUIRE(subject->numChildren() == 5); // comment, blank, about, general, endgame

	// Unregistered notice
	auto comment = asElement(child(subject, 0));
	REQUIRE(comment->elementType() == ElementType::Comment);
	REQUIRE(comment->title() == "Notice to unregistered readers");
	REQUIRE(comment->visibility() == VisibilityType::UnregisteredOnly);

	auto blank = asElement(child(subject, 1));
	REQUIRE(blank->elementType() == ElementType::Blank);
	REQUIRE(blank->visibility() == VisibilityType::UnregisteredOnly);

	// About subject
	auto aboutSubject = asElement(child(subject, 2));
	REQUIRE(aboutSubject->elementType() == ElementType::Subject);
	REQUIRE(aboutSubject->title() == "About this file");
	REQUIRE(aboutSubject->numChildren() == 1);

	auto aboutHint = asElement(child(aboutSubject, 0));
	REQUIRE(aboutHint->elementType() == ElementType::Hint);
	REQUIRE(aboutHint->title() == "What is this file?");

	// General subject — no visibility restrictions
	auto generalSubject = asElement(child(subject, 3));
	REQUIRE(generalSubject->elementType() == ElementType::Subject);
	REQUIRE(generalSubject->title() == "General Questions");
	REQUIRE(generalSubject->numChildren() == 2);

	auto nesthint1 = asElement(child(generalSubject, 0));
	REQUIRE(nesthint1->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint1->title() == "How do I open the front gate?");
	REQUIRE(nesthint1->visibility() == VisibilityType::All);

	auto nesthint2 = asElement(child(generalSubject, 1));
	REQUIRE(nesthint2->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint2->title() == "Where is the old well?");
	REQUIRE(nesthint2->visibility() == VisibilityType::All);

	// Endgame subject — registered-only nesthints
	auto endgameSubject = asElement(child(subject, 4));
	REQUIRE(endgameSubject->elementType() == ElementType::Subject);
	REQUIRE(endgameSubject->title() == "Endgame");
	REQUIRE(endgameSubject->numChildren() == 3);

	auto nesthint3 = asElement(child(endgameSubject, 0));
	REQUIRE(nesthint3->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint3->title() == "How do I enter the vault?");
	REQUIRE(nesthint3->visibility() == VisibilityType::RegisteredOnly);

	auto nesthint4 = asElement(child(endgameSubject, 1));
	REQUIRE(nesthint4->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint4->title() == "Where is the diamond key?");
	REQUIRE(nesthint4->visibility() == VisibilityType::RegisteredOnly);

	auto nesthint5 = asElement(child(endgameSubject, 2));
	REQUIRE(nesthint5->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint5->title() == "How do I defeat the guardian?");
	REQUIRE(nesthint5->visibility() == VisibilityType::RegisteredOnly);

	// Verify incentive element is preserved
	auto json = writeJSON(parsed);
	REQUIRE(json.find("\"incentive\"") != std::string::npos);
}

TEST_CASE("Version element appears in parsed document", "[integration][metadata]") {
	auto document = buildVersionMetadataDocument();
	auto parsed = roundTrip(document);

	REQUIRE(parsed->version() == VersionType::Version96a);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Polished Astrolabe Manual");

	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "How do I zero the instrument?");
	REQUIRE(firstTextBody(hint) == "Align the pointer with the north marker.");

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

} // namespace UHS

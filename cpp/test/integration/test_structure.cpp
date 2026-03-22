#include <catch2/catch_test_macros.hpp>

#include "builders.h"

namespace UHS {

TEST_CASE("Blank elements round-trip", "[integration][structure]") {
	auto document = buildBlankElementsDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->title() == "Faded Tapestry Ledger");
	REQUIRE(subject->numChildren() == 3); // hint, blank, hint

	auto hint1 = asElement(subject->firstChild());
	REQUIRE(hint1->elementType() == ElementType::Hint);
	REQUIRE(hint1->title() == "Where is the hidden lever?");

	auto blank = asElement(hint1->nextSibling());
	REQUIRE(blank->elementType() == ElementType::Blank);

	auto hint2 = asElement(blank->nextSibling());
	REQUIRE(hint2->elementType() == ElementType::Hint);
	REQUIRE(hint2->title() == "What does the lever activate?");

	auto html = writeHTML(parsed);
	REQUIRE(html.find("<hr") != std::string::npos);

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Break nodes as paragraph separators round-trip", "[integration][structure]") {
	auto document = buildBreakNodesDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	auto hint = asElement(subject->firstChild());
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "How do I descend the shaft safely?");
	REQUIRE(hint->numChildren() == 3); // group, break, group

	auto group1 = hint->firstChild();
	REQUIRE(group1->isGroup());
	REQUIRE(asTextNode(group1->firstChild())->body()
	        == "Secure the knotted rope to the beam.");

	auto breakNode = group1->nextSibling();
	REQUIRE(breakNode->isBreak());

	auto group2 = breakNode->nextSibling();
	REQUIRE(group2->isGroup());
	REQUIRE(asTextNode(group2->firstChild())->body()
	        == "Wrap the cloth around your hands before sliding.");

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Cross-links between subjects round-trip", "[integration][structure]") {
	auto document = buildCrossLinksDocument();
	auto parsed = roundTrip(document);

	auto rootSubject = firstSubject(parsed);
	REQUIRE(rootSubject->title() == "Splintered Compass Atlas");
	REQUIRE(rootSubject->numChildren() == 2); // 2 child subjects

	auto subject1 = asElement(rootSubject->firstChild());
	REQUIRE(subject1->elementType() == ElementType::Subject);
	REQUIRE(subject1->title() == "Flooded Cellar");
	REQUIRE(subject1->numChildren() == 2); // hint + link

	auto hint1 = asElement(subject1->firstChild());
	REQUIRE(hint1->elementType() == ElementType::Hint);
	REQUIRE(hint1->title() == "How do I drain the water?");
	REQUIRE(firstTextBody(hint1) == "Find the valve in the adjacent room.");

	auto link1 = asElement(hint1->nextSibling());
	REQUIRE(link1->elementType() == ElementType::Link);
	REQUIRE(link1->title() == "See also: Boiler Room");

	auto subject2 = asElement(subject1->nextSibling());
	REQUIRE(subject2->elementType() == ElementType::Subject);
	REQUIRE(subject2->title() == "Boiler Room");
	REQUIRE(subject2->numChildren() == 3); // hint + nesthint + link

	auto hint2 = asElement(subject2->firstChild());
	REQUIRE(hint2->elementType() == ElementType::Hint);
	REQUIRE(hint2->title() == "Where is the main valve?");
	REQUIRE(firstTextBody(hint2) == "Mounted on the rusted pipe near the furnace.");

	auto nesthint = asElement(hint2->nextSibling());
	REQUIRE(nesthint->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint->title() == "How do I turn it?");
	REQUIRE(firstTextBody(nesthint) == "Use the wrench from the flooded cellar.");

	auto link2 = asElement(nesthint->nextSibling());
	REQUIRE(link2->elementType() == ElementType::Link);
	REQUIRE(link2->title() == "See also: Flooded Cellar");

	// Verify link targets resolve (body contains target line number)
	REQUIRE(link1->body() == std::to_string(subject2->id()));
	REQUIRE(link2->body() == std::to_string(subject1->id()));

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

TEST_CASE("Deep nesthint nesting round-trips", "[integration][structure]") {
	auto document = buildDeepNesthintDocument();
	auto parsed = roundTrip(document);

	auto subject = firstSubject(parsed);
	REQUIRE(subject->title() == "Winding Stairwell Guide");
	REQUIRE(subject->numChildren() == 1);

	auto nesthint = asElement(subject->firstChild());
	REQUIRE(nesthint->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint->title() == "How do I unlock the iron grate?");
	REQUIRE(nesthint->numChildren() == 7); // 4 groups + 3 breaks

	// Group 1: text with paragraph breaks + hint inside
	auto group1 = nesthint->firstChild();
	REQUIRE(group1->isGroup());

	REQUIRE(firstTextBody(nesthint) == "Look for scratches on the floor.");

	auto hint = asElement(child(group1, 1));
	REQUIRE(hint->elementType() == ElementType::Hint);
	REQUIRE(hint->title() == "In what order?");
	REQUIRE(firstTextBody(hint).find("Press the three crescent-shaped tiles")
	        != std::string::npos);

	auto nesthint1 = asElement(child(group1, 2));
	REQUIRE(nesthint1->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint1->title() == "Is there another way through?");
	REQUIRE(firstTextBody(nesthint1) == "Not without the skeleton key.");

	// Group 2: nesthint with text element (skip break node)
	auto group2 = group1->nextSibling()->nextSibling();
	REQUIRE(group2->isGroup());

	auto nesthint2 = asElement(group2->firstChild());
	REQUIRE(nesthint2->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint2->title() == "What does the tile layout look like?");
	REQUIRE(firstTextBody(nesthint2) == "Refer to the diagram below.");

	auto nesthint2Group = nesthint2->firstChild();
	auto text = asElement(child(nesthint2Group, 1));
	REQUIRE(text->elementType() == ElementType::Text);
	REQUIRE(text->title() == "Tile diagram");
	auto textGroup = text->firstChild();
	REQUIRE(textGroup->isGroup());
	auto textNode = asTextNode(textGroup->firstChild());
	REQUIRE(textNode->format() == TextFormat::Monospace);
	REQUIRE(textNode->body().find("[TL]") != std::string::npos);

	// Group 3: nesthint with plain text (skip break node)
	auto group3 = group2->nextSibling()->nextSibling();
	REQUIRE(group3->isGroup());

	auto nesthint3 = asElement(group3->firstChild());
	REQUIRE(nesthint3->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint3->title() == "Can I skip this puzzle?");
	REQUIRE(firstTextBody(nesthint3) == "No, the grate blocks the only path forward.");

	// Group 4: nesthint with plain text (skip break node)
	auto group4 = group3->nextSibling()->nextSibling();
	REQUIRE(group4->isGroup());

	auto nesthint4 = asElement(group4->firstChild());
	REQUIRE(nesthint4->elementType() == ElementType::Nesthint);
	REQUIRE(nesthint4->title() == "What happens if I press the wrong tile?");
	REQUIRE(nesthint4->numChildren() == 5); // 3 groups + 2 breaks
	REQUIRE(firstTextBody(nesthint4) == "The tiles reset and you must start over.");

	auto nesthint4Group2 = nesthint4->firstChild()->nextSibling()->nextSibling();
	REQUIRE(nesthint4Group2->isGroup());
	REQUIRE(asTextNode(nesthint4Group2->firstChild())->body()
	        == "Listen for the chime after each correct press.");

	auto nesthint4Group3 = nesthint4Group2->nextSibling()->nextSibling();
	REQUIRE(nesthint4Group3->isGroup());
	REQUIRE(asTextNode(nesthint4Group3->firstChild())->body()
	        == "The sequence is always the same: top right, bottom left, center.");

	// Text encoding is non-deterministic, so re-parse and compare structure
	auto reparsed = roundTrip(parsed);
	auto reparsedSubject = firstSubject(reparsed);
	REQUIRE(reparsedSubject->numChildren() == 1);
}

TEST_CASE("Subject with multiple hints round-trips", "[integration][structure]") {
	auto document = buildMultipleHintsDocument();
	auto parsed = roundTrip(document);
	REQUIRE(parsed->title() == "Tangled Rope Cellar");

	auto subject = firstSubject(parsed);
	REQUIRE(subject->elementType() == ElementType::Subject);
	REQUIRE(subject->title() == "Tangled Rope Cellar");
	REQUIRE(subject->numChildren() == 4);

	auto hint1 = asElement(subject->firstChild());
	REQUIRE(hint1->elementType() == ElementType::Hint);
	REQUIRE(hint1->title() == "Where is the dented bucket?");
	REQUIRE(hint1->numChildren() == 5); // 3 groups + 2 breaks
	REQUIRE(firstTextBody(hint1) == "Have you searched the cellar thoroughly?");

	auto hint2 = asElement(hint1->nextSibling());
	REQUIRE(hint2->elementType() == ElementType::Hint);
	REQUIRE(hint2->title() == "How do I reach the upper ledge?");
	REQUIRE(hint2->numChildren() == 3); // group, break, group

	auto hint3 = asElement(hint2->nextSibling());
	REQUIRE(hint3->elementType() == ElementType::Hint);
	REQUIRE(hint3->title() == "What powers the grinding wheel?");
	REQUIRE(hint3->numChildren() == 5); // 3 groups + 2 breaks

	auto hint4 = asElement(hint3->nextSibling());
	REQUIRE(hint4->elementType() == ElementType::Hint);
	REQUIRE(hint4->title() == "Where is the missing sprocket?");
	REQUIRE(hint4->numChildren() == 3); // group, break, group

	auto uhsOutput = writeUHS(document);
	auto uhsOutput2 = writeUHS(parsed);
	REQUIRE(uhsOutput == uhsOutput2);
}

} // namespace UHS

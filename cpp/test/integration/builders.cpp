#include "builders.h"

#include "uhs/node/break_node.h"
#include "uhs/node/group_node.h"

namespace UHS {

// Formatting builders

std::shared_ptr<Document> buildPlainTextDocument() {
	auto [document, subject] = createDocument96a("Rattled Inkwell Primer");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Placeholder for structure."));

	auto text = Element::create(ElementType::Text, 3);
	text->title("Parchment scrawl");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(
	    TextNode::create("The crooked lantern sways above the cobblestones."));

	return document;
}

std::shared_ptr<Document> buildMonospaceTextDocument() {
	auto [document, subject] = createDocument96a("Tarnished Bracket Codex");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto hintGroup = GroupNode::create(0, 0);
	hint->appendChild(hintGroup);
	hintGroup->appendChild(TextNode::create("Filler text."));

	auto text = Element::create(ElementType::Text, 3);
	text->title("Sprocket alignment table");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(
	    TextNode::create("SLOT  PART   SIZE\nA1    Cog    Small\nB2    Gear   Large",
	        TextFormat::Monospace));

	return document;
}

std::shared_ptr<Document> buildOverflowTextDocument() {
	auto [document, subject] = createDocument96a("Scattered Ledger Notes");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto hintGroup = GroupNode::create(0, 0);
	hint->appendChild(hintGroup);
	hintGroup->appendChild(TextNode::create("Filler text."));

	auto text = Element::create(ElementType::Text, 3);
	text->title("Lengthy passage directions");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(TextNode::create(
	    "Follow the mossy canal southward past the collapsed bridge and through the "
	    "narrow gap between the limestone pillars until you reach the rusted gate.",
	    TextFormat::Overflow));

	return document;
}

std::shared_ptr<Document> buildMonospaceOverflowTextDocument() {
	auto [document, subject] = createDocument96a("Bolted Manifold Register");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto hintGroup = GroupNode::create(0, 0);
	hint->appendChild(hintGroup);
	hintGroup->appendChild(TextNode::create("Filler text."));

	auto combined = TextFormat::Monospace | TextFormat::Overflow;
	auto text = Element::create(ElementType::Text, 3);
	text->title("Valve pressure readings");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(TextNode::create(
	    "VALVE   PSI    STATUS\nAlpha   120    Nominal\nBeta    340    Critical\nGamma "
	    "  85     Low",
	    combined));

	return document;
}

std::shared_ptr<Document> buildPlainMonospacePlainTextDocument() {
	auto [document, subject] = createDocument96a("Wrinkled Folio Dispatch");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto hintGroup = GroupNode::create(0, 0);
	hint->appendChild(hintGroup);
	hintGroup->appendChild(TextNode::create("Filler text."));

	auto text = Element::create(ElementType::Text, 3);
	text->title("Console instructions");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(TextNode::create("Hold the brass handle and type: "));
	textGroup->appendChild(TextNode::create("ACTIVATE PUMP 7", TextFormat::Monospace));
	textGroup->appendChild(
	    TextNode::create(" then release the handle and wait for the chime."));

	return document;
}

std::shared_ptr<Document> buildPlainHyperlinkPlainTextDocument() {
	auto [document, subject] = createDocument96a("Folded Broadsheet Gazette");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("General guidance");
	subject->appendChild(hint);

	auto hintGroup = GroupNode::create(0, 0);
	hint->appendChild(hintGroup);
	hintGroup->appendChild(TextNode::create("Filler text."));

	auto text = Element::create(ElementType::Text, 3);
	text->title("Contact details");
	text->attr("format", 0);
	subject->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(TextNode::create("Send inquiries to "));
	textGroup->appendChild(
	    TextNode::create("quill@thornberry.net", TextFormat::Hyperlink));
	textGroup->appendChild(TextNode::create(" for further details."));

	return document;
}

std::shared_ptr<Document> buildMixedMonospaceHintDocument() {
	auto [document, subject] = createDocument96a("Creased Almanac Volume");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("What is the combination?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Rotate the outer ring to show "));
	group->appendChild(TextNode::create("VII-III-IX", TextFormat::Monospace));
	group->appendChild(TextNode::create(" then press the center button."));

	return document;
}

// Media builders

std::shared_ptr<Document> buildHyperpngSimpleDocument() {
	auto [document, subject] = createDocument96a("Gilded Porthole Chronicle");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where is the flooded corridor?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Consult the diagram below."));

	auto pngData = readFile(MediaDir + "hyperpng.png");

	auto hyperpng = Element::create(ElementType::Hyperpng, 3);
	hyperpng->title("Corridor layout diagram");
	hyperpng->body(pngData);
	subject->appendChild(hyperpng);

	return document;
}

std::shared_ptr<Document> buildHyperpngOverlayDocument() {
	auto [document, subject] = createDocument96a("Cracked Mosaic Surveyor");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("What is the correct pattern?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Examine the overlays for each quadrant."));

	auto pngData = readFile(MediaDir + "hyperpng.png");

	auto hyperpng = Element::create(ElementType::Hyperpng, 3);
	hyperpng->title("Mosaic overview");
	hyperpng->body(pngData);
	subject->appendChild(hyperpng);

	auto overlayData1 = readFile(MediaDir + "overlay1.png");

	auto overlay1 = Element::create(ElementType::Overlay, 4);
	overlay1->title("Upper left quadrant");
	overlay1->body(overlayData1);
	overlay1->attr("image-x", 200);
	overlay1->attr("image-y", 300);
	overlay1->attr("region-top-left-x", 200);
	overlay1->attr("region-top-left-y", 300);
	overlay1->attr("region-bottom-right-x", 400);
	overlay1->attr("region-bottom-right-y", 500);
	hyperpng->appendChild(overlay1);

	auto overlayData2 = readFile(MediaDir + "overlay2.png");

	auto overlay2 = Element::create(ElementType::Overlay, 5);
	overlay2->title("Lower right quadrant");
	overlay2->body(overlayData2);
	overlay2->attr("image-x", 400);
	overlay2->attr("image-y", 100);
	overlay2->attr("region-top-left-x", 400);
	overlay2->attr("region-top-left-y", 100);
	overlay2->attr("region-bottom-right-x", 600);
	overlay2->attr("region-bottom-right-y", 300);
	hyperpng->appendChild(overlay2);

	return document;
}

std::shared_ptr<Document> buildHyperpngLinkDocument() {
	auto [document, subject] = createDocument96a("Weathered Chart Binder");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("How do I read the star chart?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Click the highlighted region."));

	auto pngData = readFile(MediaDir + "hyperpng.png");

	auto hyperpng = Element::create(ElementType::Hyperpng, 3);
	hyperpng->title("Star chart overview");
	hyperpng->body(pngData);
	subject->appendChild(hyperpng);

	auto targetHint = Element::create(ElementType::Hint, 5);
	targetHint->title("Constellation details");
	subject->appendChild(targetHint);

	auto targetGroup = GroupNode::create(0, 0);
	targetHint->appendChild(targetGroup);
	targetGroup->appendChild(
	    TextNode::create("The three bright stars form the Anchor pattern."));

	auto link = Element::create(ElementType::Link, 4);
	link->title("View constellation details");
	link->body("5");
	link->attr("region-top-left-x", 100);
	link->attr("region-top-left-y", 50);
	link->attr("region-bottom-right-x", 300);
	link->attr("region-bottom-right-y", 250);
	hyperpng->appendChild(link);

	return document;
}

std::shared_ptr<Document> buildGifaDocument() {
	auto [document, subject] = createDocument96a("Flickering Lantern Digest");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where is the secret passage?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Watch the animation for the moving wall."));

	auto gifData = readFile(MediaDir + "gifa.gif");

	auto gifa = Element::create(ElementType::Gifa, 3);
	gifa->title("Wall movement animation");
	gifa->body(gifData);
	subject->appendChild(gifa);

	return document;
}

std::shared_ptr<Document> buildSoundDocument() {
	auto [document, subject] = createDocument96a("Echoing Vault Anthology");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("What sound opens the lock?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Listen carefully to the chime."));

	auto wavData = readFile(MediaDir + "sound.wav");

	auto sound = Element::create(ElementType::Sound, 3);
	sound->title("Lock chime recording");
	sound->body(wavData);
	subject->appendChild(sound);

	return document;
}

// Metadata builders

std::shared_ptr<Document> buildDocumentAttributesDocument() {
	auto [document, subject] = createDocument96a("Fraying Velvet Dossier");

	document->attr("author", "Quill Thornberry");
	document->attr("publisher", "Brass Lantern Works");
	document->attr("copyright",
	    "This compendium belongs to the Thornberry estate and may not be reproduced "
	    "without written consent from the archivist.");
	document->attr("timestamp", "2024-08-15T14:30:00");
	document->attr(
	    "notice", "Compiled under the auspices of the Bramblewood Cartography Guild.");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where do I begin?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Start with the index on the back cover."));

	return document;
}

std::shared_ptr<Document> buildCommentDocument() {
	auto [document, subject] = createDocument96a("Smudged Quarto Edition");

	auto credit1 = Element::create(ElementType::Credit, 2);
	credit1->title("Who compiled this file?");
	credit1->body("Assembled by Fern Kettlewick during the autumn surveys.");
	subject->appendChild(credit1);

	auto comment = Element::create(ElementType::Comment, 3);
	comment->title("How do I write my own UHS file?");
	comment->body("Consult the formatting guide published by the Brass Lantern Works.");
	subject->appendChild(comment);

	auto credit2 = Element::create(ElementType::Credit, 4);
	credit2->title("Who designed Smudged Quarto?");
	credit2->body("Designed by Oaken Bridge Press, Hartwick Lane.");
	subject->appendChild(credit2);

	return document;
}

std::shared_ptr<Document> buildCreditDocument() {
	auto [document, subject] = createDocument96a("Curled Broadside Pamphlet");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where do I find the broadside?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Pinned to the corkboard in the foyer."));

	auto credit1 = Element::create(ElementType::Credit, 3);
	credit1->title("Who wrote this file?");
	credit1->body("Compiled by Fern Kettlewick.");
	subject->appendChild(credit1);

	auto credit2 = Element::create(ElementType::Credit, 4);
	credit2->title("Who designed the UHS?");
	credit2->body("Jason Strautman designed the UHS format.");
	subject->appendChild(credit2);

	auto credit3 = Element::create(ElementType::Credit, 5);
	credit3->title("Who wrote Curled Broadside?");
	credit3->body("Written by the Oaken Bridge Press collective.");
	subject->appendChild(credit3);

	return document;
}

std::shared_ptr<Document> buildIncentiveDocument() {
	auto [document, subject] = createDocument96a("Tattered Reward Ledger");

	// Unregistered notice (comment + blank), like zork3's initial 3Z 8Z
	auto comment = Element::create(ElementType::Comment, 2);
	comment->title("Notice to unregistered readers");
	comment->body("Register your UHS reader to unlock all hints in this file.");
	comment->visibility(VisibilityType::UnregisteredOnly);
	subject->appendChild(comment);

	auto blank = Element::create(ElementType::Blank);
	blank->visibility(VisibilityType::UnregisteredOnly);
	subject->appendChild(blank);

	// First nested subject — about this file
	auto aboutSubject = Element::create(ElementType::Subject, 3);
	aboutSubject->title("About this file");
	subject->appendChild(aboutSubject);

	auto aboutHint = Element::create(ElementType::Hint, 4);
	aboutHint->title("What is this file?");
	aboutSubject->appendChild(aboutHint);

	auto aboutGroup = GroupNode::create(0, 0);
	aboutHint->appendChild(aboutGroup);
	aboutGroup->appendChild(
	    TextNode::create("A collection of hints for the Tattered Reward."));

	// Second nested subject — general topics, no visibility restrictions
	auto generalSubject = Element::create(ElementType::Subject, 5);
	generalSubject->title("General Questions");
	subject->appendChild(generalSubject);

	auto nesthint1 = Element::create(ElementType::Nesthint, 4);
	nesthint1->title("How do I open the front gate?");
	generalSubject->appendChild(nesthint1);

	auto nesthint1Group = GroupNode::create(0, 0);
	nesthint1->appendChild(nesthint1Group);
	nesthint1Group->appendChild(TextNode::create("Try the brass handle."));

	auto nesthint1Hint = Element::create(ElementType::Hint, 5);
	nesthint1Hint->title("It's locked!");
	nesthint1Group->appendChild(nesthint1Hint);

	auto nesthint1HintGroup = GroupNode::create(0, 0);
	nesthint1Hint->appendChild(nesthint1HintGroup);
	nesthint1HintGroup->appendChild(
	    TextNode::create("The gardener keeps a spare key under the mat."));

	auto nesthint2 = Element::create(ElementType::Nesthint, 6);
	nesthint2->title("Where is the old well?");
	generalSubject->appendChild(nesthint2);

	auto nesthint2Group = GroupNode::create(0, 0);
	nesthint2->appendChild(nesthint2Group);
	nesthint2Group->appendChild(
	    TextNode::create("Follow the cobblestone path past the orchard."));

	auto nesthint2Hint = Element::create(ElementType::Hint, 7);
	nesthint2Hint->title("I can't reach the bucket");
	nesthint2Group->appendChild(nesthint2Hint);

	auto nesthint2HintGroup = GroupNode::create(0, 0);
	nesthint2Hint->appendChild(nesthint2HintGroup);
	nesthint2HintGroup->appendChild(TextNode::create("Use the rope from the stable."));

	// Third nested subject — registered-only nesthints
	auto registeredSubject = Element::create(ElementType::Subject, 8);
	registeredSubject->title("Endgame");
	subject->appendChild(registeredSubject);

	auto nesthint3 = Element::create(ElementType::Nesthint, 9);
	nesthint3->title("How do I enter the vault?");
	nesthint3->visibility(VisibilityType::RegisteredOnly);
	registeredSubject->appendChild(nesthint3);

	auto nesthint3Group = GroupNode::create(0, 0);
	nesthint3->appendChild(nesthint3Group);
	nesthint3Group->appendChild(
	    TextNode::create("The combination is on the back of the portrait."));

	auto nesthint4 = Element::create(ElementType::Nesthint, 10);
	nesthint4->title("Where is the diamond key?");
	nesthint4->visibility(VisibilityType::RegisteredOnly);
	registeredSubject->appendChild(nesthint4);

	auto nesthint4Group = GroupNode::create(0, 0);
	nesthint4->appendChild(nesthint4Group);
	nesthint4Group->appendChild(
	    TextNode::create("Locked in the display case on the mezzanine."));

	auto nesthint5 = Element::create(ElementType::Nesthint, 11);
	nesthint5->title("How do I defeat the guardian?");
	nesthint5->visibility(VisibilityType::RegisteredOnly);
	registeredSubject->appendChild(nesthint5);

	auto nesthint5Group = GroupNode::create(0, 0);
	nesthint5->appendChild(nesthint5Group);
	nesthint5Group->appendChild(
	    TextNode::create("Reflect the light with the polished shield."));

	// Incentive element (only this builder needs one)
	auto incentive = Element::create(ElementType::Incentive);
	document->appendChild(incentive);

	return document;
}

std::shared_ptr<Document> buildVersionMetadataDocument() {
	auto [document, subject] = createDocument96a("Polished Astrolabe Manual");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("How do I zero the instrument?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Align the pointer with the north marker."));

	return document;
}

// Structure builders

std::shared_ptr<Document> buildMultipleHintsDocument() {
	auto [document, subject] = createDocument96a("Tangled Rope Cellar");

	// First hint with 3 progressive reveals
	auto hint1 = Element::create(ElementType::Hint, 2);
	hint1->title("Where is the dented bucket?");
	subject->appendChild(hint1);

	auto group1a = GroupNode::create(0, 0);
	hint1->appendChild(group1a);
	group1a->appendChild(TextNode::create("Have you searched the cellar thoroughly?"));

	hint1->appendChild(BreakNode::create());

	auto group1b = GroupNode::create(0, 0);
	hint1->appendChild(group1b);
	group1b->appendChild(TextNode::create("Check behind the warped barrel."));

	hint1->appendChild(BreakNode::create());

	auto group1c = GroupNode::create(0, 0);
	hint1->appendChild(group1c);
	group1c->appendChild(
	    TextNode::create("The bucket is wedged between the barrel and the north wall."));

	// Second hint with 2 progressive reveals
	auto hint2 = Element::create(ElementType::Hint, 3);
	hint2->title("How do I reach the upper ledge?");
	subject->appendChild(hint2);

	auto group2a = GroupNode::create(0, 0);
	hint2->appendChild(group2a);
	group2a->appendChild(TextNode::create("You need to build something to climb on."));

	hint2->appendChild(BreakNode::create());

	auto group2b = GroupNode::create(0, 0);
	hint2->appendChild(group2b);
	group2b->appendChild(TextNode::create("Stack the crates near the broken railing."));

	// Third hint with 3 progressive reveals
	auto hint3 = Element::create(ElementType::Hint, 4);
	hint3->title("What powers the grinding wheel?");
	subject->appendChild(hint3);

	auto group3a = GroupNode::create(0, 0);
	hint3->appendChild(group3a);
	group3a->appendChild(TextNode::create("The wheel needs a mechanical input."));

	hint3->appendChild(BreakNode::create());

	auto group3b = GroupNode::create(0, 0);
	hint3->appendChild(group3b);
	group3b->appendChild(TextNode::create("Look for something that fits the socket."));

	hint3->appendChild(BreakNode::create());

	auto group3c = GroupNode::create(0, 0);
	hint3->appendChild(group3c);
	group3c->appendChild(TextNode::create("Insert the bent crank into the socket."));

	// Fourth hint with 2 progressive reveals
	auto hint4 = Element::create(ElementType::Hint, 5);
	hint4->title("Where is the missing sprocket?");
	subject->appendChild(hint4);

	auto group4a = GroupNode::create(0, 0);
	hint4->appendChild(group4a);
	group4a->appendChild(TextNode::create("It is hidden somewhere in the workshop."));

	hint4->appendChild(BreakNode::create());

	auto group4b = GroupNode::create(0, 0);
	hint4->appendChild(group4b);
	group4b->appendChild(TextNode::create("Beneath the oily tarp in the corner."));

	return document;
}

std::shared_ptr<Document> buildDeepNesthintDocument() {
	auto [document, subject] = createDocument96a("Winding Stairwell Guide");

	// Single top-level nesthint under the subject
	auto nesthint = Element::create(ElementType::Nesthint, 2);
	nesthint->title("How do I unlock the iron grate?");
	subject->appendChild(nesthint);

	// Text group with hint inside
	auto textReveal = GroupNode::create(0, 0);
	nesthint->appendChild(textReveal);
	textReveal->appendChild(TextNode::create("Look for scratches on the floor."));

	auto hint = Element::create(ElementType::Hint, 3);
	hint->title("In what order?");
	textReveal->appendChild(hint);

	auto hintGroup1 = GroupNode::create(0, 0);
	hint->appendChild(hintGroup1);
	hintGroup1->appendChild(
	    TextNode::create("Press the three crescent-shaped tiles in sequence."));

	auto hintGroup2 = GroupNode::create(0, 0);
	hint->appendChild(hintGroup2);
	hintGroup2->appendChild(TextNode::create("Top right, bottom left, then center."));

	// Nesthint inside the same group as the text and hint
	auto nesthint1 = Element::create(ElementType::Nesthint, 4);
	nesthint1->title("Is there another way through?");
	textReveal->appendChild(nesthint1);

	auto nesthint1Group = GroupNode::create(0, 0);
	nesthint1->appendChild(nesthint1Group);
	nesthint1Group->appendChild(TextNode::create("Not without the skeleton key."));

	// Remaining nesthints each wrapped in their own group, separated by breaks
	nesthint->appendChild(BreakNode::create());

	auto wrap2 = GroupNode::create(0, 0);
	nesthint->appendChild(wrap2);

	auto nesthint2 = Element::create(ElementType::Nesthint, 5);
	nesthint2->title("What does the tile layout look like?");
	wrap2->appendChild(nesthint2);

	auto nesthint2Group = GroupNode::create(0, 0);
	nesthint2->appendChild(nesthint2Group);
	nesthint2Group->appendChild(TextNode::create("Refer to the diagram below."));

	auto text = Element::create(ElementType::Text, 6);
	text->title("Tile diagram");
	text->attr("format", 0);
	nesthint2Group->appendChild(text);

	auto textGroup = GroupNode::create(0, 0);
	text->appendChild(textGroup);
	textGroup->appendChild(
	    TextNode::create("[TL] [TR]\n[BL] [BR]\n [CENTER]", TextFormat::Monospace));

	nesthint->appendChild(BreakNode::create());

	auto wrap3 = GroupNode::create(0, 0);
	nesthint->appendChild(wrap3);

	auto nesthint3 = Element::create(ElementType::Nesthint, 7);
	nesthint3->title("Can I skip this puzzle?");
	wrap3->appendChild(nesthint3);

	auto nesthint3Group = GroupNode::create(0, 0);
	nesthint3->appendChild(nesthint3Group);
	nesthint3Group->appendChild(
	    TextNode::create("No, the grate blocks the only path forward."));

	nesthint->appendChild(BreakNode::create());

	auto wrap4 = GroupNode::create(0, 0);
	nesthint->appendChild(wrap4);

	auto nesthint4 = Element::create(ElementType::Nesthint, 8);
	nesthint4->title("What happens if I press the wrong tile?");
	wrap4->appendChild(nesthint4);

	auto nesthint4Group1 = GroupNode::create(0, 0);
	nesthint4->appendChild(nesthint4Group1);
	nesthint4Group1->appendChild(
	    TextNode::create("The tiles reset and you must start over."));

	nesthint4->appendChild(BreakNode::create());

	auto nesthint4Group2 = GroupNode::create(0, 0);
	nesthint4->appendChild(nesthint4Group2);
	nesthint4Group2->appendChild(
	    TextNode::create("Listen for the chime after each correct press."));

	nesthint4->appendChild(BreakNode::create());

	auto nesthint4Group3 = GroupNode::create(0, 0);
	nesthint4->appendChild(nesthint4Group3);
	nesthint4Group3->appendChild(TextNode::create(
	    "The sequence is always the same: top right, bottom left, center."));

	return document;
}

std::shared_ptr<Document> buildBreakNodesDocument() {
	auto [document, subject] = createDocument96a("Patched Satchel Compendium");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("How do I descend the shaft safely?");
	subject->appendChild(hint);

	auto group1 = GroupNode::create(0, 0);
	hint->appendChild(group1);
	group1->appendChild(TextNode::create("Secure the knotted rope to the beam."));

	auto breakNode = BreakNode::create();
	hint->appendChild(breakNode);

	auto group2 = GroupNode::create(0, 0);
	hint->appendChild(group2);
	group2->appendChild(
	    TextNode::create("Wrap the cloth around your hands before sliding."));

	return document;
}

std::shared_ptr<Document> buildBlankElementsDocument() {
	auto [document, subject] = createDocument96a("Faded Tapestry Ledger");

	auto hint1 = Element::create(ElementType::Hint, 2);
	hint1->title("Where is the hidden lever?");
	subject->appendChild(hint1);

	auto group1 = GroupNode::create(0, 0);
	hint1->appendChild(group1);
	group1->appendChild(TextNode::create("Behind the loose bark on the eastern trunk."));

	auto blank = Element::create(ElementType::Blank);
	subject->appendChild(blank);

	auto hint2 = Element::create(ElementType::Hint, 3);
	hint2->title("What does the lever activate?");
	subject->appendChild(hint2);

	auto group2 = GroupNode::create(0, 0);
	hint2->appendChild(group2);
	group2->appendChild(TextNode::create("A trapdoor beneath the root cluster."));

	return document;
}

std::shared_ptr<Document> buildCrossLinksDocument() {
	auto [document, rootSubject] = createDocument96a("Splintered Compass Atlas");

	// Child subject 1: hint + link to child subject 2
	auto subject1 = Element::create(ElementType::Subject, 2);
	subject1->title("Flooded Cellar");
	rootSubject->appendChild(subject1);

	auto hint1 = Element::create(ElementType::Hint, 3);
	hint1->title("How do I drain the water?");
	subject1->appendChild(hint1);

	auto group1 = GroupNode::create(0, 0);
	hint1->appendChild(group1);
	group1->appendChild(TextNode::create("Find the valve in the adjacent room."));

	auto link1 = Element::create(ElementType::Link, 4);
	link1->title("See also: Boiler Room");
	link1->body("5");
	subject1->appendChild(link1);

	// Child subject 2: hint + nesthint + link back to child subject 1
	auto subject2 = Element::create(ElementType::Subject, 5);
	subject2->title("Boiler Room");
	rootSubject->appendChild(subject2);

	auto hint2 = Element::create(ElementType::Hint, 6);
	hint2->title("Where is the main valve?");
	subject2->appendChild(hint2);

	auto group2 = GroupNode::create(0, 0);
	hint2->appendChild(group2);
	group2->appendChild(TextNode::create("Mounted on the rusted pipe near the furnace."));

	auto nesthint = Element::create(ElementType::Nesthint, 7);
	nesthint->title("How do I turn it?");
	subject2->appendChild(nesthint);

	auto nesthintGroup = GroupNode::create(0, 0);
	nesthint->appendChild(nesthintGroup);
	nesthintGroup->appendChild(
	    TextNode::create("Use the wrench from the flooded cellar."));

	auto link2 = Element::create(ElementType::Link, 8);
	link2->title("See also: Flooded Cellar");
	link2->body("2");
	subject2->appendChild(link2);

	return document;
}

// Version builders

std::shared_ptr<Document> build88aDocument() {
	auto document = Document::create(VersionType::Version88a);
	document->title("Wobbling Parchment Finder");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Crumpled Envelope Dilemma");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where is the speckled lantern?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Tilt the crooked sundial leftward."));

	return document;
}

std::shared_ptr<Document> build91aDocument() {
	auto [document, subject] =
	    createDocument96a("Glinting Thimble Archives", VersionType::Version91a);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("How do I open the brass valve?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(
	    TextNode::create("The amber flask rests beneath the mossy threshold."));

	return document;
}

std::shared_ptr<Document> build95aDocument() {
	auto [document, subject] =
	    createDocument96a("Muddled Keystone Gazette", VersionType::Version95a);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("What triggers the revolving shelf?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Pull the frayed cord beside the stone basin."));

	return document;
}

std::shared_ptr<Document> build96aDocument() {
	auto [document, subject] = createDocument96a("Crimpled Bellows Manual");

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Where does the tilted mirror point?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(
	    TextNode::create("Examine the chipped gargoyle near the fountain."));

	return document;
}

// Registry

std::vector<BuilderEntry> const& allBuilders() {
	static std::vector<BuilderEntry> const builders = {
	    // Formatting
	    {buildMixedMonospaceHintDocument, "mixed_monospace_hint"},
	    {buildMonospaceOverflowTextDocument, "monospace_overflow_text"},
	    {buildMonospaceTextDocument, "monospace_text"},
	    {buildOverflowTextDocument, "overflow_text"},
	    {buildPlainHyperlinkPlainTextDocument, "plain_hyperlink_plain_text"},
	    {buildPlainMonospacePlainTextDocument, "plain_monospace_plain_text"},
	    {buildPlainTextDocument, "plain_text"},

	    // Media
	    {buildGifaDocument, "gifa"},
	    {buildHyperpngLinkDocument, "hyperpng_link"},
	    {buildHyperpngOverlayDocument, "hyperpng_overlay"},
	    {buildHyperpngSimpleDocument, "hyperpng_simple"},
	    {buildSoundDocument, "sound"},

	    // Metadata
	    {buildCommentDocument, "comment"},
	    {buildCreditDocument, "credit"},
	    {buildDocumentAttributesDocument, "document_attributes"},
	    {buildIncentiveDocument, "incentive", true},
	    {buildVersionMetadataDocument, "version_metadata"},

	    // Structure
	    {buildBlankElementsDocument, "blank_elements"},
	    {buildBreakNodesDocument, "break_nodes"},
	    {buildCrossLinksDocument, "cross_links"},
	    {buildDeepNesthintDocument, "deep_nesthint"},
	    {buildMultipleHintsDocument, "multiple_hints"},

	    // Version
	    {build88aDocument, "88a_version"},
	    {build91aDocument, "91a_version"},
	    {build95aDocument, "95a_version"},
	    {build96aDocument, "96a_version"},
	};

	return builders;
}

} // namespace UHS

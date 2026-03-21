#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "uhs.h"

namespace UHS {

static std::string writeHTML(std::shared_ptr<Document> document) {
	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	HTMLWriter writer(logger, out, options);
	writer.write(document);
	return out.str();
}

TEST_CASE("HTMLWriter produces HTML5 document structure", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Test Game");

	auto output = writeHTML(document);

	REQUIRE(output.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(output.find("<html lang=\"en\">") != std::string::npos);
	REQUIRE(output.find("<title>Test Game</title>") != std::string::npos);
	REQUIRE(output.find("<meta charset=\"utf-8\"") != std::string::npos);
	REQUIRE(output.find("<main id=\"root\"") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes subject element with title", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Chapter 1");
	document->appendChild(subject);

	auto output = writeHTML(document);

	REQUIRE(output.find("data-type=\"subject\"") != std::string::npos);
	REQUIRE(output.find("class=\"title\"") != std::string::npos);
	REQUIRE(output.find("Chapter 1") != std::string::npos);
}

TEST_CASE("HTMLWriter assigns data-id to elements", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 42);
	subject->title("Sub");
	document->appendChild(subject);

	auto output = writeHTML(document);

	REQUIRE(output.find("data-id=\"42\"") != std::string::npos);
	REQUIRE(output.find("id=\"42\"") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes hint element with ordered list", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("How do I solve the puzzle?");
	subject->appendChild(hint);

	auto output = writeHTML(document);

	REQUIRE(output.find("How do I solve the puzzle?") != std::string::npos);
	REQUIRE(output.find("<ol") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes text node content", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Hint");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("Look under the rock.");
	group->appendChild(textNode);

	auto output = writeHTML(document);

	REQUIRE(output.find("Look under the rock.") != std::string::npos);
}

TEST_CASE("HTMLWriter applies monospace class to text nodes", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Hint");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("code here", TextFormat::Monospace);
	group->appendChild(textNode);

	auto output = writeHTML(document);

	REQUIRE(output.find("monospace") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes blank element as hr", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto blank = Element::create(ElementType::Blank);
	document->appendChild(blank);

	auto output = writeHTML(document);

	REQUIRE(output.find("<hr") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes comment with body", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto comment = Element::create(ElementType::Comment);
	comment->title("Note");
	comment->body("This is a comment.");
	document->appendChild(comment);

	auto output = writeHTML(document);

	REQUIRE(output.find("Note") != std::string::npos);
	REQUIRE(output.find("This is a comment.") != std::string::npos);
}

TEST_CASE(
    "HTMLWriter sets visibility attribute for non-default visibility", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto element = Element::create(ElementType::Subject, 1);
	element->title("Sub");
	element->visibility(VisibilityType::RegisteredOnly);
	document->appendChild(element);

	auto output = writeHTML(document);

	REQUIRE(output.find("data-visibility=\"registered\"") != std::string::npos);
}

TEST_CASE(
    "HTMLWriter serializes info element with document attributes", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");
	document->attr("author", "Jane Doe");
	document->attr("publisher", "Big Games");

	auto info = Element::create(ElementType::Info);
	document->appendChild(info);

	auto output = writeHTML(document);

	REQUIRE(output.find("Jane Doe") != std::string::npos);
	REQUIRE(output.find("Big Games") != std::string::npos);
	REQUIRE(output.find("<dl") != std::string::npos);
}

TEST_CASE("HTMLWriter serializes link element as anchor", "[writer][html]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Sub");
	document->appendChild(subject);

	auto target = Element::create(ElementType::Hint, 10);
	target->title("Target Hint");
	subject->appendChild(target);

	auto link = Element::create(ElementType::Link, 3);
	link->title("Go to target");
	link->body("10");
	subject->appendChild(link);

	auto output = writeHTML(document);

	REQUIRE(output.find("Go to target") != std::string::npos);
	REQUIRE(output.find("href=\"#10\"") != std::string::npos);
	REQUIRE(output.find("data-target=\"10\"") != std::string::npos);
}

} // namespace UHS

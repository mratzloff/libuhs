#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node/break_node.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/options.h"
#include "uhs/writer/json_writer.h"

namespace UHS {

TEST_CASE("JSONWriter outputs valid JSON with document", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Test Game");

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"title\"") != std::string::npos);
	REQUIRE(output.find("\"Test Game\"") != std::string::npos);
	REQUIRE(output.find("\"version\"") != std::string::npos);
	REQUIRE(output.find("\"96a\"") != std::string::npos);
	REQUIRE(output.find("\"type\"") != std::string::npos);
	REQUIRE(output.find("\"document\"") != std::string::npos);
}

TEST_CASE("JSONWriter includes validChecksum for non-88a", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");
	document->validChecksum(true);

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"validChecksum\"") != std::string::npos);
	REQUIRE(output.find("true") != std::string::npos);
}

TEST_CASE("JSONWriter serializes elements", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Chapter 1");
	document->appendChild(subject);

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"Chapter 1\"") != std::string::npos);
	REQUIRE(output.find("\"subject\"") != std::string::npos);
	REQUIRE(output.find("\"children\"") != std::string::npos);
}

TEST_CASE("JSONWriter serializes text nodes with format attributes", "[writer][json]") {
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

	auto textNode = TextNode::create("Hello", TextFormat::Monospace);
	group->appendChild(textNode);

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"body\" : \"Hello\"") != std::string::npos);
	REQUIRE(output.find("\"monospace\" : true") != std::string::npos);
	REQUIRE(output.find("\"overflow\" : false") != std::string::npos);
}

TEST_CASE("JSONWriter serializes element attributes", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");
	document->attr("author", "Test Author");

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"attributes\"") != std::string::npos);
	REQUIRE(output.find("\"author\"") != std::string::npos);
	REQUIRE(output.find("\"Test Author\"") != std::string::npos);
}

TEST_CASE("JSONWriter serializes element visibility", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto element = Element::create(ElementType::Subject, 1);
	element->title("Sub");
	element->visibility(VisibilityType::RegisteredOnly);
	document->appendChild(element);

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"registered\"") != std::string::npos);
}

TEST_CASE("JSONWriter serializes break nodes as separator", "[writer][json]") {
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

	auto textNode = TextNode::create("Before");
	group->appendChild(textNode);

	auto breakNode = BreakNode::create();
	hint->appendChild(breakNode);

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"-\"") != std::string::npos);
}

TEST_CASE("JSONWriter handles deeply nested document", "[writer][json]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	// Build a chain of nested subjects up to a safe depth
	auto parent = std::static_pointer_cast<Node>(document);
	for (int i = 1; i <= 10; ++i) {
		auto subject = Element::create(ElementType::Subject, i);
		subject->title("Depth " + std::to_string(i));
		parent->appendChild(subject);
		parent = subject;
	}

	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);

	REQUIRE_NOTHROW(writer.write(document));
	auto output = out.str();
	REQUIRE(output.find("\"Depth 10\"") != std::string::npos);
}

} // namespace UHS

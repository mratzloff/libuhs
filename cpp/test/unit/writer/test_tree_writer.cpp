#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/options.h"
#include "uhs/writer/tree_writer.h"

namespace UHS {

TEST_CASE("TreeWriter outputs document title", "[writer][tree]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Test Game");

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out;
	TreeWriter writer{logger, out, options};

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("[document]") != std::string::npos);
	REQUIRE(output.find("\"Test Game\"") != std::string::npos);
}

TEST_CASE("TreeWriter outputs element types", "[writer][tree]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Chapter 1");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint, 2);
	hint->title("Puzzle");
	subject->appendChild(hint);

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out;
	TreeWriter writer{logger, out, options};

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("[subject]") != std::string::npos);
	REQUIRE(output.find("\"Chapter 1\"") != std::string::npos);
	REQUIRE(output.find("[hint]") != std::string::npos);
	REQUIRE(output.find("\"Puzzle\"") != std::string::npos);
}

TEST_CASE("TreeWriter outputs text nodes", "[writer][tree]") {
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

	auto textNode = TextNode::create("This is a hint.");
	group->appendChild(textNode);

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out;
	TreeWriter writer{logger, out, options};

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("\"This is a hint.\"") != std::string::npos);
}

TEST_CASE("TreeWriter truncates long text", "[writer][tree]") {
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

	std::string longText(200, 'a');
	auto textNode = TextNode::create(longText);
	group->appendChild(textNode);

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out;
	TreeWriter writer{logger, out, options};

	writer.write(document);

	auto output = out.str();
	REQUIRE(output.find("...") != std::string::npos);
}

TEST_CASE("TreeWriter draws scaffold for nested nodes", "[writer][tree]") {
	auto document = Document::create(VersionType::Version96a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject, 1);
	subject->title("Sub");
	document->appendChild(subject);

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out;
	TreeWriter writer{logger, out, options};

	writer.write(document);

	auto output = out.str();

	// Should have tree-drawing characters
	REQUIRE(
	    (output.find("└") != std::string::npos || output.find("├") != std::string::npos));
}

} // namespace UHS

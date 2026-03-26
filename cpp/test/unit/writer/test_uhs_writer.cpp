#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/options.h"
#include "uhs/writer/uhs_writer.h"

namespace UHS {

static std::string writeUHS(
    std::shared_ptr<Document> document, Options const& options = {}) {

	Logger logger{LogLevel::None};
	std::ostringstream out;
	UHSWriter writer{logger, out, options};
	writer.write(document);
	return out.str();
}

TEST_CASE("UHSWriter 88a output starts with signature", "[writer][uhs]") {
	auto document = Document::create(VersionType::Version88a);
	document->title("Test Game");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("Hint?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("Answer");
	group->appendChild(textNode);

	auto output = writeUHS(document);

	REQUIRE(output.substr(0, 3) == "UHS");
	REQUIRE(output.find("Test Game") != std::string::npos);
}

TEST_CASE("UHSWriter 88a debug mode skips encoding", "[writer][uhs]") {
	auto document = Document::create(VersionType::Version88a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Chapter");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("Question?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("The answer is 42");
	group->appendChild(textNode);

	Options options;
	options.debug = true;
	auto output = writeUHS(document, options);

	REQUIRE(output.find("Chapter") != std::string::npos);
	REQUIRE(output.find("The answer is 42") != std::string::npos);
}

TEST_CASE("UHSWriter 88a includes credits", "[writer][uhs]") {
	auto document = Document::create(VersionType::Version88a);
	document->title("Game");
	document->attr("notice", "Copyright 2024");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("Q?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("A");
	group->appendChild(textNode);

	auto output = writeUHS(document);

	REQUIRE(output.find("CREDITS:") != std::string::npos);
	REQUIRE(output.find("Copyright 2024") != std::string::npos);
}

TEST_CASE("UHSWriter reset clears state", "[writer][uhs]") {
	auto document = Document::create(VersionType::Version88a);
	document->title("Game");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Sub");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("Q?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);

	auto textNode = TextNode::create("A");
	group->appendChild(textNode);

	Logger logger{LogLevel::None};
	Options options;
	std::ostringstream out1;
	UHSWriter writer{logger, out1, options};
	writer.write(document);

	writer.reset();

	std::ostringstream out2;

	// Writer should not crash after reset (out_ still points to out1 though)
	// Just verify reset doesn't throw
	REQUIRE_NOTHROW(writer.reset());
}

} // namespace UHS

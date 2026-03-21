#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

using namespace UHS;

TEST_CASE("Node::typeString", "[node]") {
	REQUIRE(Node::typeString(NodeType::Break) == "break");
	REQUIRE(Node::typeString(NodeType::Document) == "document");
	REQUIRE(Node::typeString(NodeType::Element) == "element");
	REQUIRE(Node::typeString(NodeType::Group) == "group");
	REQUIRE(Node::typeString(NodeType::Text) == "text");
}

TEST_CASE("Node type checks", "[node]") {
	auto textNode = TextNode::create("hello");
	REQUIRE(textNode->isText());
	REQUIRE_FALSE(textNode->isElement());
	REQUIRE_FALSE(textNode->isBreak());
	REQUIRE_FALSE(textNode->isGroup());
	REQUIRE_FALSE(textNode->isDocument());

	auto breakNode = BreakNode::create();
	REQUIRE(breakNode->isBreak());

	auto groupNode = GroupNode::create(1, 5);
	REQUIRE(groupNode->isGroup());

	auto element = Element::create(ElementType::Hint, 1);
	REQUIRE(element->isElement());

	auto document = Document::create(VersionType::Version96a);
	REQUIRE(document->isDocument());
}

TEST_CASE("Node::isElementOfType", "[node]") {
	auto hint = Element::create(ElementType::Hint, 1);
	REQUIRE(Node::isElementOfType(*hint, ElementType::Hint));
	REQUIRE_FALSE(Node::isElementOfType(*hint, ElementType::Text));

	auto textNode = TextNode::create("hello");
	REQUIRE_FALSE(Node::isElementOfType(*textNode, ElementType::Hint));
}

TEST_CASE("Node::appendChild builds tree", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element = Element::create(ElementType::Subject, 1);
	auto textNode = TextNode::create("hello");

	document->appendChild(element);
	element->appendChild(textNode);

	REQUIRE(document->numChildren() == 1);
	REQUIRE(document->firstChild() == element);
	REQUIRE(element->numChildren() == 1);
	REQUIRE(element->firstChild() == textNode);
	REQUIRE(textNode->parent() == element.get());
	REQUIRE(element->parent() == document.get());
}

TEST_CASE("Node::removeChild", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 2);
	auto element3 = Element::create(ElementType::Text, 3);

	document->appendChild(element1);
	document->appendChild(element2);
	document->appendChild(element3);
	REQUIRE(document->numChildren() == 3);

	SECTION("remove middle child") {
		document->removeChild(element2);
		REQUIRE(document->numChildren() == 2);
		REQUIRE(document->firstChild() == element1);
		REQUIRE(element1->nextSibling() == element3);
	}

	SECTION("remove first child") {
		document->removeChild(element1);
		REQUIRE(document->numChildren() == 2);
		REQUIRE(document->firstChild() == element2);
	}

	SECTION("remove last child") {
		document->removeChild(element3);
		REQUIRE(document->numChildren() == 2);
		REQUIRE(element2->nextSibling() == nullptr);
	}
}

TEST_CASE("Node::insertBefore", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 2);
	auto element3 = Element::create(ElementType::Text, 3);

	document->appendChild(element1);
	document->appendChild(element3);

	document->insertBefore(element2, element3.get());

	REQUIRE(document->numChildren() == 3);
	REQUIRE(document->firstChild() == element1);
	REQUIRE(element1->nextSibling() == element2);
	REQUIRE(element2->nextSibling() == element3);
}

TEST_CASE("Node::insertBefore at head", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 2);

	document->appendChild(element2);
	document->insertBefore(element1, element2.get());

	REQUIRE(document->firstChild() == element1);
	REQUIRE(element1->nextSibling() == element2);
}

TEST_CASE("Node sibling navigation", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto element2 = Element::create(ElementType::Hint, 2);

	document->appendChild(element1);
	document->appendChild(element2);

	REQUIRE(element1->hasNextSibling());
	REQUIRE(element1->nextSibling() == element2);
	REQUIRE_FALSE(element2->hasNextSibling());

	REQUIRE(element2->hasPreviousSibling());
	REQUIRE(element2->previousSibling() == element1.get());
	REQUIRE_FALSE(element1->hasPreviousSibling());
}

TEST_CASE("Node depth tracking", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element = Element::create(ElementType::Subject, 1);
	auto child = Element::create(ElementType::Hint, 2);
	auto grandchild = TextNode::create("deep");

	document->appendChild(element);
	element->appendChild(child);
	child->appendChild(grandchild);

	REQUIRE(document->depth() == 0);
	REQUIRE(element->depth() == 1);
	REQUIRE(child->depth() == 2);
	REQUIRE(grandchild->depth() == 3);
}

TEST_CASE("Node iteration traverses tree depth-first", "[node]") {
	auto document = Document::create(VersionType::Version96a);
	auto element1 = Element::create(ElementType::Subject, 1);
	auto text1 = TextNode::create("a");
	auto element2 = Element::create(ElementType::Hint, 2);
	auto text2 = TextNode::create("b");

	document->appendChild(element1);
	element1->appendChild(text1);
	document->appendChild(element2);
	element2->appendChild(text2);

	std::vector<NodeType> visited;
	for (auto& node : *document) {
		visited.push_back(node.nodeType());
	}

	REQUIRE(visited.size() == 5);
	REQUIRE(visited[0] == NodeType::Document);
	REQUIRE(visited[1] == NodeType::Element);
	REQUIRE(visited[2] == NodeType::Text);
	REQUIRE(visited[3] == NodeType::Element);
	REQUIRE(visited[4] == NodeType::Text);
}

TEST_CASE("TextNode format operations", "[node]") {
	auto textNode = TextNode::create("hello");
	REQUIRE(textNode->format() == TextFormat::None);

	textNode->addFormat(TextFormat::Monospace);
	REQUIRE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE_FALSE(textNode->hasFormat(TextFormat::Overflow));

	textNode->addFormat(TextFormat::Overflow);
	REQUIRE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE(textNode->hasFormat(TextFormat::Overflow));

	textNode->removeFormat(TextFormat::Monospace);
	REQUIRE_FALSE(textNode->hasFormat(TextFormat::Monospace));
	REQUIRE(textNode->hasFormat(TextFormat::Overflow));
}

TEST_CASE("TextNode::string returns body", "[node]") {
	auto textNode = TextNode::create("hello world");
	REQUIRE(textNode->string() == "hello world");
}

TEST_CASE("TextNode::clone produces independent copy", "[node]") {
	auto original = TextNode::create("hello", TextFormat::Monospace);
	auto cloned = original->clone();

	REQUIRE(cloned->string() == "hello");
	REQUIRE(cloned->hasFormat(TextFormat::Monospace));
	REQUIRE_FALSE(cloned->hasParent());

	cloned->body("changed");
	REQUIRE(original->string() == "hello");
}

TEST_CASE("BreakNode", "[node]") {
	auto breakNode = BreakNode::create();
	REQUIRE(breakNode->isBreak());
	REQUIRE(breakNode->nodeType() == NodeType::Break);

	auto cloned = breakNode->clone();
	REQUIRE(cloned->isBreak());
}

TEST_CASE("GroupNode", "[node]") {
	auto groupNode = GroupNode::create(5, 10);
	REQUIRE(groupNode->isGroup());
	REQUIRE(groupNode->line() == 5);
	REQUIRE(groupNode->length() == 10);

	auto cloned = groupNode->clone();
	REQUIRE(cloned->isGroup());
	REQUIRE(cloned->line() == 5);
	REQUIRE_FALSE(cloned->hasParent());
}

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "uhs/element.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"

namespace UHS {

TEST_CASE("NodeIterator traverses single node", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);

	int count = 0;
	for (auto const& node : *root) {
		REQUIRE(&node == root.get());
		++count;
	}
	REQUIRE(count == 1);
}

TEST_CASE("NodeIterator traverses children depth-first", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);
	auto child1 = Element::create(ElementType::Hint);
	auto child2 = Element::create(ElementType::Hint);
	auto grandchild = TextNode::create("hello");

	auto group = GroupNode::create(0, 0);
	group->appendChild(grandchild);
	child1->appendChild(group);
	root->appendChild(child1);
	root->appendChild(child2);

	std::vector<Node*> visited;
	for (auto const& node : *root) {
		visited.push_back(const_cast<Node*>(&node));
	}

	REQUIRE(visited.size() == 5);
	REQUIRE(visited[0] == root.get());
	REQUIRE(visited[1] == child1.get());
	REQUIRE(visited[2] == group.get());
	REQUIRE(visited[3] == grandchild.get());
	REQUIRE(visited[4] == child2.get());
}

TEST_CASE("NodeIterator visits siblings before ascending", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);
	auto child1 = Element::create(ElementType::Hint);
	auto child2 = Element::create(ElementType::Hint);
	auto child3 = Element::create(ElementType::Hint);

	root->appendChild(child1);
	root->appendChild(child2);
	root->appendChild(child3);

	std::vector<Node*> visited;
	for (auto const& node : *root) {
		visited.push_back(const_cast<Node*>(&node));
	}

	REQUIRE(visited.size() == 4);
	REQUIRE(visited[0] == root.get());
	REQUIRE(visited[1] == child1.get());
	REQUIRE(visited[2] == child2.get());
	REQUIRE(visited[3] == child3.get());
}

TEST_CASE("NodeIterator handles deep nesting", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);
	auto level1 = Element::create(ElementType::Nesthint);
	auto level2 = GroupNode::create(0, 0);
	auto level3 = TextNode::create("deep");

	level2->appendChild(level3);
	level1->appendChild(level2);
	root->appendChild(level1);

	std::vector<Node*> visited;
	for (auto const& node : *root) {
		visited.push_back(const_cast<Node*>(&node));
	}

	REQUIRE(visited.size() == 4);
	REQUIRE(visited[0] == root.get());
	REQUIRE(visited[1] == level1.get());
	REQUIRE(visited[2] == level2.get());
	REQUIRE(visited[3] == level3.get());
}

TEST_CASE("NodeIterator post-increment returns previous position", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);
	auto child = Element::create(ElementType::Hint);
	root->appendChild(child);

	auto it = root->begin();
	auto prev = it++;

	REQUIRE(&(*prev) == root.get());
	REQUIRE(&(*it) == child.get());
}

TEST_CASE("NodeIterator equality", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);

	auto begin1 = root->begin();
	auto begin2 = root->begin();
	auto end = root->end();

	REQUIRE(begin1 == begin2);
	REQUIRE(begin1 != end);

	++begin1;
	REQUIRE(begin1 == end);
}

TEST_CASE("NodeIterator end is reached after full traversal", "[node_iterator]") {
	auto root = Element::create(ElementType::Subject);
	auto child1 = Element::create(ElementType::Hint);
	auto child2 = Element::create(ElementType::Hint);
	root->appendChild(child1);
	root->appendChild(child2);

	auto it = root->begin();
	++it; // child1
	++it; // child2
	++it; // end

	REQUIRE(it == root->end());
}

} // namespace UHS

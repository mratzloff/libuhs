#include "uhs.h"

namespace UHS {

void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(lhs.line_, rhs.line_);
	swap(lhs.length_, rhs.length_);
}

ContainerNode::ContainerNode(NodeType type) : Node(type) {}

ContainerNode::ContainerNode(ContainerNode const& other)
    : Node(other), line_{other.line_}, length_{other.length_} {}

// Used only for bookkeeping while parsing
int ContainerNode::length() const {
	return length_;
}

void ContainerNode::length(int const length) {
	length_ = length;
}

int ContainerNode::line() const {
	return line_;
}

void ContainerNode::line(int const line) {
	line_ = line;
}

ContainerNode& ContainerNode::operator=(ContainerNode other) {
	Node::operator=(other);
	return *this;
}

} // namespace UHS

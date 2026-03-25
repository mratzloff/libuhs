#include "uhs/node/container_node.h"

namespace UHS {

void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(lhs.length_, rhs.length_);
	swap(lhs.line_, rhs.line_);
}

ContainerNode::ContainerNode(NodeType type) : Node(type) {}

ContainerNode::ContainerNode(ContainerNode const& other)
    : Node(other), length_{other.length_}, line_{other.line_} {}

ContainerNode::ContainerNode(ContainerNode&& other) noexcept
    : Node(std::move(other)), length_{other.length_}, line_{other.line_} {

	other.length_ = 0;
	other.line_ = 0;
}

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
	swap(*this, other);
	return *this;
}

} // namespace UHS

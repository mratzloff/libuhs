#include "uhs.h"

namespace UHS {

void swap(GroupNode& lhs, GroupNode& rhs) noexcept {
	std::swap(static_cast<ContainerNode&>(lhs), static_cast<ContainerNode&>(rhs));
}

GroupNode::GroupNode(int line, int length) : ContainerNode(NodeType::Group) {
	line_ = line;
	length_ = length;
}

GroupNode::GroupNode(GroupNode const& other) : ContainerNode(other) {}

GroupNode::GroupNode(GroupNode&& other) noexcept : ContainerNode(std::move(other)) {}

std::shared_ptr<GroupNode> GroupNode::create(int line, int length) {
	return std::make_shared<GroupNode>(line, length);
}

// Copies and returns a detached element with its children.
std::shared_ptr<GroupNode> GroupNode::clone() const {
	auto groupNode = std::make_shared<GroupNode>(*this);
	groupNode->detachParent();
	return groupNode;
}

GroupNode& GroupNode::operator=(GroupNode other) {
	swap(*this, other);
	return *this;
}

} // namespace UHS

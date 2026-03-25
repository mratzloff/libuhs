#include "uhs/node/break_node.h"

namespace UHS {

BreakNode::BreakNode() : Node(NodeType::Break) {}

BreakNode::BreakNode(BreakNode const& other) : Node(other) {}

BreakNode::BreakNode(BreakNode&& other) noexcept : Node(std::move(other)) {}

std::shared_ptr<BreakNode> BreakNode::create() {
	return std::make_shared<BreakNode>();
}

std::shared_ptr<BreakNode> BreakNode::clone() const {
	return std::make_shared<BreakNode>(*this);
}

BreakNode& BreakNode::operator=(BreakNode other) {
	std::swap(*this, other);
	return *this;
}

} // namespace UHS

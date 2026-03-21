#include "uhs.h"

namespace UHS {

void swap(Node& lhs, Node& rhs) noexcept {
	using std::swap;

	swap(lhs.nodeType_, rhs.nodeType_);
	swap(lhs.parent_, rhs.parent_);
	swap(lhs.previousSibling_, rhs.previousSibling_);
	swap(lhs.nextSibling_, rhs.nextSibling_);
	swap(lhs.depth_, rhs.depth_);

	lhs.cloneChildren(rhs);
}

Node::Node(NodeType type) : nodeType_{type} {}

Node::Node(Node const& other)
    : depth_{other.depth_}
    , nextSibling_{other.nextSibling_}
    , nodeType_{other.nodeType_}
    , parent_{other.parent_}
    , previousSibling_{other.previousSibling_} {

	this->cloneChildren(other);
}

bool Node::isElementOfType(Node const& node, ElementType type) {
	if (node.isElement()) {
		auto const& element = static_cast<Element const&>(node);
		return (element.elementType() == type);
	}
	return false;
}

std::string const Node::typeString(NodeType type) {
	switch (type) {
	case NodeType::Break:
		return "break";
	case NodeType::Document:
		return "document";
	case NodeType::Element:
		return "element";
	case NodeType::Group:
		return "group";
	case NodeType::Text:
		return "text";
	default:
		throw DataError("invalid node type");
	}
}

void Node::appendChild(std::shared_ptr<Node> node) {
	assert(node);

	auto n = node.get();
	this->appendChild(node, true);
	n->didAdd();
}

// Note that this node must be attached to a document tree in order to
// correctly set the depth_ property when appending its own children.
void Node::appendChild(std::shared_ptr<Node> node, bool) {
	assert(node);

	if (lastChild_) {
		node->previousSibling_ = lastChild_;
		lastChild_->nextSibling_ = node;
	} else {
		firstChild_ = node;
	}
	lastChild_ = node.get();
	node->parent_ = this;

	for (auto& child : *node) {
		assert(child.parent_);
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;
}

Node::iterator Node::begin() {
	return Node::iterator(this);
}

Node::const_iterator Node::begin() const {
	return Node::const_iterator(this);
}

Node::const_iterator Node::cbegin() const {
	return Node::begin();
}

Node::const_iterator Node::cend() const {
	return Node::end();
}

int Node::depth() const {
	return depth_;
}

void Node::detachParent() {
	parent_ = nullptr;
}

Node::iterator Node::end() {
	return Node::iterator(nullptr);
}

Node::const_iterator Node::end() const {
	return Node::const_iterator(nullptr);
}

Document* Node::findDocument() const {
	for (auto node = parent_; node; node = node->parent()) {
		if (node->isDocument()) {
			return static_cast<Document*>(node);
		}
	}
	return nullptr; // Orphaned element
}

std::shared_ptr<Node> Node::firstChild() const {
	return firstChild_;
}

bool Node::hasFirstChild() const {
	return firstChild_ != nullptr;
}

bool Node::hasLastChild() const {
	return lastChild_ != nullptr;
}

bool Node::hasNextSibling() const {
	return nextSibling_ != nullptr;
}

bool Node::hasParent() const {
	return parent_ != nullptr;
}

bool Node::hasPreviousSibling() const {
	return previousSibling_ != nullptr;
}

// See note for appendChild() regarding depth.
void Node::insertBefore(std::shared_ptr<Node> node, Node* ref) {
	assert(node);

	if (!ref) {
		this->appendChild(node);
		return;
	}

	auto n = node.get();

	if (ref == firstChild_.get()) {
		node->nextSibling_ = firstChild_;
		firstChild_ = node;
	} else {
		if (!ref->hasPreviousSibling()) {
			throw DataError("expected previous sibling for node");
		}

		auto previous = ref->previousSibling();
		node->nextSibling_ = previous->nextSibling_;
		previous->nextSibling_ = node;
	}
	n->parent_ = this;

	for (auto& child : *n) {
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;

	n->didAdd();
}

bool Node::isBreak() const {
	return nodeType_ == NodeType::Break;
}

bool Node::isDocument() const {
	return nodeType_ == NodeType::Document;
}

bool Node::isElement() const {
	return nodeType_ == NodeType::Element;
}

bool Node::isGroup() const {
	return nodeType_ == NodeType::Group;
}

bool Node::isText() const {
	return nodeType_ == NodeType::Text;
}

Node* Node::lastChild() const {
	return lastChild_;
}

std::shared_ptr<Node> Node::nextSibling() const {
	return nextSibling_;
}

NodeType Node::nodeType() const {
	return nodeType_;
}

std::string const Node::nodeTypeString() const {
	return Node::typeString(nodeType_);
}

int Node::numChildren() const {
	return numChildren_;
}

Node& Node::operator=(Node other) {
	swap(*this, other);
	return *this;
}

Node* Node::parent() const {
	return parent_;
}

std::shared_ptr<Node> Node::pointer() const {
	if (parent_) {
		for (auto node = parent_->firstChild(); node; node = node->nextSibling()) {
			if (this == node.get()) {
				return node;
			}
		}
	}

	return nullptr;
}

Node* Node::previousSibling() const {
	return previousSibling_;
}

void Node::removeChild(std::shared_ptr<Node> node) {
	assert(node);
	node->detachParent();
	--numChildren_;

	if (node == firstChild_) {
		if (node->nextSibling_) {
			firstChild_ = node->nextSibling_;
			node->previousSibling_ = nullptr;
		} else {
			firstChild_ = nullptr;
			lastChild_ = nullptr;
		}
		node->didRemove();
	} else if (node->previousSibling_) {
		auto previous = node->previousSibling_;
		if (node->nextSibling_) {
			auto next = node->nextSibling_;
			next->previousSibling_ = previous;
			previous->nextSibling_ = next;
		} else {
			previous->nextSibling_ = nullptr;
			lastChild_ = nullptr;
		}
		node->didRemove();
	}
}

void Node::cloneChildren(Node const& other) {
	for (auto node = other.firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Break:
			this->appendChild(std::static_pointer_cast<BreakNode>(node)->clone(), true);
			break;
		case NodeType::Document:
			this->appendChild(std::static_pointer_cast<Document>(node)->clone(), true);
			break;
		case NodeType::Element:
			this->appendChild(std::static_pointer_cast<Element>(node)->clone(), true);
			break;
		case NodeType::Group:
			this->appendChild(std::static_pointer_cast<GroupNode>(node)->clone(), true);
			break;
		case NodeType::Text:
			this->appendChild(std::static_pointer_cast<TextNode>(node)->clone(), true);
			break;
		}
	}
}

void Node::didAdd() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		auto const element = static_cast<Element*>(this);
		document->elementAdded(*element);
	}
}

void Node::didRemove() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		auto const element = static_cast<Element*>(this);
		document->elementRemoved(*element);
	}
}

} // namespace UHS

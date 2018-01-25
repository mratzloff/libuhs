#include "uhs.h"

namespace UHS {

const std::string Node::typeString(NodeType type) {
	switch (type) {
	case NodeType::Break:
		return "break";
	case NodeType::Document:
		return "document";
	case NodeType::Element:
		return "element";
	case NodeType::Text:
		return "text";
	}
}

bool Node::isElementOfType(const Node& node, ElementType type) {
	if (node.isElement()) {
		const auto& element = static_cast<const Element&>(node);
		return (element.elementType() == type);
	}
	return false;
}

Node::Node(NodeType type) : nodeType_{type} {}

Node::Node(const Node& other) : nodeType_{other.nodeType_}, parent_{other.parent_} {
	this->cloneChildren(other);
}

Node& Node::operator=(Node other) {
	swap(*this, other);
	return *this;
}

void swap(Node& lhs, Node& rhs) noexcept {
	using std::swap;

	swap(lhs.nodeType_, rhs.nodeType_);
	swap(lhs.parent_, rhs.parent_);
	lhs.cloneChildren(rhs);
}

NodeType Node::nodeType() const {
	return nodeType_;
}

const std::string Node::nodeTypeString() const {
	return Node::typeString(nodeType_);
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

bool Node::isText() const {
	return nodeType_ == NodeType::Text;
}

void Node::detachParent() {
	parent_ = nullptr;
}

std::unique_ptr<Node> Node::removeChild(Node* node) {
	assert(node);

	node->parent_ = nullptr;
	--numChildren_;

	if (node == this->firstChild()) {
		auto detachedNode = std::move(firstChild_);
		if (node->hasNextSibling()) {
			firstChild_ = std::move(node->nextSibling_);
		} else {
			lastChild_ = nullptr;
		}
		node->didRemove();

		return detachedNode;
	} else {
		Node* previous = nullptr;
		for (auto n = this->firstChild(); n; n = n->nextSibling()) {
			if (previous && node == n) {
				auto detachedNode = std::move(previous->nextSibling_);
				if (node->hasNextSibling()) {
					previous->nextSibling_ = std::move(node->nextSibling_);
				} else {
					lastChild_ = nullptr;
				}
				node->didRemove();

				return detachedNode;
			} else {
				previous = n;
			}
		}
	}

	return nullptr;
}

void Node::appendChild(std::unique_ptr<Node> node) {
	assert(node);

	auto n = node.get();
	this->appendChild(std::move(node), true);
	n->didAdd();
}

// Note that this node must be attached to a document tree in order to
// correctly set the depth_ property when appending its own children.
void Node::appendChild(std::unique_ptr<Node> node, bool) {
	assert(node);

	auto n = node.get();
	if (this->hasLastChild()) { // No children
		lastChild_->nextSibling_ = std::move(node);
	} else {
		firstChild_ = std::move(node);
	}
	lastChild_ = n;
	n->parent_ = this;

	for (auto& child : *n) {
		assert(child.parent_);
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;
}

// See note for appendChild() regarding depth.
void Node::insertBefore(std::unique_ptr<Node> node, Node* ref) {
	assert(node);

	if (!ref) {
		this->appendChild(std::move(node));
		return;
	}

	auto n = node.get();

	if (ref == this->firstChild()) {
		node->nextSibling_ = std::move(firstChild_);
		firstChild_ = std::move(node);
	} else {
		Node* previous = nullptr;

		for (auto sibling = this->firstChild(); sibling;
		     sibling = sibling->nextSibling()) {
			if (sibling == ref) {
				break;
			}
			previous = sibling;
		}
		if (!previous) {
			return; // Not found; no action to take
		}
		node->nextSibling_ = std::move(previous->nextSibling_);
		previous->nextSibling_ = std::move(node);
	}
	n->parent_ = this;

	for (auto& child : *n) {
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;

	n->didAdd();
}

bool Node::hasParent() const {
	return parent_ != nullptr;
}

Node* Node::parent() const {
	return parent_;
}

bool Node::hasNextSibling() const {
	return nextSibling_ != nullptr;
}

Node* Node::nextSibling() const {
	return nextSibling_.get();
}

bool Node::hasFirstChild() const {
	return firstChild_ != nullptr;
}

Node* Node::firstChild() const {
	return firstChild_.get();
}

bool Node::hasLastChild() const {
	return lastChild_ != nullptr;
}

Node* Node::lastChild() const {
	return lastChild_;
}

int Node::numChildren() const {
	return numChildren_;
}

int Node::depth() const {
	return depth_;
}

Node::iterator Node::begin() {
	return Node::iterator(this);
}

Node::iterator Node::end() {
	return Node::iterator(nullptr);
}

Node::const_iterator Node::begin() const {
	return Node::const_iterator(this);
}

Node::const_iterator Node::end() const {
	return Node::const_iterator(nullptr);
}

Node::const_iterator Node::cbegin() const {
	return Node::begin();
}

Node::const_iterator Node::cend() const {
	return Node::end();
}

std::unique_ptr<Node> Node::cloneInternal(Passkey<Node>) const {
	return std::make_unique<Node>(*this);
}

void Node::cloneChildren(const Node& other) {
	for (auto node = other.firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Break:
			this->appendChild(
			    static_cast<BreakNode&>(*node).cloneInternal(Passkey<Node>()), true);
			break;
		case NodeType::Document:
			this->appendChild(
			    static_cast<Document&>(*node).cloneInternal(Passkey<Node>()), true);
			break;
		case NodeType::Element:
			this->appendChild(
			    static_cast<Element&>(*node).cloneInternal(Passkey<Node>()), true);
			break;
		case NodeType::Text:
			this->appendChild(
			    static_cast<TextNode&>(*node).cloneInternal(Passkey<Node>()), true);
			break;
		}
	}
}

void Node::didRemove() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		auto& element = static_cast<Element&>(*this);
		document->elementRemoved(element);
	}
}

void Node::didAdd() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		auto& element = static_cast<Element&>(*this);
		document->elementAdded(element);
	}
}

Document* Node::findDocument() const {
	for (auto node = parent_; node; node = node->parent()) {
		if (node->isDocument()) {
			return static_cast<Document*>(node);
		}
	}
	return nullptr; // Orphaned element
}

//------------------------------ NodeIterator -------------------------------//

template<typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer node) {
	initial_ = node;
	current_ = node;
}

template<typename T>
typename NodeIterator<T>::reference NodeIterator<T>::operator*() const {
	return *current_;
}

template<typename T>
typename NodeIterator<T>::pointer NodeIterator<T>::operator->() const {
	return current_;
}

// ++it
template<typename T>
NodeIterator<T>& NodeIterator<T>::operator++() {
	do {
		if (!current_) {
			break;
		}
		if (current_->hasFirstChild() && !visited_) { // Down
			current_ = current_->firstChild();
			visited_ = false;
		} else if (current_->hasNextSibling()) { // Next
			current_ = current_->nextSibling();
			visited_ = false;
		} else { // Up
			current_ = current_->parent();
			if (current_ == initial_) {
				current_ = nullptr;
				break;
			}
			visited_ = true;
		}
	} while (visited_);

	return *this;
}

// it++
template<typename T>
NodeIterator<T> NodeIterator<T>::operator++(int) {
	auto self = *this;
	++(*this);
	return self;
}

template<typename T>
bool NodeIterator<T>::operator==(const NodeIterator<T>& rhs) {
	return current_ == rhs.current_;
}

template<typename T>
bool NodeIterator<T>::operator!=(const NodeIterator<T>& rhs) {
	return !(*this == rhs);
}

template class NodeIterator<Node>;
template class NodeIterator<const Node>;

//------------------------------- BreakNode --------------------------------//

std::unique_ptr<BreakNode> BreakNode::create() {
	return std::make_unique<BreakNode>();
}

BreakNode::BreakNode() : Node(NodeType::Break) {}

BreakNode::BreakNode(const BreakNode&) : Node(NodeType::Break) {}

BreakNode& BreakNode::operator=(BreakNode) {
	return *this;
}

std::unique_ptr<Node> BreakNode::cloneInternal(Passkey<Node>) const {
	return std::make_unique<BreakNode>(*this);
}

//-------------------------------- TextNode --------------------------------//

std::unique_ptr<TextNode> TextNode::create(const std::string body) {
	return std::make_unique<TextNode>(body);
}

std::unique_ptr<TextNode> TextNode::create(const std::string body, TextFormat format) {
	return std::make_unique<TextNode>(body, format);
}

TextNode::TextNode(const std::string body) : Node(NodeType::Text), Body(body) {}

TextNode::TextNode(const std::string body, TextFormat format)
    : Node(NodeType::Text), Body(body), format_{format} {}

TextNode::TextNode(const TextNode& other)
    : Node(other), Traits::Body(other), format_{other.format_} {}

TextNode& TextNode::operator=(TextNode other) {
	swap(*this, other);
	return *this;
}

void swap(TextNode& lhs, TextNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(lhs.format_, rhs.format_);
}

// Copies and returns a detached text node.
std::unique_ptr<TextNode> TextNode::clone() const {
	auto textNode = std::make_unique<TextNode>(*this);
	textNode->detachParent();
	return textNode;
}

const std::string& TextNode::string() const {
	return this->body();
}

void TextNode::addFormat(TextFormat format) {
	format_ |= format;
}

void TextNode::removeFormat(TextFormat format) {
	format_ &= ~format;
}

bool TextNode::hasFormat(TextFormat format) const {
	return ::UHS::hasFormat(format_, format);
}

TextFormat TextNode::format() const {
	return format_;
}

void TextNode::format(TextFormat format) {
	format_ = format;
}

std::unique_ptr<Node> TextNode::cloneInternal(Passkey<Node>) const {
	return std::make_unique<TextNode>(*this);
}

} // namespace UHS

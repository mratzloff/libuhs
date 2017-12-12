#include "uhs.h"

namespace UHS {

const std::string Node::typeString(NodeType t) {
	switch (t) {
	case NodeType::Document:
		return "document";
	case NodeType::Element:
		return "element";
	case NodeType::Text:
		return "text";
	}
}

bool Node::isElementOfType(const Node& n, ElementType t) {
	if (n.nodeType() == NodeType::Element) {
		const auto& e = static_cast<const Element&>(n);
		return (e.elementType() == t);
	}
	return false;
}

Node::Node(NodeType t) : nodeType_{t} {}

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

void Node::detachParent() {
	parent_ = nullptr;
}

std::unique_ptr<Node> Node::removeChild(Node* n) {
	assert(n);

	n->parent_ = nullptr;
	--numChildren_;

	if (n == this->firstChild()) {
		auto detachedNode = std::move(firstChild_);
		if (n->hasNextSibling()) {
			firstChild_ = std::move(n->nextSibling_);
		} else {
			lastChild_ = nullptr;
		}
		n->didRemove();

		return detachedNode;
	} else {
		Node* previous = nullptr;
		for (Node* child = this->firstChild(); child; child = child->nextSibling()) {
			if (previous && n == child) {
				auto detachedNode = std::move(previous->nextSibling_);
				if (n->hasNextSibling()) {
					previous->nextSibling_ = std::move(n->nextSibling_);
				} else {
					lastChild_ = nullptr;
				}
				n->didRemove();

				return detachedNode;
			} else {
				previous = child;
			}
		}
	}

	return nullptr;
}

void Node::appendChild(std::unique_ptr<Node> n) {
	assert(n);

	auto ptr = n.get();
	this->appendChild(std::move(n), true);
	ptr->didAdd();
}

// Note that this node must be attached to a document tree in order to
// correctly set the depth_ property when appending its own children.
void Node::appendChild(std::unique_ptr<Node> n, bool) {
	assert(n);

	auto ptr = n.get();
	if (this->hasLastChild()) { // No children
		lastChild_->nextSibling_ = std::move(n);
	} else {
		firstChild_ = std::move(n);
	}
	lastChild_ = ptr;
	ptr->parent_ = this;

	for (auto& child : *ptr) {
		assert(child.parent_);
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;
}

// See note for appendChild() regarding depth.
void Node::insertBefore(std::unique_ptr<Node> n, Node* ref) {
	assert(n);

	if (!ref) {
		this->appendChild(std::move(n));
		return;
	}

	auto ptr = n.get();

	if (ref == this->firstChild()) {
		n->nextSibling_ = std::move(firstChild_);
		firstChild_ = std::move(n);
	} else {
		Node* previous = nullptr;

		for (Node* sibling = this->firstChild(); sibling;
		     sibling = sibling->nextSibling()) {
			if (sibling == ref) {
				break;
			}
			previous = sibling;
		}
		if (!previous) {
			return; // Not found; no action to take
		}
		n->nextSibling_ = std::move(previous->nextSibling_);
		previous->nextSibling_ = std::move(n);
	}
	ptr->parent_ = this;

	for (auto& child : *ptr) {
		child.depth_ = child.parent_->depth_ + 1;
	}
	++numChildren_;

	ptr->didAdd();
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

std::unique_ptr<Node> Node::cloneInternal() const {
	return std::make_unique<Node>(*this);
}

void Node::cloneChildren(const Node& other) {
	for (Node* node = other.firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Document:
			this->appendChild(static_cast<Document&>(*node).cloneInternal(), true);
			break;
		case NodeType::Element:
			this->appendChild(static_cast<Element&>(*node).cloneInternal(), true);
			break;
		case NodeType::Text:
			this->appendChild(static_cast<TextNode&>(*node).cloneInternal(), true);
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
	for (Node* node = parent_; node; node = node->parent()) {
		if (node->nodeType() == NodeType::Document) {
			return static_cast<Document*>(node);
		}
	}
	return nullptr; // Orphaned element
}

//------------------------------ NodeIterator -------------------------------//

template<typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer n) {
	initial_ = n;
	current_ = n;
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
	auto tmp = *this;
	++(*this);
	return tmp;
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

//-------------------------------- TextNode --------------------------------//

std::unique_ptr<TextNode> TextNode::create(const std::string body) {
	return std::make_unique<TextNode>(body);
}

TextNode::TextNode(const std::string body) : Node(NodeType::Text), Body(body) {}

TextNode::TextNode(const TextNode& other)
    : Node(other), Traits::Body(other), fmt_{other.fmt_} {}

TextNode& TextNode::operator=(TextNode other) {
	swap(*this, other);
	return *this;
}

void swap(TextNode& lhs, TextNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(lhs.fmt_, rhs.fmt_);
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

void TextNode::addFormat(Format f) {
	fmt_ |= f;
}

void TextNode::removeFormat(Format f) {
	fmt_ &= ~f;
}

bool TextNode::hasFormat(Format f) const {
	return (fmt_ & f) != 0;
}

Format TextNode::format() const {
	return fmt_;
}

std::unique_ptr<Node> TextNode::cloneInternal() const {
	return std::make_unique<TextNode>(*this);
}

} // namespace UHS

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
	case NodeType::Group:
		return "group";
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

Node::Node(const Node& other)
    : nodeType_{other.nodeType_}
    , parent_{other.parent_}
    , previousSibling_{other.previousSibling_}
    , nextSibling_{other.nextSibling_}
    , depth_{other.depth_} {

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
	swap(lhs.previousSibling_, rhs.previousSibling_);
	swap(lhs.nextSibling_, rhs.nextSibling_);
	swap(lhs.depth_, rhs.depth_);

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

bool Node::isGroup() const {
	return nodeType_ == NodeType::Group;
}

bool Node::isText() const {
	return nodeType_ == NodeType::Text;
}

void Node::detachParent() {
	parent_ = nullptr;
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
			// TODO: Warn about data inconsistency
			return;
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

bool Node::hasParent() const {
	return parent_ != nullptr;
}

Node* Node::parent() const {
	return parent_;
}

bool Node::hasPreviousSibling() const {
	return previousSibling_ != nullptr;
}

Node* Node::previousSibling() const {
	return previousSibling_;
}

bool Node::hasNextSibling() const {
	return nextSibling_ != nullptr;
}

std::shared_ptr<Node> Node::nextSibling() const {
	return nextSibling_;
}

bool Node::hasFirstChild() const {
	return firstChild_ != nullptr;
}

std::shared_ptr<Node> Node::firstChild() const {
	return firstChild_;
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

Document* Node::findDocument() const {
	for (auto node = parent_; node; node = node->parent()) {
		if (node->isDocument()) {
			return static_cast<Document*>(node);
		}
	}
	return nullptr; // Orphaned element
}

void Node::cloneChildren(const Node& other) {
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

void Node::didRemove() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		const auto element = static_cast<Element*>(this);
		document->elementRemoved(*element);
	}
}

void Node::didAdd() {
	if (nodeType_ != NodeType::Element) {
		return;
	}
	if (auto document = this->findDocument()) {
		const auto element = static_cast<Element*>(this);
		document->elementAdded(*element);
	}
}

//----------------------------- ContainerNode ------------------------------//

ContainerNode::ContainerNode(NodeType type) : Node(type) {}

ContainerNode::ContainerNode(const ContainerNode& other)
    : Node(other), line_{other.line_}, length_{other.length_} {}

ContainerNode& ContainerNode::operator=(ContainerNode other) {
	Node::operator=(other);
	return *this;
}

void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(lhs.line_, rhs.line_);
	swap(lhs.length_, rhs.length_);
}

void ContainerNode::appendChild(const std::string body) {
	Node::appendChild(TextNode::create(body));
}

void ContainerNode::appendChild(const std::string body, TextFormat format) {
	Node::appendChild(TextNode::create(body, format));
}

int ContainerNode::line() const {
	return line_;
}

void ContainerNode::line(const int line) {
	line_ = line;
}

// Used only for bookkeeping while parsing
int ContainerNode::length() const {
	return length_;
}

void ContainerNode::length(const int length) {
	length_ = length;
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
			current_ = current_->firstChild().get();
			visited_ = false;
		} else if (current_->hasNextSibling()) { // Next
			current_ = current_->nextSibling().get();
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
bool NodeIterator<T>::operator==(const NodeIterator<T>& rhs) const {
	return current_ == rhs.current_;
}

template<typename T>
bool NodeIterator<T>::operator!=(const NodeIterator<T>& rhs) const {
	return !(*this == rhs);
}

template class NodeIterator<Node>;
template class NodeIterator<const Node>;

//------------------------------- BreakNode --------------------------------//

std::shared_ptr<BreakNode> BreakNode::create() {
	return std::make_shared<BreakNode>();
}

BreakNode::BreakNode() : Node(NodeType::Break) {}

BreakNode::BreakNode(const BreakNode& other) : Node(other) {}

BreakNode& BreakNode::operator=(BreakNode other) {
	std::swap(*this, other);
	return *this;
}

std::shared_ptr<BreakNode> BreakNode::clone() const {
	return std::make_shared<BreakNode>(*this);
}

//------------------------------- GroupNode --------------------------------//

std::shared_ptr<GroupNode> GroupNode::create(int line, int length) {
	return std::make_shared<GroupNode>(line, length);
}

GroupNode::GroupNode(int line, int length) : ContainerNode(NodeType::Group) {
	line_ = line;
	length_ = length;
}

GroupNode::GroupNode(const GroupNode& other) : ContainerNode(other) {}

GroupNode& GroupNode::operator=(GroupNode other) {
	swap(*this, other);
	return *this;
}

void swap(GroupNode& lhs, GroupNode& rhs) noexcept {
	std::swap(static_cast<ContainerNode&>(lhs), static_cast<ContainerNode&>(rhs));
}

// Copies and returns a detached element with its children.
std::shared_ptr<GroupNode> GroupNode::clone() const {
	auto groupNode = std::make_shared<GroupNode>(*this);
	groupNode->detachParent();
	return groupNode;
}

//-------------------------------- TextNode --------------------------------//

std::shared_ptr<TextNode> TextNode::create(const std::string body) {
	return std::make_shared<TextNode>(body);
}

std::shared_ptr<TextNode> TextNode::create(const std::string body, TextFormat format) {
	return std::make_shared<TextNode>(body, format);
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
std::shared_ptr<TextNode> TextNode::clone() const {
	auto textNode = std::make_shared<TextNode>(*this);
	textNode->detachParent();
	return textNode;
}

const std::string& TextNode::string() const {
	return this->body();
}

void TextNode::addFormat(TextFormat format) {
	format_ = ::UHS::withFormat(format_, format);
}

void TextNode::removeFormat(TextFormat format) {
	format_ = ::UHS::withoutFormat(format_, format);
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

} // namespace UHS

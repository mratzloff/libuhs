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
	default:
		return "";
	}
}

bool Node::isElementOfType(const Node& n, ElementType t) {
	if (n.nodeType() == NodeType::Element) {
		const auto& e = static_cast<const Element&>(n);
		return (e.elementType() == t);
	}
	return false;
}

Node::Node(NodeType t) : _nodeType{t} {}

Node::Node(const Node& other) : _nodeType{other._nodeType}, _parent{other._parent} {
	this->cloneChildren(other);
}

Node& Node::operator=(Node other) {
	swap(*this, other);
	return *this;
}

void swap(Node& lhs, Node& rhs) {
	using std::swap;

	swap(lhs._nodeType, rhs._nodeType);
	swap(lhs._parent, rhs._parent);
	lhs.cloneChildren(rhs);
}

NodeType Node::nodeType() const {
	return _nodeType;
}

const std::string Node::nodeTypeString() const {
	return Node::typeString(_nodeType);
}

void Node::detachParent() {
	_parent = nullptr;
}

std::unique_ptr<Node> Node::removeChild(Node* n) {
	if (n == nullptr) {
		return nullptr;
	}

	n->_parent = nullptr;
	--_numChildren;

	if (n == this->firstChild()) {
		auto detachedNode = std::move(_firstChild);
		if (n->_nextSibling == nullptr) {
			_lastChild = nullptr;
		} else {
			_firstChild = std::move(n->_nextSibling);
		}
		n->didRemove();

		return detachedNode;
	} else {
		Node* previous = nullptr;
		for (Node* child = this->firstChild(); child != nullptr;
		     child = child->nextSibling()) {
			if (n == child) {
				auto detachedNode = std::move(previous->_nextSibling);
				if (n->_nextSibling == nullptr) {
					_lastChild = nullptr;
				} else {
					previous->_nextSibling = std::move(n->_nextSibling);
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

// Note that this node must be attached to a document tree in order to
// correctly set the _depth property when appending its own children.
void Node::appendChild(std::unique_ptr<Node> n, bool fireEvent) {
	if (n == nullptr) {
		return;
	}

	auto ptr = n.get();
	if (_lastChild == nullptr) { // No children
		_firstChild = std::move(n);
		_lastChild = ptr;
	} else {
		_lastChild->_nextSibling = std::move(n);
		_lastChild = ptr;
	}
	ptr->_parent = this;

	for (auto& child : *ptr) {
		child._depth = child._parent->_depth + 1;
	}
	++_numChildren;

	if (fireEvent) {
		ptr->didAdd();
	}
}

// See note for appendChild() regarding depth.
void Node::insertBefore(std::unique_ptr<Node> n, Node* ref) {
	if (n == nullptr) {
		return;
	}
	if (ref == nullptr) {
		this->appendChild(std::move(n));
		return;
	}

	auto ptr = n.get();

	if (ref == this->firstChild()) {
		n->_nextSibling = std::move(_firstChild);
		_firstChild = std::move(n);
	} else {
		Node* previous = nullptr;

		for (Node* sibling = this->firstChild(); sibling != nullptr;
		     sibling = sibling->nextSibling()) {
			if (sibling == ref) {
				break;
			}
			previous = sibling;
		}
		if (previous == nullptr) {
			return; // Not found; no action to take
		}
		n->_nextSibling = std::move(previous->_nextSibling);
		previous->_nextSibling = std::move(n);
	}
	ptr->_parent = this;

	for (auto& child : *ptr) {
		child._depth = child._parent->_depth + 1;
	}
	++_numChildren;

	ptr->didAdd();
}

Node* Node::parent() const {
	return _parent;
}

bool Node::hasNextSibling() const {
	return _nextSibling != nullptr;
}

Node* Node::nextSibling() const {
	return _nextSibling.get();
}

bool Node::hasFirstChild() const {
	return _firstChild != nullptr;
}

Node* Node::firstChild() const {
	return _firstChild.get();
}

bool Node::hasLastChild() const {
	return _lastChild != nullptr;
}

Node* Node::lastChild() const {
	return _lastChild;
}

int Node::numChildren() const {
	return _numChildren;
}

int Node::depth() const {
	return _depth;
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
	for (Node* node = other.firstChild(); node != nullptr; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Document:
			this->appendChild(static_cast<Document&>(*node).cloneInternal(), false);
			break;
		case NodeType::Element:
			this->appendChild(static_cast<Element&>(*node).cloneInternal(), false);
			break;
		case NodeType::Text:
			this->appendChild(static_cast<TextNode&>(*node).cloneInternal(), false);
			break;
		}
	}
}

void Node::didRemove() {
	if (_nodeType != NodeType::Element) {
		return;
	}
	auto& element = static_cast<Element&>(*this);
	if (auto document = this->findDocument(); document != nullptr) {
		document->elementRemoved(element);
	}
}

void Node::didAdd() {
	if (_nodeType != NodeType::Element) {
		return;
	}
	auto& element = static_cast<Element&>(*this);
	if (auto document = this->findDocument(); document != nullptr) {
		document->elementAdded(element);
	}
}

Document* Node::findDocument() const {
	for (Node* node = _parent; node != nullptr; node = node->parent()) {
		if (node->nodeType() == NodeType::Document) {
			return static_cast<Document*>(node);
		}
	}
	return nullptr; // Orphaned element
}

//------------------------------ NodeIterator -------------------------------//

template<typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer n) {
	_initial = n;
	_current = n;
}

template<typename T>
typename NodeIterator<T>::reference NodeIterator<T>::operator*() const {
	return *_current;
}

template<typename T>
typename NodeIterator<T>::pointer NodeIterator<T>::operator->() const {
	return _current;
}

// ++it
template<typename T>
NodeIterator<T>& NodeIterator<T>::operator++() {
	do {
		if (_current == nullptr) {
			break;
		}
		if (_current->hasFirstChild() && !_visited) { // Down
			_current = _current->firstChild();
			_visited = false;
		} else if (_current->hasNextSibling()) { // Next
			_current = _current->nextSibling();
			_visited = false;
		} else { // Up
			_current = _current->parent();
			if (_current == _initial) {
				_current = nullptr;
				break;
			}
			_visited = true;
		}
	} while (_visited);

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
	return _current == rhs._current;
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
    : Node(other), Traits::Body(other), _fmt{other._fmt} {}

TextNode& TextNode::operator=(TextNode other) {
	swap(*this, other);
	return *this;
}

void swap(TextNode& lhs, TextNode& rhs) {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(lhs._fmt, rhs._fmt);
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
	_fmt |= f;
}

void TextNode::removeFormat(Format f) {
	_fmt &= ~f;
}

bool TextNode::hasFormat(Format f) const {
	return (_fmt & f) != 0;
}

Format TextNode::format() const {
	return _fmt;
}

std::unique_ptr<Node> TextNode::cloneInternal() const {
	return std::make_unique<TextNode>(*this);
}

} // namespace UHS

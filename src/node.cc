#include "uhs.h"

namespace UHS {

const std::string Node::typeString(NodeType t) {
	switch (t) {
	case NodeDocument:
		return "document";
	case NodeElement:
		return "element";
	case NodeText:
		return "text";
	default:
		return "";
	}
}

bool Node::isElementOfType(const Node& n, ElementType t) {
	if (n.nodeType() == NodeElement) {
		const auto& e = dynamic_cast<const Element&>(n);
		return (e.elementType() == t);
	}
	return false;
}

Node::Node(NodeType t) : _nodeType {t} {}

NodeType Node::nodeType() const {
	return _nodeType;
}

const std::string Node::nodeTypeString() const {
	return Node::typeString(_nodeType);
}

// Note that this node must be attached to a document tree in order to
// correctly set the _depth property when appending its own children.
void Node::appendChild(std::unique_ptr<Node> n) {
	auto ptr = n.get();
	if (_firstChild == nullptr) {
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

//------------------------------ NodeIterator -------------------------------//

template <typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer n) {
	_initial = n;
	_current = n;
}

template <typename T>
typename NodeIterator<T>::reference NodeIterator<T>::operator*() const {
	return *_current;
}

template <typename T>
typename NodeIterator<T>::pointer NodeIterator<T>::operator->() const {
	return _current;
}

// ++it
template <typename T>
NodeIterator<T>& NodeIterator<T>::operator++() {
	do {
		if (_current == nullptr) {
			break;
		}
		if (_current->hasFirstChild() && ! _visited) { // Down
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
template <typename T>
NodeIterator<T> NodeIterator<T>::operator++(int) {
	auto tmp = *this;
	++(*this);
	return tmp;
}

template <typename T>
bool NodeIterator<T>::operator==(const NodeIterator<T>& rhs) {
	return _current == rhs._current;
}

template <typename T>
bool NodeIterator<T>::operator!=(const NodeIterator<T>& rhs) {
	return !(*this == rhs);
}

template class NodeIterator<Node>;
template class NodeIterator<const Node>;

//-------------------------------- TextNode --------------------------------//

TextNode::TextNode() : Node(NodeText) {}

TextNode::TextNode(const std::string body)
	: Node(NodeText)
	, Body(body)
{}

const std::string& TextNode::toString() const {
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

}

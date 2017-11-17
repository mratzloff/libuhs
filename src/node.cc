#include "uhs.h"

namespace UHS {

const std::string Node::typeString(NodeType t) {
	switch (t) {
	case NodeElement:
		return "element";
	case NodeContainer:
		return "container";
	case NodeText:
		return "text";
	default:
		return "";
	}
}

Node::Node(NodeType t) : _nodeType {t} {}

NodeType Node::nodeType() const {
	return _nodeType;
}

const std::string Node::nodeTypeString() const {
	return Node::typeString(_nodeType);
}

void Node::appendChild(std::shared_ptr<Node> n) {
	if (_firstChild == nullptr) {
		_firstChild = n;
		_lastChild = n;
	} else {
		_lastChild->_nextSibling = n;
		_lastChild = n;
	}
	n->_parent = this->shared_from_this();
}

std::shared_ptr<Node> Node::parent() const {
	return _parent;
}

bool Node::hasNextSibling() const {
	return _nextSibling != nullptr;
}

std::shared_ptr<Node> Node::nextSibling() const {
	return _nextSibling;
}

bool Node::hasFirstChild() const {
	return _firstChild != nullptr;
}

std::shared_ptr<Node> Node::firstChild() const {
	return _firstChild;
}

bool Node::hasLastChild() const {
	return _lastChild != nullptr;
}

std::shared_ptr<Node> Node::lastChild() const {
	return _lastChild;
}

int Node::depth() const {
	if (_depth < 0) {
		auto p = this->parent();
		if (p == nullptr) {
			return 0;
		}
		_depth = p->depth() + 1;
	}
	return _depth;
}

//------------------------------ NodeIterator -------------------------------//

template <typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer n) {
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
		if (_current->hasFirstChild() && ! _visited) { // Down
			_current = _current->firstChild();
			_visited = false;
		} else if (_current->hasNextSibling()) { // Next
			_current = _current->nextSibling();
			_visited = false;
		} else { // Up
			_current = _current->parent();
			if (_current == nullptr) {
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

//------------------------------ ContainerNode ------------------------------//

ContainerNode::ContainerNode() : Node(NodeContainer) {}

//-------------------------------- TextNode --------------------------------//

TextNode::TextNode() : Node(NodeText) {}

const std::string& TextNode::toString() const {
	return _body;
}

const std::string& TextNode::body() const {
	return _body;
}

void TextNode::body(const std::string s) {
	_body = s;
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

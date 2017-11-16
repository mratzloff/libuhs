#include "uhs.h"

namespace UHS {

Node::Node(NodeType t) : _nodeType {t} {}

NodeType Node::nodeType() const {
	return _nodeType;
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

ContainerNode::ContainerNode() : Node(NodeContainer) {}

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

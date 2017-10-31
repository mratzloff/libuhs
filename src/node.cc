#include "uhs.h"

namespace UHS {

Node::Node(NodeType t) : _nodeType {t} {}

Node::~Node() {}

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
	n->_parent = shared_from_this();
}

std::vector<std::shared_ptr<Node>> Node::children() const {
	std::vector<std::shared_ptr<Node>> v;
	std::shared_ptr<Node> n {_firstChild};

	while (n != nullptr) {
		v.push_back(n);
		n = n->nextSibling();
	}
	return v;
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

ContainerNode::~ContainerNode() {}

TextNode::TextNode() : Node(NodeText) {}

TextNode::~TextNode() {}

const std::string& TextNode::toString() const {
	return _value;
}

const std::string& TextNode::value() const {
	return _value;
}

void TextNode::value(const std::string v) {
	_value = v;
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

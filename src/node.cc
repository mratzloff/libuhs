#include "uhs.h"

namespace UHS {

Node::Node(NodeType t) : _type(t) {}

Node::~Node() {}

NodeType Node::type() const {
	return _type;
}

std::shared_ptr<Document> Node::document() const {
	return _document;
}

void Node::document(std::shared_ptr<Document> d) {
	_document = d;
}

void Node::appendChild(std::shared_ptr<Node> n) {
	if (_firstChild == nullptr) {
		_firstChild = n;
		_lastChild = n;
	} else {
		_lastChild->_nextSibling = n;
		_lastChild = n;
	}
}

std::shared_ptr<Node> Node::parent() const {
	return _parent;
}

std::shared_ptr<Node> Node::nextSibling() const {
	return _nextSibling;
}

std::shared_ptr<Node> Node::firstChild() const {
	return _firstChild;
}

std::shared_ptr<Node> Node::lastChild() const {
	return _lastChild;
}

int Node::depth() const {
	return _depth;
}

void Node::depth(int d) {
	_depth = d;
}

ContainerNode::ContainerNode() : Node(NodeContainer) {}

ContainerNode::~ContainerNode() {}

TextNode::TextNode() : Node(NodeText) {}

TextNode::~TextNode() {}

const std::string TextNode::toString() const {
	return _data;
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

#include "uhs.h"

namespace UHS {

Document::Document()
	: Attributes()
	, _root {std::make_shared<Node>(NodeContainer)}
	, _version {Version88a}
{}

Document::Document(VersionType version) : Document() {
	_version = version;
};

std::string Document::toString() const {
	return "";
}

void Document::appendChild(std::shared_ptr<Node> n) {
	_root->appendChild(n);
}

const std::shared_ptr<Node> Document::root() {
	return _root;
}

void Document::header(std::shared_ptr<Document> d) {
	_header = d;
}

std::shared_ptr<Document> Document::header() const {
	return _header;
}

void Document::version(VersionType v) {
	_version = v;
}

VersionType Document::version() const {
	return _version;
}

const std::string Document::versionString() const {
	switch (_version) {
	case Version88a: return "88a";
	case Version91a: return "91a";
	case Version95a: return "95a";
	default:         return "96a";
	}
}

void Document::title(std::string s) {
	_title = s;
}

std::string Document::title() const {
	return _title;
}

void Document::validChecksum(bool value) {
	_validChecksum = value;
}

bool Document::validChecksum() const {
	return _validChecksum;
}

Node::iterator Document::begin() const {
	return Node::iterator(_root);
}

Node::iterator Document::end() const {
	return Node::iterator(nullptr);
}

Node::const_iterator Document::cbegin() const {
	return Node::const_iterator(_root);
}

Node::const_iterator Document::cend() const {
	return Node::const_iterator(nullptr);
}

}

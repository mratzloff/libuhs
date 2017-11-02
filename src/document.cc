#include "uhs.h"

namespace UHS {

Document::Document()
	: _root(std::make_shared<Node>(NodeContainer))
	, _version(Version88a)
	, _meta(std::make_shared<Metadata>())
	, _validCRC(false)
{};

Document::~Document() {}

void Document::appendChild(std::shared_ptr<Node> n) {
	_root->appendChild(n);
}

std::string Document::toString() const {
	return "";
}

const std::shared_ptr<Node> Document::root() {
	return _root;
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

const std::shared_ptr<Metadata> Document::meta() const {
	return _meta;
}

void Document::validCRC(bool valid) {
	_validCRC = valid;
}

bool Document::validCRC() const {
	return _validCRC;
}

}

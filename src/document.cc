#include "uhs.h"

namespace UHS {

Document::Document()
	: _root {std::make_shared<Node>(NodeContainer)}
	, _version {Version88a}
	, _meta {std::make_shared<std::map<std::string, std::string>>()}
{}

Document::Document(VersionType version) : Document() {
	_version = version;
};

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

void Document::length(const std::size_t len) {
	_length = len;
}

std::size_t Document::length() const {
	return _length;
}

void Document::timestamp(const std::tm time) {
	_timestamp = time;
}

std::tm Document::timestamp() const {
	return _timestamp;
}

const std::string Document::timestampString() const {
	char buf[20];
	auto len = std::strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", &_timestamp);
	if (len == 0) {
		return "";
	}
	return buf;
}

void Document::meta(std::string key, std::string value) {
	(*_meta)[key] = value;
}

const std::shared_ptr<std::map<std::string, std::string>> Document::meta() const {
	return _meta;
}

const std::string Document::meta(std::string key) const {
	return (*_meta)[key];
}

void Document::validChecksum(bool value) {
	_validChecksum = value;
}

bool Document::validChecksum() const {
	return _validChecksum;
}

}

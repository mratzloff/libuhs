#include "uhs.h"

namespace UHS {

Document::Document() : Node(NodeType::Document), _version{VersionType::Version88a} {}

Document::Document(VersionType version, const std::string title)
    : Node(NodeType::Document), Traits::Title(title), _version{version} {}

Document::Document(const Document& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , _version{other._version}
    , _validChecksum{other._validChecksum} {}

// Copies and returns a detached document with its children.
std::unique_ptr<Document> Document::clone() const {
	auto document = std::make_unique<Document>(*this);
	document->detachParent();
	return document;
}

void Document::version(VersionType v) {
	_version = v;
}

VersionType Document::version() const {
	return _version;
}

const std::string Document::versionString() const {
	switch (_version) {
	case VersionType::Version88a:
		return "88a";
	case VersionType::Version91a:
		return "91a";
	case VersionType::Version95a:
		return "95a";
	case VersionType::Version96a:
		return "96a";
	}
}

void Document::validChecksum(bool value) {
	_validChecksum = value;
}

bool Document::validChecksum() const {
	return _validChecksum;
}

std::unique_ptr<Node> Document::cloneInternal() const {
	return this->clone();
}

} // namespace UHS

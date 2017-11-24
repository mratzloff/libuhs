#include "uhs.h"

namespace UHS {

Document::Document() : Node(NodeDocument), _version {Version88a} {}

Document::Document(VersionType version, const std::string title)
	: Node(NodeDocument)
	, Traits::Title(title)
	, _version {version}
{};

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

void Document::validChecksum(bool value) {
	_validChecksum = value;
}

bool Document::validChecksum() const {
	return _validChecksum;
}

}

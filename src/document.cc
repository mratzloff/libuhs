#include "uhs.h"

namespace UHS {

Document::Document()
	: _root(std::make_unique<Node>(NodeContainer))
	, _version(Version88a)
	, _meta(std::make_unique<Metadata>())
	, _validCRC(false)
{};

Document::~Document() {}

void Document::appendChild(std::shared_ptr<Node> n) {
	_root->appendChild(n);
}

std::string Document::toString() {
	return "";
}

}

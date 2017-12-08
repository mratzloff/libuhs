#include "uhs.h"

namespace UHS {

std::unique_ptr<Document> Document::create(VersionType version) {
	return std::make_unique<Document>(version);
}

Document::Document(VersionType version) : Node(NodeType::Document), _version{version} {}

Document::Document(const Document& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , _version{other._version}
    , _validChecksum{other._validChecksum}
    , _indexed{false} {
	this->reindex();
}

Document& Document::operator=(Document other) {
	swap(*this, other);
	return *this;
}

void swap(Document& lhs, Document& rhs) {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Attributes&>(lhs), static_cast<Traits::Attributes&>(rhs));
	swap(static_cast<Traits::Title&>(lhs), static_cast<Traits::Title&>(rhs));
	swap(static_cast<Traits::Visibility&>(lhs), static_cast<Traits::Visibility&>(rhs));
	swap(lhs._version, rhs._version);
	swap(lhs._validChecksum, rhs._validChecksum);
	lhs._indexed = false;

	lhs.reindex();
}

Element* Document::find(const int id) {
	if (!_indexed) {
		this->reindex();
	}
	try {
		return _index.at(id);
	} catch (const std::out_of_range& ex) {
		return nullptr;
	}
}

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

void Document::elementRemoved(Element& element) {
	if (const auto id = element.id(); id > 0) {
		_index.erase(id);
	}
}

void Document::elementAdded(Element& element) {
	if (const auto id = element.id(); id > 0) {
		_index[id] = &element;
	}
}

void Document::reindex() {
	_index.clear();

	for (auto& node : *this) {
		if (node.nodeType() == NodeType::Element) {
			this->elementAdded(static_cast<Element&>(node));
		}
	}
	_indexed = true;
}

std::unique_ptr<Node> Document::cloneInternal() const {
	return std::make_unique<Document>(*this);
}

} // namespace UHS

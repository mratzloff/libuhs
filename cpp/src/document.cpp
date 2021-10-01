#include "uhs.h"

namespace UHS {

std::shared_ptr<Document> Document::create(VersionType version) {
	return std::make_shared<Document>(version);
}

Document::Document(VersionType version) : Node(NodeType::Document), version_{version} {}

Document::Document(const Document& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , version_{other.version_}
    , validChecksum_{other.validChecksum_}
    , indexed_{false} {

	this->reindex();
}

Document& Document::operator=(Document other) {
	swap(*this, other);
	return *this;
}

void swap(Document& lhs, Document& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Attributes&>(lhs), static_cast<Traits::Attributes&>(rhs));
	swap(static_cast<Traits::Title&>(lhs), static_cast<Traits::Title&>(rhs));
	swap(static_cast<Traits::Visibility&>(lhs), static_cast<Traits::Visibility&>(rhs));
	swap(lhs.version_, rhs.version_);
	swap(lhs.validChecksum_, rhs.validChecksum_);

	lhs.indexed_ = false;
	lhs.reindex();
}

Node* Document::find(const int id) {
	if (!indexed_) {
		this->reindex();
	}
	try {
		return index_.at(id);
	} catch (const std::out_of_range& err) {
		return nullptr;
	}
}

// Copies and returns a detached document with its children.
std::shared_ptr<Document> Document::clone() const {
	auto document = std::make_shared<Document>(*this);
	document->detachParent();
	return document;
}

void Document::version(VersionType v) {
	version_ = v;
}

VersionType Document::version() const {
	return version_;
}

bool Document::isVersion(VersionType v) const {
	return version_ == v;
}

const std::string Document::versionString() const {
	switch (version_) {
	case VersionType::Version88a:
		return "88a";
	case VersionType::Version91a:
		return "91a";
	case VersionType::Version95a:
		return "95a";
	case VersionType::Version96a:
		return "96a";
	default:
		throw Error("invalid version");
	}
}

void Document::validChecksum(bool value) {
	validChecksum_ = value;
}

bool Document::validChecksum() const {
	return validChecksum_;
}

void Document::elementRemoved(Element& element) {
	if (const auto id = element.id(); id > 0) {
		index_.erase(id);
	}
}

void Document::elementAdded(Element& element) {
	if (const auto id = element.id(); id > 0) {
		index_[id] = &element;
	}
}

void Document::reindex() {
	index_.clear();

	for (auto& node : *this) {
		if (node.isElement()) {
			this->elementAdded(static_cast<Element&>(node));
		}
	}
	indexed_ = true;
}

} // namespace UHS

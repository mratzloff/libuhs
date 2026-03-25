#include "uhs.h"

namespace UHS {

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

Document::Document(VersionType version) : Node(NodeType::Document), version_{version} {}

Document::Document(Document const& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , indexed_{false}
    , validChecksum_{other.validChecksum_}
    , version_{other.version_} {

	this->reindex();
}

Document::Document(Document&& other) noexcept
    : Node(std::move(other))
    , Traits::Attributes(std::move(other))
    , Traits::Title(std::move(other))
    , Traits::Visibility(std::move(other))
    , index_{std::move(other.index_)}
    , indexed_{other.indexed_}
    , validChecksum_{other.validChecksum_}
    , version_{other.version_} {

	other.indexed_ = false;
	other.validChecksum_ = false;
}

std::shared_ptr<Document> Document::create(VersionType version) {
	return std::make_shared<Document>(version);
}

// Copies and returns a detached document with its children.
std::shared_ptr<Document> Document::clone() const {
	auto document = std::make_shared<Document>(*this);
	document->detachParent();
	return document;
}

void Document::elementAdded(Element& element) {
	if (auto const id = element.id(); id > 0) {
		index_[id] = &element;
	}
}

void Document::elementRemoved(Element& element) {
	if (auto const id = element.id(); id > 0) {
		index_.erase(id);
	}
}

Node* Document::find(int const id) {
	if (!indexed_) {
		this->reindex();
	}
	auto const it = index_.find(id);
	if (it != index_.end()) {
		return it->second;
	}
	return nullptr;
}

bool Document::isVersion(VersionType v) const {
	return version_ == v;
}

// Ensures the document has the correct metadata node structure:
// version -> [incentive] -> info, appended as the last direct children.
//
// If no info node is present but an incentive node is, clicking into
// a registered-only node in a UHS file using the official reader results
// in an EAccessViolation (null pointer exception).
//
// Therefore, if an incentive exists but no info element follows it, a
// default info element is created. Elements are reordered if necessary.
void Document::normalize() {
	std::shared_ptr<Node> incentive;
	std::shared_ptr<Node> info;
	std::shared_ptr<Node> version;

	for (auto child = firstChild(); child; child = child->nextSibling()) {
		if (child->nodeType() != NodeType::Element) {
			continue;
		}
		switch (child->asElement().elementType()) {
		case ElementType::Incentive:
			incentive = child->pointer();
			break;
		case ElementType::Info:
			info = child->pointer();
			break;
		case ElementType::Version:
			version = child->pointer();
			break;
		default:
			break;
		}
	}

	if (incentive && !info) {
		info = Element::create(ElementType::Info);
	}

	// Detach and re-append in the correct order
	for (auto& node : {version, incentive, info}) {
		if (!node) {
			continue;
		}
		if (node->hasParent()) {
			this->removeChild(node);
		}
		this->appendChild(node);
	}
}

Document& Document::operator=(Document other) {
	swap(*this, other);
	return *this;
}

void Document::reindex() {
	index_.clear();

	for (auto& node : *this) {
		if (node.isElement()) {
			this->elementAdded(node.asElement());
		}
	}
	indexed_ = true;
}

bool Document::validChecksum() const {
	return validChecksum_;
}

void Document::validChecksum(bool value) {
	validChecksum_ = value;
}

VersionType Document::version() const {
	return version_;
}

void Document::version(VersionType v) {
	version_ = v;
}

std::string const Document::versionString() const {
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
		throw DataError("invalid version");
	}
}

} // namespace UHS

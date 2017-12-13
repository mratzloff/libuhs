#include "uhs.h"

namespace UHS {

Element::TypeAtlas Element::typeAtlas_;

std::unique_ptr<Element> Element::create(ElementType type, const int id) {
	return std::make_unique<Element>(type, id);
}

ElementType Element::elementType(const std::string& typeString) {
	return Element::typeAtlas_.findByString(typeString);
}

const std::string Element::typeString(ElementType type) {
	return Element::typeAtlas_.findByType(type);
}

Element::Element(ElementType type, const int id)
    : Node(NodeType::Element), elementType_{type}, id_{id} {}

Element::Element(const Element& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Body(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , elementType_{other.elementType_}
    , id_{other.id_}
    , line_{other.line_}
    , length_{other.length_} {}

Element& Element::operator=(Element other) {
	swap(*this, other);
	return *this;
}

void swap(Element& lhs, Element& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Attributes&>(lhs), static_cast<Traits::Attributes&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(static_cast<Traits::Title&>(lhs), static_cast<Traits::Title&>(rhs));
	swap(static_cast<Traits::Visibility&>(lhs), static_cast<Traits::Visibility&>(rhs));
	swap(lhs.elementType_, rhs.elementType_);
	swap(lhs.id_, rhs.id_);
	swap(lhs.line_, rhs.line_);
	swap(lhs.length_, rhs.length_);
}

// Copies and returns a detached element with its children.
std::unique_ptr<Element> Element::clone() const {
	auto element = std::make_unique<Element>(*this);
	element->detachParent();
	return element;
}

ElementType Element::elementType() const {
	return elementType_;
}

const std::string Element::elementTypeString() const {
	return Element::typeString(elementType_);
}

void Element::appendChild(const std::string s) {
	Node::appendChild(TextNode::create(s));
}

int Element::id() const {
	return id_;
}

int Element::line() const {
	return line_;
}

void Element::line(const int line) {
	line_ = line;
}

// Used only for bookkeeping while parsing
int Element::length() const {
	return length_;
}

void Element::length(const int length) {
	length_ = length;
}

bool Element::isMedia() const {
	return (elementType_ == ElementType::Gifa || elementType_ == ElementType::Hyperpng
	        || elementType_ == ElementType::Overlay
	        || elementType_ == ElementType::Sound);
}

const std::string Element::mediaExt() const {
	switch (elementType_) {
	case ElementType::Gifa:
		return "gif";
	case ElementType::Hyperpng:
		return "png";
	case ElementType::Overlay:
		return "png";
	case ElementType::Sound:
		return "wav";
	default:
		return "";
	}
}

std::unique_ptr<Node> Element::cloneInternal() const {
	return std::make_unique<Element>(*this);
}

Element::TypeAtlas::TypeAtlas() {
	const std::vector<std::pair<const ElementType, const std::string>> list = {
	    std::make_pair(ElementType::Unknown, "unknown"),
	    std::make_pair(ElementType::Blank, "blank"),
	    std::make_pair(ElementType::Comment, "comment"),
	    std::make_pair(ElementType::Credit, "credit"),
	    std::make_pair(ElementType::Gifa, "gifa"),
	    std::make_pair(ElementType::Hint, "hint"),
	    std::make_pair(ElementType::Hyperpng, "hyperpng"),
	    std::make_pair(ElementType::Incentive, "incentive"),
	    std::make_pair(ElementType::Info, "info"),
	    std::make_pair(ElementType::Link, "link"),
	    std::make_pair(ElementType::Nesthint, "nesthint"),
	    std::make_pair(ElementType::Overlay, "overlay"),
	    std::make_pair(ElementType::Sound, "sound"),
	    std::make_pair(ElementType::Subject, "subject"),
	    std::make_pair(ElementType::Text, "text"),
	    std::make_pair(ElementType::Version, "version"),
	};

	for (const auto& pair : list) {
		byType_.emplace(pair);
		byString_.emplace(std::make_pair(pair.second, pair.first));
	}
}

const std::string Element::TypeAtlas::findByType(const ElementType type) const {
	return byType_.at(type);
}

ElementType Element::TypeAtlas::findByString(const std::string& string) const {
	try {
		return byString_.at(string);
	} catch (const std::out_of_range&) {
		return ElementType::Unknown;
	}
}

} // namespace UHS

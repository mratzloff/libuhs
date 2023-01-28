#include "uhs.h"

namespace UHS {

Element::TypeMap Element::typeMap_;

std::shared_ptr<Element> Element::create(ElementType type, int id) {
	return std::make_shared<Element>(type, id);
}

ElementType Element::elementType(std::string const& typeString) {
	return Element::typeMap_.findByString(typeString);
}

std::string const Element::typeString(ElementType type) {
	return Element::typeMap_.findByType(type);
}

Element::Element(ElementType type, int id)
    : ContainerNode(NodeType::Element), elementType_{type}, id_{id} {}

Element::Element(Element const& other)
    : ContainerNode(other)
    , Traits::Attributes(other)
    , Traits::Body(other)
    , Traits::Title(other)
    , Traits::Inlined(other)
    , Traits::Visibility(other)
    , elementType_{other.elementType_}
    , id_{other.id_} {}

Element& Element::operator=(Element other) {
	swap(*this, other);
	return *this;
}

void swap(Element& lhs, Element& rhs) noexcept {
	using std::swap;

	swap(static_cast<ContainerNode&>(lhs), static_cast<ContainerNode&>(rhs));
	swap(static_cast<Traits::Attributes&>(lhs), static_cast<Traits::Attributes&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(static_cast<Traits::Title&>(lhs), static_cast<Traits::Title&>(rhs));
	swap(static_cast<Traits::Inlined&>(lhs), static_cast<Traits::Inlined&>(rhs));
	swap(static_cast<Traits::Visibility&>(lhs), static_cast<Traits::Visibility&>(rhs));
	swap(lhs.elementType_, rhs.elementType_);
	swap(lhs.id_, rhs.id_);
}

// Copies and returns a detached element with its children.
std::shared_ptr<Element> Element::clone() const {
	auto element = std::make_shared<Element>(*this);
	element->detachParent();
	return element;
}

ElementType Element::elementType() const {
	return elementType_;
}

std::string const Element::elementTypeString() const {
	return Element::typeString(elementType_);
}

int Element::id() const {
	return id_;
}

bool Element::isMedia() const {
	return (elementType_ == ElementType::Gifa || elementType_ == ElementType::Hyperpng
	        || elementType_ == ElementType::Overlay
	        || elementType_ == ElementType::Sound);
}

std::string const Element::mediaExt() const {
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
		throw Error("invalid media extension");
	}
}

Element::TypeMap::TypeMap() {
	std::vector<std::pair<ElementType const, std::string const>> const list = {
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

	for (auto const& pair : list) {
		byType_.emplace(pair);
		byString_.emplace(std::make_pair(pair.second, pair.first));
	}
}

std::string const Element::TypeMap::findByType(ElementType const type) const {
	return byType_.at(type);
}

ElementType Element::TypeMap::findByString(std::string const& string) const {
	try {
		return byString_.at(string);
	} catch (std::out_of_range const&) {
		return ElementType::Unknown;
	}
}

} // namespace UHS

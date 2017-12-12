#include "uhs.h"

namespace UHS {

Element::TypeAtlas Element::_typeAtlas;

std::unique_ptr<Element> Element::create(ElementType type, const int id) {
	return std::make_unique<Element>(type, id);
}

ElementType Element::elementType(const std::string& typeString) {
	return Element::_typeAtlas.findByString(typeString);
}

const std::string Element::typeString(ElementType type) {
	return Element::_typeAtlas.findByType(type);
}

Element::Element(ElementType type, const int id)
    : Node(NodeType::Element), _elementType{type}, _id{id} {}

Element::Element(const Element& other)
    : Node(other)
    , Traits::Attributes(other)
    , Traits::Body(other)
    , Traits::Title(other)
    , Traits::Visibility(other)
    , _elementType{other._elementType}
    , _id{other._id}
    , _line{other._line}
    , _length{other._length} {}

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
	swap(lhs._elementType, rhs._elementType);
	swap(lhs._id, rhs._id);
	swap(lhs._line, rhs._line);
	swap(lhs._length, rhs._length);
}

// Copies and returns a detached element with its children.
std::unique_ptr<Element> Element::clone() const {
	auto element = std::make_unique<Element>(*this);
	element->detachParent();
	return element;
}

ElementType Element::elementType() const {
	return _elementType;
}

const std::string Element::elementTypeString() const {
	return Element::typeString(_elementType);
}

void Element::appendChild(const std::string s) {
	Node::appendChild(TextNode::create(s));
}

int Element::id() const {
	return _id;
}

int Element::line() const {
	return _line;
}

void Element::line(const int line) {
	_line = line;
}

// Used only for bookkeeping while parsing
int Element::length() const {
	return _length;
}

void Element::length(const int length) {
	_length = length;
}

bool Element::isMedia() const {
	return (_elementType == ElementType::Gifa || _elementType == ElementType::Hyperpng
	        || _elementType == ElementType::Overlay
	        || _elementType == ElementType::Sound);
}

const std::string Element::mediaExt() const {
	switch (_elementType) {
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
		_byType.emplace(pair);
		_byString.emplace(std::make_pair(pair.second, pair.first));
	}
}

const std::string Element::TypeAtlas::findByType(const ElementType type) const {
	return _byType.at(type);
}

ElementType Element::TypeAtlas::findByString(const std::string& string) const {
	try {
		return _byString.at(string);
	} catch (const std::out_of_range& ex) {
		return ElementType::Unknown;
	}
}

} // namespace UHS

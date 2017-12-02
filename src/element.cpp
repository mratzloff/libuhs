#include "uhs.h"

namespace UHS {

ElementType Element::elementType(const std::string& typeString) {
	if (typeString == "blank") {
		return ElementType::Blank;
	} else if (typeString == "comment") {
		return ElementType::Comment;
	} else if (typeString == "credit") {
		return ElementType::Credit;
	} else if (typeString == "gifa") {
		return ElementType::Gifa;
	} else if (typeString == "hint") {
		return ElementType::Hint;
	} else if (typeString == "hyperpng") {
		return ElementType::Hyperpng;
	} else if (typeString == "incentive") {
		return ElementType::Incentive;
	} else if (typeString == "info") {
		return ElementType::Info;
	} else if (typeString == "link") {
		return ElementType::Link;
	} else if (typeString == "nesthint") {
		return ElementType::Nesthint;
	} else if (typeString == "overlay") {
		return ElementType::Overlay;
	} else if (typeString == "sound") {
		return ElementType::Sound;
	} else if (typeString == "subject") {
		return ElementType::Subject;
	} else if (typeString == "text") {
		return ElementType::Text;
	} else if (typeString == "version") {
		return ElementType::Version;
	} else {
		return ElementType::Unknown;
	}
}

const std::string Element::typeString(ElementType t) {
	switch (t) {
	case ElementType::Blank:
		return "blank";
	case ElementType::Comment:
		return "comment";
	case ElementType::Credit:
		return "credit";
	case ElementType::Gifa:
		return "gifa";
	case ElementType::Hint:
		return "hint";
	case ElementType::Hyperpng:
		return "hyperpng";
	case ElementType::Incentive:
		return "incentive";
	case ElementType::Info:
		return "info";
	case ElementType::Link:
		return "link";
	case ElementType::Nesthint:
		return "nesthint";
	case ElementType::Overlay:
		return "overlay";
	case ElementType::Sound:
		return "sound";
	case ElementType::Subject:
		return "subject";
	case ElementType::Text:
		return "text";
	case ElementType::Version:
		return "version";
	default:
		return "unknown";
	}
}

Element::Element(ElementType t, int index, int length)
    : Node(NodeType::Element), _elementType{t}, _index{index}, _length{length} {}

Element::Element(ElementType t, const std::string title)
    : Node(NodeType::Element), Traits::Title(title), _elementType{t} {}

ElementType Element::elementType() const {
	return _elementType;
}

const std::string Element::elementTypeString() const {
	return Element::typeString(_elementType);
}

void Element::appendString(const std::string s) {
	this->appendChild(std::make_unique<TextNode>(s));
}

int Element::index() const {
	return _index;
}

// Used only for bookkeeping while parsing
int Element::length() const {
	return _length;
}

const Element* Element::ref() const {
	return _ref;
}

void Element::ref(const Element* ref) {
	_ref = ref;
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

} // namespace UHS

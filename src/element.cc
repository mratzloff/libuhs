#include "uhs.h"

namespace UHS {

ElementType Element::elementType(const std::string& typeString) {
	if (typeString == "blank") {
		return ElementBlank;
	} else if (typeString == "comment") {
		return ElementComment;
	} else if (typeString == "credit") {
		return ElementCredit;
	} else if (typeString == "gifa") {
		return ElementGifa;
	} else if (typeString == "hint") {
		return ElementHint;
	} else if (typeString == "hyperpng") {
		return ElementHyperpng;
	} else if (typeString == "incentive") {
		return ElementIncentive;
	} else if (typeString == "info") {
		return ElementInfo;
	} else if (typeString == "link") {
		return ElementLink;
	} else if (typeString == "nesthint") {
		return ElementNesthint;
	} else if (typeString == "overlay") {
		return ElementOverlay;
	} else if (typeString == "sound") {
		return ElementSound;
	} else if (typeString == "subject") {
		return ElementSubject;
	} else if (typeString == "text") {
		return ElementText;
	} else if (typeString == "version") {
		return ElementVersion;
	} else {
		return ElementUnknown;
	}
}

const std::string Element::typeString(ElementType t) {
	switch (t) {
	case ElementBlank:
		return "blank";
	case ElementComment:
		return "comment";
	case ElementCredit:
		return "credit";
	case ElementGifa:
		return "gifa";
	case ElementHint:
		return "hint";
	case ElementHyperpng:
		return "hyperpng";
	case ElementIncentive:
		return "incentive";
	case ElementInfo:
		return "info";
	case ElementLink:
		return "link";
	case ElementNesthint:
		return "nesthint";
	case ElementOverlay:
		return "overlay";
	case ElementSound:
		return "sound";
	case ElementSubject:
		return "subject";
	case ElementText:
		return "text";
	case ElementVersion:
		return "version";
	default:
		return "unknown";
	}
}

Element::Element(ElementType t, int index, int length)
	: Node(NodeElement)
	, _elementType {t}
	, _index {index}
	, _length {length}
{}

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
	return (_elementType == ElementGifa
			|| _elementType == ElementHyperpng
			|| _elementType == ElementOverlay
			|| _elementType == ElementSound);
}

const std::string Element::mediaExt() const {
	switch (_elementType) {
	case ElementGifa:     return "gif";
	case ElementHyperpng: return "png";
	case ElementOverlay:  return "png";
	case ElementSound:    return "wav";
	default:              return "";
	}
}

}

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

Element::~Element() {}

ElementType Element::elementType() const {
	return _elementType;
}

const std::string Element::elementTypeString() const {
	return Element::typeString(_elementType);
}

int Element::index() {
	return _index;
}

int Element::length() {
	return _length;
}

const std::map<std::string, std::string>& Element::attrs() const {
	return _attrs;
}

const std::string& Element::attr(const std::string& key) const {
	return _attrs.at(key);
}

void Element::attr(const std::string& key, const std::string value) {
	_attrs[key] = value;
}

const std::string& Element::value() const {
	return _value;
}

void Element::value(const std::string v) {
	_value = v;
}

}

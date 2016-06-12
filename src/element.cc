#include "uhs.h"

namespace UHS {

const std::string contentTypeString(ContentType t) {
	switch (t) {
	case ContentText:
		return "text/plain";
	case ContentGIF:
		return "image/gif";
	case ContentPNG:
		return "image/png";
	case ContentWAV:
		return "audio/x-wav";
	}
}

ElementType Element::elementType(std::string element) {
	if (element == "blank") {
		return ElementBlank;
	} else if (element == "comment") {
		return ElementComment;
	} else if (element == "credit") {
		return ElementCredit;
	} else if (element == "gifa") {
		return ElementGifa;
	} else if (element == "hint") {
		return ElementHint;
	} else if (element == "hyperpng") {
		return ElementHyperpng;
	} else if (element == "incentive") {
		return ElementIncentive;
	} else if (element == "info") {
		return ElementInfo;
	} else if (element == "link") {
		return ElementLink;
	} else if (element == "nesthint") {
		return ElementNesthint;
	} else if (element == "overlay") {
		return ElementOverlay;
	} else if (element == "sound") {
		return ElementSound;
	} else if (element == "subject") {
		return ElementSubject;
	} else if (element == "text") {
		return ElementText;
	} else if (element == "version") {
		return ElementVersion;
	} else {
		return ElementUnknown;
	}
}

Element::Element(ElementType t, int index, int length)
	: Node(NodeElement)
	, _type {t}
	, _index {index}
	, _length {length}
	, _contentType {ContentText}
{}

Element::~Element() {}

int Element::index() {
	return _index;
}

int Element::length() {
	return _length;
}

ContentType Element::contentType() {
	return _contentType;
}

void Element::contentType(ContentType t) {
	_contentType = t;
}

const std::string Element::value() {
	return _val;
}

void Element::value(std::string v) {
	_val = v;
}

}

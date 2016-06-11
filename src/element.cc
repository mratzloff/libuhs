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

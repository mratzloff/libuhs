#include "uhs.h"

namespace UHS::Traits {

//-------------------------------- Attributes -------------------------------//

const Attributes::Type& Attributes::attrs() const {
	return _attrs;
}

const std::string Attributes::attr(const std::string& key) const {
	std::string s;
	try {
		s = _attrs.at(key);
	} catch (const std::out_of_range& ex) {
		// Ignore
	}
	return s;
}

void Attributes::attr(const std::string key, const std::string value) {
	_attrs[key] = value;
}

//---------------------------------- Body -----------------------------------//

Body::Body(const std::string s) : _body {s} {}

const std::string& Body::body() const {
	return _body;
}

void Body::body(const std::string s) {
	_body = s;
}

//---------------------------------- Title ----------------------------------//

Title::Title(const std::string s) : _title {s} {}

const std::string& Title::title() const {
	return _title;
}

void Title::title(std::string s) {
	_title = s;
}

//-------------------------------- Visibility -------------------------------//

bool Visibility::visible(bool registered) const {
	return (_visibility == VisibilityAll
		|| (! registered && _visibility == VisibilityUnregistered)
		|| (registered && _visibility == VisibilityRegistered));
}

VisibilityType Visibility::visibility() const {
	return _visibility;
}

void Visibility::visibility(VisibilityType v) {
	_visibility = v;
}

}

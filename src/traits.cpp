#include "uhs.h"

namespace UHS::Traits {

//-------------------------------- Attributes -------------------------------//

const Attributes::Type& Attributes::attrs() const {
	return attrs_;
}

optional<const std::string> Attributes::attr(const std::string key) const {
	std::string s;
	try {
		s = attrs_.at(key);
	} catch (const std::out_of_range& ex) {
		return nullopt;
	}
	return s;
}

void Attributes::attr(const std::string key, const std::string value) {
	attrs_[key] = value;
}

//---------------------------------- Body -----------------------------------//

Body::Body(const std::string s) : body_{s} {}

const std::string& Body::body() const {
	return body_;
}

void Body::body(const std::string s) {
	body_ = s;
}

//---------------------------------- Title ----------------------------------//

Title::Title(const std::string s) : title_{s} {}

const std::string& Title::title() const {
	return title_;
}

void Title::title(std::string s) {
	title_ = s;
}

//-------------------------------- Visibility -------------------------------//

bool Visibility::visible(bool registered) const {
	return (visibility_ == VisibilityType::All
	        || (!registered && visibility_ == VisibilityType::Unregistered)
	        || (registered && visibility_ == VisibilityType::Registered));
}

VisibilityType Visibility::visibility() const {
	return visibility_;
}

void Visibility::visibility(VisibilityType v) {
	visibility_ = v;
}

} // namespace UHS::Traits

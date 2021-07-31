#include "uhs.h"

namespace UHS::Traits {

//-------------------------------- Attributes -------------------------------//

const Attributes::Type& Attributes::attrs() const {
	return attrs_;
}

std::optional<const std::string> Attributes::attr(const std::string key) const {
	std::string s;
	try {
		s = attrs_.at(key);
	} catch (const std::out_of_range&) {
		return std::nullopt;
	}
	return s;
}

void Attributes::attr(const std::string key, const std::string value) {
	attrs_[key] = value;
}

void Attributes::attr(const std::string key, const int value) {
	this->attr(key, std::to_string(value));
}

//---------------------------------- Body -----------------------------------//

Body::Body(const std::string body) : body_{body} {}

Body::Body(const int body) : Body(std::to_string(body)) {}

const std::string& Body::body() const {
	return body_;
}

void Body::body(const std::string body) {
	body_ = body;
}

void Body::body(const int body) {
	this->body(std::to_string(body));
}

//---------------------------------- Title ----------------------------------//

Title::Title(const std::string title) : title_{title} {}

const std::string& Title::title() const {
	return title_;
}

void Title::title(std::string title) {
	title_ = title;
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

void Visibility::visibility(VisibilityType visibility) {
	visibility_ = visibility;
}

} // namespace UHS::Traits

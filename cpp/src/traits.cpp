#include "uhs.h"

namespace UHS::Traits {

//-------------------------------- Attributes -------------------------------//

Attributes::Type const& Attributes::attrs() const {
	return attrs_;
}

std::optional<std::string const> Attributes::attr(std::string const& key) const {
	std::string s;
	try {
		s = attrs_.at(key);
	} catch (std::out_of_range const&) {
		return std::nullopt;
	}
	return s;
}

void Attributes::attr(std::string const& key, std::string const& value) {
	attrs_[key] = value;
}

void Attributes::attr(std::string const& key, int const value) {
	this->attr(key, std::to_string(value));
}

//---------------------------------- Body -----------------------------------//

Body::Body(std::string const& body) : body_{body} {}

Body::Body(int const body) : Body(std::to_string(body)) {}

std::string const& Body::body() const {
	return body_;
}

void Body::body(std::string const& body) {
	body_ = body;
}

void Body::body(int const body) {
	this->body(std::to_string(body));
}

//--------------------------------- Inlined ---------------------------------//

Inlined::Inlined(bool inlined) : inlined_{inlined} {}

bool Inlined::inlined() const {
	return inlined_;
}

void Inlined::inlined(bool inlined) {
	inlined_ = inlined;
}

//---------------------------------- Title ----------------------------------//

Title::Title(std::string const& title) : title_{title} {}

std::string const& Title::title() const {
	return title_;
}

void Title::title(std::string const& title) {
	title_ = title;
}

//-------------------------------- Visibility -------------------------------//

VisibilityType Visibility::visibility() const {
	return visibility_;
}

void Visibility::visibility(VisibilityType visibility) {
	visibility_ = visibility;
}

std::string const Visibility::visibilityString() const {
	switch (visibility_) {
	case VisibilityType::All:
		return "all";
	case VisibilityType::None:
		return "none";
	case VisibilityType::RegisteredOnly:
		return "registered";
	case VisibilityType::UnregisteredOnly:
		return "unregistered";
	default:
		throw DataError("invalid visibility");
	}
}

} // namespace UHS::Traits

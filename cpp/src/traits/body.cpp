#include "uhs/traits/body.h"

namespace UHS::Traits {

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

} // namespace UHS::Traits

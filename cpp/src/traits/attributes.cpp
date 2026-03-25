#include "uhs/traits/attributes.h"

namespace UHS::Traits {

Attributes::Type const& Attributes::attrs() const {
	return attrs_;
}

std::optional<std::string> Attributes::attr(std::string const& key) const {
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

} // namespace UHS::Traits

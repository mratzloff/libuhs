#include "uhs.h"

namespace UHS {

const std::map<std::string, std::string>& Attributes::attrs() const {
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

}

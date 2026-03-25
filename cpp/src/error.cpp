#include <exception>

#include "uhs/error/error.h"

namespace UHS {

Error::Error() : std::runtime_error("") {}

Error::Error(std::string const& message) : std::runtime_error(message) {}

Error::Error(char const* message) : std::runtime_error(message) {}

std::string const Error::string() const {
	std::string buffer;
	buffer += this->what();
	try {
		std::rethrow_if_nested(this);
	} catch (Error const& err) {
		buffer += " (";
		buffer += err.string();
		buffer += ")";
	} catch (...) {
		// Silence internal exceptions
	}
	return buffer;
}

std::ostream& operator<<(std::ostream& out, Error const& err) {
	out << err.string();
	return out;
}

} // namespace UHS

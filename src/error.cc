#include "uhs.h"

namespace UHS {

Error::Error() : _type {ErrorUnknown} {}

Error::Error(ErrorType t) : _type {t} {}

Error::Error(ErrorType t, std::string s) : _type {t}, _message {s} {}

Error::~Error() {}

int Error::type() const {
	return _type;
}

void Error::type(ErrorType t) {
	_type = t;
}

const std::string Error::message() const {
	return _message;
}

void Error::message(const std::string s) {
	_message = s;
}

}

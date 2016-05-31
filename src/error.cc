#include "error.h"

namespace UHS {

Error::Error() : _code {ErrUnknown} {}

Error::Error(int code) : _code {code} {}

Error::Error(int code, std::string message)
	: _code {code}
	, _message {message}
{}

Error::~Error() {}

int Error::code() const {
	return _code;
}

void Error::code(int code) {
	_code = code;
}

const std::string Error::message() const {
	return _message;
}

void Error::message(const std::string message) {
	_message = message;
}

}

#include "uhs.h"

namespace UHS {

Error::Error() {}

Error::Error(ErrorType t) : _type {t} {}

Error::Error(ErrorType t, std::string s) : _type {t}, _message {s} {}

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

void Error::messagef(const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, 256, format, args);
	va_end(args);
	_message = std::string(buf);
}

void Error::finalize() {
	_message = "error: " + _message;
}

void Error::finalize(int line, int column) {
	std::ostringstream out;
	out << "error at line " << line << ", column " << column << ": " << _message;
	_message = out.str();
}

}

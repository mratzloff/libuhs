namespace UHS {

Error::Error() : _code(ErrUnknown) {}

Error::Error(int code) : _code(code) {}

Error::Error(int code, std::string message)
	: _code(code)
	, _message(std::move(message))
{}

int code() const {
	return _code;
}

void code(int code) {
	_code = code;
}

const std::string message() const {
	return _message;
}

void message(const std::string message) {
	_message = message;
}

}

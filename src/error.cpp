#include "uhs.h"

namespace UHS {

//--------------------------------- Error -----------------------------------//

Error::Error() : std::runtime_error("") {}

Error::Error(const std::string& message) : std::runtime_error(message) {}

Error::Error(const char* message) : std::runtime_error(message) {}

std::ostream& operator<<(std::ostream& out, const Error& err) {
	out << err.what();
	try {
		std::rethrow_if_nested(err);
	} catch (const Error& err) {
		out << " (" << err << ")";
	} catch (...) {
		// Silence internal exceptions
	}
	return out;
}

//------------------------------- ParseError --------------------------------//

ParseError ParseError::badLine(int line, int column, int targetLine) {
	return ParseError(line, column, "line not found: %d", targetLine);
}

ParseError ParseError::badValue(
    int line, int column, ValueType expectedType, std::string found) {
	std::string expected;
	switch (expectedType) {
	case Uint:
		expected = "positive integer";
		break;
	case Int:
		expected = "integer";
		break;
	case String:
		expected = "string";
		break;
	case Date:
		expected = "date";
		break;
	case Time:
		expected = "time";
		break;
	}
	return ParseError::badValue(line, column, expected, found);
}

ParseError ParseError::badValue(
    int line, int column, std::string expected, std::string found) {
	return ParseError(
	    line, column, "expected %s, found '%s'", expected.data(), found.data());
}

ParseError ParseError::badToken(int line, int column, TokenType type) {
	return ParseError(line, column, "unexpected %s", Token::typeString(type).data());
}

ParseError ParseError::badToken(
    int line, int column, TokenType expected, TokenType found) {
	return ParseError(line,
	    column,
	    "expected %s, found %s",
	    Token::typeString(expected).data(),
	    Token::typeString(found).data());
}

ParseError::ParseError(int line, int column, const std::string& message)
    : ParseError(line, column, message.data()) {}

ParseError::ParseError(int line, int column, const char* message) : Error() {
	static_cast<Error&>(*this) = Error(this->format("%s", line, column, message));
}

} // namespace UHS

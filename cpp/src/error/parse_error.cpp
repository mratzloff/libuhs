#include "uhs/error/parse_error.h"
#include "uhs/constants.h"
#include "uhs/token.h"

namespace UHS {

ParseError::ParseError(int line, int column, std::string const& message)
    : Error(formatMessage("%s", line, column, message)) {}

ParseError::ParseError(int line, int column, char const* message)
    : Error(formatMessage("%s", line, column, message)) {}

ParseError ParseError::badLine(int line, int column, int targetLine) {
	return ParseError(line, column, "line not found: %d", targetLine);
}

ParseError ParseError::badToken(int line, int column, TokenType type) {
	return ParseError(line, column, "unexpected %s", Token::typeString(type));
}

ParseError ParseError::badToken(
    int line, int column, TokenType expected, TokenType found) {

	return ParseError(line,
	    column,
	    "expected %s, found %s",
	    Token::typeString(expected),
	    Token::typeString(found));
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

	return ParseError(line, column, "expected %s, found '%s'", expected, found);
}

} // namespace UHS

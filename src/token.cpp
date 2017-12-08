#include "uhs.h"

namespace UHS {

const std::string Token::typeString(TokenType t) {
	switch (t) {
	case TokenType::CRC:
		return "CRC";
	case TokenType::CoordX:
		return "CoordX";
	case TokenType::CoordY:
		return "CoordY";
	case TokenType::CreditSep:
		return "CreditSep";
	case TokenType::Data:
		return "Data";
	case TokenType::DataLength:
		return "DataLength";
	case TokenType::DataOffset:
		return "DataOffset";
	case TokenType::FileEnd:
		return "FileEnd";
	case TokenType::HeaderSep:
		return "HeaderSep";
	case TokenType::Ident:
		return "Ident";
	case TokenType::Length:
		return "Length";
	case TokenType::Line:
		return "Line";
	case TokenType::NestedElementSep:
		return "NestedElementSep";
	case TokenType::NestedTextSep:
		return "NestedTextSep";
	case TokenType::NestedParagraphSep:
		return "ParagraphSep";
	case TokenType::Signature:
		return "Signature";
	case TokenType::String:
		return "String";
	case TokenType::TextFormat:
		return "TextFormat";
	}
}

Token::Token(const TokenType tokenType, std::size_t offset, int line, std::size_t column,
    std::string value)
    : _type{tokenType}, _line{line}, _column{column}, _offset{offset}, _value{value} {}

std::ostream& operator<<(std::ostream& out, const Token& t) {
	out << t.string();
	return out;
}

TokenType Token::type() const {
	return _type;
}

int Token::line() const {
	return _line;
}

std::size_t Token::column() const {
	return _column;
}

std::size_t Token::offset() const {
	return _offset;
}

const std::string& Token::value() const {
	return _value;
}

const std::string Token::typeString() const {
	return Token::typeString(_type);
}

const std::string Token::string() const {
	auto buf = "("s;
	buf += this->formatToken();

	switch (_type) {
	case TokenType::CRC:
		[[fallthrough]];
	case TokenType::Data:
		buf += this->formatByteValue();
		break;
	case TokenType::Ident:
		[[fallthrough]];
	case TokenType::String:
		buf += this->formatStringValue();
		break;
	case TokenType::DataLength:
		[[fallthrough]];
	case TokenType::DataOffset:
		[[fallthrough]];
	case TokenType::Line:
		[[fallthrough]];
	case TokenType::Length:
		[[fallthrough]];
	case TokenType::CoordX:
		[[fallthrough]];
	case TokenType::CoordY:
		[[fallthrough]];
	case TokenType::TextFormat:
		buf += this->formatIntValue();
		break;
	default:
		break; // No special processing needed
	}
	buf += ')';

	return buf;
}

const std::string Token::formatToken() const {
	std::string buf{typeString()};
	buf += ' ';
	buf += std::to_string(_line);
	buf += ':';
	buf += std::to_string(_column);
	buf += ':';
	buf += std::to_string(_offset);

	return buf;
}

const std::string Token::formatIntValue() const {
	auto buf = " ["s;
	buf += _value;
	int intVal{Strings::toInt(_value)};
	if (intVal == -1) {
		buf += '?';
	}
	buf += ']';

	return buf;
}

const std::string Token::formatStringValue() const {
	auto buf = " ["s;
	buf += _value;
	buf += ']';

	return buf;
}

const std::string Token::formatByteValue() const {
	auto buf = " ["s;
	buf += Strings::hex(_value);
	buf += ']';

	return buf;
}

} // namespace UHS

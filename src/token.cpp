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
    : type_{tokenType}, line_{line}, column_{column}, offset_{offset}, value_{value} {}

std::ostream& operator<<(std::ostream& out, const Token& t) {
	out << t.string();
	return out;
}

TokenType Token::type() const {
	return type_;
}

int Token::line() const {
	return line_;
}

std::size_t Token::column() const {
	return column_;
}

std::size_t Token::offset() const {
	return offset_;
}

const std::string& Token::value() const {
	return value_;
}

const std::string Token::typeString() const {
	return Token::typeString(type_);
}

const std::string Token::string() const {
	auto buf = "("s;
	buf += this->formatToken();

	switch (type_) {
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
	buf += std::to_string(line_);
	buf += ':';
	buf += std::to_string(column_);
	buf += ':';
	buf += std::to_string(offset_);

	return buf;
}

const std::string Token::formatIntValue() const {
	auto buf = " ["s;
	buf += value_;
	int intVal{Strings::toInt(value_)};
	if (intVal == -1) {
		buf += '?';
	}
	buf += ']';

	return buf;
}

const std::string Token::formatStringValue() const {
	auto buf = " ["s;
	buf += value_;
	buf += ']';

	return buf;
}

const std::string Token::formatByteValue() const {
	auto buf = " ["s;
	buf += Strings::hex(value_);
	buf += ']';

	return buf;
}

} // namespace UHS

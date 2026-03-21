#include "uhs.h"

namespace UHS {

Token::TypeMap Token::typeMap_;

std::ostream& operator<<(std::ostream& out, Token const& t) {
	out << t.string();
	return out;
}

Token::Token(const TokenType tokenType, std::size_t offset, int line, std::size_t column,
    std::string value)
    : column_{column}, line_{line}, offset_{offset}, type_{tokenType}, value_{value} {}

std::string const Token::typeString(TokenType type) {
	return Token::typeMap_.findByType(type);
}

std::size_t Token::column() const {
	return column_;
}

int Token::line() const {
	return line_;
}

std::size_t Token::offset() const {
	return offset_;
}

std::string const Token::string() const {
	auto buffer = "("s;
	buffer += this->formatToken();

	switch (type_) {
	case TokenType::CRC:
		[[fallthrough]];
	case TokenType::Data:
		buffer += this->formatByteValue();
		break;
	case TokenType::Ident:
		[[fallthrough]];
	case TokenType::String:
		buffer += this->formatStringValue();
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
		buffer += this->formatIntValue();
		break;
	default:
		break; // No special processing needed
	}
	buffer += ')';

	return buffer;
}

TokenType Token::type() const {
	return type_;
}

std::string const Token::typeString() const {
	return Token::typeString(type_);
}

std::string const& Token::value() const {
	return value_;
}

std::string const Token::formatByteValue() const {
	auto buffer = " ["s;
	buffer += Strings::hex(value_);
	buffer += ']';

	return buffer;
}

std::string const Token::formatIntValue() const {
	return this->formatStringValue();
}

std::string const Token::formatStringValue() const {
	auto buffer = " ["s;
	buffer += value_;
	buffer += ']';

	return buffer;
}

std::string const Token::formatToken() const {
	auto buffer = typeString();
	buffer += ' ';
	buffer += std::to_string(line_);
	buffer += ':';
	buffer += std::to_string(column_);
	buffer += ':';
	buffer += std::to_string(offset_);

	return buffer;
}

Token::TypeMap::TypeMap() {
	map_.try_emplace(TokenType::CRC, "CRC");
	map_.try_emplace(TokenType::CoordX, "CoordX");
	map_.try_emplace(TokenType::CoordY, "CoordY");
	map_.try_emplace(TokenType::CreditSep, "CreditSep");
	map_.try_emplace(TokenType::Data, "Data");
	map_.try_emplace(TokenType::DataLength, "DataLength");
	map_.try_emplace(TokenType::DataOffset, "DataOffset");
	map_.try_emplace(TokenType::FileEnd, "FileEnd");
	map_.try_emplace(TokenType::HeaderSep, "HeaderSep");
	map_.try_emplace(TokenType::Ident, "Ident");
	map_.try_emplace(TokenType::Length, "Length");
	map_.try_emplace(TokenType::Line, "Line");
	map_.try_emplace(TokenType::NestedElementSep, "NestedElementSep");
	map_.try_emplace(TokenType::NestedTextSep, "NestedTextSep");
	map_.try_emplace(TokenType::NestedParagraphSep, "NestedParagraphSep");
	map_.try_emplace(TokenType::Signature, "Signature");
	map_.try_emplace(TokenType::String, "String");
	map_.try_emplace(TokenType::TextFormat, "TextFormat");
}

std::string const Token::TypeMap::findByType(const TokenType type) const {
	return map_.at(type);
}

} // namespace UHS

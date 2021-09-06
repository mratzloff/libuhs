#include "uhs.h"

namespace UHS {

Token::TypeMap Token::typeMap_;

const std::string Token::typeString(TokenType type) {
	return Token::typeMap_.findByType(type);
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

const std::string Token::formatToken() const {
	auto buffer = typeString();
	buffer += ' ';
	buffer += std::to_string(line_);
	buffer += ':';
	buffer += std::to_string(column_);
	buffer += ':';
	buffer += std::to_string(offset_);

	return buffer;
}

const std::string Token::formatIntValue() const {
	return this->formatStringValue();
}

const std::string Token::formatStringValue() const {
	auto buffer = " ["s;
	buffer += value_;
	buffer += ']';

	return buffer;
}

const std::string Token::formatByteValue() const {
	auto buffer = " ["s;
	buffer += Strings::hex(value_);
	buffer += ']';

	return buffer;
}

Token::TypeMap::TypeMap() {
	map_.emplace(TokenType::CRC, "CRC");
	map_.emplace(TokenType::CoordX, "CoordX");
	map_.emplace(TokenType::CoordY, "CoordY");
	map_.emplace(TokenType::CreditSep, "CreditSep");
	map_.emplace(TokenType::Data, "Data");
	map_.emplace(TokenType::DataLength, "DataLength");
	map_.emplace(TokenType::DataOffset, "DataOffset");
	map_.emplace(TokenType::FileEnd, "FileEnd");
	map_.emplace(TokenType::HeaderSep, "HeaderSep");
	map_.emplace(TokenType::Ident, "Ident");
	map_.emplace(TokenType::Length, "Length");
	map_.emplace(TokenType::Line, "Line");
	map_.emplace(TokenType::NestedElementSep, "NestedElementSep");
	map_.emplace(TokenType::NestedTextSep, "NestedTextSep");
	map_.emplace(TokenType::NestedParagraphSep, "NestedParagraphSep");
	map_.emplace(TokenType::Signature, "Signature");
	map_.emplace(TokenType::String, "String");
	map_.emplace(TokenType::TextFormat, "TextFormat");
}

const std::string Token::TypeMap::findByType(const TokenType type) const {
	return map_.at(type);
}

} // namespace UHS

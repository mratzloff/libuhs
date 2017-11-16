#include <iomanip>
#include "uhs.h"

namespace UHS {

const std::string Token::typeString(TokenType t) {
	switch (t) {
	case TokenCRC:
		return "CRC";
	case TokenHeaderSep:
		return "CompatSep";
	case TokenCoordX:
		return "CoordX";
	case TokenCoordY:
		return "CoordY";
	case TokenCreditSep:
		return "CreditSep";
	case TokenData:
		return "Data";
	case TokenDataLength:
		return "DataLength";
	case TokenDataOffset:
		return "DataOffset";
	case TokenDataType:
		return "DataType";
	case TokenEOF:
		return "EOF";
	case TokenIdent:
		return "Ident";
	case TokenIndex:
		return "Index";
	case TokenLength:
		return "Length";
	case TokenNestedElementSep:
		return "NestedElementSep";
	case TokenNestedTextSep:
		return "NestedTextSep";
	case TokenParagraphSep:
		return "ParagraphSep";
	case TokenSignature:
		return "Signature";
	case TokenString:
		return "String";
	}
}

Token::Token(const TokenType tokenType, std::size_t offset, int line,
		std::size_t column, std::string value)
	: _type {tokenType}
	, _line {line}
	, _column {column}
	, _offset {offset}
	, _value {value}
{}

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

const std::string Token::toString() const {
	std::string buf {"("};
	buf += this->formatToken();

	switch (_type) {
	case TokenCRC:
		// Fall through
	case TokenData:
		buf += this->formatByteValue();
		break;
	case TokenIdent:
		// Fall through
	case TokenString:
		buf += this->formatStringValue();
		break;
	case TokenDataLength:
		// Fall through
	case TokenDataOffset:
		// Fall through
	case TokenDataType:
		// Fall through
	case TokenIndex:
		// Fall through
	case TokenLength:
		// Fall through
	case TokenCoordX:
		// Fall through
	case TokenCoordY:
		buf += this->formatIntValue();
		break;
	default:
		break; // No special processing needed
	}
	buf += ')';

	return buf;
}

std::ostream& operator<<(std::ostream& out, const Token& t) {
	out << t.toString();
	return out;
}

const std::string Token::formatToken() const {
	std::string buf {typeString()};
	buf += ' ';
	buf += std::to_string(_line);
	buf += ':';
	buf += std::to_string(_column);
	buf += ':';
	buf += std::to_string(_offset);

	return buf;
}

const std::string Token::formatIntValue() const {
	std::string buf {" ["};
	buf += _value;
	int intVal {Strings::toInt(_value)};
	if (intVal == -1) {
		buf += '?';
	}
	buf += ']';

	return buf;
}

const std::string Token::formatStringValue() const {
	std::string buf {" ["};
	buf += _value;
	buf += ']';

	return buf;
}

const std::string Token::formatByteValue() const {
	std::ostringstream ss;

	ss << std::hex << std::setfill('0') << std::uppercase;
	int i = 0;
	for (const auto c : _value) {
		if (i > 0) {
			ss << ' ';
		}
		ss << std::setw(2) << (int(c) & ~0xFFFFFF00);
		++i;
	}
	std::string buf {" ["};
	buf += ss.str();
	buf += ']';

	return buf;
}

}

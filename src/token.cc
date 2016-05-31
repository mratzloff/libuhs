#include "token.h"

namespace UHS {

IdentType Token::identType(std::string ident) {
	if (ident == "blank") {
		return IdentBlank;
	} else if (ident == "comment") {
		return IdentComment;
	} else if (ident == "credit") {
		return IdentCredit;
	} else if (ident == "gifa") {
		return IdentGifa;
	} else if (ident == "hint") {
		return IdentHint;
	} else if (ident == "hyperpng") {
		return IdentHyperpng;
	} else if (ident == "incentive") {
		return IdentIncentive;
	} else if (ident == "info") {
		return IdentInfo;
	} else if (ident == "link") {
		return IdentLink;
	} else if (ident == "nesthint") {
		return IdentNesthint;
	} else if (ident == "overlay") {
		return IdentOverlay;
	} else if (ident == "sound") {
		return IdentSound;
	} else if (ident == "subject") {
		return IdentSubject;
	} else if (ident == "text") {
		return IdentText;
	} else if (ident == "version") {
		return IdentVersion;
	} else {
		return IdentUnknown;
	}
}

Token::Token(const TokenType tokenType, int offset, int line, int column, std::string value)
	: _tokenType(tokenType)
	, _line(line)
	, _column(column)
	, _offset(offset)
	, _value(std::move(value))
{}

TokenType Token::type() const {
	return _tokenType;
}

int Token::line() const {
	return _line;
}

int Token::column() const {
	return _column;
}

int Token::offset() const {
	return _offset;
}

const std::string Token::stringValue() const {
	return _value;
}

int Token::intValue() const {
	int intVal;
	std::string::size_type idx;
	try {
		intVal = std::stoi(_value, &idx);
		if (idx != _value.length()) {
			intVal = -1;
		}
	} catch (const std::invalid_argument& e) {
		intVal = -1;
	}
	return intVal;
}

const std::string Token::typeString() const {
	switch (_tokenType) {
	case TokenCompatSep:
		return "CompatSep";
	case TokenCreditSep:
		return "CreditSep";
	case TokenData:
		return "Data";
	case TokenDataLength:
		return "DataLength";
	case TokenDataOffset:
		return "DataOffset";
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
	case TokenRegionX:
		return "RegionX";
	case TokenRegionY:
		return "RegionY";
	case TokenSignature:
		return "Signature";
	case TokenString:
		return "String";
	}
}

const std::string Token::toString() const {
	std::string buf {"("};
	buf += formatToken();

	switch (_tokenType) {
	case TokenData:
		buf += formatByteValue();
		break;
	case TokenDataLength:
		buf += formatIntValue();
		break;
	case TokenDataOffset:
		buf += formatIntValue();
		break;
	case TokenIdent:
		buf += formatStringValue();
		break;
	case TokenIndex:
		buf += formatIntValue();
		break;
	case TokenLength:
		buf += formatIntValue();
		break;
	case TokenRegionX:
		buf += formatIntValue();
		break;
	case TokenRegionY:
		buf += formatIntValue();
		break;
	case TokenString:
		buf += formatStringValue();
		break;
	default:
		break; // No special processing needed
	}
	buf += ")";

	return buf;
}

std::ostream& operator<<(std::ostream &out, const Token &t) {
	return out << t.toString();
}

const std::string Token::formatToken() const {
	std::string buf {typeString()};
	buf += " ";
	buf += std::to_string(_line);
	buf += ":";
	buf += std::to_string(_column);
	buf += ":";
	buf += std::to_string(_offset);
	return buf;
}

const std::string Token::formatIntValue() const {
	std::string buf {" ["};
	int intVal {this->intValue()};
	if (intVal == -1) {
		buf += "?";
	} else {
		buf += _value;
	}
	buf += "]";
	return buf;
}

const std::string Token::formatStringValue() const {
	std::string buf {" ["};
	buf += _value;
	buf += "]";
	return buf;
}

// todo: Fix this later
const std::string Token::formatByteValue() const {
	std::string buf {" ["};
	buf += _value;
	buf += "]";
	return buf;
}

}

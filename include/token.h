#ifndef UHS_TOKEN_H
#define UHS_TOKEN_H

#include <string>
#include <ostream>

namespace UHS {

enum TokenType {
	TokenCompatSep,
	TokenCreditSep,
	TokenData,
	TokenDataLength,
	TokenDataOffset,
	TokenEOF,
	TokenIdent,
	TokenIndex,
	TokenLength,
	TokenNestedElementSep,
	TokenNestedTextSep,
	TokenRegionX,
	TokenRegionY,
	TokenSignature,
	TokenString,
};

class Token {
public:
	Token(const TokenType tokenType, int line, int column, int offset, std::string value);
	TokenType type() const;
	int line() const;
	int column() const;
	int offset() const;
	const std::string stringValue() const;
	int intValue() const;
	const std::string typeString() const;
	const std::string toString() const;
	
	virtual ~Token();

protected:
	const TokenType _tokenType;
	int _line;
	int _column;
	int _offset;
	std::string _value;

	const std::string formatToken() const;
	const std::string formatIntValue() const;
	const std::string formatStringValue() const;
	const std::string formatByteValue() const;
};

std::ostream& operator<<(std::ostream &out, const Token &t);

}

#endif

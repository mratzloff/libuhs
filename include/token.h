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
	TokenParagraphSep,
	TokenRegionX,
	TokenRegionY,
	TokenSignature,
	TokenString,
};

enum IdentType {
	IdentUnknown,
	IdentBlank,
	IdentComment,
	IdentCredit,
	IdentGifa,
	IdentHint,
	IdentHyperpng,
	IdentIncentive,
	IdentInfo,
	IdentLink,
	IdentNesthint,
	IdentOverlay,
	IdentSound,
	IdentSubject,
	IdentText,
	IdentVersion,
};

class Token {
public:
	static constexpr const char* Signature = "UHS";
	static constexpr const char* CompatSep = "** END OF 88A FORMAT **";
	static constexpr const char* CreditSep = "CREDITS:";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static constexpr const char* ParagraphSep = " "; // i.e., "text.\r\n \r\nText"
	static const char DataSep = '\x1A';

	static IdentType identType(std::string ident);
	
	Token(const TokenType tokenType, std::size_t offset = 0, int line = 0,
		std::size_t column = 0, std::string value = "");
	virtual ~Token();
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string stringValue() const;
	int intValue() const;
	const std::string typeString() const;
	const std::string toString() const;

protected:
	const TokenType _tokenType;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _value;

	const std::string formatToken() const;
	const std::string formatIntValue() const;
	const std::string formatStringValue() const;
	const std::string formatByteValue() const;
};

std::ostream& operator<<(std::ostream &out, const Token &t);

}

#endif

#if !defined UHS_TOKEN_H
#define UHS_TOKEN_H

#include <map>
#include <ostream>
#include <string>

#include "uhs/constants.h"

namespace UHS {

class Tokenizer;

class Token {
public:
	friend std::ostream& operator<<(std::ostream& out, Token const& t);

	static constexpr auto AsciiEncBegin = "#a+";
	static constexpr auto AsciiEncEnd = "#a-";
	static constexpr auto CreditSep = "CREDITS:";
	static auto const DataSep = '\x1A';
	static constexpr auto HeaderSep = "** END OF 88A FORMAT **";
	static constexpr auto HyperlinkBegin = "#h+";
	static constexpr auto HyperlinkEnd = "#h-";
	static constexpr auto InfoKeyValueSep = "=";
	static constexpr auto InlineBegin = "#w+";
	static constexpr auto InlineEnd = "#w.";
	static constexpr auto MonospaceBegin = "#p-";
	static constexpr auto MonospaceEnd = "#p+";
	static constexpr auto NestedElementSep = "=";
	static constexpr auto NestedTextSep = "-";
	static constexpr auto NoticePrefix = ">";
	static constexpr auto NumberSign = "##";
	static constexpr auto OverflowBegin = "#w-";
	static constexpr auto OverflowEnd = "#w+";
	static constexpr auto ParagraphSep = " "; // e.g., "text.\r\n \r\nText"
	static constexpr auto RegisteredOnly = "A";
	static constexpr auto Signature = "UHS";
	static constexpr auto UnregisteredOnly = "Z";

	Token(TokenType const tokenType, std::size_t offset = 0, int line = 0,
	    std::size_t column = 0, std::string value = "");

	static std::string const typeString(TokenType t);

	std::size_t column() const;
	int line() const;
	std::size_t offset() const;
	std::string const string() const;
	TokenType type() const;
	std::string const typeString() const;
	std::string const& value() const;

private:
	class TypeMap {
	public:
		TypeMap();
		std::string const findByType(TokenType const type) const;

	private:
		std::map<TokenType const, std::string const> map_;
	};

	static Token::TypeMap typeMap_;

	std::size_t column_ = 0;
	int line_ = 0;
	std::size_t offset_ = 0;
	TokenType const type_;
	std::string value_;

	std::string const formatByteValue() const;
	std::string const formatIntValue() const;
	std::string const formatStringValue() const;
	std::string const formatToken() const;
};

} // namespace UHS

#endif

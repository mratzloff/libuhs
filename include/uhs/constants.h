#if !defined UHS_CONSTANTS_H
#define UHS_CONSTANTS_H

#include <cstdint>
#include <string>

namespace UHS {

using std::string_literals::operator""s;

enum class ElementType {
	Unknown,
	Blank,
	Comment,
	Credit,
	Gifa,
	Hint,
	Hyperpng,
	Incentive,
	Info,
	Link,
	Nesthint,
	Overlay,
	Sound,
	Subject,
	Text,
	Version,
};

enum class LogLevel {
	None,
	Error,
	Warn,
	Info,
	Debug,
};

enum class NodeType {
	Break,
	Document,
	Element,
	Group,
	Text,
};

enum class ModeType {
	Auto,
	Version88a,
	Version96a,
};

enum class TextFormat : uint8_t {
	None = 0, // Also used for binary data
	Monospace = 1,
	Overflow = 2,
	Hyperlink = 4,
};

enum class TokenType {
	CRC,
	CoordX,
	CoordY,
	CreditSep,
	Data,
	DataLength,
	DataOffset,
	FileEnd,
	HeaderSep,
	Ident,
	Length,
	Line,
	NestedElementSep,
	NestedTextSep,
	NestedParagraphSep,
	Signature,
	String,
	TextFormat,
};

enum class VersionType {
	Version88a,
	Version91a,
	Version95a,
	Version96a,
};

enum class VisibilityType {
	All,              // Visible to every user
	UnregisteredOnly, // Visible only to unregistered users
	RegisteredOnly,   // Visible only to registered users
	None,             // Visible to no one
};

static constexpr auto EOL = "\r\n";
static auto const MaxDepth = 24;

inline constexpr TextFormat operator&(TextFormat lhs, TextFormat rhs) {
	return static_cast<TextFormat>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

inline constexpr TextFormat operator|(TextFormat lhs, TextFormat rhs) {
	return static_cast<TextFormat>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline constexpr TextFormat operator^(TextFormat lhs, TextFormat rhs) {
	return static_cast<TextFormat>(static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
}

inline constexpr TextFormat operator~(TextFormat operand) {
	return static_cast<TextFormat>(~static_cast<uint8_t>(operand));
}

inline constexpr TextFormat operator&=(TextFormat& lhs, TextFormat rhs) {
	lhs = lhs & rhs;
	return lhs;
}

inline constexpr TextFormat operator|=(TextFormat& lhs, TextFormat rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline constexpr TextFormat operator^=(TextFormat& lhs, TextFormat rhs) {
	lhs = lhs ^ rhs;
	return lhs;
}

inline constexpr TextFormat withFormat(
    TextFormat const format, TextFormat const formatToAdd) {
	return format | formatToAdd;
}

inline constexpr TextFormat withoutFormat(
    TextFormat const format, TextFormat const formatToRemove) {
	return format & ~formatToRemove;
}

inline constexpr bool hasFormat(TextFormat const haystack, TextFormat const needle) {
	return (haystack & needle) == needle;
}

} // namespace UHS

#endif

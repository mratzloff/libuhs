#ifndef UHS_H
#define UHS_H

#define UHS_VERSION "1.0.0-alpha"

#include <cassert>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <queue>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "hopscotch_map.h"
#include "json.h"
#include "pugixml.hpp"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "tinyformat.h"
#pragma clang diagnostic pop

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

enum class TextFormat : uint8_t {
	None = 0, // Also used for binary data
	Monospace = 1,
	Overflow = 2,
	Hyperlink = 4,
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
static const auto MaxDepth = 24;
static constexpr auto Version = UHS_VERSION;

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
    const TextFormat format, const TextFormat formatToAdd) {
	return format | formatToAdd;
}

inline constexpr TextFormat withoutFormat(
    const TextFormat format, const TextFormat formatToRemove) {
	return format & ~formatToRemove;
}

inline constexpr bool hasFormat(const TextFormat haystack, const TextFormat needle) {
	return (haystack & needle) == needle;
}

class Error : public std::runtime_error {
public:
	Error();
	Error(const std::string& message);
	Error(const char* message);

	template<typename... Args>
	Error(const char* format, Args... args) : Error() {
		auto message = tfm::format(format, args...);
		// TODO: Review for slice
		static_cast<std::runtime_error&>(*this) = std::runtime_error(message);
	}

	const std::string string() const;
};

std::ostream& operator<<(std::ostream& out, const Error& err);

class LogicError : public Error {
	using Error::Error;
};

class ReadError : public Error {
	using Error::Error;
};

class ParseError : public Error {
public:
	enum ValueType {
		Uint,
		Int,
		String,
		Date,
		Time,
	};

	static ParseError badLine(int line, int column, int targetLine);
	static ParseError badValue(
	    int line, int column, ValueType expectedType, std::string found);
	static ParseError badValue(
	    int line, int column, std::string expected, std::string found);
	static ParseError badToken(int line, int column, TokenType type);
	static ParseError badToken(int line, int column, TokenType expected, TokenType found);

	ParseError(int line, int column, const std::string& message);
	ParseError(int line, int column, const char* message);

	template<typename... Args>
	ParseError(int line, int column, const char* format, Args... args) : Error() {
		// TODO: Review for slice
		static_cast<Error&>(*this) = Error(this->format(format, line, column, args...));
	}

private:
	template<typename... Args>
	const std::string format(const char* format, int line, int column, Args... args) {
		auto fmt = "parse error at line %d, column %d: "s + format;
		return tfm::format(fmt.data(), line, column, args...);
	}
};

class WriteError : public Error {
	using Error::Error;
};

class Logger {
public:
	void error(const char* message) const { this->error("%s", message); }

	void error(const Error& err) const { this->error("%s", err.string().c_str()); }

	template<typename... Args>
	void error(const char* format, Args... args) const {
		this->log(std::cout, "error: ", format, args...);
	}

	void warn(const char* message) const { this->warn("%s", message); }

	void warn(const Error& err) const { this->warn("%s", err.string().c_str()); }

	template<typename... Args>
	void warn(const char* format, Args... args) const {
		this->log(std::cout, "warning: ", format, args...);
	}

private:
	template<typename... Args>
	void log(std::ostream& ostream, const char* prefix, const char* format,
	    Args... args) const {

		std::string buffer;
		buffer += "uhs: ";
		buffer += prefix;
		buffer += format;
		buffer += '\n';

		tfm::format(ostream, buffer.c_str(), args...);
	}
};

struct Options {
	bool debug = false;
	std::string mediaDir;
	ModeType mode;
	bool preserve = false;
	bool quiet = false;
};

namespace Strings {

static const char AsciiStart = 0x20;
static const char AsciiEnd = 0x7F;

bool beginsWithAttachedPunctuation(const std::string& s);
std::string& chomp(std::string& s, char c);
bool endsWithAttachedPunctuation(const std::string& s);
bool endsWithFinalPunctuation(const std::string& s);
const std::string hex(const std::string& s);
const std::string hex(char s);
bool isInt(const std::string& s);
bool isPrintable(int c);
std::string join(const std::vector<std::string>& s, const std::string& sep);
std::string ltrim(const std::string& s, char c);
std::string rtrim(const std::string& s, char c);
std::vector<std::string> split(const std::string& s, const std::string& sep, int n = 0);
std::string toBase64(const std::string& s);
int toInt(const std::string& s);
std::string wrap(const std::string& s, const std::string& sep, std::size_t width);
std::string wrap(const std::string& s, const std::string& sep, std::size_t width,
    int& numLines, const std::string prefix = "");

} // namespace Strings

namespace Regex {

const std::regex Descriptor{"([0-9]+) ([a-z]{4,})"};
const std::regex DataAddress{"0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})"};
const std::regex EmailAddress{".+@.+\\..+"};
const std::regex HyperpngRegion{
    "(-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
const std::regex OverlayAddress{
    "0{6} ([0-9]{6,}) ([0-9]{6,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
const std::regex URL{"https?://.+\\..+"};

} // namespace Regex

namespace Traits {

class Attributes {
public:
	using Type = std::map<std::string, std::string>;

	const Type& attrs() const;
	std::optional<const std::string> attr(const std::string key) const;
	void attr(const std::string key, const std::string value);
	void attr(const std::string key, const int value);

private:
	Type attrs_;
};

class Body {
public:
	Body() = default;
	explicit Body(const std::string body);
	explicit Body(const int body);
	const std::string& body() const;
	void body(const std::string body);
	void body(const int body);

private:
	std::string body_;
};

class Inlined { // `inline` is a C++ keyword
public:
	Inlined() = default;
	explicit Inlined(bool inlined);
	bool inlined() const;
	void inlined(bool inlined);

private:
	bool inlined_ = false;
};

class Title {
public:
	Title() = default;
	explicit Title(const std::string title);
	const std::string& title() const;
	void title(const std::string title);

private:
	std::string title_;
};

class Visibility {
public:
	VisibilityType visibility() const;
	void visibility(VisibilityType visibility);
	const std::string visibilityString() const;

private:
	VisibilityType visibility_ = VisibilityType::All;
};

} // namespace Traits

class Pipe {
public:
	using Handler = std::function<void(const char*, std::streamsize n)>;

	explicit Pipe(std::istream& in);
	void addHandler(Handler func);
	std::exception_ptr error();
	void read();
	bool good();
	bool eof();

private:
	static const std::size_t ReadLength = 1024;

	std::istream& in_;
	std::size_t offset_ = 0;
	std::vector<Handler> handlers_;
	std::exception_ptr err_ = nullptr;
};

class CRC {
public:
	static const auto ByteLength = 2;

	CRC();
	void upstream(Pipe& pipe);
	void calculate(const char* buffer, std::streamsize length, bool bufferChecksum);
	void calculate(const char* buffer, std::streamsize length);
	uint16_t result();
	void result(std::vector<char>& out);
	bool valid();
	void reset();

private:
	static const auto TableLength = 256;
	static const uint16_t Polynomial = 0x8005;
	static const uint16_t CastMask = 0xFFFF;
	static const uint16_t MSBMask = 0x8000;
	static const uint16_t FinalXOR = 0x0100;

	Pipe* pipe_ = nullptr;
	uint16_t table_[TableLength];
	char checksum_[2];
	int checksumLength_ = 0;
	uint16_t remainder_ = 0x0000;
	bool finalized_ = false;

	void createTable();
	uint8_t reflectByte(uint8_t byte);
	void finalize();
	uint16_t checksum();
};

class Tokenizer;

class Token {
public:
	static constexpr auto AsciiEncBegin = "#a+";
	static constexpr auto AsciiEncEnd = "#a-";
	static constexpr auto CreditSep = "CREDITS:";
	static const auto DataSep = '\x1A';
	static constexpr auto HyperlinkBegin = "#h+";
	static constexpr auto HyperlinkEnd = "#h-";
	static constexpr auto HeaderSep = "** END OF 88A FORMAT **";
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

	static const std::string typeString(TokenType t);

	Token(const TokenType tokenType, std::size_t offset = 0, int line = 0,
	    std::size_t column = 0, std::string value = "");
	friend std::ostream& operator<<(std::ostream& out, const Token& t);
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string& value() const;
	const std::string typeString() const;
	const std::string string() const;

private:
	class TypeMap {
	public:
		TypeMap();
		const std::string findByType(const TokenType type) const;

	private:
		std::map<const TokenType, const std::string> map_;
	};

	static Token::TypeMap typeMap_;

	const TokenType type_;
	int line_ = 0;
	std::size_t column_ = 0;
	std::size_t offset_ = 0;
	std::string value_;

	const std::string formatToken() const;
	const std::string formatIntValue() const;
	const std::string formatStringValue() const;
	const std::string formatByteValue() const;
};

class Tokenizer {
public:
	explicit Tokenizer(Pipe& pipe);
	void tokenize(const char* buffer, std::streamsize length);
	bool hasNext();
	std::unique_ptr<const Token> next();

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& pipe);
		void send(const Token&& token);
		std::unique_ptr<const Token> receive();
		bool empty() const;
		bool ok() const;
		void close();

	private:
		std::queue<const Token> queue_;
		mutable std::mutex mutex_;
		bool open_ = true;
		Pipe& pipe_; // For exceptions
	};

	Pipe& pipe_;
	std::string buffer_;
	int line_ = 1;
	std::size_t offset_ = 0;
	bool beforeHeaderSep_ = true;
	bool binaryMode_ = false;
	int expectedLineTokenLine_ = 0;
	int expectedStringTokenLine_ = 0;
	TokenChannel out_;

	void tokenizeLine();
	ElementType tokenizeDescriptor(const std::smatch& matches);
	void tokenizeDataAddress(const std::smatch& matches);
	void tokenizeHyperpngRegion(const std::smatch& matches);
	void tokenizeOverlayAddress(const std::smatch& matches);
	void tokenizeMatches(
	    const std::smatch& matches, const std::vector<TokenType>&& tokens);
	void tokenizeData(const std::string& data, std::size_t column);
	void tokenizeCRC(const std::string& crc, std::size_t column);
	void tokenizeEOF(std::size_t column);
};

template<typename T>
class NodeIterator;

class Document;

class Node {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<const Node>;

	static const std::string typeString(NodeType type);
	static bool isElementOfType(const Node& node, ElementType type);

	explicit Node(NodeType type);
	Node(const Node& other);
	Node& operator=(Node other);
	virtual ~Node() = default;
	friend void swap(Node& lhs, Node& rhs) noexcept;
	NodeType nodeType() const;
	const std::string nodeTypeString() const;
	bool isBreak() const;
	bool isDocument() const;
	bool isElement() const;
	bool isGroup() const;
	bool isText() const;
	void detachParent();
	void removeChild(std::shared_ptr<Node> node);
	void appendChild(std::shared_ptr<Node> node);
	void appendChild(std::shared_ptr<Node> node, bool silenceEvent);
	std::shared_ptr<Node> pointer() const;
	void insertBefore(std::shared_ptr<Node> node, Node* ref);
	bool hasParent() const;
	Node* parent() const;
	bool hasPreviousSibling() const;
	Node* previousSibling() const;
	bool hasNextSibling() const;
	std::shared_ptr<Node> nextSibling() const;
	bool hasFirstChild() const;
	std::shared_ptr<Node> firstChild() const;
	bool hasLastChild() const;
	Node* lastChild() const;
	int numChildren() const;
	int depth() const;
	Document* findDocument() const;

	// Iterators
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

protected:
	void cloneChildren(const Node& node);
	virtual void didRemove();
	virtual void didAdd();

private:
	NodeType nodeType_;
	Node* parent_ = nullptr;
	Node* previousSibling_ = nullptr;
	std::shared_ptr<Node> nextSibling_ = nullptr;
	std::shared_ptr<Node> firstChild_ = nullptr;
	Node* lastChild_ = nullptr;
	int numChildren_ = 0;
	int depth_ = 0;
};

class ContainerNode : public Node {
public:
	using Node::appendChild;

	explicit ContainerNode(NodeType type);
	ContainerNode(const ContainerNode& other);
	ContainerNode& operator=(ContainerNode other);
	virtual ~ContainerNode() = default;
	friend void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept;
	int line() const;
	void line(int line);
	int length() const;
	void length(int length); // Used by UHSWriter

protected:
	int line_ = 0;
	int length_ = 0;
};

template<typename T>
class NodeIterator {
public:
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T*;
	using reference = T&;
	using iterator_category = std::forward_iterator_tag;

	NodeIterator() = default;
	explicit NodeIterator(pointer node);
	reference operator*() const;
	pointer operator->() const;
	NodeIterator<T>& operator++();
	NodeIterator<T> operator++(int);
	bool operator==(const NodeIterator<T>& rhs) const;
	bool operator!=(const NodeIterator<T>& rhs) const;

private:
	pointer initial_ = nullptr;
	pointer current_ = nullptr;
	bool visited_ = false;
};

class BreakNode : public Node {
public:
	static std::shared_ptr<BreakNode> create();
	BreakNode();
	BreakNode(const BreakNode&);
	BreakNode& operator=(BreakNode);
	std::shared_ptr<BreakNode> clone() const;
};

class GroupNode : public ContainerNode {
public:
	static std::shared_ptr<GroupNode> create(int line, int length);
	GroupNode(int line, int length);
	GroupNode(const GroupNode&);
	GroupNode& operator=(GroupNode);
	friend void swap(GroupNode& lhs, GroupNode& rhs) noexcept;
	std::shared_ptr<GroupNode> clone() const;
};

class TextNode
    : public Node
    , public Traits::Body {
public:
	static std::shared_ptr<TextNode> create(const std::string body);
	static std::shared_ptr<TextNode> create(const std::string body, TextFormat format);

	explicit TextNode(const std::string body);
	TextNode(const std::string body, TextFormat format);
	TextNode(const TextNode& other);
	TextNode& operator=(TextNode other);
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;
	std::shared_ptr<TextNode> clone() const;
	const std::string& string() const;
	void addFormat(TextFormat format);
	void removeFormat(TextFormat format);
	bool hasFormat(TextFormat format) const;
	TextFormat format() const;
	void format(TextFormat format);

private:
	TextFormat format_ = TextFormat::None;
};

class Document;

class Element
    : public ContainerNode
    , public Traits::Attributes
    , public Traits::Body
    , public Traits::Title
    , public Traits::Inlined
    , public Traits::Visibility {
public:
	static std::shared_ptr<Element> create(ElementType type, const int id = 0);
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType type);

	Element(ElementType type, int id = 0);
	Element(const Element& other);
	Element& operator=(Element other);
	friend void swap(Element& lhs, Element& rhs) noexcept;
	std::shared_ptr<Element> clone() const;
	ElementType elementType() const;
	const std::string elementTypeString() const;
	int id() const;
	bool isMedia() const;
	const std::string mediaExt() const;

private:
	class TypeMap {
	public:
		TypeMap();
		const std::string findByType(const ElementType type) const;
		ElementType findByString(const std::string& string) const;

	private:
		std::map<const ElementType, const std::string> byType_;
		std::map<const std::string, const ElementType> byString_;
	};

	static Element::TypeMap typeMap_;

	ElementType elementType_;
	int id_ = 0;
};

class Document
    : public Node
    , public Traits::Attributes
    , public Traits::Title
    , public Traits::Visibility {
public:
	static std::shared_ptr<Document> create(VersionType version);

	Document(VersionType version);
	Document(const Document& other);
	Document& operator=(Document other);
	friend void swap(Document& lhs, Document& rhs) noexcept;
	Node* find(const int id);
	std::shared_ptr<Document> clone() const;
	void version(VersionType v);
	VersionType version() const;
	bool isVersion(VersionType v) const;
	const std::string versionString() const;
	void validChecksum(bool value);
	bool validChecksum() const;
	void elementRemoved(Element& element);
	void elementAdded(Element& element);
	void reindex();

private:
	VersionType version_;
	bool validChecksum_ = false;
	std::map<const int, Node*> index_;
	bool indexed_ = true;
};

class Codec {
public:
	Codec(const Options options = {});
	const std::string createKey(std::string secret) const;
	const std::string decode88a(std::string encoded) const;
	const std::string decode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	const std::string decodeSpecialChars(const std::string& encoded) const;
	const std::string encode88a(std::string decoded) const;
	const std::string encode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	const std::string encodeSpecialChars(const std::string& decoded) const;

private:
	using UnicodeToAsciiMap = const std::map<const char32_t, const std::string>;
	using AsciiToUnicodeMap = const std::map<const std::string, const std::string>;

	static constexpr auto KeySeed = "key";

	UnicodeToAsciiMap fromChars_ = {
	    {U'Š', "S^"},
	    {U'Œ', "OE"},
	    {U'–', "-"},
	    {U'—', "--"},
	    {U'™', "TM"},
	    {U'š', "s^"},
	    {U'œ', "oe"},
	    {U'Ÿ', "Y:"},
	    {U'©', "(C)"},
	    {U'®', "(R)"},
	    {U'À', "A`"},
	    {U'Á', "A'"},
	    {U'Â', "A^"},
	    {U'Ã', "A~"},
	    {U'Ä', "A:"},
	    {U'Å', "Ao"},
	    {U'Æ', "AE"},
	    {U'Ç', "C,"},
	    {U'È', "E`"},
	    {U'É', "E'"},
	    {U'Ê', "E^"},
	    {U'Ë', "E:"},
	    {U'Ì', "I`"},
	    {U'Í', "I'"},
	    {U'Î', "I^"},
	    {U'Ï', "I:"},
	    {U'Ð', "D-"},
	    {U'Ñ', "N~"},
	    {U'Ò', "O`"},
	    {U'Ó', "O'"},
	    {U'Ô', "O^"},
	    {U'Õ', "O~"},
	    {U'Ö', "O:"},
	    {U'Ø', "O/"},
	    {U'Ù', "U`"},
	    {U'Ú', "U'"},
	    {U'Û', "U^"},
	    {U'Ü', "U:"},
	    {U'Ý', "Y'"},
	    {U'ß', "ss"},
	    {U'à', "a`"},
	    {U'á', "a'"},
	    {U'â', "a^"},
	    {U'ã', "a~"},
	    {U'ä', "a:"},
	    {U'å', "ao"},
	    {U'æ', "ae"},
	    {U'ç', "c,"},
	    {U'è', "e`"},
	    {U'é', "e'"},
	    {U'ê', "e^"},
	    {U'ë', "e:"},
	    {U'ì', "i`"},
	    {U'í', "i'"},
	    {U'î', "i^"},
	    {U'ï', "i:"},
	    {U'ð', "d-"},
	    {U'ñ', "n~"},
	    {U'ò', "o`"},
	    {U'ó', "o'"},
	    {U'ô', "o^"},
	    {U'õ', "o~"},
	    {U'ö', "o:"},
	    {U'ø', "o/"},
	    {U'ù', "u`"},
	    {U'ú', "u'"},
	    {U'û', "u^"},
	    {U'ü', "u:"},
	    {U'ý', "y'"},
	    {U'ÿ', "y:"},
	};

	Logger logger_;
	const Options options_;

	AsciiToUnicodeMap toChars_ = {
	    {"S^", "Š"},
	    {"OE", "Œ"},
	    {"-", "–"},
	    {"--", "—"},
	    {"TM", "™"},
	    {"s^", "š"},
	    {"oe", "œ"},
	    {"Y:", "Ÿ"},
	    {"(C)", "©"},
	    {"(R)", "®"},
	    {"A`", "À"},
	    {"A'", "Á"},
	    {"A^", "Â"},
	    {"A~", "Ã"},
	    {"A:", "Ä"},
	    {"Ao", "Å"},
	    {"AE", "Æ"},
	    {"C,", "Ç"},
	    {"E`", "È"},
	    {"E'", "É"},
	    {"E^", "Ê"},
	    {"E:", "Ë"},
	    {"I`", "Ì"},
	    {"I'", "Í"},
	    {"I^", "Î"},
	    {"I:", "Ï"},
	    {"D-", "Ð"},
	    {"N~", "Ñ"},
	    {"O`", "Ò"},
	    {"O'", "Ó"},
	    {"O^", "Ô"},
	    {"O~", "Õ"},
	    {"O:", "Ö"},
	    {"O/", "Ø"},
	    {"U`", "Ù"},
	    {"U'", "Ú"},
	    {"U^", "Û"},
	    {"U:", "Ü"},
	    {"Y'", "Ý"},
	    {"ss", "ß"},
	    {"a`", "à"},
	    {"a'", "á"},
	    {"a^", "â"},
	    {"a~", "ã"},
	    {"a:", "ä"},
	    {"ao", "å"},
	    {"ae", "æ"},
	    {"c,", "ç"},
	    {"e`", "è"},
	    {"e'", "é"},
	    {"e^", "ê"},
	    {"e:", "ë"},
	    {"i`", "ì"},
	    {"i'", "í"},
	    {"i^", "î"},
	    {"i:", "ï"},
	    {"d-", "ð"},
	    {"n~", "ñ"},
	    {"o`", "ò"},
	    {"o'", "ó"},
	    {"o^", "ô"},
	    {"o~", "õ"},
	    {"o:", "ö"},
	    {"o/", "ø"},
	    {"u`", "ù"},
	    {"u'", "ú"},
	    {"u^", "û"},
	    {"u:", "ü"},
	    {"y'", "ý"},
	    {"y:", "ÿ"},
	};

	int keystream(
	    std::string key, std::size_t keyLength, std::size_t line, bool isText) const;
	char toPrintable(int c) const;
};

class Parser {
public:
	explicit Parser(const Logger logger, const Options options = {});
	std::shared_ptr<Document> parse(std::istream& in);
	std::shared_ptr<Document> parseFile(const std::string& filename);
	void reset();

private:
	using NodeMap = std::map<const int, Node*>;
	using DataCallback = std::function<void(std::string)>;

	struct NodeRange {
		Node& node;
		int min = 0;
		int max = 0;

		NodeRange(Node& node, const int min, const int max);
	};

	struct NodeRangeList {
		std::vector<NodeRange> data = {};

		Node* find(const int min, const int max);
		void add(Node& node, const int min, const int max);
		void clear();
	};

	struct LinkData {
		Element& link;
		int line = 0;
		int column = 0;

		LinkData(Element& link, const int line, const int column);
	};

	struct VisibilityData {
		int targetLine = 0;
		VisibilityType visibility;

		VisibilityData(int targetLine, VisibilityType visibility);
	};

	struct DataHandler {
		int line = 0;
		int column = 0;
		std::size_t offset = 0;
		std::size_t length = 0;
		DataCallback func;

		DataHandler(int line, int column, std::size_t offset, std::size_t length,
		    DataCallback func);
	};

	static const auto FormatTokenLength = 3;
	static const auto HeaderLength = 4;

	// Fix for bad 88a backwards-compatibility header found in 401 files
	static constexpr auto BadHeaderTitle = "Why aren't there any more hints here?";
	static const auto BadHeaderFirstChildLine = HeaderLength + 16;
	static const auto GoodHeaderFirstChildLine = HeaderLength + 15;

	Codec codec_;
	std::unique_ptr<CRC> crc_ = nullptr;
	std::vector<DataHandler> dataHandlers_;
	std::vector<LinkData> deferredLinks_;
	std::vector<VisibilityData> deferredVisibilities_;
	std::shared_ptr<Document> document_ = nullptr;
	bool done_ = false;
	bool isTitleSet_ = false;
	std::string key_;
	int lineOffset_ = 0;
	Logger logger_;
	const Options options_;
	NodeRangeList parents_;
	std::unique_ptr<Tokenizer> tokenizer_ = nullptr;

	// 88a
	void parse88a();
	void parse88aElements(int firstHintTextLine, NodeMap& parents);
	void parse88aTextNodes(int lastHintTextLine, NodeMap& parents);
	void parse88aCredits(std::unique_ptr<const Token> token);
	void parseHeaderSep(std::unique_ptr<const Token> token);

	// 96a
	void parse96a();
	std::shared_ptr<Element> parseElement(std::unique_ptr<const Token> token);
	void parseCommentElement(Element& element);
	void parseDataElement(Element& element);
	void parseHintElement(Element& element);
	void parseHyperpngElement(Element& element);
	void parseIncentiveElement(Element& element);
	void parseInfoElement(Element& element);
	void parseLinkElement(Element& element);
	void parseOverlayElement(Element& element);
	void parseSubjectElement(Element& element);
	void parseTextElement(Element& element);
	void parseVersionElement(Element& element);

	// Parse helpers
	std::unique_ptr<const Token> next();
	std::unique_ptr<const Token> expect(TokenType expected);
	void addDataCallback(
	    int line, int column, std::size_t offset, std::size_t length, DataCallback func);
	void appendText(std::string& text, TextFormat& format, ContainerNode& node);
	void checkCRC();
	Node* findParent(ContainerNode& node);
	int fixHeaderFirstChildLine(std::string title, int firstChildLine) const;
	void addNodeToParentIndex(ContainerNode& node);
	int offsetLine(int line);
	void parseData(std::unique_ptr<const Token> token);
	void parseDate(const std::string& date, std::tm& tm) const;
	void parseTime(const std::string& time, std::tm& tm) const;
	void parseWithFormat(const std::string& text, TextFormat& format, ContainerNode& node,
	    ElementType elementType);
	void processLinks();
	void processVisibility();
};

class Writer {
public:
	Writer(const Logger logger, std::ostream& out, const Options options = {});
	virtual ~Writer() = default;
	virtual void write(const std::shared_ptr<Document> document) = 0;
	virtual void reset();

protected:
	Codec codec_;
	Logger logger_;
	std::ostream& out_;
	const Options options_;
};

class TreeWriter : public Writer {
public:
	TreeWriter(const Logger logger, std::ostream& out, const Options options = {});
	void write(const std::shared_ptr<Document> document) override;

private:
	void draw(const Document& document);
	void draw(const Element& element);
	void draw(const GroupNode& groupNode);
	void draw(const TextNode& textNode);
	void drawScaffold(const Node& node);
};

class HTMLWriter : public Writer {
public:
	HTMLWriter(const Logger logger, std::ostream& out, const Options options = {});
	void write(const std::shared_ptr<Document> document) override;

private:
	using NodeMap = tsl::hopscotch_map<const Node*, const pugi::xml_node>;

	class Serializer {
	public:
		using Func = void (HTMLWriter::*)(const Element& element, pugi::xml_node xmlNode);

		Serializer();
		void invoke(HTMLWriter& writer, const Element& element, pugi::xml_node xmlNode);

	private:
		std::map<const ElementType, Func> map_;
	};

	void serialize(const Document& document, pugi::xml_document& xml);
	void serializeDocument(const Document& document, pugi::xml_node root) const;
	void serializeElement(const Element& element, pugi::xml_node xmlNode);
	void serializeBlankElement(const Element& element, pugi::xml_node xmlNode);
	void serializeCommentElement(const Element& element, pugi::xml_node xmlNode);
	void serializeDataElement(const Element& element, pugi::xml_node xmlNode);
	void serializeGifaElement(const Element& element, pugi::xml_node xmlNode);
	void serializeHintElement(const Element& element, pugi::xml_node xmlNode);
	void serializeHyperpngElement(const Element& element, pugi::xml_node xmlNode);
	void serializeIncentiveElement(const Element& element, pugi::xml_node xmlNode);
	void serializeInfoElement(const Element& element, pugi::xml_node xmlNode);
	void serializeLinkElement(const Element& element, pugi::xml_node xmlNode);
	void serializeOverlayElement(const Element& element, pugi::xml_node xmlNode);
	void serializeSoundElement(const Element& element, pugi::xml_node xmlNode);
	void serializeSubjectElement(const Element& element, pugi::xml_node xmlNode);
	void serializeTextElement(const Element& element, pugi::xml_node xmlNode);
	void serializeTextNode(const TextNode& textNode, pugi::xml_node xmlNode) const;
	std::shared_ptr<Document> addEntryPointTo88aDocument(const Document& document) const;
	std::optional<pugi::xml_node> appendBody(
	    const Element& element, pugi::xml_node xmlNode) const;
	void appendClassNames(
	    pugi::xml_node xmlNode, std::vector<std::string> classNames) const;
	pugi::xml_node appendTitle(const Element& element, pugi::xml_node xmlNode) const;
	pugi::xml_node appendMedia(const Element& element, pugi::xml_node xmlNode) const;
	void appendVisibility(const Traits::Visibility& node, pugi::xml_node xmlNode) const;
	pugi::xml_node createHTMLDocument(
	    const Document& document, pugi::xml_document& xml) const;
	std::optional<pugi::xml_node> findHyperpngContainer(
	    const Element& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findOrCreateMap(const Element& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findXMLParent(const Node& node, const pugi::xml_node parent,
	    const NodeMap parents, const int depth) const;
	std::string getDataURI(const std::string& contentType, const std::string& data) const;
	std::tuple<int, int> getImageSize(const Element& element) const;
	Element* getParentElement(const Element& element) const;
	std::tuple<int, int, int, int> getRegionCoordinates(const Element& element) const;
	void populateArea(const Element& element, pugi::xml_node area) const;

	static HTMLWriter::Serializer serializer_;

	std::string css_;
	std::string js_;
	std::map<const ElementType, std::string> mediaContentTypes_;
	std::map<const ElementType, std::string> mediaTagTypes_;
};

class JSONWriter : public Writer {
public:
	JSONWriter(const Logger logger, std::ostream& out, const Options options = {});
	void write(const std::shared_ptr<Document> document) override;

private:
	void serialize(const Document& document, Json::Value& root) const;
	void serializeDocument(const Document& document, Json::Value& object) const;
	void serializeElement(const Element& element, Json::Value& object) const;
	void serializeTextNode(const TextNode& textNode, Json::Value& object) const;
	void serializeMap(const Traits::Attributes::Type& attrs, Json::Value& object) const;
};

class UHSWriter : public Writer {
public:
	// The official readers work with lines longer than 76 characters, but
	// lines were capped at 76 so that DOS readers would display correctly
	// within the standard 80-character window (with border and padding).
	static const std::size_t LineLength = 76;

	UHSWriter(const Logger logger, std::ostream& out, const Options options = {});
	void write(const std::shared_ptr<Document> document) override;
	void reset() override;

private:
	class Serializer {
	public:
		using Func = int (UHSWriter::*)(Element&, std::string&);

		Serializer();
		int invoke(UHSWriter& writer, Element& element, std::string& out);

	private:
		std::map<const ElementType, Func> map_;
	};

	using DataQueue = std::queue<std::pair<ElementType, const std::string>>;

	static const std::size_t InitialBufferLength = 204'800; // 200 KiB
	static const std::size_t MediaSizeLength = 6;  // Up to 999,999 bytes per media file
	static const std::size_t RegionSizeLength = 4; // Up to 9,999 × 9,999 px per region
	static const std::size_t FileSizeLength = 7;   // Up to 9,999,999 bytes per document
	static const auto InitialElementLength = 2;    // Element descriptor and title
	static constexpr auto DataAddressMarker = "000000";
	static constexpr auto InfoLengthMarker = "length=0000000";
	static constexpr auto LinkMarker = "** LINK **";

	void serialize88a(const Document& document, std::string& out);
	void serialize96a(std::string& out);
	int serializeElement(Element& element, std::string& out);
	int serializeBlankElement(Element& element, std::string& out);
	int serializeCommentElement(Element& element, std::string& out);
	int serializeDataElement(Element& element, std::string& out);
	int serializeHintChild(Node& node, Element& parentElement,
	    std::map<const int, TextFormat>& formats, std::string& textBuffer,
	    std::string& out);
	int serializeHintElement(Element& element, std::string& out);
	int serializeHyperpngElement(Element& element, std::string& out);
	int serializeIncentiveElement(Element& element, std::string& out);
	int serializeInfoElement(Element& element, std::string& out);
	int serializeLinkElement(Element& element, std::string& out);
	int serializeOverlayElement(Element& element, std::string& out);
	int serializeSubjectElement(Element& element, std::string& out);
	int serializeTextElement(Element& element, std::string& out);
	void serializeElementHeader(Element& element, std::string& out) const;
	void updateLinkTargets(std::string& out) const;
	void serializeData(std::string& out);
	void serializeCRC(std::string& out);
	std::string formatText(const TextNode& textNode, const TextFormat previousFormat);
	std::string encodeText(const std::string text, const ElementType parentType);
	void addData(const Element& element);
	std::string createDataAddress(
	    std::size_t bodyLength, std::string textFormat = "") const;
	std::string createOverlayAddress(std::size_t bodyLength, int x, int y) const;
	std::string createRegion(int x1, int y1, int x2, int y2) const;
	void convertTo96a();

	static UHSWriter::Serializer serializer_;

	Codec codec_;
	CRC crc_;
	int currentLine_ = 1;
	std::shared_ptr<Document> document_ = nullptr;
	std::string key_;
	DataQueue data_;
	std::vector<Node*> deferredLinks_;
};

} // namespace UHS

#endif

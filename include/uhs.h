#if !defined UHS_H

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
#include "httplib.h"
#include "json.h"
extern "C" {
#include "puff.h"
}
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

class Error : public std::runtime_error {
public:
	Error();
	Error(std::string const& message);
	Error(char const* message);

	template<typename... Args>
	Error(char const* format, Args... args) : Error() {
		auto message = tfm::format(format, args...);
		// TODO: Review for slice
		static_cast<std::runtime_error&>(*this) = std::runtime_error(message);
	}

	std::string const string() const;
};

std::ostream& operator<<(std::ostream& out, Error const& err);

class HTTPError : public Error {
public:
	template<typename... Args>
	HTTPError(httplib::Result const& res, char const* format, Args... args)
	    : Error(), response_{res.value()} {

		// TODO: Review for slice
		static_cast<Error&>(*this) = Error(this->format(format, args...));
	}

	httplib::Response const getResponse() const;

private:
	template<typename... Args>
	std::string const format(char const* format, Args... args) {
		auto fmt = "HTTP %d: "s + format;
		return tfm::format(fmt.data(), response_.status, args...);
	}

	httplib::Response response_;
};

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

	ParseError(int line, int column, std::string const& message);
	ParseError(int line, int column, char const* message);

	template<typename... Args>
	ParseError(int line, int column, char const* format, Args... args) : Error() {
		// TODO: Review for slice
		static_cast<Error&>(*this) = Error(this->format(format, line, column, args...));
	}

private:
	template<typename... Args>
	std::string const format(char const* format, int line, int column, Args... args) {
		auto fmt = "parse error at line %d, column %d: "s + format;
		return tfm::format(fmt.data(), line, column, args...);
	}
};

class WriteError : public Error {
	using Error::Error;
};

class ZipError : public Error {
	using Error::Error;
};

class Logger {
public:
	Logger(LogLevel level = LogLevel::Error) : level_{level} {}
	void level(LogLevel level) { level_ = level; }

	// Error
	void error(char const* message) const { this->error("%s", message); }
	void error(Error const& err) const { this->error("%s", err.string().c_str()); }
	template<typename... Args>
	void error(char const* format, Args... args) const {
		if (level_ != LogLevel::None) {
			this->log(std::cout, "error: ", format, args...);
		}
	}

	// Warn
	void warn(char const* message) const { this->warn("%s", message); }
	void warn(Error const& err) const { this->warn("%s", err.string().c_str()); }
	template<typename... Args>
	void warn(char const* format, Args... args) const {
		if (level_ == LogLevel::Warn || level_ == LogLevel::Info
		    || level_ == LogLevel::Debug) {

			this->log(std::cout, "warning: ", format, args...);
		}
	}

	// Info
	void info(char const* message) const { this->info("%s", message); }
	template<typename... Args>
	void info(char const* format, Args... args) const {
		if (level_ == LogLevel::Info || level_ == LogLevel::Debug) {
			this->log(std::cout, "info: ", format, args...);
		}
	}

	// Debug
	void debug(char const* message) const { this->debug("%s", message); }
	template<typename... Args>
	void debug(char const* format, Args... args) const {
		if (level_ == LogLevel::Debug) {
			this->log(std::cout, "debug: ", format, args...);
		}
	}

private:
	LogLevel level_;

	template<typename... Args>
	void log(std::ostream& ostream, char const* prefix, char const* format,
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

static char const AsciiStart = 0x20;
static char const AsciiEnd = 0x7F;

bool beginsWithAttachedPunctuation(std::string const& s);
std::string& chomp(std::string& s, char c);
bool endsWithAttachedPunctuation(std::string const& s);
bool endsWithFinalPunctuation(std::string const& s);
std::string const hex(std::string const& s);
std::string const hex(char s);
bool isInt(std::string const& s);
bool isPrintable(int c);
std::string join(std::vector<std::string> const& s, std::string const& sep);
std::string ltrim(std::string const& s, char c);
std::string rtrim(std::string const& s, char c);
std::vector<std::string> split(std::string const& s, std::string const& sep, int n = 0);
std::string toBase64(std::string const& s);
int toInt(std::string const& s);
void toLower(std::string& s);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width,
    int& numLines, std::string const prefix = "");

} // namespace Strings

namespace Regex {

std::regex const Descriptor{"([0-9]+) ([a-z]{4,})"};
std::regex const DataAddress{"0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})"};
std::regex const EmailAddress{".+@.+\\..+"};
std::regex const HyperpngRegion{
    "(-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
std::regex const OverlayAddress{
    "0{6} ([0-9]{6,}) ([0-9]{6,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
std::regex const URL{"https?://.+\\..+"};

} // namespace Regex

namespace Traits {

class Attributes {
public:
	using Type = std::map<std::string, std::string>;

	Type const& attrs() const;
	std::optional<std::string const> attr(std::string const key) const;
	void attr(std::string const key, std::string const value);
	void attr(std::string const key, int const value);

private:
	Type attrs_;
};

class Body {
public:
	Body() = default;
	explicit Body(std::string const body);
	explicit Body(int const body);
	std::string const& body() const;
	void body(std::string const body);
	void body(int const body);

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
	explicit Title(std::string const title);
	std::string const& title() const;
	void title(std::string const title);

private:
	std::string title_;
};

class Visibility {
public:
	VisibilityType visibility() const;
	void visibility(VisibilityType visibility);
	std::string const visibilityString() const;

private:
	VisibilityType visibility_ = VisibilityType::All;
};

} // namespace Traits

class Pipe {
public:
	using Handler = std::function<void(char const*, std::streamsize n)>;

	explicit Pipe(std::istream& in);
	void addHandler(Handler func);
	std::exception_ptr error();
	void read();
	bool good();
	bool eof();

private:
	static std::size_t const ReadLength = 1024;

	std::istream& in_;
	std::size_t offset_ = 0;
	std::vector<Handler> handlers_;
	std::exception_ptr err_ = nullptr;
};

class CRC {
public:
	static auto const ByteLength = 2;

	CRC();
	void upstream(Pipe& pipe);
	void calculate(char const* buffer, std::streamsize length, bool bufferChecksum);
	void calculate(char const* buffer, std::streamsize length);
	uint16_t result();
	void result(std::vector<char>& out);
	bool valid();
	void reset();

private:
	static auto const TableLength = 256;
	static uint16_t const Polynomial = 0x8005;
	static uint16_t const CastMask = 0xFFFF;
	static uint16_t const MSBMask = 0x8000;
	static uint16_t const FinalXOR = 0x0100;

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
	static auto const DataSep = '\x1A';
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

	static std::string const typeString(TokenType t);

	Token(TokenType const tokenType, std::size_t offset = 0, int line = 0,
	    std::size_t column = 0, std::string value = "");
	friend std::ostream& operator<<(std::ostream& out, Token const& t);
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	std::string const& value() const;
	std::string const typeString() const;
	std::string const string() const;

private:
	class TypeMap {
	public:
		TypeMap();
		std::string const findByType(TokenType const type) const;

	private:
		std::map<TokenType const, std::string const> map_;
	};

	static Token::TypeMap typeMap_;

	TokenType const type_;
	int line_ = 0;
	std::size_t column_ = 0;
	std::size_t offset_ = 0;
	std::string value_;

	std::string const formatToken() const;
	std::string const formatIntValue() const;
	std::string const formatStringValue() const;
	std::string const formatByteValue() const;
};

class Tokenizer {
public:
	explicit Tokenizer(Pipe& pipe);
	void tokenize(char const* buffer, std::streamsize length);
	bool hasNext();
	std::unique_ptr<Token const> next();

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& pipe);
		void send(Token const&& token);
		std::unique_ptr<Token const> receive();
		bool empty() const;
		bool ok() const;
		void close();

	private:
		std::queue<Token const> queue_;
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
	ElementType tokenizeDescriptor(std::smatch const& matches);
	void tokenizeDataAddress(std::smatch const& matches);
	void tokenizeHyperpngRegion(std::smatch const& matches);
	void tokenizeOverlayAddress(std::smatch const& matches);
	void tokenizeMatches(
	    std::smatch const& matches, std::vector<TokenType> const&& tokens);
	void tokenizeData(std::string const& data, std::size_t column);
	void tokenizeCRC(std::string const& crc, std::size_t column);
	void tokenizeEOF(std::size_t column);
};

template<typename T>
class NodeIterator;

class Document;

class Node {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<Node const>;

	static std::string const typeString(NodeType type);
	static bool isElementOfType(Node const& node, ElementType type);

	explicit Node(NodeType type);
	Node(Node const& other);
	Node& operator=(Node other);
	virtual ~Node() = default;
	friend void swap(Node& lhs, Node& rhs) noexcept;
	NodeType nodeType() const;
	std::string const nodeTypeString() const;
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
	void cloneChildren(Node const& node);
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
	ContainerNode(ContainerNode const& other);
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
	bool operator==(NodeIterator<T> const& rhs) const;
	bool operator!=(NodeIterator<T> const& rhs) const;

private:
	pointer initial_ = nullptr;
	pointer current_ = nullptr;
	bool visited_ = false;
};

class BreakNode : public Node {
public:
	static std::shared_ptr<BreakNode> create();
	BreakNode();
	BreakNode(BreakNode const&);
	BreakNode& operator=(BreakNode);
	std::shared_ptr<BreakNode> clone() const;
};

class GroupNode : public ContainerNode {
public:
	static std::shared_ptr<GroupNode> create(int line, int length);
	GroupNode(int line, int length);
	GroupNode(GroupNode const&);
	GroupNode& operator=(GroupNode);
	friend void swap(GroupNode& lhs, GroupNode& rhs) noexcept;
	std::shared_ptr<GroupNode> clone() const;
};

class TextNode
    : public Node
    , public Traits::Body {
public:
	static std::shared_ptr<TextNode> create(std::string const body);
	static std::shared_ptr<TextNode> create(std::string const body, TextFormat format);

	explicit TextNode(std::string const body);
	TextNode(std::string const body, TextFormat format);
	TextNode(TextNode const& other);
	TextNode& operator=(TextNode other);
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;
	std::shared_ptr<TextNode> clone() const;
	std::string const& string() const;
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
	static std::shared_ptr<Element> create(ElementType type, int const id = 0);
	static ElementType elementType(std::string const& typeString);
	static std::string const typeString(ElementType type);

	Element(ElementType type, int id = 0);
	Element(Element const& other);
	Element& operator=(Element other);
	friend void swap(Element& lhs, Element& rhs) noexcept;
	std::shared_ptr<Element> clone() const;
	ElementType elementType() const;
	std::string const elementTypeString() const;
	int id() const;
	bool isMedia() const;
	std::string const mediaExt() const;

private:
	class TypeMap {
	public:
		TypeMap();
		std::string const findByType(ElementType const type) const;
		ElementType findByString(std::string const& string) const;

	private:
		std::map<ElementType const, std::string const> byType_;
		std::map<std::string const, ElementType const> byString_;
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
	Document(Document const& other);
	Document& operator=(Document other);
	friend void swap(Document& lhs, Document& rhs) noexcept;
	Node* find(int const id);
	std::shared_ptr<Document> clone() const;
	void version(VersionType v);
	VersionType version() const;
	bool isVersion(VersionType v) const;
	std::string const versionString() const;
	void validChecksum(bool value);
	bool validChecksum() const;
	void elementRemoved(Element& element);
	void elementAdded(Element& element);
	void reindex();

private:
	VersionType version_;
	bool validChecksum_ = false;
	std::map<int const, Node*> index_;
	bool indexed_ = true;
};

class Codec {
public:
	Codec(Options const options = {});
	std::string const createKey(std::string secret) const;
	std::string const decode88a(std::string encoded) const;
	std::string const decode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	std::string const decodeSpecialChars(std::string const& encoded) const;
	std::string const encode88a(std::string decoded) const;
	std::string const encode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	std::string const encodeSpecialChars(std::string const& decoded) const;

private:
	using UnicodeToAsciiMap = std::map<char32_t const, std::string const> const;
	using AsciiToUnicodeMap = std::map<std::string const, std::string const> const;

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

	Logger const logger_;
	Options const options_;

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
	explicit Parser(Logger const logger, Options const options = {});
	std::shared_ptr<Document> parse(std::istream& in);
	std::shared_ptr<Document> parseFile(std::string const& filename);
	void reset();

private:
	using NodeMap = std::map<int const, Node*>;
	using DataCallback = std::function<void(std::string)>;

	struct NodeRange {
		Node& node;
		int min = 0;
		int max = 0;

		NodeRange(Node& node, int const min, int const max);
	};

	struct NodeRangeList {
		std::vector<NodeRange> data = {};

		Node* find(int const min, int const max);
		void add(Node& node, int const min, int const max);
		void clear();
	};

	struct LinkData {
		Element& link;
		int line = 0;
		int column = 0;

		LinkData(Element& link, int const line, int const column);
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

	static auto const FormatTokenLength = 3;
	static auto const HeaderLength = 4;

	// Fix for bad 88a backwards-compatibility header found in 401 files
	static constexpr auto BadHeaderTitle = "Why aren't there any more hints here?";
	static auto const BadHeaderFirstChildLine = HeaderLength + 16;
	static auto const GoodHeaderFirstChildLine = HeaderLength + 15;

	Codec const codec_;
	std::unique_ptr<CRC> crc_ = nullptr;
	std::vector<DataHandler> dataHandlers_;
	std::vector<LinkData> deferredLinks_;
	std::vector<VisibilityData> deferredVisibilities_;
	std::shared_ptr<Document> document_ = nullptr;
	bool done_ = false;
	bool isTitleSet_ = false;
	std::string key_;
	int lineOffset_ = 0;
	Logger const logger_;
	Options const options_;
	NodeRangeList parents_;
	std::unique_ptr<Tokenizer> tokenizer_ = nullptr;

	// 88a
	void parse88a();
	void parse88aElements(int firstHintTextLine, NodeMap& parents);
	void parse88aTextNodes(int lastHintTextLine, NodeMap& parents);
	void parse88aCredits(std::unique_ptr<Token const> token);
	void parseHeaderSep(std::unique_ptr<Token const> token);

	// 96a
	void parse96a();
	std::shared_ptr<Element> parseElement(std::unique_ptr<Token const> token);
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
	std::unique_ptr<Token const> next();
	std::unique_ptr<Token const> expect(TokenType expected);
	void addDataCallback(
	    int line, int column, std::size_t offset, std::size_t length, DataCallback func);
	void appendText(std::string& text, TextFormat& format, ContainerNode& node);
	void checkCRC();
	Node* findParent(ContainerNode& node);
	int fixHeaderFirstChildLine(std::string title, int firstChildLine) const;
	void addNodeToParentIndex(ContainerNode& node);
	int offsetLine(int line);
	void parseData(std::unique_ptr<Token const> token);
	void parseDate(std::string const& date, std::tm& tm) const;
	void parseTime(std::string const& time, std::tm& tm) const;
	void parseWithFormat(std::string const& text, TextFormat& format, ContainerNode& node,
	    ElementType elementType);
	void processLinks();
	void processVisibility();
};

class Writer {
public:
	Writer(Logger const logger, std::ostream& out, Options const options = {});
	virtual ~Writer() = default;
	virtual void write(std::shared_ptr<Document> const document) = 0;
	virtual void reset();

protected:
	Codec const codec_;
	Logger const logger_;
	std::ostream& out_;
	Options const options_;
};

class TreeWriter : public Writer {
public:
	TreeWriter(Logger const logger, std::ostream& out, Options const options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	void draw(Document const& document);
	void draw(Element const& element);
	void draw(GroupNode const& groupNode);
	void draw(TextNode const& textNode);
	void drawScaffold(Node const& node);
};

class HTMLWriter : public Writer {
public:
	HTMLWriter(Logger const logger, std::ostream& out, Options const options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	using NodeMap = tsl::hopscotch_map<Node const*, pugi::xml_node const>;

	class Serializer {
	public:
		using Func = void (HTMLWriter::*)(Element const& element, pugi::xml_node xmlNode);

		Serializer();
		void invoke(HTMLWriter& writer, Element const& element, pugi::xml_node xmlNode);

	private:
		std::map<ElementType const, Func> map_;
	};

	void serialize(Document const& document, pugi::xml_document& xml);
	void serializeDocument(Document const& document, pugi::xml_node root) const;
	void serializeElement(Element const& element, pugi::xml_node xmlNode);
	void serializeBlankElement(Element const& element, pugi::xml_node xmlNode);
	void serializeCommentElement(Element const& element, pugi::xml_node xmlNode);
	void serializeDataElement(Element const& element, pugi::xml_node xmlNode);
	void serializeGifaElement(Element const& element, pugi::xml_node xmlNode);
	void serializeHintElement(Element const& element, pugi::xml_node xmlNode);
	void serializeHyperpngElement(Element const& element, pugi::xml_node xmlNode);
	void serializeIncentiveElement(Element const& element, pugi::xml_node xmlNode);
	void serializeInfoElement(Element const& element, pugi::xml_node xmlNode);
	void serializeLinkElement(Element const& element, pugi::xml_node xmlNode);
	void serializeOverlayElement(Element const& element, pugi::xml_node xmlNode);
	void serializeSoundElement(Element const& element, pugi::xml_node xmlNode);
	void serializeSubjectElement(Element const& element, pugi::xml_node xmlNode);
	void serializeTextElement(Element const& element, pugi::xml_node xmlNode);
	void serializeTextNode(TextNode const& textNode, pugi::xml_node xmlNode) const;
	std::shared_ptr<Document> addEntryPointTo88aDocument(Document const& document) const;
	std::optional<pugi::xml_node> appendBody(
	    Element const& element, pugi::xml_node xmlNode) const;
	void appendClassNames(
	    pugi::xml_node xmlNode, std::vector<std::string> classNames) const;
	pugi::xml_node appendTitle(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node appendMedia(Element const& element, pugi::xml_node xmlNode) const;
	void appendVisibility(Traits::Visibility const& node, pugi::xml_node xmlNode) const;
	pugi::xml_node createHTMLDocument(
	    Document const& document, pugi::xml_document& xml) const;
	std::optional<pugi::xml_node> findHyperpngContainer(
	    Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findOrCreateMap(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findXMLParent(Node const& node, pugi::xml_node const parent,
	    NodeMap const parents, int const depth) const;
	std::string getDataURI(std::string const& contentType, std::string const& data) const;
	std::tuple<int, int> getImageSize(Element const& element) const;
	Element* getParentElement(Element const& element) const;
	std::tuple<int, int, int, int> getRegionCoordinates(Element const& element) const;
	void populateArea(Element const& element, pugi::xml_node area) const;

	static HTMLWriter::Serializer serializer_;

	std::string css_;
	std::string js_;
	std::map<ElementType const, std::string> mediaContentTypes_;
	std::map<ElementType const, std::string> mediaTagTypes_;
};

class JSONWriter : public Writer {
public:
	JSONWriter(Logger const logger, std::ostream& out, Options const options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	void serialize(Document const& document, Json::Value& root) const;
	void serializeDocument(Document const& document, Json::Value& object) const;
	void serializeElement(Element const& element, Json::Value& object) const;
	void serializeTextNode(TextNode const& textNode, Json::Value& object) const;
	void serializeMap(Traits::Attributes::Type const& attrs, Json::Value& object) const;
};

class UHSWriter : public Writer {
public:
	// The official readers work with lines longer than 76 characters, but
	// lines were capped at 76 so that DOS readers would display correctly
	// within the standard 80-character window (with border and padding).
	static std::size_t const LineLength = 76;

	UHSWriter(Logger const logger, std::ostream& out, Options const options = {});
	void write(std::shared_ptr<Document> const document) override;
	void reset() override;

private:
	class Serializer {
	public:
		using Func = int (UHSWriter::*)(Element&, std::string&);

		Serializer();
		int invoke(UHSWriter& writer, Element& element, std::string& out);

	private:
		std::map<ElementType const, Func> map_;
	};

	using DataQueue = std::queue<std::pair<ElementType, std::string const>>;

	static std::size_t const InitialBufferLength = 204'800; // 200 KiB
	static std::size_t const MediaSizeLength = 6;  // Up to 999,999 bytes per media file
	static std::size_t const RegionSizeLength = 4; // Up to 9,999 × 9,999 px per region
	static std::size_t const FileSizeLength = 7;   // Up to 9,999,999 bytes per document
	static auto const InitialElementLength = 2;    // Element descriptor and title
	static constexpr auto DataAddressMarker = "000000";
	static constexpr auto InfoLengthMarker = "length=0000000";
	static constexpr auto LinkMarker = "** LINK **";

	void serialize88a(Document const& document, std::string& out);
	void serialize96a(std::string& out);
	int serializeElement(Element& element, std::string& out);
	int serializeBlankElement(Element& element, std::string& out);
	int serializeCommentElement(Element& element, std::string& out);
	int serializeDataElement(Element& element, std::string& out);
	int serializeHintChild(Node& node, Element& parentElement,
	    std::map<int const, TextFormat>& formats, std::string& textBuffer,
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
	std::string formatText(TextNode const& textNode, TextFormat const previousFormat);
	std::string encodeText(std::string const text, ElementType const parentType);
	void addData(Element const& element);
	std::string createDataAddress(
	    std::size_t bodyLength, std::string textFormat = "") const;
	std::string createOverlayAddress(std::size_t bodyLength, int x, int y) const;
	std::string createRegion(int x1, int y1, int x2, int y2) const;
	void convertTo96a();

	static UHSWriter::Serializer serializer_;

	Codec const codec_;
	CRC crc_;
	int currentLine_ = 1;
	std::shared_ptr<Document> document_ = nullptr;
	std::string key_;
	DataQueue data_;
	std::vector<Node*> deferredLinks_;
};

class Downloader {
public:
	struct FileMetadata {
		int compressedSize;                   // fsize
		std::string filename;                 // fname
		std::vector<std::string> otherTitles; // falttitle
		std::vector<std::string> relatedKeys; // frelated
		std::string title;                    // ftitle
		int uncompressedSize;                 // ffullsize
		std::string url;                      // furl
	};

	using FileIndex = std::map<std::string const, FileMetadata const>;

	Downloader(Logger const logger, Options const options = {});
	void download(std::string const& file, std::string const& dir);
	void download(std::vector<std::string const> const& files, std::string const& dir);
	FileIndex const& fileIndex();

private:
	static constexpr auto FileIndexPath = "/cgi-bin/update.cgi";
	static constexpr auto Host = "www.invisiclues.com";
	static constexpr auto Protocol = "http";

	void loadFileIndex();

	FileIndex fileIndex_;
	httplib::Client httpClient_;
	httplib::Headers httpHeaders_;
	Logger const logger_;
	Options const options_;
};

class Zip {
public:
	Zip(std::string const& data);
	bool isZip();
	void unzip(std::string const& dir);

private:
	static int const CompressedSizeOffset = 18;
	static int const ExtraFieldLengthOffset = 28;
	static int const FilenameLengthOffset = 26;
	static int const FilenameOffset = 30;
	static uint32_t const Signature = 0x04034b50;
	static int const SignatureOffset = 0;
	static int const UncompressedSizeOffset = 22;

	uint16_t readUint16LE(int offset);
	uint32_t readUint32LE(int offset);

	std::string const data_;
};

bool write(Logger const logger, std::string const format, std::string const infile,
    std::string const outfile = "", Options const options = {});

} // namespace UHS

extern "C" {
#include "uhs_bridge.h"
}

#endif

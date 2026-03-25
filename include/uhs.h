#if !defined UHS_H

#define UHS_H
#define UHS_VERSION "1.0.0-alpha"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
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

#include "httplib.h"
#include "json/json.h"
extern "C" {
#include "puff.h"
}
#include "pugixml.hpp"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
#include "tinyformat.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "tinyutf8/tinyutf8.h"
#include "tsl/hopscotch_map.h"

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
	httplib::Response response_;

	template<typename... Args>
	std::string const format(char const* format, Args... args) {
		auto fmt = "HTTP %d: "s + format;
		return tfm::format(fmt.data(), response_.status, args...);
	}
};

class DataError : public Error {
	using Error::Error;
};

class FileError : public Error {
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

	ParseError(int line, int column, std::string const& message);
	ParseError(int line, int column, char const* message);

	template<typename... Args>
	ParseError(int line, int column, char const* format, Args... args) : Error() {
		// TODO: Review for slice
		static_cast<Error&>(*this) = Error(this->format(format, line, column, args...));
	}

	static ParseError badLine(int line, int column, int targetLine);
	static ParseError badToken(int line, int column, TokenType type);
	static ParseError badToken(int line, int column, TokenType expected, TokenType found);
	static ParseError badValue(
	    int line, int column, ValueType expectedType, std::string found);
	static ParseError badValue(
	    int line, int column, std::string expected, std::string found);

private:
	template<typename... Args>
	std::string const format(char const* format, int line, int column, Args... args) {
		auto fmt = "parse error at line %d, column %d: "s + format;
		return tfm::format(fmt.data(), line, column, args...);
	}
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
			this->log("error: ", format, args...);
		}
	}

	// Warn
	void warn(char const* message) const { this->warn("%s", message); }
	void warn(Error const& err) const { this->warn("%s", err.string().c_str()); }
	template<typename... Args>
	void warn(char const* format, Args... args) const {
		if (level_ == LogLevel::Warn || level_ == LogLevel::Info
		    || level_ == LogLevel::Debug) {

			this->log("warning: ", format, args...);
		}
	}

	// Info
	void info(char const* message) const { this->info("%s", message); }
	template<typename... Args>
	void info(char const* format, Args... args) const {
		if (level_ == LogLevel::Info || level_ == LogLevel::Debug) {
			this->log("info: ", format, args...);
		}
	}

	// Debug
	void debug(char const* message) const { this->debug("%s", message); }
	template<typename... Args>
	void debug(char const* format, Args... args) const {
		if (level_ == LogLevel::Debug) {
			this->log("debug: ", format, args...);
		}
	}

private:
	LogLevel level_;

	template<typename... Args>
	void log(char const* prefix, char const* format, Args... args) const {
		std::string buffer;
		buffer += "uhs: ";
		buffer += prefix;
		buffer += format;
		buffer += '\n';

		tfm::format(std::cerr, buffer.c_str(), args...);
	}
};

struct Options {
	bool debug = false;
	std::string mediaDir;
	ModeType mode = ModeType::Auto;
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
std::vector<std::string> split(std::string const& s, std::regex const& sep);
std::string toBase64(std::string const& s);
int toInt(std::string const& s);
void toLower(std::string& s);
std::string trim(std::string s, char c);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width,
    int& numLines, std::string const prefix = "");

} // namespace Strings

namespace Regex {

std::regex const Descriptor{"([0-9]+) ([a-z]{4,})"};
std::regex const DataAddress{"0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})"};
std::regex const EmailAddress{".+@.+\\..+"};
std::regex const HorizontalLine{"(?:-{3,} *)+"};
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
	std::optional<std::string const> attr(std::string const& key) const;
	void attr(std::string const& key, std::string const& value);
	void attr(std::string const& key, int const value);

private:
	Type attrs_;
};

class Body {
public:
	Body() = default;
	explicit Body(std::string const& body);
	explicit Body(int const body);
	std::string const& body() const;
	void body(std::string const& body);
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
	explicit Title(std::string const& title);
	std::string const& title() const;
	void title(std::string const& title);

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
	bool eof();
	std::exception_ptr error();
	bool good();
	void read();

private:
	static std::size_t const ReadLength = 1024;

	std::exception_ptr err_ = nullptr;
	std::vector<Handler> handlers_;
	std::istream& in_;
	std::size_t offset_ = 0;
};

class CRC {
public:
	static auto const ByteLength = 2;

	CRC();

	void calculate(char const* buffer, std::streamsize length, bool bufferChecksum);
	void calculate(char const* buffer, std::streamsize length);
	void reset();
	uint16_t result();
	void result(std::vector<char>& out);
	void upstream(Pipe& pipe);
	bool valid();

private:
	static uint16_t const CastMask = 0xFFFF;
	static uint16_t const FinalXOR = 0x0100;
	static uint16_t const MSBMask = 0x8000;
	static uint16_t const Polynomial = 0x8005;
	static auto const TableLength = 256;

	char checksum_[2];
	int checksumLength_ = 0;
	bool finalized_ = false;
	Pipe* pipe_ = nullptr;
	uint16_t remainder_ = 0x0000;
	uint16_t table_[TableLength];

	uint16_t checksum();
	void createTable();
	void finalize();
	uint8_t reflectByte(uint8_t byte);
};

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

class Tokenizer {
public:
	explicit Tokenizer(Pipe& pipe);

	bool hasNext();
	std::unique_ptr<Token const> next();
	void tokenize(char const* buffer, std::streamsize length);

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& pipe);

		void close();
		bool empty() const;
		bool ok() const;
		std::unique_ptr<Token const> receive();
		void send(Token const&& token);

	private:
		mutable std::mutex mutex_;
		bool open_ = true;
		Pipe& pipe_; // For exceptions
		std::queue<Token> queue_;
	};

	bool beforeHeaderSep_ = true;
	bool binaryMode_ = false;
	std::string buffer_;
	int expectedLineTokenLine_ = 0;
	int expectedStringTokenLine_ = 0;
	int line_ = 1;
	std::size_t offset_ = 0;
	TokenChannel out_;
	Pipe& pipe_;

	void tokenizeCRC(std::string const& crc, std::size_t column);
	void tokenizeData(std::string const& data, std::size_t column);
	void tokenizeDataAddress(std::smatch const& matches);
	ElementType tokenizeDescriptor(std::smatch const& matches);
	void tokenizeEOF(std::size_t column);
	void tokenizeHyperpngRegion(std::smatch const& matches);
	void tokenizeLine();
	void tokenizeMatches(
	    std::smatch const& matches, std::vector<TokenType> const&& tokens);
	void tokenizeOverlayAddress(std::smatch const& matches);
};

template<typename T>
class NodeIterator;

class Document;

class Node {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<Node const>;

	friend void swap(Node& lhs, Node& rhs) noexcept;

	explicit Node(NodeType type);
	Node(Node const& other);
	Node(Node&& other) noexcept;
	virtual ~Node() = default;

	static bool isElementOfType(Node const& node, ElementType type);
	static std::string const typeString(NodeType type);

	void appendChild(std::shared_ptr<Node> node);
	void appendChild(std::shared_ptr<Node> node, bool silenceEvent);
	iterator begin();
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator cend() const;
	int depth() const;
	void detachParent();
	iterator end();
	const_iterator end() const;
	Document* findDocument() const;
	std::shared_ptr<Node> firstChild() const;
	bool hasFirstChild() const;
	bool hasLastChild() const;
	bool hasNextSibling() const;
	bool hasParent() const;
	bool hasPreviousSibling() const;
	void insertBefore(std::shared_ptr<Node> node, Node* ref);
	bool isBreak() const;
	bool isDocument() const;
	bool isElement() const;
	bool isGroup() const;
	bool isText() const;
	Node* lastChild() const;
	std::shared_ptr<Node> nextSibling() const;
	NodeType nodeType() const;
	std::string const nodeTypeString() const;
	int numChildren() const;
	Node& operator=(Node other);
	Node* parent() const;
	std::shared_ptr<Node> pointer() const;
	Node* previousSibling() const;
	void removeChild(std::shared_ptr<Node> node);

protected:
	void cloneChildren(Node const& node);
	virtual void didAdd();
	virtual void didRemove();

private:
	int depth_ = 0;
	std::shared_ptr<Node> firstChild_ = nullptr;
	Node* lastChild_ = nullptr;
	std::shared_ptr<Node> nextSibling_ = nullptr;
	NodeType nodeType_;
	int numChildren_ = 0;
	Node* parent_ = nullptr;
	Node* previousSibling_ = nullptr;
};

class ContainerNode : public Node {
public:
	using Node::appendChild;

	friend void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept;

	explicit ContainerNode(NodeType type);
	ContainerNode(ContainerNode const& other);
	ContainerNode(ContainerNode&& other) noexcept;
	virtual ~ContainerNode() = default;

	int length() const;
	void length(int length); // Used by UHSWriter
	int line() const;
	void line(int line);
	ContainerNode& operator=(ContainerNode other);

protected:
	int length_ = 0;
	int line_ = 0;
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
	BreakNode();
	BreakNode(BreakNode const&);
	BreakNode(BreakNode&&) noexcept;

	static std::shared_ptr<BreakNode> create();

	std::shared_ptr<BreakNode> clone() const;
	BreakNode& operator=(BreakNode);
};

class GroupNode : public ContainerNode {
public:
	friend void swap(GroupNode& lhs, GroupNode& rhs) noexcept;

	GroupNode(int line, int length);
	GroupNode(GroupNode const&);
	GroupNode(GroupNode&&) noexcept;

	static std::shared_ptr<GroupNode> create(int line, int length);

	std::shared_ptr<GroupNode> clone() const;
	GroupNode& operator=(GroupNode);
};

class TextNode
    : public Node
    , public Traits::Body {
public:
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;

	explicit TextNode(std::string const& body);
	TextNode(std::string const& body, TextFormat format);
	TextNode(TextNode const& other);
	TextNode(TextNode&& other) noexcept;

	static std::shared_ptr<TextNode> create(std::string const& body);
	static std::shared_ptr<TextNode> create(std::string const& body, TextFormat format);

	void addFormat(TextFormat format);
	std::shared_ptr<TextNode> clone() const;
	TextFormat format() const;
	void format(TextFormat format);
	bool hasFormat(TextFormat format) const;
	TextNode& operator=(TextNode other);
	void removeFormat(TextFormat format);
	std::string const& string() const;

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
	friend void swap(Element& lhs, Element& rhs) noexcept;

	Element(ElementType type, int id = 0);
	Element(Element const& other);
	Element(Element&& other) noexcept;

	static std::shared_ptr<Element> create(ElementType type, int const id = 0);
	static ElementType elementType(std::string const& typeString);
	static std::string const typeString(ElementType type);

	std::shared_ptr<Element> clone() const;
	ElementType elementType() const;
	std::string const elementTypeString() const;
	int id() const;
	bool isMedia() const;
	std::string const mediaExt() const;
	Element& operator=(Element other);

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
	friend void swap(Document& lhs, Document& rhs) noexcept;

	Document(VersionType version);
	Document(Document const& other);
	Document(Document&& other) noexcept;

	static std::shared_ptr<Document> create(VersionType version);

	std::shared_ptr<Document> clone() const;
	void elementAdded(Element& element);
	void elementRemoved(Element& element);
	Node* find(int const id);
	bool isVersion(VersionType v) const;
	void normalize();
	Document& operator=(Document other);
	void reindex();
	bool validChecksum() const;
	void validChecksum(bool value);
	VersionType version() const;
	void version(VersionType v);
	std::string const versionString() const;

private:
	std::map<int, Node*> index_;
	bool indexed_ = true;
	bool validChecksum_ = false;
	VersionType version_;
};

class Codec {
public:
	std::string const createKey(std::string const secret) const;
	std::string const decode88a(std::string encoded) const;
	std::string const decode96a(
	    std::string encoded, std::string const& key, bool isTextElement) const;
	std::string const decodeSpecialChars(std::string const& encoded) const;
	std::string const encode88a(std::string decoded) const;
	std::string const encode96a(
	    std::string encoded, std::string const& key, bool isTextElement) const;
	std::string const encodeSpecialChars(std::string const& decoded) const;

private:
	using AsciiToUnicodeMap = tsl::hopscotch_map<std::string, std::string> const;
	using UnicodeToAsciiMap = tsl::hopscotch_map<char32_t, std::string> const;

	static constexpr auto KeySeed = "key";

	static UnicodeToAsciiMap fromChars_;
	static AsciiToUnicodeMap toChars_;

	Logger const logger_;

	int keystream(std::string const& key, std::size_t keyLength, std::size_t line,
	    bool isText) const;
	char toPrintable(int c) const;
};

class Parser {
public:
	explicit Parser(Logger const& logger, Options const& options = {});

	std::shared_ptr<Document> parse(std::istream& in);
	std::shared_ptr<Document> parseFile(std::string const& filename);
	void reset();

private:
	using NodeMap = std::map<int const, Node*>;
	using DataCallback = std::function<void(std::string)>;

	struct NodeRange {
		NodeRange(Node& node, int const min, int const max);

		int max = 0;
		int min = 0;
		Node& node;
	};

	struct NodeRangeList {
		std::vector<NodeRange> data = {};

		void add(Node& node, int const min, int const max);
		void clear();
		Node* find(int const min, int const max);
	};

	struct LinkData {
		LinkData(Element& link, int const line, int const column);

		int column = 0;
		int line = 0;
		Element& link;
	};

	struct VisibilityData {
		VisibilityData(int targetLine, VisibilityType visibility);

		int targetLine = 0;
		VisibilityType visibility;
	};

	struct DataHandler {
		DataHandler(int line, int column, std::size_t offset, std::size_t length,
		    DataCallback func);

		int column = 0;
		DataCallback func;
		std::size_t length = 0;
		int line = 0;
		std::size_t offset = 0;
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
	Options const& options_;
	NodeRangeList parents_;
	std::unique_ptr<Tokenizer> tokenizer_ = nullptr;

	void addDataCallback(
	    int line, int column, std::size_t offset, std::size_t length, DataCallback func);
	void addNodeToParentIndex(ContainerNode& node);
	void appendText(std::string& text, TextFormat& format, ContainerNode& node);
	void checkCRC();
	std::unique_ptr<Token const> expect(TokenType expected);
	Node* findParent(ContainerNode& node);
	int fixHeaderFirstChildLine(std::string title, int firstChildLine) const;
	std::unique_ptr<Token const> next();
	int offsetLine(int line);
	void parse88a();
	void parse88aCredits(std::unique_ptr<Token const> token);
	void parse88aElements(int firstHintTextLine, NodeMap& parents);
	void parse88aTextNodes(int lastHintTextLine, NodeMap& parents);
	void parse96a();
	void parseCommentElement(Element& element);
	void parseData(std::unique_ptr<Token const> token);
	void parseDataElement(Element& element);
	void parseDate(std::string const& date, std::tm& tm) const;
	std::shared_ptr<Element> parseElement(std::unique_ptr<Token const> token);
	void parseHeaderSep(std::unique_ptr<Token const> token);
	void parseHintElement(Element& element);
	void parseHyperpngElement(Element& element);
	void parseIncentiveElement(Element& element);
	void parseInfoElement(Element& element);
	void parseLinkElement(Element& element);
	void parseOverlayElement(Element& element);
	void parseSubjectElement(Element& element);
	void parseTextElement(Element& element);
	void parseTime(std::string const& time, std::tm& tm) const;
	void parseVersionElement(Element& element);
	void parseWithFormat(std::string const& text, TextFormat& format, ContainerNode& node,
	    ElementType elementType);
	void processLinks();
	void processVisibility();
};

class Writer {
public:
	Writer(Logger const& logger, std::ostream& out, Options const& options = {});
	virtual ~Writer() = default;
	virtual void write(std::shared_ptr<Document> const document) = 0;
	virtual void reset();

protected:
	Codec const codec_;
	Logger const logger_;
	std::ostream& out_;
	Options const& options_;
};

template<typename Func>
class ElementDispatcher {
public:
	void add(ElementType type, Func func) { map_.try_emplace(type, func); }

	template<typename Writer, typename Elem, typename... Args>
	auto dispatch(Writer& writer, Elem& element, Args&&... args) const {
		auto func = map_.at(element.elementType());
		return (writer.*func)(element, std::forward<Args>(args)...);
	}

private:
	tsl::hopscotch_map<ElementType, Func> map_;
};

class TreeWriter : public Writer {
public:
	TreeWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	using NodeToStyleMap = tsl::hopscotch_map<std::string, std::string> const;

	static auto constexpr StyleReset = "\033[0m";

	static NodeToStyleMap styles_;

	void draw(Document const& document);
	void draw(Element const& element);
	void draw(GroupNode const& groupNode);
	void draw(TextNode const& textNode);
	void drawScaffold(Node const& node);
	std::string drawText(std::string const& text, std::string const& name);
	std::string drawType(std::string const& name);
	std::string nodeStyle(Node const& node);
	std::string typeStyle(std::string const& name);
};

class HTMLWriter : public Writer {
public:
	struct TableAccessor;

	HTMLWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

	class Table {
		friend struct HTMLWriter::TableAccessor;

	public:
		static constexpr double HeaderlessConsensusThreshold = 0.6;

		Table(std::vector<std::string> const& lines);

		std::size_t endLine() const;
		bool hasPrecedingText() const;
		void parse();
		void serialize(pugi::xml_node& xmlNode) const;
		std::size_t startLine() const;
		bool valid() const;

	private:
		using Boundaries = std::vector<std::pair<std::size_t, std::size_t>>;

		bool charGrid_ = false;
		int demarcationLine_ = 0;
		std::size_t endLine_ = 0;
		bool headerless_ = false;
		std::vector<std::string> const lines_;
		bool pipeDelimited_ = false;
		std::vector<std::vector<std::string>> rows_;
		std::size_t startLine_ = 0;
		bool valid_ = false;

		void addHeaderRows(Boundaries const& columnBoundaries);
		Boundaries detectBoundariesFromLine(std::string const& line) const;
		bool detectCharGrid(Boundaries const& columnBoundaries) const;
		Boundaries detectColumnBoundaries() const;
		Boundaries detectHeaderlessColumnBoundaries() const;
		std::vector<std::string> extractCellsByBoundaries(
		    std::string const& line, Boundaries const& boundaries) const;
		std::vector<std::string> extractDataRowCells(std::string const& line,
		    Boundaries const& naturalBounds, Boundaries const& columnBoundaries,
		    int expectedNumColumns) const;
		int findDemarcationLine() const;
		bool isContinuationLine(std::string const& line, Boundaries const& naturalBounds,
		    Boundaries const& columnBoundaries, std::size_t tableWidth,
		    bool afterSeparator) const;
		bool isNextTableBoundary(std::vector<std::string>::const_iterator start) const;
		void mergeContinuationLine(std::string const& line,
		    Boundaries const& naturalBounds, Boundaries const& columnBoundaries);
		bool parsePipeDelimitedTable();
		void serializeBody(pugi::xml_node& table) const;
		void serializeHeader(pugi::xml_node& table) const;
		void serializeTextFallback(pugi::xml_node& parent) const;
	};

private:
	using NodeMap = tsl::hopscotch_map<Node const*, pugi::xml_node const>;

	using SerializeFunc = void (HTMLWriter::*)(
	    Element const& element, pugi::xml_node xmlNode);
	using Dispatcher = ElementDispatcher<SerializeFunc>;

	static Dispatcher dispatcher_;

	static void appendLinesToNode(
	    pugi::xml_node node, std::vector<std::string> const& lines);
	static std::vector<std::vector<std::string>> splitIntoParagraphs(
	    std::vector<std::string> const& segment);

	std::string css_;
	std::string js_;
	std::map<ElementType, std::string> mediaContentTypes_;
	std::map<ElementType, std::string> mediaTagTypes_;

	std::shared_ptr<Document> addEntryPointTo88aDocument(Document const& document) const;
	std::optional<pugi::xml_node> appendBody(
	    Element const& element, pugi::xml_node xmlNode) const;
	void appendClassNames(
	    pugi::xml_node xmlNode, std::vector<std::string> classNames) const;
	pugi::xml_node appendMedia(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node appendTitle(Element const& element, pugi::xml_node xmlNode) const;
	void appendVisibility(Traits::Visibility const& node, pugi::xml_node xmlNode) const;
	void applyHyperlinkFormat(TextNode const& textNode, pugi::xml_node xmlNode) const;
	void cleanUpTableContainerBreaks(pugi::xml_node root) const;
	pugi::xml_node createHTMLDocument(
	    Document const& document, pugi::xml_document& xml) const;
	std::optional<pugi::xml_node> findHyperpngContainer(
	    Element const& element, pugi::xml_node xmlNode) const;
	std::pair<std::vector<std::string>, std::vector<std::string>::const_iterator>
	    findNextSegment(std::vector<std::string>::const_iterator it,
	        std::vector<std::string>::const_iterator end) const;
	pugi::xml_node findOrCreateMap(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findXMLParent(Node const& node, pugi::xml_node const parent,
	    NodeMap const parents, int const depth) const;
	std::string getDataURI(std::string const& contentType, std::string const& data) const;
	std::tuple<int, int> getImageSize(Element const& element) const;
	Element* getParentElement(Element const& element) const;
	std::tuple<int, int, int, int> getRegionCoordinates(Element const& element) const;
	void populateArea(Element const& element, pugi::xml_node area) const;
	void removeEmptyParagraphs(pugi::xml_node root) const;
	void removeTrailingBreaks(pugi::xml_node xmlNode) const;
	std::pair<int, bool> scanForTables(
	    std::vector<std::string> const& lines, bool isMonoOrOverflow) const;
	void serialize(Document const& document, pugi::xml_document& xml);
	void serializeBlankElement(Element const& element, pugi::xml_node xmlNode);
	void serializeCommentElement(Element const& element, pugi::xml_node xmlNode);
	void serializeDataElement(Element const& element, pugi::xml_node xmlNode);
	void serializeDocument(Document const& document, pugi::xml_node root) const;
	void serializeElement(Element const& element, pugi::xml_node xmlNode);
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
	void serializeTextNodeToContainer(TextNode const& textNode, pugi::xml_node xmlNode,
	    std::vector<std::string> const& lines, bool hasTrailingBreak) const;
	void splitMonospaceOverflowSpans(pugi::xml_node root) const;
	void wrapInlineRuns(pugi::xml_node root) const;
};

class JSONWriter : public Writer {
public:
	JSONWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	void serialize(Document const& document, Json::Value& root) const;
	void serializeDocument(Document const& document, Json::Value& object) const;
	void serializeElement(Element const& element, Json::Value& object) const;
	void serializeMap(Traits::Attributes::Type const& attrs, Json::Value& object) const;
	void serializeTextNode(TextNode const& textNode, Json::Value& object) const;
};

class UHSWriter : public Writer {
public:
	// The official readers work with lines longer than 76 characters, but
	// lines were capped at 76 so that DOS readers would display correctly
	// within the standard 80-character window (with border and padding).
	static std::size_t const LineLength = 76;

	UHSWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;
	void reset() override;

private:
	using SerializeFunc = int (UHSWriter::*)(Element&, std::string&);
	using Dispatcher = ElementDispatcher<SerializeFunc>;
	using DataQueue = std::queue<std::pair<ElementType, std::string const>>;

	static constexpr auto DataAddressMarker = "000000";
	static auto const InitialElementLength = 2;  // Element descriptor and title
	static std::size_t const FileSizeLength = 7; // Up to 9,999,999 bytes per document
	static constexpr auto InfoLengthMarker = "length=0000000";
	static std::size_t const InitialBufferLength = 204'800; // 200 KiB
	static constexpr auto LinkMarker = "** LINK **";
	static std::size_t const MediaSizeLength = 6;  // Up to 999,999 bytes per media file
	static std::size_t const RegionSizeLength = 4; // Up to 9,999 × 9,999 px per region

	static Dispatcher dispatcher_;

	Codec const codec_;
	CRC crc_;
	int currentLine_ = 1;
	DataQueue data_;
	std::vector<Node*> deferredLinks_;
	std::shared_ptr<Document> document_ = nullptr;
	std::string key_;

	void addData(Element const& element);
	void convertTo96a();
	std::string createDataAddress(
	    std::size_t bodyLength, std::string textFormat = "") const;
	std::string createOverlayAddress(std::size_t bodyLength, int x, int y) const;
	std::string createRegion(int x1, int y1, int x2, int y2) const;
	std::string encodeText(std::string const& text, ElementType const parentType);
	std::string formatText(TextNode const& textNode, TextFormat const previousFormat);
	void serialize88a(Document const& document, std::string& out);
	void serialize96a(std::string& out);
	int serializeBlankElement(Element& element, std::string& out);
	int serializeCommentElement(Element& element, std::string& out);
	void serializeCRC(std::string& out);
	void serializeData(std::string& out);
	int serializeDataElement(Element& element, std::string& out);
	int serializeElement(Element& element, std::string& out);
	void serializeElementHeader(Element& element, std::string& out) const;
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
	void updateLinkTargets(std::string& out) const;
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

	class HTTPClient {
	public:
		virtual ~HTTPClient() = default;

		virtual httplib::Result Get(
		    std::string const& path, httplib::Headers const& headers) = 0;
		virtual httplib::Result Get(std::string const& path,
		    httplib::Headers const& headers,
		    httplib::ContentReceiver contentReceiver) = 0;
	};

	class DefaultHTTPClient : public HTTPClient {
	public:
		DefaultHTTPClient(std::string const& url);

		httplib::Result Get(
		    std::string const& path, httplib::Headers const& headers) override;
		httplib::Result Get(std::string const& path, httplib::Headers const& headers,
		    httplib::ContentReceiver contentReceiver) override;

	private:
		httplib::Client client_;
	};

	using FileIndex = std::map<std::string const, FileMetadata const>;

	Downloader(Logger const& logger);
	Downloader(Logger const& logger, std::unique_ptr<HTTPClient> httpClient);

	void download(std::string const& dir, std::string const& file);
	void download(std::string const& dir, std::vector<std::string> const& files);
	FileIndex const& fileIndex();

private:
	static constexpr auto BaseURL = "http://www.invisiclues.com";
	static constexpr auto FileIndexPath = "/cgi-bin/update.cgi";

	FileIndex fileIndex_;
	std::unique_ptr<HTTPClient> httpClient_;
	httplib::Headers httpHeaders_;
	Logger const logger_;

	void loadFileIndex();
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

	std::string const data_;

	uint16_t readUint16LE(int offset);
	uint32_t readUint32LE(int offset);
};

bool write(Logger const& logger, std::string const format, std::string const infile,
    std::string const outfile = "", Options const& options = {});

} // namespace UHS

extern "C" {
#include "uhs_bridge.h"
}

#endif

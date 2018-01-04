#ifndef UHS_H
#define UHS_H

#include "json.h"
#include "tinyformat.h"
#include <cassert>
#include <cstdint>
#include <ctime>
#include <exception>
#include <experimental/optional>
#include <fstream>
#include <istream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace UHS {

using std::experimental::nullopt;
using std::experimental::optional;
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

enum class TextElementType {
	None, // Also used for binary data
	Monospace,
	NoneAlt,
	MonospaceAlt,
};

enum class TextFormat : uint8_t {
	None = 00,
	WordWrap = 01,
	Proportional = 02,
	Hyperlink = 04,
};

enum class NodeType {
	Break,
	Document,
	Element,
	Text,
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
	All,          // Visible to every user
	Unregistered, // Visible only to unregistered users
	Registered,   // Visible only to registered users
	None,         // Visible to no one
};

static constexpr auto EOL = "\r\n";
static const auto MaxDepth = 16;
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

template<typename T>
class Passkey {
private:
	friend T;

	Passkey() = default;
	Passkey(const Passkey&) {}
	Passkey& operator=(const Passkey&) = delete;
};

class Error : public std::runtime_error {
public:
	Error();
	Error(const std::string& message);
	Error(const char* message);

	template<typename... Args>
	Error(const char* format, Args... args) : Error() {
		auto message = tfm::format(format, args...);
		static_cast<std::runtime_error&>(*this) = std::runtime_error(message);
	}
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

namespace Strings {

using CharacterMap = std::map<const std::string, const std::string>;

bool isInt(const std::string& s);
int toInt(const std::string& s);
std::string ltrim(const std::string& s, char c);
std::string rtrim(const std::string& s, char c);
std::string& chomp(std::string& s, char c);
std::vector<std::string> split(const std::string& s, const std::string& sep, int n = 0);
std::string join(const std::vector<std::string>& s, const std::string& sep);
std::string wrap(const std::string& s, const std::string& sep, std::size_t width);
std::string wrap(const std::string& s, const std::string& sep, std::size_t width,
    int& numLines, const std::string prefix = "");
const std::string hex(const std::string& s);
const std::string hex(char s);

} // namespace Strings

namespace Regex {

const std::regex Descriptor{"([0-9]+) ([a-z]{4,})"};
const std::regex DataAddress{"0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})"};
const std::regex HyperpngRegion{
    "(-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
const std::regex OverlayAddress{
    "0{6} ([0-9]{6,}) ([0-9]{6,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
} // namespace Regex

namespace Traits {

class Attributes {
public:
	using Type = std::map<std::string, std::string>;

	const Type& attrs() const;
	optional<const std::string> attr(const std::string key) const;
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
	bool visible(bool registered) const;
	VisibilityType visibility() const;
	void visibility(VisibilityType visibility);

private:
	VisibilityType visibility_ = VisibilityType::All;
};

} // namespace Traits

class Pipe {
public:
	using Handler = std::function<void(const char*, std::streamsize n)>;

	explicit Pipe(std::ifstream& in);
	void addHandler(Handler func);
	std::exception_ptr error();
	void read();
	bool good();
	bool eof();

private:
	static const std::size_t ReadLength = 1024;

	std::ifstream& in_;
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
	static constexpr auto AsciiEncStart = "#a+";
	static constexpr auto AsciiEncEnd = "#a-";
	static constexpr auto CreditSep = "CREDITS:";
	static const auto DataSep = '\x1A';
	static const auto Escape = '#';
	static constexpr auto HyperlinkStart = "#h+";
	static constexpr auto HyperlinkEnd = "#h-";
	static constexpr auto HeaderSep = "** END OF 88A FORMAT **";
	static constexpr auto InfoKeyValueSep = "=";
	static constexpr auto NestedElementSep = "=";
	static constexpr auto NestedTextSep = "-";
	static constexpr auto NoticePrefix = ">";
	static constexpr auto NumberSign = "##";
	static constexpr auto ParagraphSep = " "; // e.g., "text.\r\n \r\nText"
	static constexpr auto ProportionalStart = "#p+";
	static constexpr auto ProportionalEnd = "#p-";
	static constexpr auto Registered = "A";
	static constexpr auto Signature = "UHS";
	static constexpr auto Unregistered = "Z";
	static constexpr auto WordWrapStart = "#w.";
	static constexpr auto WordWrapStartInline = "#w+";
	static constexpr auto WordWrapEnd = "#w-";

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
	bool isText() const;
	void detachParent();
	std::unique_ptr<Node> removeChild(Node* node);
	void appendChild(std::unique_ptr<Node> node);
	void appendChild(std::unique_ptr<Node> node, bool silenceEvent);
	void insertBefore(std::unique_ptr<Node> node, Node* ref);
	bool hasParent() const;
	Node* parent() const;
	bool hasNextSibling() const;
	Node* nextSibling() const;
	bool hasFirstChild() const;
	Node* firstChild() const;
	bool hasLastChild() const;
	Node* lastChild() const;
	int numChildren() const;
	int depth() const;
	virtual std::unique_ptr<Node> cloneInternal(Passkey<Node>) const;

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
	std::unique_ptr<Node> nextSibling_ = nullptr;
	std::unique_ptr<Node> firstChild_ = nullptr;
	Node* lastChild_ = nullptr;
	int numChildren_ = 0;
	int depth_ = 0;

	Document* findDocument() const;
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
	bool operator==(const NodeIterator<T>& rhs);
	bool operator!=(const NodeIterator<T>& rhs);

private:
	pointer initial_ = nullptr;
	pointer current_ = nullptr;
	bool visited_ = false;
};

class BreakNode : public Node {
public:
	static std::unique_ptr<BreakNode> create();
	BreakNode();
	BreakNode(const BreakNode&);
	BreakNode& operator=(BreakNode);
	std::unique_ptr<Node> cloneInternal(Passkey<Node>) const;
};

class TextFormatter {
public:
	TextFormatter() = default;
	explicit TextFormatter(TextFormat format);
	void add(TextFormat format);
	void remove(TextFormat format);
	bool is(TextFormat format) const;
	TextFormat value() const;

private:
	TextFormat format_ = TextFormat::WordWrap | TextFormat::Proportional;
};

class TextNode
    : public Node
    , public Traits::Body {
public:
	static std::unique_ptr<TextNode> create(const std::string body);
	static std::unique_ptr<TextNode> create(
	    const std::string body, TextFormatter formatter);

	explicit TextNode(const std::string body);
	TextNode(const std::string body, TextFormatter formatter);
	TextNode(const TextNode& other);
	TextNode& operator=(TextNode other);
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;
	std::unique_ptr<TextNode> clone() const;
	const std::string& string() const;
	void addFormat(TextFormat format);
	void removeFormat(TextFormat format);
	bool hasFormat(TextFormat format) const;
	TextFormat format() const;
	TextFormatter formatter() const;
	std::unique_ptr<Node> cloneInternal(Passkey<Node>) const override;

private:
	TextFormatter formatter_;
};

class Document;

class Element
    : public Node
    , public Traits::Attributes
    , public Traits::Body
    , public Traits::Title
    , public Traits::Visibility {
public:
	using Node::appendChild;

	static std::unique_ptr<Element> create(ElementType type, const int id = 0);
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType type);

	Element(ElementType type, int id = 0);
	Element(const Element& other);
	Element& operator=(Element other);
	friend void swap(Element& lhs, Element& rhs) noexcept;
	std::unique_ptr<Element> clone() const;
	ElementType elementType() const;
	const std::string elementTypeString() const;
	void appendChild(const std::string body);
	void appendChild(const std::string body, TextFormatter formatter);
	int id() const;
	int line() const;
	void line(int line);
	int length() const;
	void length(int length); // Used by UHSWriter
	bool isMedia() const;
	const std::string mediaExt() const;
	std::unique_ptr<Node> cloneInternal(Passkey<Node>) const override;

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
	int id_;
	int line_ = 0;
	int length_ = 0;
};

class Document
    : public Node
    , public Traits::Attributes
    , public Traits::Title
    , public Traits::Visibility {
public:
	static std::unique_ptr<Document> create(VersionType version);

	Document(VersionType version);
	Document(const Document& other);
	Document& operator=(Document other);
	friend void swap(Document& lhs, Document& rhs) noexcept;
	Element* find(const int id);
	std::unique_ptr<Document> clone() const;
	void version(VersionType v);
	VersionType version() const;
	const std::string versionString() const;
	void validChecksum(bool value);
	bool validChecksum() const;
	void elementRemoved(Element& element);
	void elementAdded(Element& element);
	void reindex();
	std::unique_ptr<Node> cloneInternal(Passkey<Node>) const override;

private:
	VersionType version_;
	bool validChecksum_ = false;
	std::map<const int, Element*> index_;
	bool indexed_ = true;
};

class Codec {
public:
	const std::string decode88a(std::string encoded) const;
	const std::string encode88a(std::string decoded) const;
	const std::string decode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	const std::string encode96a(
	    std::string encoded, std::string key, bool isTextElement) const;
	const std::string createKey(std::string secret) const;

private:
	static const char AsciiStart = 0x20;
	static const char AsciiEnd = 0x7F;
	static constexpr auto KeySeed = "key";

	int keystream(
	    std::string key, std::size_t keyLength, std::size_t line, bool isText) const;
	bool isPrintable(int c) const;
	char toPrintable(int c) const;
};

struct ParserOptions {
	bool debug = false;
	bool force88aMode = false;
};

class Parser {
public:
	explicit Parser(const ParserOptions options = {});
	std::unique_ptr<Document> parse(std::ifstream& in);
	void reset();

private:
	using NodeMap = std::map<const int, Node*>;
	using DataCallback = std::function<void(std::string)>;

	struct NodeRange {
		Node& node;
		int min;
		int max;

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
		int line;
		int column;

		LinkData(Element& link, const int line, const int column);
	};

	struct VisibilityData {
		int targetLine;
		VisibilityType visibility;

		VisibilityData(int targetLine, VisibilityType visibility);
	};

	struct DataHandler {
		std::size_t offset;
		std::size_t length;
		DataCallback func;

		DataHandler(std::size_t offset, std::size_t length, DataCallback func);
	};

	static const auto HeaderLength = 4;
	static const auto FormatTokenLength = 3;

	Codec codec_;
	std::unique_ptr<CRC> crc_ = nullptr;
	std::vector<DataHandler> dataHandlers_;
	std::vector<LinkData> deferredLinks_;
	std::vector<VisibilityData> deferredVisibilities_;
	std::unique_ptr<Document> document_ = nullptr;
	bool done_ = false;
	bool isTitleSet_ = false;
	std::string key_;
	int lineOffset_ = 0;
	const ParserOptions options_;
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
	Element* parseElement(std::unique_ptr<const Token> token);
	void parseCommentElement(Element* const element);
	void parseDataElement(Element* const element);
	void parseHintElement(Element* const element);
	void parseHyperpngElement(Element* const element);
	void parseIncentiveElement(Element* const element);
	void parseInfoElement(Element* const element);
	void parseLinkElement(Element* const element);
	void parseOverlayElement(Element* const element);
	void parseSubjectElement(Element* const element);
	void parseTextElement(Element* const element);
	void parseVersionElement(Element* const element);

	// Parse helpers
	std::unique_ptr<const Token> next();
	std::unique_ptr<const Token> expect(TokenType expected);
	void addDataCallback(std::size_t offset, std::size_t length, DataCallback func);
	void checkCRC();
	void findParentAndAppend(std::unique_ptr<Element> element);
	bool isPunctuation(char c);
	int offsetLine(int line);
	void parseData(std::unique_ptr<const Token> token);
	void parseDate(const std::string& date, std::tm& tm) const;
	void parseTime(const std::string& time, std::tm& tm) const;
	void parseWithFormat(
	    const std::string& text, TextFormatter& formatter, Element& element) const;
	void processLinks();
	void processVisibility();
};

struct WriterOptions {
	bool debug = false;
	bool force88aMode = false;
	std::string mediaDir;
	bool registered = true; // JSONWriter only
};

class Writer {
public:
	Writer(std::ostream& out, const WriterOptions options = {});
	virtual ~Writer() = default;
	virtual void write(const Document& document) = 0;
	virtual void reset();

protected:
	std::ostream& out_;
	const WriterOptions options_;
};

class TreeWriter : public Writer {
public:
	TreeWriter(std::ostream& out, const WriterOptions options = {});
	void write(const Document& document) override;

private:
	void draw(const Document& document);
	void draw(const Element& element);
	void drawScaffold(const Node& node);
};

class JSONWriter : public Writer {
public:
	JSONWriter(std::ostream& out, const WriterOptions options = {});
	void write(const Document& document) override;

private:
	Json::Value serialize(const Document& document, Json::Value& root) const;
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

	UHSWriter(std::ostream& out, const WriterOptions options = {});
	void write(const Document& document) override;
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
	std::string serializeTextNode(const ElementType parentType, const TextNode& textNode,
	    const TextFormatter previousFormatter, int& length);
	void addData(const Element& element);
	std::string createDataAddress(
	    std::size_t bodyLength, std::string textFormat = "") const;
	std::string createOverlayAddress(std::size_t bodyLength, int x, int y) const;
	std::string createRegion(int x1, int y1, int x2, int y2) const;
	void convertTo91a();

	static UHSWriter::Serializer serializer_;

	Codec codec_;
	CRC crc_;
	int currentLine_ = 1;
	std::unique_ptr<Document> document_ = nullptr;
	std::string key_;
	DataQueue data_;
	std::vector<Element*> deferredLinks_;
};

} // namespace UHS

#endif

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

enum class TextFormatType {
	None, // Also used for binary data
	Monospace,
	NoneAlt,
	MonospaceAlt,
};

enum class TypefaceType {
	Proportional,
	Monospace,
};

enum class NodeType {
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

static constexpr const char* EOL = "\r\n";
static const int MaxDepth = 16;
static constexpr const char* Version = UHS_VERSION;

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

typedef std::map<const std::string, const std::string> CharacterMap;

bool isInt(const std::string& s);
int toInt(const std::string& s);
std::string ltrim(const std::string& s, char c);
std::string rtrim(const std::string& s, char c);
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
	typedef std::map<std::string, std::string> Type;

	const Type& attrs() const;
	optional<const std::string> attr(const std::string key) const;
	void attr(const std::string key, const std::string value);

private:
	Type _attrs;
};

class Body {
public:
	Body() = default;
	explicit Body(const std::string s);
	const std::string& body() const;
	void body(const std::string s);

private:
	std::string _body;
};

class Title {
public:
	Title() = default;
	explicit Title(const std::string s);
	const std::string& title() const;
	void title(const std::string s);

private:
	std::string _title;
};

class Visibility {
public:
	bool visible(bool registered) const;
	VisibilityType visibility() const;
	void visibility(VisibilityType v);

private:
	VisibilityType _visibility = VisibilityType::All;
};

} // namespace Traits

class Pipe {
public:
	typedef std::function<void(const char*, std::streamsize n)> Handler;

	explicit Pipe(std::ifstream& in);
	void addHandler(Handler func);
	std::exception_ptr error();
	void read();
	bool good();
	bool eof();

private:
	static const std::size_t ReadLen = 1024;

	std::ifstream& _in;
	std::size_t _offset = 0;
	std::vector<Handler> _handlers;
	std::exception_ptr _err = nullptr;
};

class CRC {
public:
	static const int ByteLen = 2;

	CRC();
	void upstream(Pipe& p);
	void calculate(const char* buf, std::streamsize n, bool bufferChecksum);
	void calculate(const char* buf, std::streamsize n);
	uint16_t result();
	void result(std::vector<char>& out);
	bool valid();
	void reset();

private:
	static const int TableLen = 256;
	static const uint16_t Polynomial = 0x8005;
	static const uint16_t CastMask = 0xFFFF;
	static const uint16_t MSBMask = 0x8000;
	static const uint16_t FinalXOR = 0x0100;

	Pipe* _pipe = nullptr;
	uint16_t _table[TableLen];
	char _buf[2]; // Checksum buffer
	int _bufLen = 0;
	uint16_t _rem = 0x0000;
	bool _finalized = false;

	void createTable();
	uint8_t reflectByte(uint8_t byte);
	void finalize();
	uint16_t checksum();
};

class Tokenizer;

class Token {
	friend class Tokenizer;

public:
	static constexpr const char* AsciiEncStart = "#a+";
	static constexpr const char* AsciiEncEnd = "#a-";
	static constexpr const char* CreditSep = "CREDITS:";
	static const char DataSep = '\x1A';
	static constexpr const char* HyperlinkStart = "#h+";
	static constexpr const char* HyperlinkEnd = "#h-";
	static constexpr const char* HeaderSep = "** END OF 88A FORMAT **";
	static constexpr const char* InfoKeyValueSep = "=";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static constexpr const char* NoticePrefix = ">";
	static constexpr const char* NumberSign = "##";
	static constexpr const char* ParagraphSep = " "; // e.g., "text.\r\n \r\nText"
	static constexpr const char* ProportionalStart = "#p+";
	static constexpr const char* ProportionalEnd = "#p-";
	static constexpr const char* Registered = "A";
	static constexpr const char* Signature = "UHS";
	static constexpr const char* Unregistered = "Z";
	static constexpr const char* WordWrapStart = "#w+";
	static constexpr const char* WordWrapEnd = "#w-";
	static constexpr const char* WordWrapEndAlt = "#w.";

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
	const TokenType _type;
	int _line = 0;
	std::size_t _column = 0;
	std::size_t _offset = 0;
	std::string _value;

	const std::string formatToken() const;
	const std::string formatIntValue() const;
	const std::string formatStringValue() const;
	const std::string formatByteValue() const;
};

class Tokenizer {
public:
	explicit Tokenizer(Pipe& p);
	void tokenize(const char* buf, std::streamsize n);
	bool hasNext();
	std::unique_ptr<const Token> next();

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& p);
		void send(const Token&& t);
		std::unique_ptr<const Token> receive();
		bool empty() const;
		bool ok() const;
		void close();

	private:
		std::queue<const Token> _queue;
		mutable std::mutex _mutex;
		bool _open = true;
		Pipe& _pipe; // For exceptions
	};

	Pipe& _pipe;
	std::string _buf;
	int _line = 1;
	std::size_t _offset = 0;
	bool _beforeHeaderSep = true;
	bool _binaryMode = false;
	int _expectedLineTokenLine = 0;
	int _expectedStringTokenLine = 0;
	TokenChannel _out;

	void tokenizeLine();
	ElementType tokenizeDescriptor(const std::smatch& m);
	void tokenizeDataAddress(const std::smatch& m);
	void tokenizeHyperpngRegion(const std::smatch& m);
	void tokenizeOverlayAddress(const std::smatch& m);
	void tokenizeMatches(const std::smatch& m, const std::vector<TokenType>&& tokens);
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

	static const std::string typeString(NodeType t);
	static bool isElementOfType(const Node& n, ElementType t);

	explicit Node(NodeType t);
	Node(const Node& other);
	Node& operator=(Node other);
	friend void swap(Node& lhs, Node& rhs) noexcept;
	virtual ~Node() = default;
	NodeType nodeType() const;
	const std::string nodeTypeString() const;
	void detachParent();
	std::unique_ptr<Node> removeChild(Node* n);
	void appendChild(std::unique_ptr<Node> n);
	void appendChild(std::unique_ptr<Node> n, bool silenceEvent);
	void insertBefore(std::unique_ptr<Node> n, Node* ref);
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

	// Iterators
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

protected:
	virtual std::unique_ptr<Node> cloneInternal() const;
	void cloneChildren(const Node& node);
	virtual void didRemove();
	virtual void didAdd();

private:
	NodeType _nodeType;
	Node* _parent = nullptr;
	std::unique_ptr<Node> _nextSibling = nullptr;
	std::unique_ptr<Node> _firstChild = nullptr;
	Node* _lastChild = nullptr;
	int _numChildren = 0;
	int _depth = 0;

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
	explicit NodeIterator(pointer n);
	reference operator*() const;
	pointer operator->() const;
	NodeIterator<T>& operator++();
	NodeIterator<T> operator++(int);
	bool operator==(const NodeIterator<T>& rhs);
	bool operator!=(const NodeIterator<T>& rhs);

private:
	pointer _initial = nullptr;
	pointer _current = nullptr;
	bool _visited = false;
};

typedef uint8_t Format;

class TextNode
    : public Node
    , public Traits::Body {
public:
	friend class Node;

	static std::unique_ptr<TextNode> create(const std::string body);

	explicit TextNode(const std::string body);
	TextNode(const TextNode& other);
	TextNode& operator=(TextNode other);
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;
	std::unique_ptr<TextNode> clone() const;
	const std::string& string() const;
	void addFormat(Format f);
	void removeFormat(Format f);
	bool hasFormat(Format f) const;
	Format format() const;

private:
	Format _fmt;

	std::unique_ptr<Node> cloneInternal() const override;
};

class Element
    : public Node
    , public Traits::Attributes
    , public Traits::Body
    , public Traits::Title
    , public Traits::Visibility {
public:
	friend class Node;

	using Node::appendChild;

	static std::unique_ptr<Element> create(ElementType type, const int id = 0);
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType t);

	Element(ElementType type, const int id = 0);
	Element(const Element& other);
	Element& operator=(Element other);
	friend void swap(Element& lhs, Element& rhs) noexcept;
	std::unique_ptr<Element> clone() const;
	ElementType elementType() const;
	const std::string elementTypeString() const;
	void appendChild(const std::string s);
	int id() const;
	int line() const;
	void line(int line);
	int length() const;
	void length(int len); // Used by UHSWriter
	bool isMedia() const;
	const std::string mediaExt() const;

private:
	ElementType _elementType;
	int _id;
	int _line = 0;
	int _length = 0;

	std::unique_ptr<Node> cloneInternal() const override;
};

class Document
    : public Node
    , public Traits::Attributes
    , public Traits::Title
    , public Traits::Visibility {
public:
	friend class Node;

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

private:
	VersionType _version;
	bool _validChecksum = false;
	std::map<const int, Element*> _index;
	bool _indexed = true;

	std::unique_ptr<Node> cloneInternal() const override;
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
	static constexpr const char* KeySeed = "key";

	int keystream(
	    std::string key, std::size_t keyLen, std::size_t line, bool isText) const;
	bool isPrintable(int c) const;
	char toPrintable(int c) const;
};

struct ParserOptions {
	bool debug = false;
	bool force88aMode = false;
};

class Parser {
public:
	explicit Parser(const ParserOptions opt = {});
	std::unique_ptr<Document> parse(std::ifstream& in);
	void reset();

private:
	typedef std::map<const int, Node*> NodeMap;
	typedef std::function<void(std::string)> DataCallback;

	struct NodeRange {
		Node& node;
		int min;
		int max;

		NodeRange(Node& n, const int min, const int max);
	};

	struct NodeRangeList {
		std::vector<NodeRange> data = {};

		Node* find(const int min, const int max);
		void add(Node& n, const int min, const int max);
		void clear();
	};

	struct LinkData {
		int targetLine;
		int line;
		int column;

		LinkData() = default;
		LinkData(int targetLine, const int line, const int column);
	};

	struct DataHandler {
		std::size_t offset;
		std::size_t length;
		DataCallback func;

		DataHandler(std::size_t offset, std::size_t length, DataCallback func);
	};

	static const int HeaderLen = 4;
	static const int FormatTokenLen = 3;

	const ParserOptions _opt;
	std::unique_ptr<Tokenizer> _tokenizer = nullptr;
	std::unique_ptr<CRC> _crc = nullptr;
	Codec _codec;
	std::unique_ptr<Document> _document = nullptr;
	NodeRangeList _parents;
	std::vector<LinkData> _deferredLinkChecks;
	std::vector<DataHandler> _dataHandlers;
	std::string _key;
	int _lineOffset = 0;
	bool _isTitleSet = false;
	bool _done = false;

	// 88a
	void parse88a();
	void parse88aElements(int firstHintTextLine, NodeMap& parents);
	void parse88aTextNodes(int lastHintTextLine, NodeMap& parents);
	void parse88aCredits(std::unique_ptr<const Token> t);
	void parseHeaderSep(std::unique_ptr<const Token> t);

	// 96a
	void parse96a();
	Element* parseElement(std::unique_ptr<const Token> t);
	void parseCommentElement(Element* const e);
	void parseDataElement(Element* const e);
	void parseHintElement(Element* const e);
	void parseHyperpngElement(Element* const e);
	void parseInfoElement(Element* const e);
	void parseIncentiveElement(Element* const e);
	void parseLinkElement(Element* const e);
	void parseOverlayElement(Element* const e);
	void parseSubjectElement(Element* const e);
	void parseTextElement(Element* const e);
	void parseVersionElement(Element* const e);

	// Parse helpers
	std::unique_ptr<const Token> next();
	std::unique_ptr<const Token> expect(TokenType expected);
	void findParentAndAppend(std::unique_ptr<Element> e, std::unique_ptr<const Token> t);
	void deferLinkCheck(int targetLine, const int line, const int column);
	void checkLinks();
	Element* findTarget(const int line);
	void addDataCallback(std::size_t offset, std::size_t length, DataCallback func);
	void parseData(std::unique_ptr<const Token> t);
	void checkCRC();
	void parseDate(const std::string& s, std::tm& tm) const;
	void parseTime(const std::string& s, std::tm& tm) const;
	bool isPunctuation(char c);
	int offsetLine(int line);
};

struct WriterOptions {
	bool debug = false;
	bool force88aMode = false;
	std::string mediaDir;
	bool registered = true; // JSONWriter only
};

class Writer {
public:
	Writer(std::ostream& out, const WriterOptions opt = {});
	virtual ~Writer() = default;
	virtual void write(const Document& d) = 0;
	virtual void reset();

protected:
	std::ostream& _out;
	const WriterOptions _opt;
};

class TreeWriter : public Writer {
public:
	TreeWriter(std::ostream& out, const WriterOptions opt = {});
	void write(const Document& d) override;

private:
	void draw(const Document& d);
	void draw(const Element& e);
	void drawScaffold(const Node& n);
};

class JSONWriter : public Writer {
public:
	JSONWriter(std::ostream& out, const WriterOptions opt = {});
	void write(const Document& d) override;

private:
	Json::Value serialize(const Document& d, Json::Value& root) const;
	void serializeElement(const Element& e, Json::Value& obj) const;
	void serializeDocument(const Document& d, Json::Value& obj) const;
	void serializeMap(const Traits::Attributes::Type& attrs, Json::Value& obj) const;
};

class UHSWriter : public Writer {
public:
	// The official readers work with lines longer than 76 characters, but
	// lines were capped at 76 so that DOS readers would display correctly
	// within the standard 80-character window (with border and padding).
	static const std::size_t LineLen = 76;

	UHSWriter(std::ostream& out, const WriterOptions opt = {});
	void write(const Document& d) override;
	void reset() override;

private:
	typedef std::queue<std::pair<ElementType, const std::string>> DataQueue;

	static const std::size_t InitialBufferLen = 204'800; // 200 KiB
	static const std::size_t MediaSizeLen = 6; // Up to 999,999 bytes per media file
	static const std::size_t FileSizeLen = 7;  // Up to 9,999,999 bytes per document
	static constexpr const char* DataAddressMarker = "000000";
	static constexpr const char* InfoLengthMarker = "length=0000000";

	void serialize88a(const Document& d, std::string& out);
	void serialize96a(std::string& out);
	void serializeElement(const Element& e, std::string& out, int& len);
	void serializeCommentElement(const Element& e, std::string& out, int& len);
	void serializeDataElement(const Element& e, std::string& out, int& len);
	void serializeHintElement(const Element& e, std::string& out, int& len);
	void serializeInfoElement(std::string& out, int& len);
	void serializeSubjectElement(const Element& e, std::string& out, int& len);
	void serializeTextElement(const Element& e, std::string& out, int& len);
	void serializeData(std::string& out);
	void serializeCRC(std::string& out);
	std::string createDataAddress(std::size_t bodyLen, std::string textFormat = "");
	void convertTo91a();

	Codec _codec;
	CRC _crc;
	std::unique_ptr<Document> _document = nullptr;
	std::string _key;
	DataQueue _data;
};

} // namespace UHS

#endif

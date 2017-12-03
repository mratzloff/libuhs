#ifndef UHS_H
#define UHS_H

#include "json.h"
#include <cassert>
#include <cstdint>
#include <ctime>
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
#include <string>
#include <vector>

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

enum class ErrorType {
	Unknown,
	FileEnd,
	Read,
	Write,
	Value,
	Token,
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
	Index,
	Length,
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

namespace Strings {

typedef std::map<const std::string, const std::string> CharacterMap;

constexpr int NaN = INT_MIN;

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
	const std::string attr(const std::string& key) const;
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

class Error {
public:
	Error() = default;
	explicit Error(ErrorType t);
	Error(ErrorType t, std::string s);
	ErrorType type() const;
	void type(ErrorType t);
	const std::string& message() const;
	void message(const std::string s);
	void messagef(const char* format, ...);
	void finalize();
	void finalize(int line, int column);

private:
	ErrorType _type = ErrorType::Unknown;
	std::string _message;
};

class Pipe {
public:
	typedef std::function<void(const char*, std::streamsize n)> Handler;

	explicit Pipe(std::ifstream& in);
	std::unique_ptr<Error> error();
	void addHandler(Handler func);
	void read();
	bool good();
	bool eof();

private:
	static const std::size_t ReadLen = 1024;

	std::ifstream& _in;
	std::unique_ptr<Error> _err = nullptr;
	std::size_t _offset = 0;
	std::vector<Handler> _handlers;
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
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string& value() const;
	const std::string typeString() const;
	const std::string string() const;
	friend std::ostream& operator<<(std::ostream& out, const Token& t);

private:
	friend class Tokenizer;

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
	std::unique_ptr<Error> error();
	void tokenize(const char* buf, std::streamsize n);
	bool hasNext();
	std::unique_ptr<const Token> next();

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& p);
		std::unique_ptr<Error> error();
		bool send(const Token&& t);
		std::unique_ptr<const Token> receive();
		bool empty() const;
		bool ok() const;
		void close();

	private:
		Pipe& _pipe; // For errors
		std::unique_ptr<Error> _err = nullptr;
		std::queue<const Token> _queue;
		mutable std::mutex _mutex;
		bool _open = true;
	};

	Pipe& _pipe;
	std::unique_ptr<Error> _err = nullptr;
	std::string _buf;
	int _line = 1;
	std::size_t _offset = 0;
	bool _beforeHeaderSep = true;
	bool _binaryMode = false;
	int _expectedIndexLine = 0;
	int _expectedStringLine = 0;
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

class Node {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<const Node>;

	static const std::string typeString(NodeType t);
	static bool isElementOfType(const Node& n, ElementType t);

	explicit Node(NodeType t);
	Node(const Node& other);
	virtual ~Node() = default;
	virtual std::unique_ptr<Node> clone() const;
	NodeType nodeType() const;
	const std::string nodeTypeString() const;
	std::unique_ptr<Node> removeChild(Node* n);
	void appendChild(std::unique_ptr<Node> n);
	void insertBefore(std::unique_ptr<Node> n, Node* ref);
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

private:
	NodeType _nodeType;
	Node* _parent = nullptr;
	std::unique_ptr<Node> _nextSibling = nullptr;
	std::unique_ptr<Node> _firstChild = nullptr;
	Node* _lastChild = nullptr;
	int _numChildren = 0;
	int _depth = 0;
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
	TextNode();
	explicit TextNode(const std::string body);
	TextNode(const TextNode& other);
	std::unique_ptr<Node> clone() const override;
	std::unique_ptr<TextNode> cloneTextNode() const;
	const std::string& string() const;
	void addFormat(Format f);
	void removeFormat(Format f);
	bool hasFormat(Format f) const;
	Format format() const;

private:
	Format _fmt;
};

class Element
    : public Node
    , public Traits::Attributes
    , public Traits::Body
    , public Traits::Title
    , public Traits::Visibility {
public:
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType t);

	Element(ElementType t, int index = 0, int length = 0);
	Element(ElementType t, const std::string title);
	Element(const Element& other);
	std::unique_ptr<Node> clone() const override;
	std::unique_ptr<Element> cloneElement() const;
	ElementType elementType() const;
	const std::string elementTypeString() const;
	void appendString(const std::string s);
	int index() const;
	int length() const;
	void length(int len); // Used by UHSWriter
	const Element* ref() const;
	void ref(const Element* ref);
	bool isMedia() const;
	const std::string mediaExt() const;

private:
	ElementType _elementType;
	int _index = 0;
	int _length = 0;
	const Element* _ref = nullptr;
};

class Document
    : public Node
    , public Traits::Attributes
    , public Traits::Title
    , public Traits::Visibility {
public:
	Document();
	Document(VersionType version, const std::string title = "");
	Document(const Document& other);
	std::unique_ptr<Node> clone() const override;
	std::unique_ptr<Document> cloneDocument() const;
	void version(VersionType v);
	VersionType version() const;
	const std::string versionString() const;
	void validChecksum(bool value);
	bool validChecksum() const;

private:
	VersionType _version;
	bool _validChecksum = false;
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
	    std::string key, std::size_t keyLen, std::size_t index, bool isText) const;
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
	std::unique_ptr<Error> error();
	std::unique_ptr<Document> parse(std::ifstream& in);
	void reset();

private:
	typedef std::map<const int, Node*> NodeMap;
	typedef std::map<const int, Element*> ElementMap;
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
		Element* fromElement = nullptr;
		int toIndex;
		int line;
		int column;

		LinkData() = default;
		LinkData(
		    Element* fromElement, const int toIndex, const int line, const int column);
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
	std::unique_ptr<Error> _err = nullptr;
	std::unique_ptr<Pipe> _pipe = nullptr;
	std::unique_ptr<Tokenizer> _tokenizer = nullptr;
	std::unique_ptr<CRC> _crc = nullptr;
	Codec _codec;
	std::unique_ptr<Document> _document = nullptr;
	NodeRangeList _parents;
	ElementMap _elements;
	std::map<int, LinkData> _deferredLinks;
	std::vector<DataHandler> _dataHandlers;
	std::string _key;
	int _indexOffset = 0;
	bool _isTitleSet = false;
	bool _done = false;

	// 88a
	bool parse88a();
	bool parse88aElements(int firstHintTextIndex, NodeMap& parents);
	bool parse88aTextNodes(int lastHintTextIndex, NodeMap& parents);
	bool parse88aCredits(std::unique_ptr<const Token> t);
	void parseHeaderSep(std::unique_ptr<const Token> t);

	// 96a
	bool parse96a();
	Element* parseElement(std::unique_ptr<const Token> t, bool indexByRegion = false);
	bool parseCommentElement(Element* const e);
	bool parseDataElement(Element* const e);
	bool parseHintElement(Element* const e);
	bool parseHyperpngElement(Element* const e);
	bool parseInfoElement(Element* const e);
	bool parseIncentiveElement(Element* const e);
	bool parseLinkElement(Element* const e);
	bool parseOverlayElement(Element* const e);
	bool parseSubjectElement(Element* const e);
	bool parseTextElement(Element* const e);
	bool parseVersionElement(Element* const e);

	// Parse helpers
	std::unique_ptr<const Token> next();
	std::unique_ptr<const Token> expect(TokenType expected);
	bool findParentAndAppend(std::unique_ptr<Element> e, std::unique_ptr<const Token> t);
	bool linkOrDefer(
	    Element* const fromElement, const int toIndex, const int line, const int column);
	bool link(
	    Element* const fromElement, const int toIndex, const int line, const int column);
	bool handleDeferredLink(int index);
	void addDataCallback(std::size_t offset, std::size_t length, DataCallback func);
	void parseData(std::unique_ptr<const Token> t);
	void checkCRC();
	bool parseDate(const std::string& s, std::tm& tm) const;
	bool parseTime(const std::string& s, std::tm& tm) const;
	bool isPunctuation(char c);
	int offsetIndex(int index);

	// Error helpers
	void indexNotFound(int index, int line, int column);
	void expected(std::string expected, std::string found, int line, int column);
	void expectedInt(std::string found, int line, int column);
	void unexpected(TokenType type, int line, int column);
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
	std::unique_ptr<Error> error();
	virtual bool write(const Document& d) = 0;
	virtual void reset();

protected:
	std::ostream& _out;
	const WriterOptions _opt;
	std::unique_ptr<Error> _err = nullptr;
};

class TreeWriter : public Writer {
public:
	TreeWriter(std::ostream& out, const WriterOptions opt = {});
	bool write(const Document& d) override;

private:
	void draw(const Document& d);
	void draw(const Element& e);
	void drawScaffold(const Node& n);
};

class JSONWriter : public Writer {
public:
	JSONWriter(std::ostream& out, const WriterOptions opt = {});
	bool write(const Document& d) override;

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
	bool write(const Document& d) override;
	void reset() override;

private:
	typedef std::queue<std::pair<ElementType, const std::string>> DataQueue;

	static const std::size_t InitialBufferLen = 204'800; // 200 KiB
	static const std::size_t MediaSizeLen = 6; // Up to 999,999 bytes per media file
	static const std::size_t FileSizeLen = 7;  // Up to 9,999,999 bytes per document
	static constexpr const char* DataAddressMarker = "000000";
	static constexpr const char* InfoLengthMarker = "length=0000000";

	bool serialize88a(const Document& d, std::string& out);
	bool serialize96a(std::string& out);
	bool serializeElement(const Element& e, std::string& out, int& len);
	bool serializeCommentElement(const Element& e, std::string& out, int& len);
	bool serializeDataElement(const Element& e, std::string& out, int& len);
	bool serializeHintElement(const Element& e, std::string& out, int& len);
	bool serializeInfoElement(std::string& out, int& len);
	bool serializeSubjectElement(const Element& e, std::string& out, int& len);
	bool serializeTextElement(const Element& e, std::string& out, int& len);
	bool serializeData(std::string& out);
	void serializeCRC(std::string& out);
	std::string createDataAddress(std::size_t bodyLen, std::string textFormat = "");
	bool convertTo91a();

	Codec _codec;
	CRC _crc;
	std::unique_ptr<Document> _document = nullptr;
	std::string _key;
	DataQueue _data;
};

} // namespace UHS

#endif

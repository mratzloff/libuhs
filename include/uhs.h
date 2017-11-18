#ifndef UHS_H
#define UHS_H

#define UHS_VERSION "1.0.0-alpha"
#define UHS_MAX_DEPTH 16

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
#include "json.h"

namespace UHS {

enum ElementType {
	ElementUnknown,
	ElementBlank,
	ElementComment,
	ElementCredit,
	ElementGifa,
	ElementHint,
	ElementHyperpng,
	ElementIncentive,
	ElementInfo,
	ElementLink,
	ElementNesthint,
	ElementOverlay,
	ElementSound,
	ElementSubject,
	ElementText,
	ElementVersion,
};

enum ErrorType {
	ErrorUnknown,
	ErrorEOF,
	ErrorRead,
	ErrorValue,
	ErrorToken,
};

enum TextFormatType {
	TextFormatNone,         // Also used for binary data
	TextFormatMonospace,
	TextFormatNoneAlt,
	TextFormatMonospaceAlt,
};

enum TypefaceType {
	TypefaceProportional,
	TypefaceMonospace,
};

enum NodeType {
	NodeDocument,
	NodeElement,
	NodeText,
};

enum TokenType {
	TokenCRC,
	TokenCoordX,
	TokenCoordY,
	TokenCreditSep,
	TokenData,
	TokenDataLength,
	TokenDataOffset,
	TokenDataType,
	TokenEOF,
	TokenHeaderSep,
	TokenIdent,
	TokenIndex,
	TokenLength,
	TokenNestedElementSep,
	TokenNestedTextSep,
	TokenParagraphSep,
	TokenSignature,
	TokenString,
};

enum VersionType {
	Version88a,
	Version91a,
	Version95a,
	Version96a,
};

enum VisibilityType {
	VisibilityAll,          // Visible to every user
	VisibilityUnregistered, // Visible only to unregistered users
	VisibilityRegistered,   // Visible only to registered users
	VisibilityNone,         // Visible to no one
};

static constexpr const char* EOL = "\r\n";
static constexpr const char* Version = UHS_VERSION;

namespace Strings {

typedef std::map<const std::string, const std::string> CharacterMap;

constexpr int NaN = INT_MIN;

bool isInt(const std::string& s);
int toInt(const std::string& s);
std::string ltrim(const std::string& s, char c);
std::string rtrim(const std::string& s, char c);
std::vector<std::string> split(const std::string& s, const std::string sep, int n = 0);
std::string join(const std::vector<std::string>& s, const std::string sep);

}

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
	const std::string& body() const;
	void body(const std::string s);

private:
	std::string _body;
};

class Title {
public:
	const std::string title() const;
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
	VisibilityType _visibility = VisibilityAll;
};

}

class Error {
public:
	Error() = default;
	Error(ErrorType t);
	Error(ErrorType t, std::string s);
	virtual ~Error() = default;
	int type() const;
	void type(ErrorType t);
	const std::string message() const;
	void message(const std::string s);
	void messagef(const char* format, ...);
	void finalize();
	void finalize(int line, int column);

private:
	int _type = ErrorUnknown;
	std::string _message;
};

class Pipe {
public:
	typedef std::function<void(const char*, std::streamsize n)> Handler;

	Pipe(std::ifstream& in);
	virtual ~Pipe() = default;
	void addHandler(Handler h);
	void read();
	bool good();
	bool eof();
	std::shared_ptr<Error> error();

private:
	static const std::size_t ReadLen = 1024;
	std::ifstream& _in;
	std::size_t _offset = 0;
	std::vector<Handler> _handlers;
	std::shared_ptr<Error> _err;
};

class CRC {
public:
	static const int Size = 2;

	CRC(std::shared_ptr<Pipe> p);
	virtual ~CRC() = default;
	void update(const char* buf, std::streamsize n);
	void finalize();
	bool valid();

private:
	static const int TableLen = 256;
	static const uint16_t Polynomial = 0x8005;
	static const uint16_t CastMask = 0xFFFF;
	static const uint16_t MSBMask = 0x8000;
	static const uint16_t FinalXor = 0x0100;

	std::shared_ptr<Pipe> _pipe;
	uint16_t _table[TableLen];
	char _buf[2]; // Checksum buffer
	int _bufLen = 0;
	uint16_t _rem = 0x0000;

	void createTable();
	void calculate(const char* buf, std::streamsize n);
	uint8_t reflectByte(uint8_t byte);
	uint16_t checksum();
};

class Tokenizer;

class Token {
public:
	static constexpr const char* ParagraphSep = " "; // e.g., "text.\r\n \r\nText"

	static const std::string typeString(TokenType t);
	
	Token(const TokenType tokenType, std::size_t offset = 0, int line = 0,
		std::size_t column = 0, std::string value = "");
	virtual ~Token() = default;
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string& value() const;
	const std::string typeString() const;
	const std::string toString() const;
	friend std::ostream& operator<<(std::ostream& out, const Token& t);

private:
	friend class Tokenizer;

	static constexpr const char* Signature = "UHS";
	static constexpr const char* HeaderSep = "** END OF 88A FORMAT **";
	static constexpr const char* CreditSep = "CREDITS:";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static const char DataSep = '\x1A';

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
	Tokenizer(std::shared_ptr<Pipe> p);
	virtual ~Tokenizer() = default;
	void tokenize(const char* buf, std::streamsize n);
	bool hasNext();
	std::shared_ptr<Token> next();
	std::shared_ptr<Error> error();

private:
	class TokenChannel {
	public:
		TokenChannel();
		virtual ~TokenChannel() = default;
		bool send(std::shared_ptr<Token> t);
		std::shared_ptr<Token> receive();
		bool empty();
		bool ok();
		void close();

	private:
		std::queue<std::shared_ptr<Token>> _queue;
		std::mutex _mutex;
		bool _open = true;
	};

	const std::regex _descriptorRegex {"^([0-9]+) ([a-z]{4,})$"};
	const std::regex _dataAddressRegex {"^0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _hyperpngRegionRegex {"^(-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) (-?[0-9]{3,}) (-?[0-9]{3,})$"};

	std::shared_ptr<Pipe> _pipe;
	std::shared_ptr<Error> _err;
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
	void tokenizeMatches(const std::smatch& m, const std::vector<TokenType>& tokens);
	void tokenizeData(const std::string& data, std::size_t column);
	void tokenizeCRC(const std::string& crc, std::size_t column);
	void tokenizeEOF(std::size_t column);
};

template <typename T>
class NodeIterator;

class Node : public std::enable_shared_from_this<Node> {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<const Node>;

	static const std::string typeString(NodeType t);

	Node(NodeType t);
	virtual ~Node() = default;
	NodeType nodeType() const;
	const std::string nodeTypeString() const;
	void appendChild(std::shared_ptr<Node> n);
	std::shared_ptr<Node> parent() const;
	bool hasNextSibling() const;
	std::shared_ptr<Node> nextSibling() const;
	bool hasFirstChild() const;
	std::shared_ptr<Node> firstChild() const;
	bool hasLastChild() const;
	std::shared_ptr<Node> lastChild() const;
	int depth() const;

	// Iterators
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

private:
	NodeType _nodeType;
	std::shared_ptr<Node> _parent;
	std::shared_ptr<Node> _nextSibling;
	std::shared_ptr<Node> _firstChild;
	std::shared_ptr<Node> _lastChild;
	mutable int _depth = -1;
};

template <typename T>
class NodeIterator {
public:
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = std::shared_ptr<T>;
	using reference = T&;
	using iterator_category = std::forward_iterator_tag;

	NodeIterator() = default;
	NodeIterator(pointer n);
	reference operator*() const;
	pointer operator->() const;
	NodeIterator<T>& operator++();
	NodeIterator<T> operator++(int);
	bool operator==(const NodeIterator<T>& rhs);
	bool operator!=(const NodeIterator<T>& rhs);

private:
	pointer _current;
	bool _visited = false;
};

typedef uint8_t Format;

class TextNode
	: public Node
	, public Traits::Body
{
public:
	TextNode();
	virtual ~TextNode() = default;
	const std::string& toString() const;
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
	, public Traits::Visibility
{
public:
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType t);

	Element(ElementType t, int index, int length = 0);
	virtual ~Element() = default;
	ElementType elementType() const;
	const std::string elementTypeString() const;
	void appendString(const std::string s);
	int index() const;
	void index(int i);
	int length() const;
	void length(int l);
	const std::weak_ptr<Element> ref() const;
	void ref(const std::weak_ptr<Element> ref);
	bool isMedia() const;
	const std::string mediaExt() const;

private:
	ElementType _elementType;
	int _index = 0;
	int _length = 0;
	std::weak_ptr<Element> _ref;
};

class Document
	: public Node
	, public Traits::Attributes
	, public Traits::Title
	, public Traits::Visibility
{
public:
	Document();
	Document(VersionType version);
	virtual ~Document() = default;
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
	Codec() = default;
	virtual ~Codec() = default;
	const std::string decode88a(std::string encoded) const;
	const std::string decode96a(std::string encoded, std::string key, bool isTextElement, bool createKey = false) const;
	const std::string createKey(std::string secret) const;

private:
	static const char AsciiStart = 0x20;
	static const char AsciiEnd = 0x7F;
	static constexpr const char* KeySeed = "key";

	int keystream(std::string key, std::size_t keyLen, std::size_t index, bool isText) const;
	bool isPrintable(int c) const;
	char toPrintable(int c) const;
};

struct ParserOptions {
	VersionType version {Version96a};
	bool debug {false};
};

class Parser {
public:
	Parser(std::ifstream& in, const ParserOptions& opt);
	virtual ~Parser() = default;
	std::shared_ptr<Error> error();
	std::shared_ptr<Document> parse();

private:
	typedef std::map<const int, std::shared_ptr<Node>> NodeMap;
	typedef std::map<const int, std::shared_ptr<Element>> ElementMap;
	typedef std::function<void(std::string)> DataCallback;

	struct NodeRange {
		std::shared_ptr<Node> node;
		int min;
		int max;

		NodeRange(std::shared_ptr<Node> n, int min, int max);
		virtual ~NodeRange() = default;
	};

	struct NodeRangeList {
		std::vector<std::shared_ptr<NodeRange>> data; // TODO: Remove unnecessary shared_ptr

		NodeRangeList();
		virtual ~NodeRangeList() = default;
		std::shared_ptr<Node> find(int min, int max);
		void add(std::shared_ptr<Node> n, int min, int max);
	};

	struct LinkData {
		const std::shared_ptr<Token> fromToken;
		const std::shared_ptr<Element> fromElement;
		int toIndex;

		LinkData(const std::shared_ptr<Token> fromToken, const std::shared_ptr<Element> fromElement, int toIndex);
		virtual ~LinkData() = default;
	};

	struct DataHandler {
		std::size_t offset;
		std::size_t length;
		DataCallback func;

		DataHandler(std::size_t offset, std::size_t length, DataCallback func);
		virtual ~DataHandler() = default;
	};

	static const int HeaderLen = 4;
	static const int FormatTokenLen = 3;
	static constexpr const char* AsciiEncStartToken = "#a+";
	static constexpr const char* AsciiEncEndToken = "#a-";
	static constexpr const char* HyperlinkStartToken = "#h+";
	static constexpr const char* HyperlinkEndToken = "#h-";
	static constexpr const char* InfoKeyValueSep = "=";
	static constexpr const char* NoticeToken = ">";
	static constexpr const char* NumberSignToken = "##";
	static constexpr const char* ProportionalStartToken = "#p+";
	static constexpr const char* ProportionalEndToken = "#p-";
	static constexpr const char* RegisteredToken = "A";
	static constexpr const char* UnregisteredToken = "Z";
	static constexpr const char* WordWrapStartToken = "#w+";
	static constexpr const char* WordWrapEndToken = "#w-";
	static constexpr const char* WordWrapEndTokenAlt = "#w.";

	VersionType _version = Version96a;
	bool _debug = false;
	std::shared_ptr<Error> _err;
	std::shared_ptr<Pipe> _pipe;
	Tokenizer _tokenizer;
	CRC _crc;
	Codec _codec;
	std::shared_ptr<Document> _document;
	NodeRangeList _parents;
	ElementMap _elements;
	std::map<const int, std::shared_ptr<LinkData>> _deferredLinks; // TODO: unique_ptr with move
	std::vector<DataHandler> _dataHandlers;
	std::string _key;
	int _indexOffset = 0;
	bool _isTitleSet = false;
	bool _done = false;

	// 88a
	bool parse88a();
	bool parse88aElements(int firstHintIndex, NodeMap& parents);
	bool parse88aTextNodes(int lastHintIndex, NodeMap& parents);
	bool parse88aCreditElement(int index);
	void parseHeaderSep(std::shared_ptr<Token> t);

	// 96a
	bool parse96a();
	std::shared_ptr<Element> parseElement(std::shared_ptr<Token> t, bool indexByRegion = false);
	bool parseCommentElement(std::shared_ptr<Element> e);
	bool parseDataElement(std::shared_ptr<Element> e);
	bool parseHintElement(std::shared_ptr<Element> e);
	bool parseHyperpngElement(std::shared_ptr<Element> e);
	bool parseInfoElement(std::shared_ptr<Element> e);
	bool parseIncentiveElement(std::shared_ptr<Element> e);
	bool parseLinkElement(std::shared_ptr<Element> e);
	bool parseOverlayElement(std::shared_ptr<Element> e);
	bool parseSubjectElement(std::shared_ptr<Element> e);
	bool parseTextElement(std::shared_ptr<Element> e);
	bool parseVersionElement(std::shared_ptr<Element> e);

	// Parse helpers
	std::shared_ptr<Token> next();
	std::shared_ptr<Token> expect(TokenType expected);
	bool findAndLinkParent(std::shared_ptr<Element> e, std::shared_ptr<Token> t);
	bool linkOrDefer(std::shared_ptr<Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex);
	bool link(std::shared_ptr<Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex);
	bool handleDeferredLink(int index);
	void addDataCallback(std::size_t offset, std::size_t length, DataCallback func);
	void parseData(std::shared_ptr<Token> t);
	void checkCRC();
	bool parseDate(const std::string& s, std::tm& tm) const;
	bool parseTime(const std::string& s, std::tm& tm) const;
	bool isPunctuation(char c);
	int offsetIndex(int index);

	// Error helpers
	void indexNotFound(std::shared_ptr<Token> t, int index);
	void expectedString(std::shared_ptr<Token> t, std::string expected, std::string found);
	void expected(std::shared_ptr<Token> t, std::string expected);
	void expectedInt(std::shared_ptr<Token> t);
	void unexpected(std::shared_ptr<Token> t);
};

struct WriterOptions {
	bool registered = true; // JSONWriter only
	std::string mediaDir;
};

class Writer {
public:
	Writer(std::ostream& out, const WriterOptions opt = {});
	virtual ~Writer() = default;
	std::shared_ptr<Error> error();
	virtual bool write(const std::shared_ptr<const Document> d) const;

protected:
	std::ostream& _out;
	std::shared_ptr<Error> _err;
	std::string _mediaDir;
	bool _registered;
};

class JSONWriter : public Writer {
public:
	JSONWriter(std::ostream& out, const WriterOptions opt = {});
	virtual ~JSONWriter() = default;
	bool write(const std::shared_ptr<const Document> d) const override;

private:
	Json::Value serialize(const Document& d, Json::Value& root) const;
	void serializeElement(const Element& e, Json::Value& obj) const;
	void serializeDocument(const Document& d, Json::Value& obj) const;
	void serializeMap(const Traits::Attributes::Type& attrs, Json::Value& obj) const;
};

}

#endif

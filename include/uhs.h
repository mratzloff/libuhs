#ifndef UHS_H
#define UHS_H

#define UHS_VERSION "1.0.0-alpha"
#define UHS_MAX_DEPTH 16

#include <ctime>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <regex>
#include <sstream>
#include <string>

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
	TextFormatNone,            // Also used for binary data
	TextFormatPreformatted,
	TextFormatNoneAlt,         // These two values seem to have the same effect
	TextFormatPreformattedAlt, // when rendered, but all four are used
};

enum NodeType {
	NodeElement,
	NodeContainer,
	NodeText,
};

enum TokenType {
	TokenCompatSep,
	TokenCreditSep,
	TokenData,
	TokenDataLength,
	TokenDataOffset,
	TokenDataType,
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

bool isInt(const std::string& s);
int toInt(const std::string& s);
std::string ltrim(const std::string& s, char c);
std::string rtrim(const std::string& s, char c);
std::vector<std::string> split(const std::string& s, const std::string sep, int n = 0);
std::string join(const std::vector<std::string>& s, const std::string sep);

}

class Error {
public:
	Error();
	Error(ErrorType t);
	Error(ErrorType t, std::string s);
	virtual ~Error();
	int type() const;
	void type(ErrorType t);
	const std::string message() const;
	void message(const std::string s);
	void messagef(const char* format, ...);
	void finalize();
	void finalize(int line, int column);

private:
	int _type;
	std::string _message;
};

class Scanner;

class Token {
public:
	static constexpr const char* ParagraphSep = " "; // e.g., "text.\r\n \r\nText"

	static const std::string typeString(TokenType t);
	
	Token(const TokenType tokenType, std::size_t offset = 0, int line = 0,
		std::size_t column = 0, std::string value = "");
	virtual ~Token();
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string& value() const;
	const std::string typeString() const;
	const std::string toString() const;
	friend std::ostream& operator<<(std::ostream& out, const Token& t);

private:
	friend class Scanner;

	static constexpr const char* Signature = "UHS";
	static constexpr const char* CompatSep = "** END OF 88A FORMAT **";
	static constexpr const char* CreditSep = "CREDITS:";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static const char DataSep = '\x1A';

	const TokenType _type;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _value;

	const std::string formatToken() const;
	const std::string formatIntValue() const;
	const std::string formatStringValue() const;
	const std::string formatByteValue() const;
};

class Scanner {
public:
	Scanner(std::istream& in);
	virtual ~Scanner();
	std::shared_ptr<Error> error();
	void scan();
	bool hasNext();
	std::shared_ptr<Token> next();

private:
	class TokenQueue {
	public:
		TokenQueue();
		virtual ~TokenQueue();
		bool send(std::shared_ptr<Token> t);
		std::shared_ptr<Token> receive();
		bool empty();
		bool ok();
		void close();

	private:
		std::queue<std::shared_ptr<Token>> _queue;
		std::mutex _mutex;
		bool _open;
	};

	static const int LineLen = 80;

	const std::regex _descriptorRegex {"^([0-9]+) ([a-z]{4,})$"};
	const std::regex _dataAddressRegex {"^0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _overlayRegionRegex {"^([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) ([0-9]{4,}) ([0-9]{4,})$"};

	std::istream& _in;
	std::shared_ptr<Error> _err;
	TokenQueue _out;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _buf;

	ElementType scanDescriptor(std::smatch m, std::size_t offset);
	void scanData(std::smatch m, std::size_t offset, std::vector<TokenType> tokens);
	void scanDataAddress(std::smatch m, std::size_t offset);
	void scanOverlayRegion(std::smatch m, std::size_t offset);
	void scanOverlayAddress(std::smatch m, std::size_t offset);
	void eof();
	char peek();
	char read();
	void handleReadError();
	std::shared_ptr<Error> formatError(std::shared_ptr<Error> err) const;
};

class Node : public std::enable_shared_from_this<Node> {
public:
	Node(NodeType t);
	virtual ~Node();
	NodeType nodeType() const;
	void appendChild(std::shared_ptr<Node> n);
	std::vector<std::shared_ptr<Node>> children() const;
	std::shared_ptr<Node> parent() const;
	bool hasNextSibling() const;
	std::shared_ptr<Node> nextSibling() const;
	bool hasFirstChild() const;
	std::shared_ptr<Node> firstChild() const;
	bool hasLastChild() const;
	std::shared_ptr<Node> lastChild() const;

private:
	NodeType _nodeType;
	std::shared_ptr<Node> _parent;
	std::shared_ptr<Node> _nextSibling;
	std::shared_ptr<Node> _firstChild;
	std::shared_ptr<Node> _lastChild;
};

class ContainerNode : public Node {
public:
	ContainerNode();
	virtual ~ContainerNode();
};

typedef uint8_t Format;

class TextNode : public Node {
public:
	TextNode();
	virtual ~TextNode();
	const std::string& toString() const;
	const std::string& value() const;
	void value(const std::string v);
	void addFormat(Format f);
	void removeFormat(Format f);
	bool hasFormat(Format f) const;
	Format format() const;

private:
	std::string _value;
	Format _fmt;
};

class Element : public Node {
public:
	static ElementType elementType(const std::string& typeString);
	static const std::string typeString(ElementType t);

	Element(ElementType t, int index, int length = 0);
	virtual ~Element();
	ElementType elementType() const;
	const std::string elementTypeString() const;
	int index();
	void index(int i);
	int length();
	void length(int l);
	bool visible(bool registered) const;
	VisibilityType visibility() const;
	void visibility(VisibilityType v);
	const std::map<std::string, std::string>& attrs() const;
	const std::string& attr(const std::string& key) const;
	void attr(const std::string& key, const std::string value);
	const std::string& value() const;
	void value(const std::string v);

private:
	ElementType _elementType;
	int _index;
	int _length;
	VisibilityType _visibility;
	std::map<std::string, std::string> _attrs;
	std::string _value;
};

class Document {
public:
	Document();
	virtual ~Document();
	void appendChild(std::shared_ptr<Node> n);
	std::string toString() const;
	const std::shared_ptr<Node> root();
	void version(VersionType v);
	VersionType version() const;
	const std::string versionString() const;
	void title(std::string s);
	std::string title() const;
	void length(const std::size_t len);
	std::size_t length() const;
	void timestamp(const std::tm time);
	std::tm timestamp() const;
	const std::string timestampString() const;
	void meta(std::string key, std::string value);
	const std::shared_ptr<std::map<std::string, std::string>> meta() const;
	const std::string meta(std::string key) const;
	void validCRC(bool valid);
	bool validCRC() const;

private:
	std::shared_ptr<Node> _root;
	VersionType _version;
	std::string _title;
	std::size_t _length;
	std::tm _timestamp;
	std::shared_ptr<std::map<std::string, std::string>> _meta;
	bool _validCRC;
};

class Codec {
public:
	Codec();
	virtual ~Codec();
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
	Parser(std::istream& in, const ParserOptions& opt);
	virtual ~Parser();
	std::shared_ptr<Error> error();
	std::shared_ptr<Document> parse();

private:
	typedef std::map<const int, std::shared_ptr<Node>> NodeMap;
	typedef std::map<const int, std::shared_ptr<Element>> ElementMap;

	struct NodeRange {
		std::shared_ptr<Node> node;
		int min;
		int max;

		NodeRange(std::shared_ptr<Node> n, int min, int max);
		virtual ~NodeRange();
	};

	typedef std::vector<std::shared_ptr<NodeRange>> NodeRangeList;

	typedef std::function<void(std::string)> DataCallback;

	struct DataHandler {
		std::size_t offset;
		std::size_t length;
		DataCallback func;

		DataHandler(std::size_t offset, std::size_t length, DataCallback func);
		virtual ~DataHandler();
	};

	static const int HeaderLen = 4;
	static const int FormatTokenLen = 3;
	static constexpr const char* NoticeToken = ">";
	static constexpr const char* InfoKeyValueSep = "=";
	static constexpr const char* InlineStartToken = "#w+";
	static constexpr const char* InlineEndToken = "#w.";
	static constexpr const char* PreformattedStartToken = "#p-";
	static constexpr const char* PreformattedEndToken = "#p+";

	VersionType _version;
	bool _debug;
	std::shared_ptr<Error> _err;
	std::unique_ptr<Scanner> _scanner;
	std::unique_ptr<Codec> _codec;
	std::shared_ptr<Document> _document;
	std::vector<DataHandler> _dataHandlers;
	ElementMap _elementMap;
	std::string _key;
	int _indexOffset;
	bool _isTitleSet;
	bool _done;

	bool parse88a();
	bool parse88aElements(NodeMap& parents, int firstHintIndex);
	bool parse88aTextNodes(NodeMap& parents, int lastHintIndex);
	bool parse88aCreditElement(int index);
	bool parse96a();
	int parseElement(NodeRangeList& parents, std::shared_ptr<Token> t);
	void parseData(std::shared_ptr<Token> t);
	bool parseCommentElement(std::shared_ptr<Element> e);
	bool parseDataElement(std::shared_ptr<Element> e);
	bool parseHintElement(std::shared_ptr<Element> e);
	bool parseInfoElement(std::shared_ptr<Element> e);
	bool parseIncentiveElement(std::shared_ptr<Element> e);
	bool parseLinkElement(std::shared_ptr<Element> e);
	bool parseNesthintElement(std::shared_ptr<Element> e);
	bool parseOverlayElement(std::shared_ptr<Element> e);
	bool parseSubjectElement(std::shared_ptr<Element> e);
	bool parseTextElement(std::shared_ptr<Element> e);
	bool parseVersionElement(std::shared_ptr<Element> e);
	std::shared_ptr<Token> next();
	std::shared_ptr<Token> expect(TokenType expected);

	// Parse helpers
	void addDataCallback(std::size_t offset, std::size_t length, DataCallback func);
	bool parseDate(const std::string& s, std::tm& tm) const;
	bool parseTime(const std::string& s, std::tm& tm) const;
	bool isPunctuation(char c);

	// Error helpers
	void indexNotFound(std::shared_ptr<Token> t, int index);
	void expectedString(std::shared_ptr<Token> t, std::string expected, std::string found);
	void expected(std::shared_ptr<Token> t, std::string expected);
	void expectedInt(std::shared_ptr<Token> t);
	void unexpected(std::shared_ptr<Token> t);
};

struct WriterOptions {
	bool registered {true};
};

class Writer {
public:
	Writer(std::ostream& out, const WriterOptions& opt);
	virtual ~Writer();
	std::shared_ptr<Error> error();
	virtual bool write(std::shared_ptr<Document>) const;

protected:
	std::ostream& _out;
	std::shared_ptr<Error> _err;
	bool _registered;
};

class JSONWriter : public Writer {
public:
	JSONWriter(std::ostream& out, const WriterOptions& opt);
	virtual ~JSONWriter();
	bool write(std::shared_ptr<Document>) const override;
};

}

#endif

#ifndef UHS_H
#define UHS_H

#define UHS_VERSION "1.0.0-alpha"

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

enum ContentType {
	ContentText,
	ContentGIF,
	ContentPNG,
	ContentWAV,
};

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

enum FormatType {
	FormatWhitespace,
	FormatPreformatted,
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

static constexpr const char* Version = UHS_VERSION;

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
	static const std::string typeString(TokenType t);
	
	Token(const TokenType tokenType, std::size_t offset = 0, int line = 0,
		std::size_t column = 0, std::string value = "");
	virtual ~Token();
	TokenType type() const;
	int line() const;
	std::size_t column() const;
	std::size_t offset() const;
	const std::string stringValue() const;
	int intValue() const;
	const std::string typeString() const;
	const std::string toString() const;

private:
	friend class Scanner;

	static constexpr const char* Signature = "UHS";
	static constexpr const char* CompatSep = "** END OF 88A FORMAT **";
	static constexpr const char* CreditSep = "CREDITS:";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static constexpr const char* ParagraphSep = " "; // i.e., "text.\r\n \r\nText"
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

std::ostream& operator<<(std::ostream &out, const Token &t);

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
	const std::regex _dataAddressRegex {"^0{6}(?: [0-9])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _overlayRegionRegex {"^([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) ([0-9]{4,}) ([0-9]{4,})$"};

	std::istream& _in;
	std::shared_ptr<Error> _err;
	TokenQueue _out;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _buf;

	ElementType scanDescriptor(std::string s, std::smatch m, std::size_t offset);
	void scanDataAddress(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayRegion(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayAddress(std::string s, std::smatch m, std::size_t offset);
	void eof();
	char peek();
	char read();
	void handleReadError();
	std::shared_ptr<Error> formatError(std::shared_ptr<Error> err) const;
	bool isNumber(std::string s) const;
	const std::string ltrim(std::string s, char c) const;
	const std::string rtrim(std::string s, char c) const;
};

class Node {
public:
	Node(NodeType t);
	virtual ~Node();
	NodeType type() const;
	void appendChild(std::shared_ptr<Node> n);
	std::shared_ptr<Node> parent() const;
	std::shared_ptr<Node> nextSibling() const;
	std::shared_ptr<Node> firstChild() const;
	std::shared_ptr<Node> lastChild() const;

private:
	NodeType _type;
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
	const std::string toString() const;
	const std::string value();
	void value(std::string v);
	void addFormat(Format f);
	void removeFormat(Format f);
	bool hasFormat(Format f) const;
	Format format() const;

private:
	std::string _val;
	Format _fmt;
};

class Element : public Node {
public:
	enum Visibility {
		VisibleToRegistered,
		VisibleToUnregistered,
	};

	static const std::string contentTypeString(ContentType t);
	static ElementType elementType(std::string element);

	Element(ElementType t, int index, int length = 0);
	virtual ~Element();
	int index();
	void index(int i);
	int length();
	void length(int l);
	ContentType contentType();
	void contentType(ContentType t);
	const std::string value();
	void value(std::string v);

private:
	ElementType _type;
	int _index;
	int _length;
	ContentType _contentType;
	std::string _val;
};

struct Metadata {
	std::size_t length;
	time_t time;
	std::string notice;
	std::map<std::string, std::string> info;
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
	void title(std::string s);
	std::string title() const;
	const std::shared_ptr<Metadata> meta() const;
	void validCRC(bool valid);
	bool validCRC() const;

private:
	std::shared_ptr<Node> _root;
	VersionType _version;
	std::string _title;
	std::shared_ptr<Metadata> _meta;
	bool _validCRC;
};

class Codec {
public:
	Codec();
	virtual ~Codec();
	const std::string decode88a(std::string encoded);
	// const std::string decode96a(std::string encoded, bool isTextNode);

private:
	static const char AsciiStart = 0x20;
	static const char AsciiEnd = 0x7F;
	static constexpr const char* KeySeed = "key";

	std::string _key;
	std::size_t _keyLen;

	bool isPrintable(char c);
};

struct ParserOptions {
	VersionType version {Version96a};
	bool registered {true};
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

	struct NodeRange {
		std::shared_ptr<Node> node;
		int min;
		int max;

		NodeRange(std::shared_ptr<Node> n, int min, int max);
		virtual ~NodeRange();
	};

	typedef std::vector<std::shared_ptr<NodeRange>> NodeRangeList;

	static const int HeaderLen = 4;
	static const int FormatTokenLen = 3;
	static constexpr const char* InlineStartToken = "#w+";
	static constexpr const char* InlineEndToken = "#w.";
	static constexpr const char* PreformattedStartToken = "#p-";
	static constexpr const char* PreformattedEndToken = "#p+";

	VersionType _version;
	bool _registered;
	bool _debug;
	std::shared_ptr<Error> _err;
	std::unique_ptr<Scanner> _scanner;
	std::unique_ptr<Codec> _codec;
	std::shared_ptr<Document> _document;
	bool _isTitleSet;
	bool _done;

	bool parse88a();
	bool parse88aSubjects(NodeMap& parents, int firstHintIndex);
	bool parse88aHints(NodeMap& parents, int lastHintIndex);
	bool parse88aCredits(int index);
	bool parse96a();
	bool parseComment(std::shared_ptr<Element> e);
	bool parseHint(std::shared_ptr<Element> e);
	bool parseSubject(std::shared_ptr<Element> e);
	int parseElement(NodeRangeList& parents, std::shared_ptr<Token> t);
	std::shared_ptr<Token> next();
	std::shared_ptr<Token> expect(TokenType expected);
	void expected(std::shared_ptr<Token> t, std::string expected);
	void expectedInt(std::shared_ptr<Token> t);
	void unexpected(std::shared_ptr<Token> t);
	bool isPunctuation(char c);
};

}

#endif

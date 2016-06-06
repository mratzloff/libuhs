#ifndef UHS_H
#define UHS_H

#include <ctime>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <regex>
#include <string>

namespace UHS {

constexpr const char* Version = "1.0.0-alpha";

typedef uint8_t Format;

enum FormatType {
	FormatWhitespace,
	FormatPreformatted,
};

enum ErrorType {
	ErrorUnknown,
	ErrorEOF,
	ErrorRead,
	ErrorValue,
	ErrorToken,
};

enum IdentType {
	IdentUnknown,
	IdentBlank,
	IdentComment,
	IdentCredit,
	IdentGifa,
	IdentHint,
	IdentHyperpng,
	IdentIncentive,
	IdentInfo,
	IdentLink,
	IdentNesthint,
	IdentOverlay,
	IdentSound,
	IdentSubject,
	IdentText,
	IdentVersion,
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

protected:
	int _type;
	std::string _message;
};

class Token {
public:
	static constexpr const char* Signature = "UHS";
	static constexpr const char* CompatSep = "** END OF 88A FORMAT **";
	static constexpr const char* CreditSep = "CREDITS:";
	static constexpr const char* NestedElementSep = "=";
	static constexpr const char* NestedTextSep = "-";
	static constexpr const char* ParagraphSep = " "; // i.e., "text.\r\n \r\nText"
	static const char DataSep = '\x1A';

	static const std::string typeString(TokenType t);
	static IdentType identType(std::string ident);
	
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

protected:
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

class TokenQueue {
public:
	TokenQueue();
	virtual ~TokenQueue();
	bool send(std::shared_ptr<Token> t);
	std::shared_ptr<Token> receive();
	bool empty();
	bool ok();
	void close();

protected:
	std::queue<std::shared_ptr<Token>> _queue;
	std::mutex _mutex;
	bool _open;
};

class Scanner {
public:
	Scanner(std::istream& in);
	virtual ~Scanner();
	std::shared_ptr<Error> error();
	void scan();
	bool hasNext();
	std::shared_ptr<Token> next();

protected:
	static const int LineLen = 80;

	const std::regex _descriptorRegex {"^([0-9]+) ([a-z]{4,})$"};
	const std::regex _dataAddressRegex {"^0{6}(?: [0-9])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _overlayRegionRegex {"^([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) ([0-9]{4,}) ([0-9]{4,})$"};

	std::shared_ptr<Error> _err;
	std::istream& _in;
	TokenQueue _out;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _buf;

	IdentType scanDescriptor(std::string s, std::smatch m, std::size_t offset);
	void scanDataAddress(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayRegion(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayAddress(std::string s, std::smatch m, std::size_t offset);
	void eof();
	char read();
	std::shared_ptr<Error> formatError(std::shared_ptr<Error> err) const;
	bool isNumber(std::string s) const;
	const std::string ltrim(std::string s, char c) const;
	const std::string rtrim(std::string s, char c) const;
};

class Document;

class Node {
public:
	Node(NodeType t);
	virtual ~Node();
	NodeType type() const;
	std::shared_ptr<Document> document() const;
	void document(std::shared_ptr<Document> d);
	void appendChild(std::shared_ptr<Node> n);
	std::shared_ptr<Node> parent() const;
	std::shared_ptr<Node> nextSibling() const;
	std::shared_ptr<Node> firstChild() const;
	std::shared_ptr<Node> lastChild() const;
	int depth() const;
	void depth(int d);

protected:
	NodeType _type;
	std::shared_ptr<Document> _document;
	std::shared_ptr<Node> _parent;
	std::shared_ptr<Node> _nextSibling;
	std::shared_ptr<Node> _firstChild;
	std::shared_ptr<Node> _lastChild;
	int _depth;
};

class ContainerNode : public Node {
public:
	ContainerNode();
	virtual ~ContainerNode();
};

class TextNode : public Node {
public:
	TextNode();
	virtual ~TextNode();
	const std::string toString() const;
	void addFormat(Format f);
	void removeFormat(Format f);
	bool hasFormat(Format f) const;
	Format format() const;

protected:
	std::string _data;
	Format _fmt;
};

struct Metadata {
	std::size_t length;
	time_t time;
	std::string notice;
	std::map<std::string,std::string> info;
};

class Document {
public:
	Document();
	virtual ~Document();
	void appendChild(std::shared_ptr<Node> n);
	std::string toString();

protected:
	std::unique_ptr<Node> _root;
	VersionType _version;
	std::string _title;
	std::unique_ptr<Metadata> _meta;
	bool _validCRC;
};

class Parser {
public:
	Parser(std::istream& in);
	virtual ~Parser();
	std::shared_ptr<Error> error();
	std::shared_ptr<Document> parse();

protected:
	static const int HeaderLen = 4;
	static const int FormatTokenLen = 3;
	static constexpr const char* InlineStartToken = "#w+";
	static constexpr const char* InlineEndToken = "#w.";
	static constexpr const char* PreformattedStartToken = "#p-";
	static constexpr const char* PreformattedEndToken = "#p+";

	std::shared_ptr<Error> _err;
	VersionType _version;
	bool _registered;
	bool _debug;
	std::unique_ptr<Scanner> _scanner;
	std::shared_ptr<Document> _document;
	std::shared_ptr<Token> _token;
	std::string _key;
	bool _isTitleSet;
	bool _done;

	bool parse88a();
	bool parse96a();
	std::shared_ptr<Token> next();
	std::shared_ptr<Token> expect(TokenType expected);
};

}

#endif

#include "uhs.h"
#include <climits>
#include <iostream>
#include <thread>

namespace UHS {

Parser::Parser(const ParserOptions opt) : _opt{opt} {
	// TODO: Guard these by platform
	setenv("TZ", "", 1);
	tzset();
	// _putenv_s("TZ", ""); // MSVC
	// _tzset(); // MSVC
}

Parser::NodeRange::NodeRange(Node& n, const int min, const int max)
    : node{n}, min{min}, max{max} {}

Node* Parser::NodeRangeList::find(const int min, const int max) {
	Node* n = nullptr;

	for (const auto& nr : data) {
		if (min > nr.min) {
			if (max <= nr.max) {
				n = &nr.node;
			}
		} else {
			break;
		}
	}
	return n;
}

void Parser::NodeRangeList::add(Node& n, const int min, const int max) {
	data.emplace_back(n, min, max);
}

void Parser::NodeRangeList::clear() {
	data.clear();
}

Parser::DataHandler::DataHandler(
    std::size_t offset, std::size_t length, DataCallback func)
    : offset{offset}, length{length}, func{func} {}

Parser::LinkData::LinkData(const int targetLine, const int line, const int column)
    : targetLine{targetLine}, line{line}, column{column} {}

std::unique_ptr<Document> Parser::parse(std::ifstream& in) {
	auto pipe = std::make_unique<Pipe>(in);
	_tokenizer = std::make_unique<Tokenizer>(*pipe);
	_crc = std::make_unique<CRC>();
	_crc->upstream(*pipe);

	// Interleave disk reads with tokenization and CRC calculation
	std::thread thread{[&pipe] { pipe->read(); }};

	try {
		// Meanwhile, build out document by parsing emitted tokens in parallel
		_document = Document::create(VersionType::Version88a);

		this->parse88a();

		if (!_done && !_opt.force88aMode) {
			// Set 88a header as first (hidden) child of 96a document
			_document->visibility(VisibilityType::None);

			auto d = Document::create(VersionType::Version96a);
			_document.swap(d);
			_document->appendChild(std::move(d));

			this->parse96a();
		}

		thread.join();
		return std::move(_document);
	} catch (const Error& err) {
		thread.join();
		throw;
	}
}

void Parser::reset() {
	_tokenizer.reset();
	_crc.reset();
	_document.reset();
	_parents.clear();
	_deferredLinkChecks.clear();
	_dataHandlers.clear();
	_key.clear();
	_lineOffset = 0;
	_isTitleSet = false;
	_done = false;
}

//--------------------------------- UHS 88a ---------------------------------//

void Parser::parse88a() {
	std::unique_ptr<const Token> t;

	// Signature
	t = this->expect(TokenType::Signature);

	// Title
	t = this->expect(TokenType::String);
	_document->title(t->value());

	// First hint line
	t = this->expect(TokenType::Line);
	int firstHintTextLine;
	try {
		firstHintTextLine = Strings::toInt(t->value());
		if (firstHintTextLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	firstHintTextLine += HeaderLen;

	// Last hint line
	t = this->expect(TokenType::Line);
	int lastHintTextLine;
	try {
		lastHintTextLine = Strings::toInt(t->value());
		if (lastHintTextLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	lastHintTextLine += HeaderLen;

	// Subject and hint elements
	NodeMap parents{{0, _document.get()}};
	this->parse88aElements(firstHintTextLine, parents);

	// Hint text nodes
	this->parse88aTextNodes(lastHintTextLine, parents);

	// Credits

	// Anything from this point on is part of the 88a credits
	// until EOF or the backwards compatibility token.
	t = this->next();

	auto tokenType = t->type();
	int line = t->line();
	int column = t->column();

	switch (t->type()) {
	case TokenType::CreditSep: // This is informal but common
		[[fallthrough]];
	case TokenType::String:
		this->parse88aCredits(std::move(t));
		return;
	case TokenType::HeaderSep:
		this->parseHeaderSep(std::move(t));
		return;
	case TokenType::FileEnd:
		_done = true;
		return;
	default:
		throw ParseError::badToken(line, column, tokenType);
	}
}

// This (correctly) errors out on sq4g.uhs, which appears to be miscompiled
// from the original 88a precompile format. The official Windows reader
// doesn't support it, and as far as I can tell no one bothered to ever check
// it. It mostly works in the DOS reader, but it looks glitchy.
void Parser::parse88aElements(int firstHintTextLine, NodeMap& parents) {
	std::unique_ptr<const Token> t;
	auto elementType = ElementType::Subject;

	for (;;) {
		t = this->expect(TokenType::String);
		std::string encodedTitle{t->value()};
		int line = t->line();

		t = this->expect(TokenType::Line);
		int firstChildLine;
		try {
			firstChildLine = Strings::toInt(t->value());
			if (firstChildLine < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			throw ParseError::badValue(
			    t->line(), t->column(), ParseError::Uint, t->value());
		}
		firstChildLine += HeaderLen;

		if (firstChildLine == firstHintTextLine) {
			elementType = ElementType::Hint;
		}

		Node* parent = nullptr;
		for (int i = line; !parent; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (!parent) {
			throw ParseError(t->line(), t->column(), "could not find parent node");
		}

		auto e = Element::create(elementType, line);
		e->line(line);
		auto ptr = e.get();
		parent->appendChild(std::move(e));

		std::string title{_codec.decode88a(encodedTitle)};
		if (parent->nodeType() != NodeType::Document) {
			char finalChar = title[title.length() - 1];
			if (!this->isPunctuation(finalChar)) {
				title += '?';
			}
		}
		ptr->title(title);

		if (line < firstHintTextLine) {
			parents[firstChildLine] = ptr;
		}
		if (line + 2 == firstHintTextLine) {
			break; // Done parsing subjects
		}
	}
}

void Parser::parse88aTextNodes(int lastHintTextLine, NodeMap& parents) {
	std::unique_ptr<const Token> t;
	int line = 0;

	do {
		t = this->expect(TokenType::String);
		line = t->line();

		Node* parent = nullptr;
		for (int i = line; !parent; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (!parent) {
			throw ParseError(t->line(), t->column(), "could not find parent node");
		}
		if (parent->nodeType() != NodeType::Element) {
			throw ParseError(t->line(),
			    t->column(),
			    "unexpected parent type: %s",
			    parent->nodeTypeString());
		}

		auto& element = static_cast<Element&>(*parent);
		element.appendChild(_codec.decode88a(t->value()));
	} while (line < lastHintTextLine);
}

void Parser::parse88aCredits(std::unique_ptr<const Token> t) {
	std::string s;
	bool continuation = false;

	for (;;) {
		switch (t->type()) {
		case TokenType::FileEnd:
			if (!s.empty()) {
				_document->attr("notice", s);
			}
			_done = true;
			return;
		case TokenType::HeaderSep:
			if (!s.empty()) {
				_document->attr("notice", s);
			}
			this->parseHeaderSep(std::move(t));
			return;
		case TokenType::CreditSep:
			break; // Ignore
		case TokenType::String:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		default:
			throw ParseError::badToken(t->line(), t->column(), t->type());
		}

		t = this->next();
	}
}

//--------------------------------- UHS 96a ---------------------------------//

void Parser::parseHeaderSep(std::unique_ptr<const Token> t) {
	if (_opt.force88aMode) {
		_done = true;
		return;
	}
	// 96a element line numbers don't include compatibility header
	_lineOffset = t->line();
}

void Parser::parse96a() {
	std::unique_ptr<const Token> t;
	_parents.add(*_document, 0, INT_MAX);

	// Parse elements
	for (;;) {
		t = this->next();

		switch (t->type()) {
		case TokenType::Length: {
			this->parseElement(std::move(t));
			break;
		}
		case TokenType::Data:
			this->parseData(std::move(t));
			break;
		case TokenType::CRC:
			this->checkCRC();
			break;
		case TokenType::FileEnd:
			_done = true;
			return;
		default:
			throw ParseError::badToken(t->line(), t->column(), t->type());
		}
	}

	this->checkLinks();
}

// Elements are automatically appended to their parents
Element* Parser::parseElement(std::unique_ptr<const Token> t) {
	Element* ptr;

	// Length
	int len;
	try {
		len = Strings::toInt(t->value());
		if (len < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}

	// Ident
	t = this->expect(TokenType::Ident);
	const auto& ident = t->value();

	// Create element
	auto elementType = Element::elementType(ident);
	int line = this->offsetLine(t->line());
	auto e = Element::create(elementType, line);
	e->line(line);
	e->length(len);
	ptr = e.get();

	this->findParentAndAppend(std::move(e), std::move(t));

	// Title
	t = this->expect(TokenType::String);
	ptr->title(t->value());

	switch (elementType) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		this->parseCommentElement(ptr);
		break;
	case ElementType::Credit:
		this->parseCommentElement(ptr);
		break;
	case ElementType::Gifa:
		this->parseDataElement(ptr);
		break;
	case ElementType::Hint:
		this->parseHintElement(ptr);
		break;
	case ElementType::Hyperpng:
		this->parseHyperpngElement(ptr);
		break;
	case ElementType::Incentive:
		this->parseIncentiveElement(ptr);
		break;
	case ElementType::Info:
		this->parseInfoElement(ptr);
		break;
	case ElementType::Link:
		this->parseLinkElement(ptr);
		break;
	case ElementType::Nesthint:
		this->parseHintElement(ptr);
		break;
	case ElementType::Overlay:
		this->parseOverlayElement(ptr);
		break;
	case ElementType::Sound:
		this->parseDataElement(ptr);
		break;
	case ElementType::Subject:
		this->parseSubjectElement(ptr);
		break;
	case ElementType::Text:
		this->parseTextElement(ptr);
		break;
	case ElementType::Version:
		this->parseVersionElement(ptr);
		break;
	}

	return ptr;
}

void Parser::parseCommentElement(Element* const e) {
	std::unique_ptr<const Token> t;

	int len = e->length();
	std::string s;
	bool continuation = false;

	for (int i = 3; i <= len; ++i) {
		t = this->next();

		switch (t->type()) {
		case TokenType::String:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		case TokenType::NestedTextSep:
			[[fallthrough]];
		case TokenType::NestedParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		default:
			throw ParseError::badToken(t->line(), t->column(), t->type());
		}
	}

	e->body(s);
}

void Parser::parseDataElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Offset
	std::size_t offset;

	for (;;) {
		auto t = this->next();

		switch (t->type()) {
		case TokenType::DataOffset:
			int intOffset;
			try {
				intOffset = Strings::toInt(t->value());
				if (intOffset < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(
				    t->line(), t->column(), ParseError::Uint, t->value());
			}
			offset = intOffset;
			goto expectDataLength;
		case TokenType::TextFormat:
			continue; // Ignore
		default:
			throw ParseError::badToken(t->line(), t->column(), t->type());
		}
	}

expectDataLength:
	// Length
	t = this->expect(TokenType::DataLength);
	int intLen;
	try {
		intLen = Strings::toInt(t->value());
		if (intLen < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) { e->body(data); });
}

// A hint element ending with a nested text separator ("-") should be an
// error, but bluforce.uhs has an instance of this. This actually screws
// up the official reader UI for that particular hint.
void Parser::parseHintElement(Element* const e) {
	std::unique_ptr<const Token> t;
	std::string s;

	// Parse child elements
	int len = e->length();
	bool continuation = false;

	for (int i = 3; i <= len; ++i) {
		t = this->next();

		switch (t->type()) {
		case TokenType::String:
			if (continuation) {
				s += ' ';
			}
			if (e->elementType() == ElementType::Nesthint) {
				s += _codec.decode96a(t->value(), _key, false);
			} else {
				s += _codec.decode88a(t->value());
			}
			continuation = true;
			break;
		case TokenType::NestedTextSep:
			if (!s.empty()) {
				e->appendChild(s);
				s.clear();
			}
			continuation = false;
			break;
		case TokenType::NestedParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		case TokenType::NestedElementSep:
			if (i == len || e->elementType() != ElementType::Nesthint) {
				throw ParseError::badValue(
				    t->line(), t->column(), ParseError::String, t->value());
			}

			// Append current text node
			if (!s.empty()) {
				e->appendChild(s);
				s.clear();
			}
			continuation = false;

			// Length
			t = this->expect(TokenType::Length);

			{ // Child element
				const auto child = this->parseElement(std::move(t));
				i += child->length();
			}
			break;
		default:
			throw ParseError::badToken(t->line(), t->column(), t->type());
		}
	}

	// Append current text node
	if (!s.empty()) {
		e->appendChild(s);
	}
}

void Parser::parseHyperpngElement(Element* const e) {
	std::unique_ptr<const Token> t;

	this->parseDataElement(e);

	// Parse child elements
	int len = e->length();
	int childLen = 0;

	for (int i = 4; i < len; i += 1 + childLen) {
		// Interactive region top-left X coordinate
		t = this->expect(TokenType::CoordX);
		auto x1 = t->value();
		try {
			Strings::toInt(x1);
		} catch (const Error& err) {
			throw ParseError::badValue(t->line(), t->column(), ParseError::Int, x1);
		}

		// Interactive region top-left Y coordinate
		t = this->expect(TokenType::CoordY);
		auto y1 = t->value();
		try {
			Strings::toInt(y1);
		} catch (const Error& err) {
			throw ParseError::badValue(t->line(), t->column(), ParseError::Int, y1);
		}

		// Interactive region bottom-right X coordinate
		t = this->expect(TokenType::CoordX);
		auto x2 = t->value();
		try {
			Strings::toInt(x2);
		} catch (const Error& err) {
			throw ParseError::badValue(t->line(), t->column(), ParseError::Int, x2);
		}

		// Interactive region bottom-right Y coordinate
		t = this->expect(TokenType::CoordY);
		auto y2 = t->value();
		try {
			Strings::toInt(y2);
		} catch (const Error& err) {
			throw ParseError::badValue(t->line(), t->column(), ParseError::Int, y2);
		}

		// Child element (overlay or link)
		t = this->expect(TokenType::Length);
		const auto child = this->parseElement(std::move(t));
		childLen = child->length();

		// Backfill coordinates
		child->attr("region:top-left-x", x1);
		child->attr("region:top-left-y", y1);
		child->attr("region:bottom-right-x", x2);
		child->attr("region:bottom-right-y", y2);
	}
}

// A bad instruction ("12") is found at the beginning or end of the incentive
// list for four files, so we skip bad instructions of that form. Fourteen
// other files have lines pointing to nowhere, so we skip those, too.
void Parser::parseIncentiveElement(Element* const e) {
	std::unique_ptr<const Token> t;
	std::string s;

	e->visibility(VisibilityType::None);

	// Read and decode visibility instructions
	int len = e->length();
	bool continuation = false;

	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenType::String);
		if (continuation) {
			s += ' ';
		}
		s += _codec.decode96a(t->value(), _key, false);
		continuation = true;
	}
	e->body(s);

	if (s.empty()) {
		return; // No instructions to process
	}

	// Process instructions
	auto markers = Strings::split(s, " ");
	std::size_t markerLen;

	for (const auto& marker : markers) {
		markerLen = marker.length();

		// Split instruction (e.g., "3Z")
		if (markerLen < 2) {
			continue; // TODO: Warn
		}
		auto lineStr = marker.substr(0, markerLen - 1);
		auto instruction = marker.substr(markerLen - 1, 1);

		int targetLine;
		try {
			targetLine = Strings::toInt(lineStr);
			if (targetLine < 0) {
				throw Error();
			}
		} catch (const Error& err) {
			continue; // TODO: Warn
		}

		// Set visibility
		if (auto target = this->findTarget(targetLine)) {
			if (instruction == Token::Registered) {
				target->visibility(VisibilityType::Registered);
			} else if (instruction == Token::Unregistered) {
				target->visibility(VisibilityType::Unregistered);
			}
		}
	}
}

void Parser::parseInfoElement(Element* const e) {
	std::unique_ptr<const Token> t;
	std::string s;
	std::string key;
	std::string val;
	std::string date;
	std::tm tm;

	e->visibility(VisibilityType::None);

	// Key-value pairs followed by a notice
	int len = e->length();
	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenType::String);
		s = t->value();

		if (s.substr(0, 1) == Token::NoticePrefix) {
			key = "notice";
			val = s.substr(1);
		} else {
			auto parts = Strings::split(s, Token::InfoKeyValueSep, 2);
			if (parts.size() != 2) {
				throw ParseError::badValue(
				    t->line(), t->column(), "key-value pair", t->value());
			}
			key = parts[0];
			val = parts[1];
		}

		if (key == "length") {
			int intVal;
			try {
				intVal = Strings::toInt(val);
				if (intVal < 0) {
					throw Error();
				}
			} catch (const Error& err) {
				throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, val);
			}
			_document->attr("length", std::to_string(intVal));
		} else if (key == "date") {
			try {
				this->parseDate(val, tm);
			} catch (const Error& err) {
				std::throw_with_nested(
				    ParseError::badValue(t->line(), t->column(), ParseError::Date, val));
			}
		} else if (key == "time") {
			try {
				this->parseTime(val, tm);
			} catch (const Error& err) {
				std::throw_with_nested(
				    ParseError::badValue(t->line(), t->column(), ParseError::Time, val));
			}
			char buf[20];
			auto len = std::strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", &tm);
			_document->attr("timestamp", std::string(buf, len));
		} else {
			if (auto currentValue = _document->attr(key)) {
				_document->attr(key, *currentValue + " " + val);
			} else {
				_document->attr(key, val);
			}
		}
	}
}

void Parser::parseLinkElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Ref line
	t = this->expect(TokenType::Line);
	auto body = t->value();
	int targetLine;
	try {
		targetLine = Strings::toInt(body);
		if (targetLine < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, body);
	}
	e->body(body);

	this->deferLinkCheck(targetLine, t->line(), t->column());
}

void Parser::parseOverlayElement(Element* const e) {
	std::unique_ptr<const Token> t;

	this->parseDataElement(e);

	// Image top-left X coordinate
	t = this->expect(TokenType::CoordX);
	const auto x = t->value();
	try {
		Strings::toInt(x);
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Int, x);
	}
	e->attr("image:x", x);

	// Image bottom-right Y coordinate
	t = this->expect(TokenType::CoordY);
	const auto y = t->value();
	try {
		Strings::toInt(y);
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Int, y);
	}
	e->attr("image:y", y);
}

void Parser::parseSubjectElement(Element* const e) {
	std::unique_ptr<const Token> t;

	if (!_isTitleSet) {
		_document->title(e->title());
		_key = _codec.createKey(_document->title());
		_isTitleSet = true;
	}

	const int len = e->length();
	int childLen = 0;

	for (int i = 3; i <= len; i += childLen) {
		// Length
		t = this->expect(TokenType::Length);

		// Child element
		const auto child = this->parseElement(std::move(t));
		childLen = child->length();
	}
}

void Parser::parseTextElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Format
	t = this->expect(TokenType::TextFormat);
	int format;
	try {
		format = Strings::toInt(t->value());
		if (format < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	if (format > 3) {
		throw ParseError(t->line(),
		    t->column(),
		    "text format byte must be between 0 and 3; found %d",
		    format);
	}
	const auto formatType = static_cast<TextFormatType>(format);

	if (formatType == TextFormatType::Monospace
	    || formatType == TextFormatType::MonospaceAlt) {
		e->attr("typeface", "monospace");
	} else {
		e->attr("typeface", "proportional");
	}

	// Offset
	t = this->expect(TokenType::DataOffset);
	int intOffset;
	try {
		intOffset = Strings::toInt(t->value());
		if (intOffset < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	std::size_t offset = intOffset;

	// Length
	t = this->expect(TokenType::DataLength);
	int intLen;
	try {
		intLen = Strings::toInt(t->value());
		if (intLen < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw ParseError::badValue(t->line(), t->column(), ParseError::Uint, t->value());
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) {
		auto lines = Strings::split(data, UHS::EOL);
		for (auto& line : lines) {
			line = _codec.decode96a(line, _key, true);
			if (line == Token::ParagraphSep) {
				line = "";
			}
		}
		auto value = Strings::rtrim(Strings::join(lines, "\n"), '\n');
		e->body(value);
	});
}

void Parser::parseVersionElement(Element* const e) {
	VersionType v = VersionType::Version96a;

	e->visibility(VisibilityType::None);
	this->parseCommentElement(e);

	auto versionStr = e->title();
	if (versionStr == "91a") {
		v = VersionType::Version91a;
	} else if (versionStr == "95a") {
		v = VersionType::Version95a;
	}
	_document->version(v);
}

std::unique_ptr<const Token> Parser::next() {
	auto token = _tokenizer->next();

	if (_opt.debug) {
		std::cout << token->string() << std::endl;
	}
	return token;
}

std::unique_ptr<const Token> Parser::expect(TokenType expected) {
	auto token = this->next();
	auto tokenType = token->type();

	if (tokenType == TokenType::FileEnd && expected != TokenType::FileEnd) {
		throw ParseError(token->line(), token->column(), "unexpected end of file");
	} else if (tokenType != expected) {
		throw ParseError::badToken(
		    token->line(), token->column(), expected, token->type());
	}
	return token;
}

void Parser::findParentAndAppend(
    std::unique_ptr<Element> e, std::unique_ptr<const Token> t) {
	assert(e);
	assert(t);

	int min = e->line();
	int max = min + e->length();
	auto parent = _parents.find(min, max);
	_parents.add(*e, min, max);

	if (!parent) {
		throw ParseError(t->line(), t->column(), "orphaned element");
	}
	parent->appendChild(std::move(e));
}

void Parser::deferLinkCheck(int targetLine, const int line, const int column) {
	_deferredLinkChecks.emplace_back(targetLine, line, column);
}

void Parser::checkLinks() {
	for (const auto [targetLine, line, column] : _deferredLinkChecks) {
		if (const auto target = this->findTarget(targetLine); !target) {
			// TODO: Warning using ld.line and ld.column
		}
	}
}

// A link pointing to the child of a hyperpng element is a special case. Instead of
// referencing the descriptor line, it points to the region, even if it doesn't
// share a parent hyperpng. A link outside of a hyperpng pointing to a child of
// a hyperpng references the descriptor line, as usual.
Element* Parser::findTarget(const int line) {
	auto target = _document->find(line);
	if (!target) {
		target = _document->find(line + 1);
		if (!target) {
			return nullptr;
		}
		if (const auto parent = target->parent()) {
			const auto& parentElement = static_cast<Element&>(*parent);
			if (parentElement.elementType() != ElementType::Hyperpng) {
				return nullptr; // We found an element, but shouldn't have
			}
		}
	}
	return target;
}

void Parser::addDataCallback(std::size_t offset, std::size_t length, DataCallback func) {
	_dataHandlers.push_back(DataHandler(offset, length, func));
}

void Parser::parseData(std::unique_ptr<const Token> t) {
	std::size_t offset;

	for (const auto& handler : _dataHandlers) {
		offset = handler.offset - t->offset();
		handler.func(t->value().substr(offset, handler.length));
	}
}

void Parser::checkCRC() {
	_document->validChecksum(_crc->valid());
}

// Format: DD-Mon-YY
void Parser::parseDate(const std::string& s, std::tm& tm) const {
	auto parts = Strings::split(s, "-", 3);

	int year;
	try {
		year = Strings::toInt(parts[2]);
		if (year < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid year: %d", year);
	}
	if (year < 95) { // Info elements were introduced in 1995
		year += 100;
	}

	int month;
	if (parts[1] == "Jan") {
		month = 0;
	} else if (parts[1] == "Feb") {
		month = 1;
	} else if (parts[1] == "Mar") {
		month = 2;
	} else if (parts[1] == "Apr") {
		month = 3;
	} else if (parts[1] == "May") {
		month = 4;
	} else if (parts[1] == "Jun") {
		month = 5;
	} else if (parts[1] == "Jul") {
		month = 6;
	} else if (parts[1] == "Aug") {
		month = 7;
	} else if (parts[1] == "Sep") {
		month = 8;
	} else if (parts[1] == "Oct") {
		month = 9;
	} else if (parts[1] == "Nov") {
		month = 10;
	} else if (parts[1] == "Dec") {
		month = 11;
	} else {
		throw Error("invalid month: %d", parts[1]);
	}

	int day;
	try {
		day = Strings::toInt(parts[0]);
		if (day < 1 || day > 31) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid day: %d", day);
	}

	tm.tm_year = year;
	tm.tm_mon = month;
	tm.tm_mday = day;
}

// Format: HH:MM:SS
void Parser::parseTime(const std::string& s, std::tm& tm) const {
	auto parts = Strings::split(s, ":", 3);

	int hour;
	try {
		hour = Strings::toInt(parts[0]);
		if (hour < 0 || hour > 23) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid hour: %d", hour);
	}

	int min;
	try {
		min = Strings::toInt(parts[1]);
		if (min < 0 || min > 59) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid minute: %d", min);
	}

	int sec;
	try {
		sec = Strings::toInt(parts[2]);
		if (sec < 0 || sec > 59) {
			throw Error();
		}
	} catch (const Error& err) {
		throw Error("invalid second: %d", sec);
	}

	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
}

bool Parser::isPunctuation(char c) {
	return c == '?' || c == '!' || c == '.' || c == ',' || c == ';';
}

int Parser::offsetLine(int line) {
	return line - _lineOffset;
}

} // namespace UHS

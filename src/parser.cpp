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

Parser::LinkData::LinkData(
    Element* sourceElement, const int targetLine, const int line, const int column)
    : sourceElement{sourceElement}, targetLine{targetLine}, line{line}, column{column} {}

std::unique_ptr<Error> Parser::error() {
	return std::move(_err);
}

std::unique_ptr<Document> Parser::parse(std::ifstream& in) {
	_pipe = std::make_unique<Pipe>(in);
	_tokenizer = std::make_unique<Tokenizer>(*_pipe);
	_crc = std::make_unique<CRC>();
	_crc->upstream(*_pipe.get());

	// Interleave disk reads with tokenization and CRC calculation
	std::thread thread{[&] { _pipe->read(); }};

	// Meanwhile, build out document by parsing emitted tokens in parallel
	_document = std::make_unique<Document>(VersionType::Version88a);
	bool ok = this->parse88a();
	if (!ok || _done) {
		goto exit;
	}

	if (!_opt.force88aMode) {
		// Set 88a header as first (hidden) child of 96a document
		_document->visibility(VisibilityType::None);

		auto d = std::make_unique<Document>(VersionType::Version96a);
		_document.swap(d);
		_document->appendChild(std::move(d));

		ok = this->parse96a();
		if (!ok) {
			goto exit;
		}
	}

exit:
	thread.join();
	return std::move(_document);
}

void Parser::reset() {
	_err.reset();
	_pipe.reset();
	_tokenizer.reset();
	_crc.reset();
	_document.reset();
	_parents.clear();
	_elements.clear();
	_deferredLinks.clear();
	_dataHandlers.clear();
	_key.clear();
	_lineOffset = 0;
	_isTitleSet = false;
	_done = false;
}

//--------------------------------- UHS 88a ---------------------------------//

bool Parser::parse88a() {
	std::unique_ptr<const Token> t;

	// Signature
	t = this->expect(TokenType::Signature);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	// Title
	t = this->expect(TokenType::String);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	_document->title(t->value());

	// First hint line
	t = this->expect(TokenType::Line);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int firstHintTextLine = Strings::toInt(t->value());
	if (firstHintTextLine < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	firstHintTextLine += HeaderLen;

	// Last hint line
	t = this->expect(TokenType::Line);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int lastHintTextLine = Strings::toInt(t->value());
	if (lastHintTextLine < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	lastHintTextLine += HeaderLen;

	// Subject and hint elements
	NodeMap parents{{0, _document.get()}};
	bool ok = this->parse88aElements(firstHintTextLine, parents);
	if (!ok) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	// Hint text nodes
	ok = this->parse88aTextNodes(lastHintTextLine, parents);
	if (!ok) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	// Credits

	// Anything from this point on is part of the 88a credits
	// until EOF or the backwards compatibility token.
	t = this->next();
	if (_err != nullptr) {
		return false;
	}

	auto tokenType = t->type();
	int line = t->line();
	int column = t->column();

	switch (t->type()) {
	case TokenType::CreditSep: // This is informal but common
		[[fallthrough]];
	case TokenType::String:
		ok = this->parse88aCredits(std::move(t));
		if (!ok) {
			this->unexpected(tokenType, line, column);
		}
		return ok;
	case TokenType::HeaderSep:
		this->parseHeaderSep(std::move(t));
		return true;
	case TokenType::FileEnd:
		_done = true;
		return true;
	default:
		this->unexpected(tokenType, line, column);
		return false;
	}
}

// This (correctly) errors out on sq4g.uhs, which appears to be miscompiled
// from the original 88a precompile format. The official Windows reader
// doesn't support it, and as far as I can tell no one bothered to ever check
// it. It mostly works in the DOS reader, but it looks glitchy.
bool Parser::parse88aElements(int firstHintTextLine, NodeMap& parents) {
	std::unique_ptr<const Token> t;
	auto elementType = ElementType::Subject;

	while (true) {
		t = this->expect(TokenType::String);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		std::string encodedTitle{t->value()};
		int line = t->line();

		t = this->expect(TokenType::Line);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		int firstChildLine = Strings::toInt(t->value());
		if (firstChildLine < 0) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}
		firstChildLine += HeaderLen;

		if (firstChildLine == firstHintTextLine) {
			elementType = ElementType::Hint;
		}

		Node* parent = nullptr;
		for (int i = line; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}

		if (parent == nullptr) {
			_err =
			    std::make_unique<Error>(ErrorType::Value, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto e = std::make_unique<Element>(elementType, line);
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
	return true;
}

bool Parser::parse88aTextNodes(int lastHintTextLine, NodeMap& parents) {
	std::unique_ptr<const Token> t;

	while (true) {
		t = this->expect(TokenType::String);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		int line = t->line();

		Node* parent = nullptr;
		for (int i = line; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (parent == nullptr) {
			_err =
			    std::make_unique<Error>(ErrorType::Value, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto n = std::make_unique<TextNode>();
		n->body(_codec.decode88a(t->value()));
		parent->appendChild(std::move(n));

		if (line == lastHintTextLine) {
			break; // Done parsing hints
		}
	}
	return true;
}

bool Parser::parse88aCredits(std::unique_ptr<const Token> t) {
	// Body
	std::string s;
	bool continuation = false;

	while (true) {
		switch (t->type()) {
		case TokenType::FileEnd:
			if (!s.empty()) {
				_document->attr("notice", s);
			}
			_done = true;
			return true;
		case TokenType::HeaderSep:
			if (!s.empty()) {
				_document->attr("notice", s);
			}
			this->parseHeaderSep(std::move(t));
			return true;
		case TokenType::CreditSep:
			// Ignore
			break;
		case TokenType::String:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		default:
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}

		t = this->next();
		if (_err != nullptr) {
			return false;
		}
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

bool Parser::parse96a() {
	std::unique_ptr<const Token> t;
	_parents.add(*_document, 0, INT_MAX);

	// Parse elements
	while (true) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenType::Length: {
			const auto e = this->parseElement(std::move(t));
			if (e == nullptr) {
				return false;
			}
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
			return true;
		default:
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}
	}
	return true;
}

// Elements are automatically appended to their parents
Element* Parser::parseElement(std::unique_ptr<const Token> t, bool indexByRegion) {
	bool ok = false;

	// Length
	int len = Strings::toInt(t->value());
	if (len < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return nullptr;
	}

	// Ident
	t = this->expect(TokenType::Ident);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return nullptr;
	}
	std::string ident{t->value()};

	// Create element
	auto elementType = Element::elementType(ident);
	int line = this->offsetLine(t->line());
	auto e = std::make_unique<Element>(elementType, line, len);
	auto ptr = e.get();

	// Store a reference for Link and Incentive elements
	_elements[line] = ptr;

	// Internal hyperpng links refer to region line instead of element line
	if (indexByRegion) {
		_elements[line - 1] = ptr;
	}

	ok = this->findParentAndAppend(std::move(e), std::move(t));
	if (!ok) {
		return nullptr;
	}

	ok = handleDeferredLink(line);
	if (!ok) {
		return nullptr;
	}

	// Title
	t = this->expect(TokenType::String);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return nullptr;
	}
	ptr->title(t->value());

	switch (elementType) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		ok = this->parseCommentElement(ptr);
		break;
	case ElementType::Credit:
		ok = this->parseCommentElement(ptr);
		break;
	case ElementType::Gifa:
		ok = this->parseDataElement(ptr);
		break;
	case ElementType::Hint:
		ok = this->parseHintElement(ptr);
		break;
	case ElementType::Hyperpng:
		ok = this->parseHyperpngElement(ptr);
		break;
	case ElementType::Incentive:
		ok = this->parseIncentiveElement(ptr);
		break;
	case ElementType::Info:
		ok = this->parseInfoElement(ptr);
		break;
	case ElementType::Link:
		ok = this->parseLinkElement(ptr);
		break;
	case ElementType::Nesthint:
		ok = this->parseHintElement(ptr);
		break;
	case ElementType::Overlay:
		ok = this->parseOverlayElement(ptr);
		break;
	case ElementType::Sound:
		ok = this->parseDataElement(ptr);
		break;
	case ElementType::Subject:
		ok = this->parseSubjectElement(ptr);
		break;
	case ElementType::Text:
		ok = this->parseTextElement(ptr);
		break;
	case ElementType::Version:
		ok = this->parseVersionElement(ptr);
		break;
	}

	if (!ok) {
		return nullptr;
	}
	return ptr;
}

bool Parser::parseCommentElement(Element* const e) {
	std::unique_ptr<const Token> t;

	int len = e->length();
	std::string s;
	bool continuation = false;

	for (int i = 3; i <= len; ++i) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

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
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}
	}
	e->body(s);

	return true;
}

bool Parser::parseDataElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Offset
	std::size_t offset;
	int intOffset;

	while (true) {
		auto t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenType::DataOffset:
			intOffset = Strings::toInt(t->value());
			if (intOffset < 0) {
				this->expectedInt(t->value(), t->line(), t->column());
				return false;
			}
			offset = intOffset;
			goto expectDataLength;
		case TokenType::TextFormat:
			continue; // Ignore
		default:
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}
	}

expectDataLength:
	// Length
	t = this->expect(TokenType::DataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) { e->body(data); });

	return true;
}

// A hint element ending with a nested text separator ("-") should be an
// error, but bluforce.uhs has an instance of this. This actually screws
// up the official reader UI for that particular hint.
bool Parser::parseHintElement(Element* const e) {
	std::unique_ptr<const Token> t;
	std::string s;

	// Parse child elements
	int len = e->length();
	bool continuation = false;

	for (int i = 3; i <= len; ++i) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

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
				e->appendString(s);
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
				this->expected("string", t->value(), t->line(), t->column());
				return false;
			}

			// Append current text node
			if (!s.empty()) {
				e->appendString(s);
				s.clear();
			}
			continuation = false;

			// Length
			t = this->expect(TokenType::Length);
			if (_err != nullptr) {
				if (_err->type() == ErrorType::FileEnd) {
					this->unexpected(t->type(), t->line(), t->column());
				}
				return false;
			}

			{ // Child element
				const auto child = this->parseElement(std::move(t));
				if (child == nullptr) {
					return false;
				}
				i += child->length();
			}
			break;
		default:
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}
	}

	// Append current text node
	if (!s.empty()) {
		e->appendString(s);
	}

	return true;
}

bool Parser::parseHyperpngElement(Element* const e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (!ok) {
		return false;
	}

	// Parse child elements
	int len = e->length();
	int childLen = 0;

	for (int i = 4; i < len; i += 1 + childLen) {
		// Interactive region top-left X coordinate
		t = this->expect(TokenType::CoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		auto x1 = t->value();
		if (Strings::toInt(x1) == Strings::NaN) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}

		// Interactive region top-left Y coordinate
		t = this->expect(TokenType::CoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		auto y1 = t->value();
		if (Strings::toInt(y1) == Strings::NaN) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}

		// Interactive region bottom-right X coordinate
		t = this->expect(TokenType::CoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		auto x2 = t->value();
		if (Strings::toInt(x2) == Strings::NaN) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}

		// Interactive region bottom-right Y coordinate
		t = this->expect(TokenType::CoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		auto y2 = t->value();
		if (Strings::toInt(y2) == Strings::NaN) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}

		// Child element (overlay or link)
		t = this->expect(TokenType::Length);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		const auto child = this->parseElement(std::move(t), true);
		if (child == nullptr) {
			return false;
		}
		childLen = child->length();

		// Backfill coordinates
		child->attr("region:top-left-x", x1);
		child->attr("region:top-left-y", y1);
		child->attr("region:bottom-right-x", x2);
		child->attr("region:bottom-right-y", y2);
	}

	return true;
}

// A bad instruction ("12") is found at the beginning or end of the incentive
// list for four files, so we skip bad instructions of that form. Fourteen
// other files have lines pointing to nowhere, so we skip those, too.
bool Parser::parseIncentiveElement(Element* const e) {
	std::unique_ptr<const Token> t;
	std::string s;

	e->visibility(VisibilityType::None);

	// Read and decode visibility instructions
	int len = e->length();
	bool continuation = false;

	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenType::String);
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		if (continuation) {
			s += ' ';
		}
		s += _codec.decode96a(t->value(), _key, false);
		continuation = true;
	}
	e->body(s);

	if (s.empty()) {
		return true; // No instructions to process
	}

	// Process instructions
	auto markers = Strings::split(s, " ");
	std::size_t markerLen;

	for (const auto& marker : markers) {
		markerLen = marker.length();

		// Split instruction (e.g., "3Z")
		if (markerLen < 2) {
			this->expected("incentive instruction", marker, t->line(), t->column());
			return false;
		}
		auto lineStr = marker.substr(0, markerLen - 1);
		auto instruction = marker.substr(markerLen - 1, 1);

		int line = Strings::toInt(lineStr);
		if (line < 0) {
			this->expected("valid incentive reference", lineStr, t->line(), t->column());
			return false;
		}

		// Look up referenced element
		Element* ref = nullptr;
		try {
			ref = _elements.at(line);
		} catch (const std::out_of_range& ex) {
			// Skip bad instructions
			continue;
		}

		// Set visibility
		if (instruction == Token::Registered) {
			ref->visibility(VisibilityType::Registered);
		} else if (instruction == Token::Unregistered) {
			ref->visibility(VisibilityType::Unregistered);
		} else {
			// Skip bad instructions
		}
	}

	return true;
}

bool Parser::parseInfoElement(Element* const e) {
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
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		s = t->value();

		if (s.substr(0, 1) == Token::NoticePrefix) {
			key = "notice";
			val = s.substr(1);
		} else {
			auto parts = Strings::split(s, Token::InfoKeyValueSep, 2);
			if (parts.size() != 2) {
				this->expected(
				    Token::InfoKeyValueSep, t->value(), t->line(), t->column());
				return false;
			}
			key = parts[0];
			val = parts[1];
		}

		if (key == "length") {
			int intVal = Strings::toInt(val);
			if (intVal < 0) {
				this->expectedInt(t->value(), t->line(), t->column());
				return false;
			}
			_document->attr("length", std::to_string(intVal));
		} else if (key == "date") {
			bool ok = this->parseDate(val, tm);
			if (!ok) {
				this->expected("valid date", t->value(), t->line(), t->column());
				return false;
			}
		} else if (key == "time") {
			bool ok = this->parseTime(val, tm);
			if (!ok) {
				this->expected("valid time", t->value(), t->line(), t->column());
				return false;
			}
			char buf[20];
			auto len = std::strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", &tm);
			_document->attr("timestamp", std::string(buf, len));
		} else {
			auto v = _document->attr(key);
			if (!v.empty()) {
				v += ' ';
			}
			v += val;
			_document->attr(key, v);
		}
	}

	return true;
}

bool Parser::parseLinkElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Ref line
	t = this->expect(TokenType::Line);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	auto body = t->value();
	const int targetLine = Strings::toInt(body);
	if (targetLine < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->body(body);

	return this->linkOrDefer(e, targetLine, t->line(), t->column());
}

bool Parser::parseOverlayElement(Element* const e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (!ok) {
		return false;
	}

	// Image top-left X coordinate
	t = this->expect(TokenType::CoordX);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	const auto x = t->value();
	if (Strings::toInt(x) == Strings::NaN) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->attr("image:x", x);

	// Image bottom-right Y coordinate
	t = this->expect(TokenType::CoordY);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	const auto y = t->value();
	if (Strings::toInt(y) == Strings::NaN) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->attr("image:y", y);

	return true;
}

bool Parser::parseSubjectElement(Element* const e) {
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
		if (_err != nullptr) {
			if (_err->type() == ErrorType::FileEnd) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}

		// Child element
		const auto child = this->parseElement(std::move(t));
		if (child == nullptr) {
			return false;
		}
		childLen = child->length();
	}

	return true;
}

bool Parser::parseTextElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Format
	t = this->expect(TokenType::TextFormat);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	const int format = Strings::toInt(t->value());
	if (format < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	if (format > 3) {
		_err = std::make_unique<Error>(ErrorType::Value);
		_err->messagef("text format byte must be between 0 and 3; found %d", format);
		_err->finalize(t->line(), t->column());
		return false;
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
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	const int intOffset = Strings::toInt(t->value());
	if (intOffset < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	std::size_t offset = intOffset;

	// Length
	t = this->expect(TokenType::DataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorType::FileEnd) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	const int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
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

	return true;
}

bool Parser::parseVersionElement(Element* const e) {
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

	return true;
}

std::unique_ptr<const Token> Parser::next() {
	auto t = _tokenizer->next();
	if (t == nullptr) {
		auto err = _tokenizer->error();
		if (err != nullptr) {
			_err = std::move(err);
			return t;
		}
	}
	if (_opt.debug) {
		std::cout << t->string() << std::endl;
	}
	return t;
}

std::unique_ptr<const Token> Parser::expect(TokenType expected) {
	auto t = this->next();
	if (_err != nullptr) {
		return t;
	}

	if (t->type() == TokenType::FileEnd && expected != TokenType::FileEnd) {
		_err = std::make_unique<Error>(ErrorType::FileEnd);
		_err->finalize(t->line(), t->column());
	} else if (t->type() != expected) {
		_err = std::make_unique<Error>(ErrorType::Token);
		_err->messagef("expected %s, found %s",
		    Token::typeString(expected).data(),
		    t->typeString().data());
		_err->finalize(t->line(), t->column());
	}
	return t;
}

bool Parser::findParentAndAppend(
    std::unique_ptr<Element> e, std::unique_ptr<const Token> t) {
	if (e == nullptr) {
		return false;
	}

	int min = e->line();
	int max = min + e->length();
	auto parent = _parents.find(min, max);
	_parents.add(*e, min, max);

	if (parent == nullptr) {
		_err = std::make_unique<Error>(ErrorType::Value, "orphaned element");
		_err->finalize(t->line(), t->column());
		return false;
	}
	parent->appendChild(std::move(e));

	return true;
}

bool Parser::linkOrDefer(
    Element* sourceElement, const int targetLine, const int line, const int column) {
	if (sourceElement == nullptr) {
		return false;
	}

	if (sourceElement->line() > targetLine) {
		return this->link(sourceElement, targetLine, line, column);
	} else {
		_deferredLinks.emplace(std::piecewise_construct,
		    std::make_tuple(targetLine),
		    std::make_tuple(sourceElement, targetLine, line, column));
	}
	return true;
}

bool Parser::link(
    Element* sourceElement, const int targetLine, const int line, const int column) {
	if (sourceElement == nullptr) {
		return false;
	}

	Element* ref = nullptr;
	try {
		ref = _elements.at(targetLine);
	} catch (const std::out_of_range& ex) {
		this->lineNotFound(targetLine, line, column);
		return false;
	}
	sourceElement->ref(ref);

	return true;
}

bool Parser::handleDeferredLink(int line) {
	if (_deferredLinks.count(line) == 0) {
		return true;
	}
	const auto& ld = _deferredLinks[line];
	return this->link(ld.sourceElement, ld.targetLine, ld.line, ld.column);
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
bool Parser::parseDate(const std::string& s, std::tm& tm) const {
	auto parts = Strings::split(s, "-", 3);

	int year = Strings::toInt(parts[2]);
	if (year < 0) {
		return false;
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
		return false;
	}

	int day = Strings::toInt(parts[0]);
	if (day < 1 || day > 31) {
		return false;
	}

	tm.tm_year = year;
	tm.tm_mon = month;
	tm.tm_mday = day;

	return true;
}

// Format: HH:MM:SS
bool Parser::parseTime(const std::string& s, std::tm& tm) const {
	auto parts = Strings::split(s, ":", 3);

	int hour = Strings::toInt(parts[0]);
	if (hour < 0 || hour > 23) {
		return false;
	}

	int min = Strings::toInt(parts[1]);
	if (min < 0 || min > 59) {
		return false;
	}

	int sec = Strings::toInt(parts[2]);
	if (sec < 0 || sec > 59) {
		return false;
	}

	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;

	return true;
}

bool Parser::isPunctuation(char c) {
	return c == '?' || c == '!' || c == '.' || c == ',' || c == ';';
}

int Parser::offsetLine(int line) {
	return line - _lineOffset;
}

void Parser::lineNotFound(int targetLine, int line, int column) {
	_err = std::make_unique<Error>(ErrorType::Value);
	_err->messagef("line not found: %d", targetLine);
	_err->finalize(line, column);
}

void Parser::expected(std::string expected, std::string found, int line, int column) {
	_err = std::make_unique<Error>(ErrorType::Value);
	_err->messagef("expected %s, found '%s'", expected.data(), found.data());
	_err->finalize(line, column);
}

void Parser::expectedInt(std::string found, int line, int column) {
	this->expected("valid integer", found, line, column);
}

void Parser::unexpected(TokenType type, int line, int column) {
	_err = std::make_unique<Error>(ErrorType::Token);
	_err->messagef("unexpected %s token", Token::typeString(type).data());
	_err->finalize(line, column);
}

} // namespace UHS

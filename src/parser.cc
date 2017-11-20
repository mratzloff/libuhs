#include <climits>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Parser::Parser(std::ifstream& in, const ParserOptions& opt)
	: _version {opt.version}
	, _debug {opt.debug}
	, _pipe {Pipe(in)}
	, _tokenizer {_pipe}
	, _crc {_pipe}
{}

Parser::NodeRange::NodeRange(std::shared_ptr<Node> n, int min, int max)
	: node {n}, min {min}, max {max} {}

Parser::NodeRangeList::NodeRangeList() : data({}) {}

std::shared_ptr<Node> Parser::NodeRangeList::find(int min, int max) {
	std::shared_ptr<Node> n;

	for (auto nr : data) {
		if (min > nr->min) {
			if (max <= nr->max) {
				n = nr->node;
			}
		} else {
			break;
		}
	}
	return n;
}

void Parser::NodeRangeList::add(std::shared_ptr<Node> n, int min, int max) {
	data.push_back(std::make_shared<NodeRange>(n, min, max));
}

Parser::DataHandler::DataHandler(std::size_t offset, std::size_t length, DataCallback func)
	: offset {offset}
	, length {length}
	, func {func}
{}

Parser::LinkData::LinkData(
		std::unique_ptr<const Token> fromToken, const std::shared_ptr<Element> fromElement, int toIndex)
	: fromToken {std::move(fromToken)}
	, fromElement {fromElement}
	, toIndex {toIndex}
{}

const Error* Parser::error() {
	return _err.get();
}

std::shared_ptr<Document> Parser::parse() {
	// Interleave disk reads with tokenization and CRC calculation
	std::thread thread {[&] {
		_pipe.read();
	}};

	// Meanwhile, build out document by parsing emitted tokens in parallel
	_document = std::make_shared<Document>(Version88a);
	bool ok = this->parse88a();
	if (! ok || _done) {
		thread.join();
		return _document;
	}

	if (_version != Version88a) {
		// Set 88a header as first (hidden) child of 96a document
		_document->visibility(VisibilityNone);
		auto document = std::make_shared<Document>(Version96a);
		document->appendChild(_document);
		_document = document;

		ok = this->parse96a();
		if (! ok) {
			thread.join();
			return _document;
		}
	}
	
	thread.join();
	return _document;
}

//--------------------------------- UHS 88a ---------------------------------//

bool Parser::parse88a() {
	std::unique_ptr<const Token> t;

	// Signature
	t = this->expect(TokenSignature);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}

	// Title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	_document->title(t->value());

	// First hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	int firstHintIndex = Strings::toInt(t->value());
	if (firstHintIndex < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	firstHintIndex += HeaderLen;

	// Last hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	int lastHintIndex = Strings::toInt(t->value());
	if (lastHintIndex < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	lastHintIndex += HeaderLen;

	// Subjects
	NodeMap parents {{0, _document}};
	bool ok = this->parse88aElements(firstHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}

	// Hints
	ok = this->parse88aTextNodes(lastHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
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

	switch (t->type()) {
	case TokenCreditSep: // This is informal but common
		// Fall through
	case TokenString:
		ok = this->parse88aCreditElement(std::move(t));
		if (! ok) {
			this->unexpected(std::move(t));
		}
		return ok;
	case TokenHeaderSep:
		this->parseHeaderSep(std::move(t));
		return true;
	case TokenEOF:
		_done = true;
		return true;
	default:
		this->unexpected(std::move(t));
		return false;
	}
}

// This (correctly) errors out on sq4g.uhs, which appears to be miscompiled
// from the original 88a precompile format. The official Windows reader
// doesn't support it, and as far as I can tell no one bothered to ever check
// it. It mostly works in the DOS reader, but it looks glitchy.
bool Parser::parse88aElements(int firstHintIndex, NodeMap& parents) {
	std::unique_ptr<const Token> t;
	ElementType elementType {ElementSubject};

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		std::string encodedTitle {t->value()};
		int index = t->line();

		t = this->expect(TokenIndex);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		int firstChildIndex = Strings::toInt(t->value());
		if (firstChildIndex < 0) {
			this->expectedInt(std::move(t));
			return false;
		}
		firstChildIndex += HeaderLen;

		if (firstChildIndex == firstHintIndex) {
			elementType = ElementHint;
		}

		std::shared_ptr<Node> parent;
		for (int i = index; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}

		if (parent == nullptr) {
			_err = std::make_unique<Error>(ErrorValue, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto e = std::make_shared<Element>(elementType, index);

		std::string title {_codec.decode88a(encodedTitle)};
		if (parent->nodeType() != NodeDocument) {
			char finalChar = title[title.length()-1];
			if (!this->isPunctuation(finalChar)) {
				title += '?';
			}
		}
		e->title(title);

		parent->appendChild(e);

		if (index < firstHintIndex) {
			parents[firstChildIndex] = e;
		}
		if (index + 2 == firstHintIndex) {
			break; // Done parsing subjects
		}
	}
	return true;
}

bool Parser::parse88aTextNodes(int lastHintIndex, NodeMap& parents) {
	std::unique_ptr<const Token> t;

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		int index = t->line();

		std::shared_ptr<Node> parent;
		for (int i = index; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (parent == nullptr) {
			_err = std::make_unique<Error>(ErrorValue, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto n = std::make_shared<TextNode>();
		n->body(_codec.decode88a(t->value()));
		parent->appendChild(n);

		if (index == lastHintIndex) {
			break; // Done parsing hints
		}
	}
	return true;
}

bool Parser::parse88aCreditElement(std::unique_ptr<const Token> t) {
	auto e = std::make_shared<Element>(ElementCredit, t->line());
	e->title("Credits");
	_document->appendChild(e);

	// Body
	std::string s;
	bool continuation = false;

	while (true) {
		switch (t->type()) {
		case TokenEOF:
			if (! s.empty()) {
				e->appendString(s);
			}
			_done = true;
			return true;
		case TokenHeaderSep:
			this->parseHeaderSep(std::move(t));
			if (! s.empty()) {
				e->appendString(s);
			}
			return true;
		case TokenCreditSep:
			// Ignore
			break;
		case TokenString:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		default:
			this->unexpected(std::move(t));
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
	if (_version == Version88a) {
		_done = true;
		return;
	}
	// 96a element indexes don't include compatibility header
	_indexOffset = t->line();
}

bool Parser::parse96a() {
	std::unique_ptr<const Token> t;
	std::shared_ptr<Element> e;

	_parents.add(_document, 0, INT_MAX);

	// Parse elements
	while (true) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenLength:
			e = this->parseElement(std::move(t));
			if (e == nullptr) {
				return false;
			}
			break;
		case TokenData:
			this->parseData(std::move(t));
			break;
		case TokenCRC:
			this->checkCRC();
			break;
		case TokenEOF:
			_done = true;
			return true;
		default:
			this->unexpected(std::move(t));
			return false;
		}
	}
	return true;
}

// Elements are automatically appended to their parents
std::shared_ptr<Element> Parser::parseElement(std::unique_ptr<const Token> t, bool indexByRegion) {
	bool ok = false;

	// Length
	int len = Strings::toInt(t->value());
	if (len < 0) {
		this->expectedInt(std::move(t));
		return nullptr;
	}

	// Ident
	t = this->expect(TokenIdent);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return nullptr;
	}
	std::string ident {t->value()};

	// Create element
	auto elementType = Element::elementType(ident);
	int index = this->offsetIndex(t->line());
	auto e = std::make_shared<Element>(elementType, index, len);

	// Store a reference for Link and Incentive elements
	_elements[index] = e;

	// Internal hyperpng links refer to region index instead of element index
	if (indexByRegion) {
		_elements[index-1] = e;
	}

	ok = this->findAndLinkParent(e, std::move(t));
	if (! ok) {
		return nullptr;
	}

	ok = handleDeferredLink(index);
	if (! ok) {
		return nullptr;
	}

	// Title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return nullptr;
	}
	e->title(t->value());

	switch (elementType) {
	case ElementUnknown:   /* No further processing required */ break;
	case ElementBlank:     /* No further processing required */ break;
	case ElementComment:   ok = this->parseCommentElement(e);   break;
	case ElementCredit:    ok = this->parseCommentElement(e);   break;
	case ElementGifa:      ok = this->parseDataElement(e);      break;
	case ElementHint:      ok = this->parseHintElement(e);      break;
	case ElementHyperpng:  ok = this->parseHyperpngElement(e);  break;
	case ElementIncentive: ok = this->parseIncentiveElement(e); break;
	case ElementInfo:      ok = this->parseInfoElement(e);      break;
	case ElementLink:      ok = this->parseLinkElement(e);      break;
	case ElementNesthint:  ok = this->parseHintElement(e);      break;
	case ElementOverlay:   ok = this->parseOverlayElement(e);   break;
	case ElementSound:     ok = this->parseDataElement(e);      break;
	case ElementSubject:   ok = this->parseSubjectElement(e);   break;
	case ElementText:      ok = this->parseTextElement(e);      break;
	case ElementVersion:   ok = this->parseVersionElement(e);   break;
	}

	if (! ok) {
		return nullptr;
	}
	return e;
}

bool Parser::parseCommentElement(std::shared_ptr<Element> e) {
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
		case TokenString:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		case TokenNestedTextSep:
			// Fall through
		case TokenParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		default:
			this->unexpected(std::move(t));
			return false;
		}
	}
	e->body(s);

	return true;
}

bool Parser::parseDataElement(std::shared_ptr<Element> e) {
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
		case TokenDataType:
			continue; // Ignore
		case TokenDataOffset:
			intOffset = Strings::toInt(t->value());
			if (intOffset < 0) {
				this->expectedInt(std::move(t));
				return false;
			}
			offset = intOffset;
			goto expectDataLength;
		default:
			this->unexpected(std::move(t));
			return false;
		}
	}

expectDataLength:
	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) {
		e->body(data);
	});

	return true;
}

// A hint element ending with a nested text separator ("-") should be an
// error, but bluforce.uhs has an instance of this. This actually screws
// up the official reader UI for that particular hint.
bool Parser::parseHintElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;
	std::shared_ptr<Element> child;
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
		case TokenString:
			if (continuation) {
				s += ' ';
			}
			if (e->elementType() == ElementNesthint) {
				s += _codec.decode96a(t->value(), _key, false);
			} else {
				s += _codec.decode88a(t->value());
			}
			continuation = true;
			break;
		case TokenNestedTextSep:
			if (! s.empty()) {
				e->appendString(s);
				s.clear();
			}
			continuation = false;
			break;
		case TokenParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		case TokenNestedElementSep:
			if (i == len || e->elementType() != ElementNesthint) {
				this->expected(std::move(t), "string");
				return false;
			}

			// Append current text node
			if (! s.empty()) {
				e->appendString(s);
				s.clear();
			}
			continuation = false;

			// Length
			t = this->expect(TokenLength);
			if (_err != nullptr) {
				if (_err->type() == ErrorEOF) {
					this->unexpected(std::move(t));
				}
				return false;
			}

			// Child element
			child = this->parseElement(std::move(t));
			if (child == nullptr) {
				return false;
			}
			i += child->length();

			break;
		default:
			this->unexpected(std::move(t));
			return false;
		}
	}

	// Append current text node
	if (! s.empty()) {
		e->appendString(s);
	}

	return true;
}

bool Parser::parseHyperpngElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (! ok) {
		return false;
	}

	// Parse child elements
	int len = e->length();
	std::shared_ptr<Element> child;

	for (int i = 4; i < len; i += 1 + child->length()) {
		// Interactive region top-left X coordinate
		t = this->expect(TokenCoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		auto x1 = t->value();
		if (Strings::toInt(x1) == Strings::NaN) {
			this->expectedInt(std::move(t));
			return false;
		}

		// Interactive region top-left Y coordinate
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		auto y1 = t->value();
		if (Strings::toInt(y1) == Strings::NaN) {
			this->expectedInt(std::move(t));
			return false;
		}

		// Interactive region bottom-right X coordinate
		t = this->expect(TokenCoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		auto x2 = t->value();
		if (Strings::toInt(x2) == Strings::NaN) {
			this->expectedInt(std::move(t));
			return false;
		}

		// Interactive region bottom-right Y coordinate
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		auto y2 = t->value();
		if (Strings::toInt(y2) == Strings::NaN) {
			this->expectedInt(std::move(t));
			return false;
		}

		// Child element (overlay or link)
		t = this->expect(TokenLength);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		child = this->parseElement(std::move(t), true);
		if (child == nullptr) {
			return false;
		}

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
// other files have indexes pointing to nowhere, so we skip those, too.
bool Parser::parseIncentiveElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;
	std::string s;

	e->visibility(VisibilityNone);

	// Read and decode visibility instructions
	int len = e->length();
	bool continuation = false;

	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
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
			this->expectedString(std::move(t), "incentive instruction", marker);
			return false;
		}
		auto indexString = marker.substr(0, markerLen-1);
		auto instruction = marker.substr(markerLen-1, 1);

		int index = Strings::toInt(indexString);
		if (index < 0) {
			this->expectedString(std::move(t), "valid incentive index", indexString);
			return false;
		}

		// Look up referenced element
		std::shared_ptr<Element> ref;
		try {
			ref = _elements.at(index);
		} catch (const std::out_of_range& ex) {
			// Skip bad instructions
			continue;
		}

		// Set visibility
		if (instruction == Token::Registered) {
			ref->visibility(VisibilityRegistered);
		} else if (instruction == Token::Unregistered) {
			ref->visibility(VisibilityUnregistered);
		} else {
			// Skip bad instructions
		}
	}

	return true;
}

bool Parser::parseInfoElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;
	std::string s;
	std::string key;
	std::string val;
	std::string date;
	std::tm tm;

	e->visibility(VisibilityNone);

	// Key-value pairs followed by a notice
	int len = e->length();
	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}
		s = t->value();

		if (s.substr(0, 1) == Token::Notice) {
			key = "notice";
			val = s.substr(1);
		} else {
			auto parts = Strings::split(s, Token::InfoKeyValueSep, 2);
			if (parts.size() != 2) {
				this->expected(std::move(t), Token::InfoKeyValueSep);
				return false;
			}
			key = parts[0];
			val = parts[1];
		}

		if (key == "length") {
			int intVal = Strings::toInt(val);
			if (intVal < 0) {
				this->expectedInt(std::move(t));
				return false;
			}
			_document->attr("length", std::to_string(intVal));
		} else if (key == "date") {
			bool ok = this->parseDate(val, tm);
			if (! ok) {
				this->expected(std::move(t), "valid date");
				return false;
			}
		} else if (key == "time") {
			bool ok = this->parseTime(val, tm);
			if (! ok) {
				this->expected(std::move(t), "valid time");
				return false;
			}
			auto time = mktime(&tm);
			if (time == -1) {
				this->expected(std::move(t), "valid timestamp");
				return false;
			}
			char buf[20];
			auto len = std::strftime(buf, 20, "%Y-%m-%dT%H:%M:%S", &tm);
			_document->attr("timestamp", std::string(buf, len));
		} else {
			auto v = _document->attr(key);
			if (! v.empty()) {
				v += ' ';
			}
			v += val;
			_document->attr(key, v);
		}
	}

	return true;
}

bool Parser::parseLinkElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;

	// Ref index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	auto body = t->value();
	int refIndex = Strings::toInt(body);
	if (refIndex < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	e->body(body);

	return this->linkOrDefer(std::move(t), e, refIndex);
}

bool Parser::parseOverlayElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (! ok) {
		return false;
	}

	// Image top-left X coordinate
	t = this->expect(TokenCoordX);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	auto x = t->value();
	if (Strings::toInt(x) == Strings::NaN) {
		this->expectedInt(std::move(t));
		return false;
	}
	e->attr("image:x", x);

	// Image bottom-right Y coordinate
	t = this->expect(TokenCoordY);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	auto y = t->value();
	if (Strings::toInt(y) == Strings::NaN) {
		this->expectedInt(std::move(t));
		return false;
	}
	e->attr("image:y", y);

	return true;
}

bool Parser::parseSubjectElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;
	std::shared_ptr<Element> child;

	if (! _isTitleSet) {
		_document->title(e->title());
		_key = _codec.createKey(_document->title());
		_isTitleSet = true;
	}
	int len = e->length();

	for (int i = 3; i <= len; i += child->length()) {
		// Length
		t = this->expect(TokenLength);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(std::move(t));
			}
			return false;
		}

		// Child element
		child = this->parseElement(std::move(t));
		if (child == nullptr) {
			return false;
		}
	}

	return true;
}

bool Parser::parseTextElement(std::shared_ptr<Element> e) {
	std::unique_ptr<const Token> t;

	// Format
	t = this->expect(TokenDataType);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}

	int format = Strings::toInt(t->value());
	if (format < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	if (format == TextFormatMonospace || format == TextFormatMonospaceAlt) {
		e->attr("preformatted", "true");
	}

	// Offset
	t = this->expect(TokenDataOffset);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	int intOffset = Strings::toInt(t->value());
	if (intOffset < 0) {
		this->expectedInt(std::move(t));
		return false;
	}
	std::size_t offset = intOffset;

	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(std::move(t));
		}
		return false;
	}
	int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(std::move(t));
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

bool Parser::parseVersionElement(std::shared_ptr<Element> e) {
	VersionType v = Version96a;

	e->visibility(VisibilityNone);
	this->parseCommentElement(e);

	auto versionString = e->title();
	if (versionString == "91a") {
		v = Version91a;
	} else if (versionString == "95a") {
		v = Version95a;
	}
	_document->version(v);

	return true;
}

std::unique_ptr<const Token> Parser::next() {
	auto t = _tokenizer.next();
	if (t == nullptr) {
		auto err = _tokenizer.error();
		if (err != nullptr) {
			_err = std::move(err);
			return t;
		}
	}
	if (_debug) {
		std::cout << t->toString() << std::endl;
	}
	return t;
}

std::unique_ptr<const Token> Parser::expect(TokenType expected) {
	auto t = this->next();
	if (_err != nullptr) {
		return t;
	}

	if (t->type() == TokenEOF && expected != TokenEOF) {
		_err = std::make_unique<Error>(ErrorEOF);
		_err->finalize(t->line(), t->column());
	} else if (t->type() != expected) {
		_err = std::make_unique<Error>(ErrorToken);
		_err->messagef("expected %s, found %s",
			Token::typeString(expected).data(), t->typeString().data());
		_err->finalize(t->line(), t->column());
	}
	return t;
}

bool Parser::findAndLinkParent(std::shared_ptr<Element> e, std::unique_ptr<const Token> t) {
	int min = e->index();
	int max = min + e->length();
	auto parent = _parents.find(min, max);

	if (parent == nullptr) {
		_err = std::make_unique<Error>(ErrorValue, "orphaned element");
		_err->finalize(t->line(), t->column());
		return false;
	}
	parent->appendChild(e);
	_parents.add(e, min, max);

	return true;
}

bool Parser::linkOrDefer(std::unique_ptr<const Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex) {
	if (fromElement->index() > toIndex) {
		return this->link(std::move(fromToken), fromElement, toIndex);
	} else {
		_deferredLinks[toIndex] = std::make_shared<LinkData>(std::move(fromToken), fromElement, toIndex);
	}
	return true;
}

bool Parser::link(std::unique_ptr<const Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex) {
	std::weak_ptr<Element> ref; 

	try {
		ref = _elements.at(toIndex);
	} catch (const std::out_of_range& ex) {
		this->indexNotFound(std::move(fromToken), toIndex);
		return false;
	}
	fromElement->ref(ref);

	return true;
}

bool Parser::handleDeferredLink(int index) {
	if (_deferredLinks.count(index) == 0) {
		return true;
	}
	auto linkData = _deferredLinks[index];
	return this->link(
		std::move(linkData->fromToken)
		, linkData->fromElement
		, linkData->toIndex);
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
	_crc.finalize();
	_document->validChecksum(_crc.valid());
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

int Parser::offsetIndex(int index) {
	return index - _indexOffset;
}

void Parser::indexNotFound(std::unique_ptr<const Token> t, int index) {
	_err = std::make_unique<Error>(ErrorValue);
	_err->messagef("index not found: %d", index);
	_err->finalize(t->line(), t->column());
}

void Parser::expectedString(std::unique_ptr<const Token> t, std::string expected, std::string found) {
	_err = std::make_unique<Error>(ErrorValue);
	_err->messagef("expected %s, found '%s'", expected.data(), found.data());
	_err->finalize(t->line(), t->column());
}

void Parser::expected(std::unique_ptr<const Token> t, std::string expected) {
	this->expectedString(std::move(t), expected, t->value());
}

void Parser::expectedInt(std::unique_ptr<const Token> t) {
	this->expected(std::move(t), "valid integer");
}

void Parser::unexpected(std::unique_ptr<const Token> t) {
	_err = std::make_unique<Error>(ErrorToken);
	_err->messagef("unexpected %s token", t->typeString().data());
	_err->finalize(t->line(), t->column());
}

}

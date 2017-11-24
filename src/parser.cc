#include <climits>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Parser::Parser(const ParserOptions opt) : _opt {opt} {}

Parser::NodeRange::NodeRange(Node& n, int min, int max)
	: node {n}, min {min}, max {max} {}

Parser::NodeRangeList::NodeRangeList() : data({}) {}

Node* Parser::NodeRangeList::find(int min, int max) {
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

void Parser::NodeRangeList::add(Node& n, int min, int max) {
	data.emplace_back(n, min, max);
}

void Parser::NodeRangeList::clear() {
	data.clear();
}

Parser::DataHandler::DataHandler(std::size_t offset, std::size_t length, DataCallback func)
	: offset {offset}
	, length {length}
	, func {func}
{}

Parser::LinkData::LinkData(Element* fromElement, int toIndex, int line, int column)
	: fromElement {fromElement}
	, toIndex {toIndex}
	, line {line}
	, column {column}
{}

std::unique_ptr<Error> Parser::error() {
	return std::move(_err);
}

std::unique_ptr<Document> Parser::parse(std::ifstream& in) {
	_pipe = std::make_unique<Pipe>(in);
	_tokenizer = std::make_unique<Tokenizer>(*_pipe);
	_crc = std::make_unique<CRC>();
	_crc->upstream(*_pipe.get());

	// Interleave disk reads with tokenization and CRC calculation
	std::thread thread {[&] {
		_pipe->read();
	}};

	// Meanwhile, build out document by parsing emitted tokens in parallel
	_document = std::make_unique<Document>(Version88a);
	bool ok = this->parse88a();
	if (! ok || _done) {
		goto exit;
	}

	if (! _opt.force88aMode) {
		// Set 88a header as first (hidden) child of 96a document
		_document->visibility(VisibilityNone);

		auto d = std::make_unique<Document>(Version96a);
		_document.swap(d);
		_document->appendChild(std::move(d));

		ok = this->parse96a();
		if (! ok) {
			goto exit;
		}
	}

exit:
	thread.join();
	return std::move(_document);
}

void Parser::reset() {
	_err.release();
	_pipe.release();
	_tokenizer.release();
	_crc.release();
	_document.release();
	_parents.clear();
	_elements.clear();
	_deferredLinks.clear();
	_dataHandlers.clear();
	_key.clear();
	_indexOffset = 0;
	_isTitleSet = false;
	_done = false;
}

//--------------------------------- UHS 88a ---------------------------------//

bool Parser::parse88a() {
	std::unique_ptr<const Token> t;

	// Signature
	t = this->expect(TokenSignature);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	// Title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	_document->title(t->value());

	// First hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int firstHintIndex = Strings::toInt(t->value());
	if (firstHintIndex < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	firstHintIndex += HeaderLen;

	// Last hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int lastHintIndex = Strings::toInt(t->value());
	if (lastHintIndex < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	lastHintIndex += HeaderLen;

	// Subjects
	NodeMap parents {{0, _document.get()}};
	bool ok = this->parse88aElements(firstHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	// Hints
	ok = this->parse88aTextNodes(lastHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
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
	case TokenCreditSep: // This is informal but common
		// Fall through
	case TokenString:
		ok = this->parse88aCreditElement(std::move(t));
		if (! ok) {
			this->unexpected(tokenType, line, column);
		}
		return ok;
	case TokenHeaderSep:
		this->parseHeaderSep(std::move(t));
		return true;
	case TokenEOF:
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
bool Parser::parse88aElements(int firstHintIndex, NodeMap& parents) {
	std::unique_ptr<const Token> t;
	ElementType elementType {ElementSubject};

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		std::string encodedTitle {t->value()};
		int index = t->line();

		t = this->expect(TokenIndex);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		int firstChildIndex = Strings::toInt(t->value());
		if (firstChildIndex < 0) {
			this->expectedInt(t->value(), t->line(), t->column());
			return false;
		}
		firstChildIndex += HeaderLen;

		if (firstChildIndex == firstHintIndex) {
			elementType = ElementHint;
		}

		Node* parent = nullptr;
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

		auto e = std::make_unique<Element>(elementType, index);
		auto ptr = e.get();
		parent->appendChild(std::move(e));

		std::string title {_codec.decode88a(encodedTitle)};
		if (parent->nodeType() != NodeDocument) {
			char finalChar = title[title.length()-1];
			if (!this->isPunctuation(finalChar)) {
				title += '?';
			}
		}
		ptr->title(title);

		if (index < firstHintIndex) {
			parents[firstChildIndex] = ptr;
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
				this->unexpected(t->type(), t->line(), t->column());
			}
			return false;
		}
		int index = t->line();

		Node* parent = nullptr;
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

		auto n = std::make_unique<TextNode>();
		n->body(_codec.decode88a(t->value()));
		parent->appendChild(std::move(n));

		if (index == lastHintIndex) {
			break; // Done parsing hints
		}
	}
	return true;
}

bool Parser::parse88aCreditElement(std::unique_ptr<const Token> t) {
	auto e = std::make_unique<Element>(ElementCredit, t->line());
	e->title("Credits");

	// Body
	std::string s;
	bool continuation = false;
	bool ok = false;

	while (true) {
		switch (t->type()) {
		case TokenEOF:
			if (! s.empty()) {
				e->body(s);
			}
			_done = true;
			ok = true;
			goto exit;
		case TokenHeaderSep:
			if (! s.empty()) {
				e->body(s);
			}
			this->parseHeaderSep(std::move(t));
			ok = true;
			goto exit;
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
			this->unexpected(t->type(), t->line(), t->column());
			ok = false;
			goto exit;
		}

		t = this->next();
		if (_err != nullptr) {
			ok = false;
			goto exit;
		}
	}

exit:
	_document->appendChild(std::move(e));
	return ok;
}

//--------------------------------- UHS 96a ---------------------------------//

void Parser::parseHeaderSep(std::unique_ptr<const Token> t) {
	if (_opt.force88aMode) {
		_done = true;
		return;
	}
	// 96a element indexes don't include compatibility header
	_indexOffset = t->line();
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
		case TokenLength:
			{
				const auto e = this->parseElement(std::move(t));
				if (e == nullptr) {
					return false;
				}
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
	t = this->expect(TokenIdent);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return nullptr;
	}
	std::string ident {t->value()};

	// Create element
	auto elementType = Element::elementType(ident);
	int index = this->offsetIndex(t->line());
	auto e = std::make_unique<Element>(elementType, index, len);
	auto ptr = e.get();

	// Store a reference for Link and Incentive elements
	_elements[index] = ptr;

	// Internal hyperpng links refer to region index instead of element index
	if (indexByRegion) {
		_elements[index-1] = ptr;
	}

	ok = this->findParentAndAppend(std::move(e), std::move(t));
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
			this->unexpected(t->type(), t->line(), t->column());
		}
		return nullptr;
	}
	ptr->title(t->value());

	switch (elementType) {
	case ElementUnknown:   /* No further processing required */   break;
	case ElementBlank:     /* No further processing required */   break;
	case ElementComment:   ok = this->parseCommentElement(ptr);   break;
	case ElementCredit:    ok = this->parseCommentElement(ptr);   break;
	case ElementGifa:      ok = this->parseDataElement(ptr);      break;
	case ElementHint:      ok = this->parseHintElement(ptr);      break;
	case ElementHyperpng:  ok = this->parseHyperpngElement(ptr);  break;
	case ElementIncentive: ok = this->parseIncentiveElement(ptr); break;
	case ElementInfo:      ok = this->parseInfoElement(ptr);      break;
	case ElementLink:      ok = this->parseLinkElement(ptr);      break;
	case ElementNesthint:  ok = this->parseHintElement(ptr);      break;
	case ElementOverlay:   ok = this->parseOverlayElement(ptr);   break;
	case ElementSound:     ok = this->parseDataElement(ptr);      break;
	case ElementSubject:   ok = this->parseSubjectElement(ptr);   break;
	case ElementText:      ok = this->parseTextElement(ptr);      break;
	case ElementVersion:   ok = this->parseVersionElement(ptr);   break;
	}

	if (! ok) {
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
		case TokenDataOffset:
			intOffset = Strings::toInt(t->value());
			if (intOffset < 0) {
				this->expectedInt(t->value(), t->line(), t->column());
				return false;
			}
			offset = intOffset;
			goto expectDataLength;
		case TokenTextFormat:
			continue; // Ignore
		default:
			this->unexpected(t->type(), t->line(), t->column());
			return false;
		}
	}

expectDataLength:
	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
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
	this->addDataCallback(offset, len, [=](std::string data) {
		e->body(data);
	});

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
				this->expected("string", t->value(), t->line(), t->column());
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
	if (! s.empty()) {
		e->appendString(s);
	}

	return true;
}

bool Parser::parseHyperpngElement(Element* const e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (! ok) {
		return false;
	}

	// Parse child elements
	int len = e->length();
	int childLen = 0;

	for (int i = 4; i < len; i += 1 + childLen) {
		// Interactive region top-left X coordinate
		t = this->expect(TokenCoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
		t = this->expect(TokenCoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
		t = this->expect(TokenLength);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
// other files have indexes pointing to nowhere, so we skip those, too.
bool Parser::parseIncentiveElement(Element* const e) {
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
		auto indexString = marker.substr(0, markerLen-1);
		auto instruction = marker.substr(markerLen-1, 1);

		int index = Strings::toInt(indexString);
		if (index < 0) {
			this->expected("valid incentive index", indexString, t->line(), t->column());
			return false;
		}

		// Look up referenced element
		Element* ref = nullptr;
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

bool Parser::parseInfoElement(Element* const e) {
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
				this->unexpected(t->type(), t->line(), t->column());
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
				this->expected(Token::InfoKeyValueSep, t->value(), t->line(), t->column());
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
			if (! ok) {
				this->expected("valid date", t->value(), t->line(), t->column());
				return false;
			}
		} else if (key == "time") {
			bool ok = this->parseTime(val, tm);
			if (! ok) {
				this->expected("valid time", t->value(), t->line(), t->column());
				return false;
			}
			auto time = mktime(&tm);
			if (time == -1) {
				this->expected("valid timestamp", t->value(), t->line(), t->column());
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

bool Parser::parseLinkElement(Element* const e) {
	std::unique_ptr<const Token> t;

	// Ref index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	auto body = t->value();
	int refIndex = Strings::toInt(body);
	if (refIndex < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->body(body);

	return this->linkOrDefer(e, refIndex, t->line(), t->column());
}

bool Parser::parseOverlayElement(Element* const e) {
	std::unique_ptr<const Token> t;

	bool ok = this->parseDataElement(e);
	if (! ok) {
		return false;
	}

	// Image top-left X coordinate
	t = this->expect(TokenCoordX);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	auto x = t->value();
	if (Strings::toInt(x) == Strings::NaN) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->attr("image:x", x);

	// Image bottom-right Y coordinate
	t = this->expect(TokenCoordY);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	auto y = t->value();
	if (Strings::toInt(y) == Strings::NaN) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	e->attr("image:y", y);

	return true;
}

bool Parser::parseSubjectElement(Element* const e) {
	std::unique_ptr<const Token> t;

	if (! _isTitleSet) {
		_document->title(e->title());
		_key = _codec.createKey(_document->title());
		_isTitleSet = true;
	}

	int len = e->length();
	int childLen = 0;

	for (int i = 3; i <= len; i += childLen) {
		// Length
		t = this->expect(TokenLength);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
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
	t = this->expect(TokenTextFormat);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}

	int format = Strings::toInt(t->value());
	if (format < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	if (format == TextFormatMonospace || format == TextFormatMonospaceAlt) {
		e->attr("preformatted", "true");
	}

	// Offset
	t = this->expect(TokenDataOffset);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t->type(), t->line(), t->column());
		}
		return false;
	}
	int intOffset = Strings::toInt(t->value());
	if (intOffset < 0) {
		this->expectedInt(t->value(), t->line(), t->column());
		return false;
	}
	std::size_t offset = intOffset;

	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
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
	auto t = _tokenizer->next();
	if (t == nullptr) {
		auto err = _tokenizer->error();
		if (err != nullptr) {
			_err = std::move(err);
			return t;
		}
	}
	if (_opt.debug) {
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

bool Parser::findParentAndAppend(std::unique_ptr<Element> e, std::unique_ptr<const Token> t) {
	if (e == nullptr) {
		return false;
	}

	int min = e->index();
	int max = min + e->length();
	auto parent = _parents.find(min, max);
	_parents.add(*e, min, max);

	if (parent == nullptr) {
		_err = std::make_unique<Error>(ErrorValue, "orphaned element");
		_err->finalize(t->line(), t->column());
		return false;
	}
	parent->appendChild(std::move(e));

	return true;
}

bool Parser::linkOrDefer(Element* fromElement, int toIndex, int line, int column) {
	if (fromElement == nullptr) {
		return false;
	}

	if (fromElement->index() > toIndex) {
		return this->link(fromElement, toIndex, line, column);
	} else {
		_deferredLinks.emplace(
			std::piecewise_construct
			, std::make_tuple(toIndex)
			, std::make_tuple(fromElement, toIndex, line, column));
	}
	return true;
}

bool Parser::link(Element* fromElement, int toIndex, int line, int column) {
	if (fromElement == nullptr) {
		return false;
	}

	Element* ref = nullptr;
	try {
		ref = _elements.at(toIndex);
	} catch (const std::out_of_range& ex) {
		this->indexNotFound(toIndex, line, column);
		return false;
	}
	fromElement->ref(ref);

	return true;
}

bool Parser::handleDeferredLink(int index) {
	if (_deferredLinks.count(index) == 0) {
		return true;
	}
	const auto& ld = _deferredLinks[index];
	return this->link(ld.fromElement, ld.toIndex, ld.line, ld.column);
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
	_crc->finalize();
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

int Parser::offsetIndex(int index) {
	return index - _indexOffset;
}

void Parser::indexNotFound(int index, int line, int column) {
	_err = std::make_unique<Error>(ErrorValue);
	_err->messagef("index not found: %d", index);
	_err->finalize(line, column);
}

void Parser::expected(std::string expected, std::string found, int line, int column) {
	_err = std::make_unique<Error>(ErrorValue);
	_err->messagef("expected %s, found '%s'", expected.data(), found.data());
	_err->finalize(line, column);
}

void Parser::expectedInt(std::string found, int line, int column) {
	this->expected("valid integer", found, line, column);
}

void Parser::unexpected(TokenType type, int line, int column) {
	_err = std::make_unique<Error>(ErrorToken);
	_err->messagef("unexpected %s token", Token::typeString(type).data());
	_err->finalize(line, column);
}

}

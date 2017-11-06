#include <climits>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Parser::Parser(std::istream& in, const ParserOptions& opt)
	: _version {opt.version}
	, _debug {opt.debug}
	, _scanner {std::make_unique<Scanner>(in)}
	, _codec {std::make_unique<Codec>()}
	, _document {std::make_shared<Document>()}
	, _isTitleSet {false}
	, _done {false}
{}

Parser::~Parser() {}

Parser::NodeRange::NodeRange(std::shared_ptr<Node> n, int min, int max)
	: node(n), min(min), max(max) {}

Parser::NodeRange::~NodeRange() {}

Parser::NodeRangeList::NodeRangeList() : data({}) {}

Parser::NodeRangeList::~NodeRangeList() {}

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
	: offset(offset), length(length), func(func) {}

Parser::DataHandler::~DataHandler() {}

Parser::LinkData::LinkData(
	const std::shared_ptr<Token> fromToken, const std::shared_ptr<Element> fromElement, int toIndex)
	: fromToken(fromToken), fromElement(fromElement), toIndex(toIndex) {}

Parser::LinkData::~LinkData() {}

std::shared_ptr<Error> Parser::error() {
	return _err;
}

std::shared_ptr<Document> Parser::parse() {
	std::thread thread {[&] {
		_scanner->scan();
	}};

	bool ok = this->parse88a();
	if (! ok || _done) {
		thread.join();
		return _document;
	}

	if (_version != Version88a) {
		ok = this->parse96a();
		if (! ok) {
			thread.join();
			return _document;
		}
	}
	
	thread.join();
	// _crc.finalize();
	// _document.validCRC = _crc.valid();

	return _document;
}

//================================= UHS 88a =================================//

bool Parser::parse88a() {
	std::shared_ptr<Token> t;

	// Signature
	t = this->expect(TokenSignature);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	_document->title(t->value());

	// First hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int firstHintIndex = Strings::toInt(t->value());
	if (firstHintIndex < 0) {
		this->expectedInt(t);
		return false;
	}
	firstHintIndex += HeaderLen;

	// Last hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int lastHintIndex = Strings::toInt(t->value());
	if (lastHintIndex < 0) {
		this->expectedInt(t);
		return false;
	}
	lastHintIndex += HeaderLen;

	// Subjects
	NodeMap parents {{0, _document->root()}};
	bool ok = this->parse88aElements(firstHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Hints
	ok = this->parse88aTextNodes(lastHintIndex, parents);
	if (! ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Credits (or compatibility separator)
	while (true) {
		auto t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenEOF:
			_done = true;
			return true;
		case TokenCreditSep:
			ok = this->parse88aCreditElement(t->line());
			if (! ok) {
				this->unexpected(t);
				return false;
			}
			if (_done) {
				return true;
			}
			// Fall through
		case TokenCompatSep:
			if (_version == Version88a) {
				_done = true;
			} else {
				// 96a element indexes don't include compatibility header
				_indexOffset = t->line();

				// Throw away what we've done so far
				_document = std::make_shared<Document>();
				_document->version(Version96a);
			}
			return true;
		default:
			this->unexpected(t);
			return false;
		}
	}
}

bool Parser::parse88aElements(int firstHintIndex, NodeMap& parents) {
	std::shared_ptr<Token> t;
	ElementType elementType {ElementSubject};
	std::shared_ptr<Element> p;

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		std::string encodedLabel {t->value()};
		int index {t->line()};

		t = this->expect(TokenIndex);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		int firstChildIndex = Strings::toInt(t->value());
		if (firstChildIndex < 0) {
			this->expectedInt(t);
			return false;
		}
		firstChildIndex += HeaderLen;

		if (firstChildIndex == firstHintIndex) {
			elementType = ElementHint;
		}

		std::shared_ptr<Node> parent;
		for (int i {index}; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}

		if (parent == nullptr) {
			_err = std::make_shared<Error>(ErrorValue, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto e = std::make_shared<Element>(elementType, index);

		std::string label {_codec->decode88a(encodedLabel)};
		if (parent != _document->root()) {
			char finalChar = label[label.length()-1];
			if (!this->isPunctuation(finalChar)) {
				label += '?';
			}
		}
		e->value(label);

		parent->appendChild(e);

		if (index < firstHintIndex) {
			parents[firstChildIndex] = e;
		}
		if (index+2 == firstHintIndex) {
			break; // Done parsing subjects
		}
	}
	return true;
}

bool Parser::parse88aTextNodes(int lastHintIndex, NodeMap& parents) {
	std::shared_ptr<Token> t;
	std::shared_ptr<Element> p;
	std::shared_ptr<TextNode> c;
	std::vector<std::shared_ptr<Node>> children;

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		std::string encodedLabel {t->value()};
		int index {t->line()};

		std::shared_ptr<Node> parent;
		for (int i {index}; parent == nullptr; --i) {
			if (parents.count(i) == 1) {
				parent = parents[i];
				break;
			}
		}
		if (parent == nullptr) {
			_err = std::make_shared<Error>(ErrorValue, "could not find parent node");
			_err->finalize(t->line(), t->column());
			return false;
		}

		auto n = std::make_shared<TextNode>();
		std::string label {_codec->decode88a(encodedLabel)};
		n->value(label);
		parent->appendChild(n);

		if (index == lastHintIndex) {
			break; // Done parsing hints
		}
	}
	return true;
}

bool Parser::parse88aCreditElement(int index) {
	std::shared_ptr<Token> t;
	auto e = std::make_shared<Element>(ElementCredit, index);
	e->value("Credits");
	_document->appendChild(e);

	// Body
	auto n = std::make_shared<TextNode>();
	std::string s;
	bool continuation = false;

	while (true) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenEOF:
			n->value(s);
			e->appendChild(n);
			_done = true;
			return true;
		case TokenString:
			if (continuation) {
				s += ' ';
			}
			s += t->value();
			continuation = true;
			break;
		case TokenCompatSep:
			n->value(s);
			e->appendChild(n);
			return true;
		default:
			this->unexpected(t);
			return false;
		}
	}
}

//================================= UHS 96a =================================//

bool Parser::parse96a() {
	std::shared_ptr<Token> t;
	std::shared_ptr<Element> e;

	_parents.add(_document->root(), 0, INT_MAX);

	// Parse elements
	while (true) {
		t = this->next();
		if (_err != nullptr) {
			return false;
		}

		switch (t->type()) {
		case TokenEOF:
			_done = true;
			return true;
		case TokenLength:
			e = this->parseElement(t);
			if (e == nullptr) {
				return false;
			}
			break;
		case TokenData:
			this->parseData(t);
			break;
		default:
			// TODO: Once all elements are handled,
			// return this->unexpected(t);
			break;
		}
	}
	return true;
}

// Elements are automatically appended to their parents
std::shared_ptr<Element> Parser::parseElement(std::shared_ptr<Token> t) {
	bool ok = false;

	// Length
	int len = Strings::toInt(t->value());
	if (len < 0) {
		this->expectedInt(t);
		return nullptr;
	}

	// Ident
	t = this->expect(TokenIdent);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return nullptr;
	}
	std::string ident {t->value()};

	// Create element
	auto elementType = Element::elementType(ident);
	int index {this->offsetIndex(t->line())};
	auto e = std::make_shared<Element>(elementType, index, len);

	// Store a reference for Link and Incentive elements
	_elementMap[index] = e;

	ok = this->findAndLinkParent(e, t);
	if (! ok) {
		return nullptr;
	}

	ok = handleDeferredLink(index);
	if (! ok) {
		return nullptr;
	}

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
	if (elementType == ElementSubject && ! _isTitleSet) {
		_document->title(e->value());
		_key = _codec->createKey(_document->title());
		_isTitleSet = true;
	}

	return e;
}

bool Parser::parseCommentElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Label
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	std::string label {t->value()};
	e->attr("label", label);

	// Body
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
			this->unexpected(t);
			return false;
		}
	}
	e->value(s);

	return true;
}

bool Parser::parseDataElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Label
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->attr("label", t->value());

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
				this->expectedInt(t);
				return false;
			}
			offset = intOffset;
			goto expectDataLength;
		default:
			this->unexpected(t);
			return false;
		}
	}

expectDataLength:
	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(t);
		return false;
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) {
		e->value(data);
	});

	return true;
}

bool Parser::parseHintElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;
	std::shared_ptr<Element> child;
	std::string s;

	// Label
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->attr("label", t->value());

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
				s += _codec->decode96a(t->value(), _key, false);
			} else {
				s += _codec->decode88a(t->value());
			}
			continuation = true;
			break;
		case TokenNestedTextSep:
			if (i == len) {
				this->expected(t, "string");
				return false;
			}
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
				this->expected(t, "string");
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
					this->unexpected(t);
				}
				return false;
			}

			// Child element
			child = this->parseElement(t);
			if (child == nullptr) {
				return false;
			}
			i += child->length();

			break;
		default:
			this->unexpected(t);
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
	std::shared_ptr<Token> t;

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
				this->unexpected(t);
			}
			return false;
		}
		auto x1 = t->value();
		if (Strings::toInt(x1) < 0) {
			this->expectedInt(t);
			return false;
		}

		// Interactive region top-left Y coordinate
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		auto y1 = t->value();
		if (Strings::toInt(y1) < 0) {
			this->expectedInt(t);
			return false;
		}

		// Interactive region bottom-right X coordinate
		t = this->expect(TokenCoordX);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		auto x2 = t->value();
		if (Strings::toInt(x2) < 0) {
			this->expectedInt(t);
			return false;
		}

		// Interactive region bottom-right Y coordinate
		t = this->expect(TokenCoordY);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		auto y2 = t->value();
		if (Strings::toInt(y2) < 0) {
			this->expectedInt(t);
			return false;
		}

		// Child element (overlay or link)
		t = this->expect(TokenLength);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		child = this->parseElement(t);
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

bool Parser::parseIncentiveElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;
	std::string s;

	e->visibility(VisibilityNone);

	// Ignore nested text separator
	t = this->expect(TokenNestedTextSep);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Read and decode visibility instructions
	int len = e->length();
	bool continuation = false;

	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		if (continuation) {
			s += " ";
		}
		s += _codec->decode96a(t->value(), _key, false);
		continuation = true;
	}
	e->value(s);

	// Process instructions
	auto markers = Strings::split(s, " ");
	std::size_t markerLen;

	for (const auto& marker : markers) {
		markerLen = marker.length();

		// Split index-instruction pair
		if (markerLen < 2) {
			this->expectedString(t, "incentive index-instruction pair", marker);
		}
		auto indexString = marker.substr(0, markerLen-1);
		auto instruction = marker.substr(markerLen-1, 1);

		int index = Strings::toInt(indexString);
		if (index < 0) {
			this->expectedString(t, "valid incentive index integer", indexString);
			return false;
		}

		// Look up referenced element
		std::shared_ptr<Element> ref;
		try {
			ref = _elementMap.at(index);
		} catch (const std::out_of_range& ex) {
			this->indexNotFound(t, index);
			return false;
		}

		// Set visibility
		if (instruction == "A") {
			ref->visibility(VisibilityRegistered);
		} else if (instruction == "Z") {
			ref->visibility(VisibilityUnregistered);
		} else {
			this->expectedString(t, "valid incentive instruction", instruction);
		}
	}

	return true;
}

bool Parser::parseInfoElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;
	std::string s;
	std::string key;
	std::string val;
	std::string date;
	std::tm tm;

	e->visibility(VisibilityNone);

	// Ignore nested text separator
	t = this->expect(TokenNestedTextSep);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Key-value pairs followed by a notice
	int len = e->length();
	for (int i = 2; i < len; ++i) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		s = t->value();

		if (s.substr(0, 1) == NoticeToken) {
			key = "notice";
			val = s.substr(1);
		} else {
			auto parts = Strings::split(s, InfoKeyValueSep, 2);
			if (parts.size() != 2) {
				this->expected(t, InfoKeyValueSep);
			}
			key = parts[0];
			val = parts[1];
		}

		if (key == "length") {
			int fileLen = Strings::toInt(val);
			if (fileLen < 0) {
				this->expectedInt(t);
				return false;
			}
			_document->length(fileLen);
		} else if (key == "date") {
			bool ok = this->parseDate(val, tm);
			if (! ok) {
				this->expected(t, "valid date");
			}
		} else if (key == "time") {
			bool ok = this->parseTime(val, tm);
			if (! ok) {
				this->expected(t, "valid time");
			}
			auto time = mktime(&tm);
			if (time == -1) {
				this->expected(t, "valid timestamp");
				return false;
			}
			_document->timestamp(tm);
		} else {
			auto v = _document->meta(key);
			if (! v.empty()) {
				v += " ";
			}
			v += val;
			_document->meta(key, v);
		}
	}

	return true;
}

bool Parser::parseLinkElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Label
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->attr("label", t->value());

	// Ref index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int index = Strings::toInt(t->value());
	if (index < 0) {
		this->expectedInt(t);
		return false;
	}
	auto parent = std::static_pointer_cast<Element>(e->parent());
	if (parent->elementType() == ElementHyperpng) {
		index += 1; // Links contained in hyperpngs point to the interactive region
	}

	return this->linkOrDefer(t, e, index);
}

bool Parser::parseOverlayElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	bool ok = this->parseDataElement(e);
	if (! ok) {
		return false;
	}

	// Image top-left X coordinate
	t = this->expect(TokenCoordX);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	auto x = t->value();
	if (Strings::toInt(x) < 0) {
		this->expectedInt(t);
		return false;
	}
	e->attr("image:x", x);

	// Image bottom-right Y coordinate
	t = this->expect(TokenCoordY);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	auto y = t->value();
	if (Strings::toInt(y) < 0) {
		this->expectedInt(t);
		return false;
	}
	e->attr("image:y", y);

	return true;
}

bool Parser::parseSubjectElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->value(t->value());

	return true;
}

bool Parser::parseTextElement(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Label
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->attr("label", t->value());

	// Format
	t = this->expect(TokenDataType);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	int format = Strings::toInt(t->value());
	if (format < 0) {
		this->expectedInt(t);
		return false;
	}
	if (format == TextFormatPreformatted || format == TextFormatPreformattedAlt) {
		e->attr("preformatted", "true");
	}

	// Offset
	t = this->expect(TokenDataOffset);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int intOffset = Strings::toInt(t->value());
	if (intOffset < 0) {
		this->expectedInt(t);
		return false;
	}
	std::size_t offset = intOffset;

	// Length
	t = this->expect(TokenDataLength);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int intLen = Strings::toInt(t->value());
	if (intLen < 0) {
		this->expectedInt(t);
		return false;
	}
	std::size_t len = intLen;

	// Data
	this->addDataCallback(offset, len, [=](std::string data) {
		auto lines = Strings::split(data, UHS::EOL);
		for (auto line = lines.begin(); line != lines.end(); ++line) {
			*line = _codec->decode96a(*line, _key, true);
			if (*line == Token::ParagraphSep) {
				*line = "";
			}
		}
		auto value = Strings::rtrim(Strings::join(lines, "\n"), '\n');
		e->value(value);
	});

	return true;
}

bool Parser::parseVersionElement(std::shared_ptr<Element> e) {
	VersionType v {Version96a};

	e->visibility(VisibilityNone);
	this->parseCommentElement(e);

	auto versionString = e->attr("label");
	if (versionString == "91a") {
		v = Version91a;
	} else if (versionString == "95a") {
		v = Version95a;
	}
	_document->version(v);

	return true;
}

std::shared_ptr<Token> Parser::next() {
	auto t = _scanner->next();
	auto err = _scanner->error();

	if (err != nullptr) {
		_err = err;
		return t;
	}
	if (t == nullptr) {
		_err = std::make_shared<Error>(ErrorRead, "received null token from scanner");
		_err->finalize();
		return t;
	}
	if (_debug) {
		std::cout << t->toString() << std::endl;
	}
	return t;
}

std::shared_ptr<Token> Parser::expect(TokenType expected) {
	auto t = this->next();
	if (_err != nullptr) {
		return t;
	}

	if (t->type() == TokenEOF && expected != TokenEOF) {
		_err = std::make_shared<Error>(ErrorEOF);
		_err->finalize(t->line(), t->column());
	} else if (t->type() != expected) {
		_err = std::make_shared<Error>(ErrorToken);
		_err->messagef("expected %s, found %s",
			Token::typeString(expected).data(), t->typeString().data());
		_err->finalize(t->line(), t->column());
	}
	return t;
}

bool Parser::findAndLinkParent(std::shared_ptr<Element> e, std::shared_ptr<Token> t) {
	int min = e->index();
	int max = min + e->length();
	auto parent = _parents.find(min, max);

	if (parent == nullptr) {
		_err = std::make_shared<Error>(ErrorValue, "orphaned element");
		_err->finalize(t->line(), t->column());
		return false;
	}
	parent->appendChild(e);
	_parents.add(e, min, max);

	return true;
}

bool Parser::linkOrDefer(std::shared_ptr<Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex) {
	if (fromElement->index() > toIndex) {
		return this->link(fromToken, fromElement, toIndex);
	} else {
		_deferredLinks[toIndex] = std::make_shared<LinkData>(fromToken, fromElement, toIndex);
	}
	return true;
}

bool Parser::link(std::shared_ptr<Token> fromToken, std::shared_ptr<Element> fromElement, int toIndex) {
	std::weak_ptr<Element> ref; 

	try {
		ref = _elementMap.at(toIndex);
	} catch (const std::out_of_range& ex) {
		this->indexNotFound(fromToken, toIndex);
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
	return this->link(linkData->fromToken, linkData->fromElement, linkData->toIndex);
}

void Parser::addDataCallback(std::size_t offset, std::size_t length, DataCallback func) {
	_dataHandlers.push_back(DataHandler(offset, length, func));
}

void Parser::parseData(std::shared_ptr<Token> t) {
	std::size_t offset;

	for (const auto& handler : _dataHandlers) {
		offset = handler.offset - t->offset();
		handler.func(t->value().substr(offset, handler.length));
	}
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

void Parser::indexNotFound(std::shared_ptr<Token> t, int index) {
	_err = std::make_shared<Error>(ErrorValue);
	_err->messagef("index not found: %d", index);
	_err->finalize(t->line(), t->column());
}

void Parser::expectedString(std::shared_ptr<Token> t, std::string expected, std::string found) {
	_err = std::make_shared<Error>(ErrorValue);
	_err->messagef("expected %s, found '%s'", expected.data(), found.data());
	_err->finalize(t->line(), t->column());
}

void Parser::expected(std::shared_ptr<Token> t, std::string expected) {
	this->expectedString(t, expected, t->value());
}

void Parser::expectedInt(std::shared_ptr<Token> t) {
	this->expected(t, "valid integer");
}

void Parser::unexpected(std::shared_ptr<Token> t) {
	_err = std::make_shared<Error>(ErrorToken);
	_err->messagef("unexpected %s", t->typeString().data());
	_err->finalize(t->line(), t->column());
}

}

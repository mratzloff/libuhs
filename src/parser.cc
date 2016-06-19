#include <climits>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Parser::Parser(std::istream& in, const ParserOptions& opt)
	: _version {opt.version}
	, _registered {opt.registered}
	, _debug {opt.debug}
	, _scanner {std::make_unique<Scanner>(in)}
	, _codec {std::make_unique<Codec>()}
	, _document {std::make_shared<Document>()}
	, _isTitleSet {false}
	, _done {false}
{}

Parser::~Parser() {}

std::shared_ptr<Error> Parser::error() {
	return _err;
}

std::shared_ptr<Document> Parser::parse() {
	std::thread thread {[&] {
		_scanner->scan();
	}};

	bool ok = this->parse88a();
	if (!ok || _done) {
		thread.join();
		return _document;
	}

	ok = this->parse96a();
	if (!ok) {
		thread.join();
		return _document;
	}

	// _crc.finalize();
	// _document.validCRC = _crc.valid();

	thread.join();

	return _document;
}

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
	if (_debug) {
		std::cerr << "=> \"" << _document->title() << "\"\n";
	}

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
	bool ok = this->parse88aElements(parents, firstHintIndex);
	if (!ok) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}

	// Hints
	ok = this->parse88aTextNodes(parents, lastHintIndex);
	if (!ok) {
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
			ok = this->parse88aCredit(t->line());
			if (!ok) {
				this->unexpected(t);
				return false;
			}
			if (_done) {
				return true;
			}
			// Fall through
		case TokenCompatSep:
			if (_version != Version88a) {
				// Throw away what we've done so far
				_document = std::make_shared<Document>();
				_document->version(Version96a);
				return true;
			}
			break;
		default:
			this->unexpected(t);
			return true;
		}
	}
}

bool Parser::parse88aElements(NodeMap& parents, int firstHintIndex) {
	std::shared_ptr<Token> t;
	ElementType elementType {ElementSubject};

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		std::string encodedTitle {t->value()};
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

		std::shared_ptr<Element> e = std::make_shared<Element>(elementType, index);

		std::string title {_codec->decode88a(encodedTitle)};
		if (parent != _document->root()) {
			char finalChar = title[title.length()-1];
			if (!this->isPunctuation(finalChar)) {
				title += '?';
			}
		}
		e->value(title);
		if (_debug) {
			std::cerr << "<" << Element::typeString(elementType) << " [" << title << "]>\n";
		}

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

bool Parser::parse88aTextNodes(NodeMap& parents, int lastHintIndex) {
	std::shared_ptr<Token> t;

	while (true) {
		t = this->expect(TokenString);
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->unexpected(t);
			}
			return false;
		}
		std::string encodedTitle {t->value()};
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
		std::string title {_codec->decode88a(encodedTitle)};
		n->value(title);
		if (_debug) {
			std::cerr << "\"" << title << "\"\n";
		}
		parent->appendChild(n);

		if (index == lastHintIndex) {
			break; // Done parsing hints
		}
	}
	return true;
}

bool Parser::parse88aCredit(int index) {
	std::shared_ptr<Token> t;
	auto e = std::make_shared<Element>(ElementCredit, index);
	_document->appendChild(e);

	// Add body
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

bool Parser::parse96a() {
	std::shared_ptr<Token> t;
	NodeRangeList parents;
	parents.push_back(std::make_shared<NodeRange>(_document->root(), 0, INT_MAX));
	int len = 0;

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
			len = this->parseElement(parents, t);
			if (len < 0) {
				return false;
			}
			break;
		case TokenData:
			// todo
			break;
		default:
			// todo: Once all elements are handled,
			// return this->unexpected(t);
			break;
		}
	}
	return true;
}

bool Parser::parseComment(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Add title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	std::string title {t->value()};
	e->attr("title", title);
	if (_debug) {
		std::cerr << "=> \"" << title << "\"\n";
	}

	// Add body
	int len = e->length();
	auto n = std::make_shared<TextNode>();
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
		case TokenParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		default:
			this->unexpected(t);
			return false;
		}
	}
	n->value(s);
	e->appendChild(n);
	if (_debug) {
		std::cerr << "=> \"" << n->value() << "\"\n";
	}

	return true;
}

bool Parser::parseHint(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	// Add title
	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->attr("title", t->value());
	if (_debug) {
		std::cerr << "=> \"" << e->value() << "\"\n";
	}

	// Add body
	int len = e->length();
	auto n = std::make_shared<TextNode>();
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
			s += _codec->decode88a(t->value());
			continuation = true;
			break;
		case TokenNestedTextSep:
			if (i == len) {
				this->expected(t, "string");
				return false;
			}
			n->value(s);
			e->appendChild(n);
			if (_debug) {
				std::cerr << "=> \"" << n->value() << "\"\n";
			}
			n = std::make_shared<TextNode>();
			s.clear();
			continuation = false;
			break;
		case TokenParagraphSep:
			s += "\n\n";
			continuation = false;
			break;
		default:
			this->unexpected(t);
			return false;
		}
	}
	n->value(s);
	e->appendChild(n);
	if (_debug) {
		std::cerr << "=> \"" << n->value() << "\"\n";
	}

	return true;
}

bool Parser::parseSubject(std::shared_ptr<Element> e) {
	std::shared_ptr<Token> t;

	t = this->expect(TokenString);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	e->value(t->value());
	if (_debug) {
		std::cerr << "=> \"" << e->value() << "\"\n";
	}

	return true;
}

int Parser::parseElement(NodeRangeList& parents, std::shared_ptr<Token> t) {
	// Length
	int len = Strings::toInt(t->value());
	if (len < 0) {
		this->expectedInt(t);
		return -1;
	}

	// Ident
	t = this->expect(TokenIdent);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return -1;
	}
	std::string ident {t->value()};
	int index {t->line()};

	// Create element
	auto elementType = Element::elementType(ident);
	if (elementType == ElementUnknown) { // Ignore it and move on
		return len;
	}
	auto e = std::make_shared<Element>(elementType, index, len);
	int min = index;
	int max = index + len;

	// Link element to parent
	std::shared_ptr<Node> parent;
	for (auto p : parents) {
		if (min > p->min) {
			if (max <= p->max) {
				parent = p->node;
			}
		} else {
			break;
		}
	}
	if (parent == nullptr) {
		_err = std::make_shared<Error>(ErrorValue, "orphaned element");
		_err->finalize(t->line(), t->column());
		return -1;
	}
	parent->appendChild(e);
	parents.push_back(std::make_shared<NodeRange>(e, min, max));

	bool ok = false;

	// Process content
	switch (elementType) {
	case ElementBlank:
		// No further processing required
		break;
	case ElementCredit:
		// Fall through
	case ElementComment:
		ok = this->parseComment(e);
		if (!ok) {
			return -1;
		}
		break;
	case ElementHint:
		ok = this->parseHint(e);
		if (!ok) {
			return -1;
		}
		break;
	case ElementSubject:
		ok = this->parseSubject(e);
		if (!ok) {
			return -1;
		}
		if (!_isTitleSet) {
			_document->title(e->value());
			// todo: Once 96a decoding methods are implemented
			// _codec->key(_document->title());
			_isTitleSet = true;
		}
		break;
	}

	return len;
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
		std::cerr << t->toString() << std::endl;
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

void Parser::expected(std::shared_ptr<Token> t, std::string expected) {
	_err = std::make_shared<Error>(ErrorValue);
	_err->messagef("expected %s, found '%s'",
		expected.data(), t->value().data());
	_err->finalize(t->line(), t->column());
}

void Parser::expectedInt(std::shared_ptr<Token> t) {
	this->expected(t, "valid integer");
}

void Parser::unexpected(std::shared_ptr<Token> t) {
	_err = std::make_shared<Error>(ErrorToken);
	_err->messagef("unexpected %s", t->typeString().data());
	_err->finalize(t->line(), t->column());
}

bool Parser::isPunctuation(char c) {
	return c == '?' || c == '!' || c == '.' || c == ',' || c == ';';
}

Parser::NodeRange::NodeRange(std::shared_ptr<Node> n, int min, int max)
	: node(n), min(min), max(max) {}

Parser::NodeRange::~NodeRange() {}

}

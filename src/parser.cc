#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Parser::Parser(std::istream& in)
	: _scanner {std::make_unique<Scanner>(in)}
	, _document {std::make_shared<Document>()}
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
		return _document;
	}

	ok = this->parse96a();
	if (!ok) {
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
	_document->title(t->stringValue());

	// First hint index
	t = this->expect(TokenIndex);
	if (_err != nullptr) {
		if (_err->type() == ErrorEOF) {
			this->unexpected(t);
		}
		return false;
	}
	int firstHintIndex = t->intValue();
	if (firstHintIndex < 0) {
		this->expectedInt(t);
		return false;
	}
	firstHintIndex += HeaderLen;

	// while (_scanner->hasNext()) {
	// 	t = _scanner->next();
	// 	if (t == nullptr) {
	// 		break;
	// 	}
	// 	std::cout << t->toString() << std::endl;
	// }
	//
	// auto err = _scanner->error();
	// if (err != nullptr) {
	// 	std::cerr << err->message() << std::endl;
	// }

	return true;
}

bool Parser::parse96a() {
	return true;
}

std::shared_ptr<Token> Parser::next() {
	auto t = _scanner->next();
	auto err = _scanner->error();

	if (err != nullptr || t == nullptr) {
		if (err == nullptr) {
			err = std::make_shared<Error>(ErrorRead, "received null token from scanner");
		}
		_err = err;
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
	} else if (t->type() != expected) {
		_err = std::make_shared<Error>(ErrorToken);
		_err->messagef("expected %s, found %s",
			Token::typeString(expected).data(), t->typeString().data());
	}
	return t;
}

void Parser::expected(std::shared_ptr<Token> t, std::string expected) {
	_err = std::make_shared<Error>(ErrorValue);
	_err->messagef("expected %s, found '%s'",
		expected.data(), t->stringValue().data());
}

void Parser::expectedInt(std::shared_ptr<Token> t) {
	this->expected(t, "valid integer");
}

void Parser::unexpected(std::shared_ptr<Token> t) {
	_err = std::make_shared<Error>(ErrorToken);
	_err->messagef("unexpected %s", t->typeString().data());
}

}

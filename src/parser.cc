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
	_token = _scanner->next();
	if (_debug && _token != nullptr) { // Should never be null
		std::cerr << _token->toString() << std::endl;
	}
	return _token;
}

std::shared_ptr<Token> Parser::expect(TokenType expected) {
	auto t = this->next();
	if (t == nullptr) { // Should never be null
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

}

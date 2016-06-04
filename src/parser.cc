#include <iostream>
#include <thread>
#include "parser.h"

namespace UHS {

Parser::Parser(std::istream& in) : _scanner {std::make_unique<Scanner>(in)} {}

Parser::~Parser() {}

void Parser::parse() {
	std::thread thread {[&] {
		_scanner->scan();
	}};

	std::shared_ptr<Token> t;
	while (_scanner->hasNext()) {
		t = _scanner->next();
		if (t == nullptr) {
			break;
		}
		std::cout << t->toString() << std::endl;
	}
	thread.join();

	auto err = _scanner->err();
	if (err != nullptr) {
		std::cerr << err->message() << std::endl;
		return;
	}
}

}

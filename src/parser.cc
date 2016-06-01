#include <iostream>
#include "parser.h"

namespace UHS {

Parser::Parser(std::istream& in) : _scanner {std::make_unique<Scanner>(in)} {}

Parser::~Parser() {}

void Parser::parse() {
	_scanner->scan();

	auto err = _scanner->err();
	if (err != nullptr) {
		std::cout << err->message() << '\n';
		return;
	}

	while (auto t = _scanner->next()) {
		std::cout << t << '\n';
	}
}

}

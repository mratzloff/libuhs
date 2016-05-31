#include <iostream>
#include "parser.h"

namespace UHS {

Parser::Parser(std::istream& in) : _scanner {std::make_unique<Scanner>(in)} {}

Parser::~Parser() {}

void Parser::parse() {
	_scanner->scan();

	while (auto t = _scanner->next()) {
		std::cout << t << '\n';
	}
}

}

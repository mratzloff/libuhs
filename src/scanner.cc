#include <future>
#include <sstream>
#include "error.h"
#include "scanner.h"
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::istream& in) : _in(in) {}

void Scanner::scan() {
	// auto future = std::async(std::launch::async, asyncScan());
}

Token* Scanner::next() {
	return _out->front();
}

Error* Scanner::err() {
	if (_err->code() == ErrEOF) {
		return nullptr;
	}
	return _err;
}

void Scanner::asyncScan() {
	
}

std::string Scanner::scanDescriptor(std::string s, int offset) {
	
}

void Scanner::scanDataAddress(std::string s, int offset) {
	
}

void Scanner::scanOverlayRegion(std::string s, int offset) {
	
}

void Scanner::scanOverlayAddress(std::string s, int offset) {
	
}

void Scanner::eof() {
	Token t {TokenEOF, _line, _column, _offset, ""};
	_out->push(&t);
}

char Scanner::read() {
	char c = _in.get();
	if (!_in.good()) {
		Error err;
		if (_in.eof()) {
			err.code(ErrEOF);
		} else {
			err.code(ErrRead);
		}
		_err = this->formatError(&err);
		return c;
	}
	++_column;
	++_offset;
	return c;
}

Error* Scanner::formatError(Error* err) const {
	std::ostringstream out {"Error at line "};
	out << _line;
	out << ", column ";
	out << _column;
	out << ": ";
	out << err->message();
	err->message(out.str());
	return err;
}

bool Scanner::isNumber(std::string s) const {
	for (char c : s) {
		if (c < '0' || '9' < c) {
			return false;
		}
	}
	return true;
}

const std::string Scanner::ltrimZeroes(std::string s) const {
	return s.substr(s.find_first_not_of("0"));
}

}

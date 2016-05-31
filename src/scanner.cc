#include <future>
#include <memory>
#include <sstream>
#include "error.h"
#include "scanner.h"
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::istream& in) : _in(in) {}

void Scanner::scan() {
	// auto future = std::async(std::launch::async, asyncScan());
}

std::shared_ptr<Token> Scanner::next() {
	return _out->front();
}

std::shared_ptr<Error> Scanner::err() {
	if (_err->code() == ErrEOF) {
		return nullptr;
	}
	return _err;
}

void Scanner::asyncScan() {
	bool beforeCompatSep {true};
	int expectedIndexLine {-1};
	int prevTextLen {0};
	int offset {0};
	char c {'\0'};
	
	while (true) {
		++_line;
		_column = 0;
		_offset += prevTextLen;
		offset = _offset;

		// Check for binary data
		c = _in.peek();
		if (!_in.good()) {
			std::shared_ptr<Error> err;
			if (_in.eof()) {
				err = std::make_shared<Error>(ErrEOF);
			} else {
				err = std::make_shared<Error>(ErrRead);
			}
			_err = this->formatError(err);
			this->eof();
			return;
		}
		if (c == Token::DataSep) {
			this->read(); // Skip binary separator token
			int line = _line;
			int column = _column;
			offset = _offset;
			
			while (true) {
				c = this->read();
				if (_err != nullptr) {
					if (_err->code() == ErrEOF) {
						auto t = std:: make_shared<Token>(TokenData, _offset, _line, _column, _buf);
						_out->push(t);
						this->eof();
					}
					return;
				}
				_buf += c;
			}
		}
		
		// Everything else is line-based
		// Handle errors at the end, after any text that was returned
		char raw[LineLen+1];
		_in.getline(raw, LineLen);
		std::string text {raw};
		prevTextLen = text.length();

		// UHS uses DOS-style line endings
		text.erase(text.find_last_not_of('\r'));

		// All numbers are indexes in 88a, and link elements contain an index
		if (this->isNumber(text) && (beforeCompatSep || _line == expectedIndexLine)) {
			auto t = std:: make_shared<Token>(TokenIndex, _offset, _line, _column, this->ltrim(text, '0'));
			_out->push(t);
			expectedIndexLine = -1;
			continue;
		}

		// Check for exact line matches
		if (text == Token::CompatSep) {
			beforeCompatSep = false;
			auto t = std:: make_shared<Token>(TokenCompatSep, _offset, _line);
			_out->push(t);
		} else if (text == Token::CreditSep) {
			auto t = std:: make_shared<Token>(TokenCreditSep, _offset, _line);
			_out->push(t);
		} else if (text == Token::NestedElementSep) {
			auto t = std:: make_shared<Token>(TokenNestedElementSep, _offset, _line);
			_out->push(t);
		} else if (text == Token::NestedTextSep) {
			auto t = std:: make_shared<Token>(TokenNestedTextSep, _offset, _line);
			_out->push(t);
		} else if (text == Token::ParagraphSep) {
			auto t = std:: make_shared<Token>(TokenParagraphSep, _offset, _line);
			_out->push(t);
		} else if (text == Token::Signature) {
			auto t = std:: make_shared<Token>(TokenSignature, _offset, _line);
			_out->push(t);
		} else {
			// Check for line match patterns
			std::smatch matches;
			if (std::regex_match(text, matches, _descriptorRegex)) {
				IdentType ident = this->scanDescriptor(text, offset);
				if (ident == IdentLink) {
					expectedIndexLine = _line + 2;
				}
			} else if (std::regex_match(text, matches, _dataAddressRegex)) {
				this->scanDataAddress(text, offset);
			} else if (std::regex_match(text, matches, _overlayRegionRegex)) {
				this->scanOverlayRegion(text, offset);
			} else if (std::regex_match(text, matches, _overlayAddressRegex)) {
				this->scanOverlayAddress(text, offset);
			} else {
				auto t = std:: make_shared<Token>(TokenString, offset, _line, 0, text);
				_out->push(t);
			}
		}
	}
}

IdentType Scanner::scanDescriptor(std::string s, int offset) {
	std::string ident {"test"};
	return Token::identType(ident);
}

void Scanner::scanDataAddress(std::string s, int offset) {
	
}

void Scanner::scanOverlayRegion(std::string s, int offset) {
	
}

void Scanner::scanOverlayAddress(std::string s, int offset) {
	
}

void Scanner::eof() {
	auto t = std:: make_shared<Token>(TokenEOF, _offset, _line, _column);
	_out->push(t);
}

char Scanner::read() {
	char c = _in.get();
	if (!_in.good()) {
		std::shared_ptr<Error> err;
		if (_in.eof()) {
			err = std::make_shared<Error>(ErrEOF);
		} else {
			err = std::make_shared<Error>(ErrRead);
		}
		_err = this->formatError(err);
		return c;
	}
	++_column;
	++_offset;
	return c;
}

std::shared_ptr<Error> Scanner::formatError(std::shared_ptr<Error> err) const {
	std::ostringstream out {"Error at line "};
	out << _line << ", column " << _column << ": " << err->message();
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

const std::string Scanner::ltrim(std::string s, char c) const {
	return s.substr(s.find_first_not_of(c));
}

}

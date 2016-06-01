#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include "error.h"
#include "scanner.h"
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::istream& in) : _in {in} {}

Scanner::~Scanner() {}

void Scanner::scan() {
	// auto future = std::async(std::launch::async, asyncScan());
	this->asyncScan();
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
	try {
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
			std::cout << "Check for binary data\n";
			c = _in.peek();
			if (!_in.good()) {
				std::cout << "No good\n";
				ErrorCode code;
				if (_in.eof()) {
					code = ErrEOF;
				} else {
					code = ErrRead;
				}
				_err = this->formatError(std::make_shared<Error>(code));
				this->eof();
				return;
			}
			if (c == Token::DataSep) {
				std::cout << "Matched DataSep\n";
				this->read(); // Skip binary separator token
				int line = _line;
				int column = _column;
				offset = _offset;
			
				while (true) {
					c = this->read();
					if (_err != nullptr) {
						if (_err->code() == ErrEOF) {
							_out->push(std::make_shared<Token>(TokenData, offset, line, column, _buf));
							this->eof();
							std::cout << "Done\n";
						}
						return;
					}
					_buf += c;
				}
			}

			std::cout << "Reading line by line\n";

			// Everything else is line-based
			// Handle errors at the end, after any text that was returned
			char raw[LineLen+1];
			_in.getline(raw, LineLen);
			std::string text {raw};
			prevTextLen = text.length();

			std::cout << "Read this: " << text << '\n';

			// UHS uses DOS-style line endings
			text.erase(text.find_last_not_of('\r'));

			std::cout << "Removed CR\n";

			// All numbers are indexes in 88a, and link elements contain an index
			if (this->isNumber(text) && (beforeCompatSep || _line == expectedIndexLine)) {
				std::cout << "That's an index\n";
				_out->push(std::make_shared<Token>(
					TokenIndex, _offset, _line, 0, this->ltrim(text, '0')));
				expectedIndexLine = -1;
				continue;
			}

			std::cout << "Checking line text\n";

			// Check for exact line matches
			if (text == Token::CompatSep) {
				std::cout << "Matched CompatSep\n";
				beforeCompatSep = false;
				_out->push(std::make_shared<Token>(TokenCompatSep, offset, _line));
			} else if (text == Token::CreditSep) {
				std::cout << "Matched CreditSep\n";
				_out->push(std::make_shared<Token>(TokenCreditSep, offset, _line));
			} else if (text == Token::NestedElementSep) {
				std::cout << "Matched NestedElementSep\n";
				_out->push(std::make_shared<Token>(TokenNestedElementSep, offset, _line));
			} else if (text == Token::NestedTextSep) {
				std::cout << "Matched NestedTextSep\n";
				_out->push(std::make_shared<Token>(TokenNestedTextSep, offset, _line));
			} else if (text == Token::ParagraphSep) {
				std::cout << "Matched ParagraphSep\n";
				_out->push(std::make_shared<Token>(TokenParagraphSep, offset, _line));
			} else if (text == Token::Signature) {
				std::cout << "Matched Signature\n";
				_out->push(std::make_shared<Token>(TokenSignature, offset, _line));
			} else {
				// Check for line match patterns
				std::cout << "Check for line match patterns\n";
				std::smatch matches;
				if (std::regex_match(text, matches, _descriptorRegex)) {
					std::cout << "Matched descriptor\n";
					IdentType ident = this->scanDescriptor(text, matches, offset);
					if (ident == IdentLink) {
						expectedIndexLine = _line + 2;
					}
				} else if (std::regex_match(text, matches, _dataAddressRegex)) {
					std::cout << "Matched data address\n";
					this->scanDataAddress(text, matches, offset);
				} else if (std::regex_match(text, matches, _overlayRegionRegex)) {
					std::cout << "Matched overlay region\n";
					this->scanOverlayRegion(text, matches, offset);
				} else if (std::regex_match(text, matches, _overlayAddressRegex)) {
					std::cout << "Matched overlay address\n";
					this->scanOverlayAddress(text, matches, offset);
				} else {
					std::cout << "Matched string\n";
					_out->push(std::make_shared<Token>(TokenString, offset, _line, 0, text));
				}
			}
		}
	} catch (const std::exception& e) {
		_err = std::make_shared<Error>(ErrRead, e.what());
	}

	std::cout << "Done\n";
}

IdentType Scanner::scanDescriptor(std::string s, std::smatch m, int offset) {
	_out->push(std::make_shared<Token>(
		TokenLength, offset, _line, 0, this->ltrim(m[1].str(), '0')));
	std::string ident {m[2].str()};
	_out->push(std::make_shared<Token>(
		TokenIdent, offset, _line, -1, ident));
	return Token::identType(ident);
}

void Scanner::scanDataAddress(std::string s, std::smatch m, int offset) {
	_out->push(std::make_shared<Token>(
		TokenDataOffset, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenDataLength, offset, _line, -1, this->ltrim(m[5].str(), '0')));
}

void Scanner::scanOverlayRegion(std::string s, std::smatch m, int offset) {
	_out->push(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[1].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[2].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[4].str(), '0')));
}

void Scanner::scanOverlayAddress(std::string s, std::smatch m, int offset) {
	_out->push(std::make_shared<Token>(
		TokenDataOffset, offset, _line, -1, this->ltrim(m[1].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenDataLength, offset, _line, -1, this->ltrim(m[2].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out->push(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[4].str(), '0')));
}

void Scanner::eof() {
	_out->push(std::make_shared<Token>(TokenEOF, _offset, _line, _column));
}

char Scanner::read() {
	char c = _in.get();
	if (!_in.good()) {
		ErrorCode code;
		if (_in.eof()) {
			code = ErrEOF;
		} else {
			code = ErrRead;
		}
		_err = this->formatError(std::make_shared<Error>(code));
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
	auto pos = s.find_first_not_of(c);
	if (pos == std::string::npos) {
		return s;
	} else {
		return s.substr(pos);
	}
}

}

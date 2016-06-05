#include <sstream>
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::istream& in) : _in {in} {}

Scanner::~Scanner() {}

void Scanner::scan() {
	try {
		bool beforeCompatSep {true};
		int expectedIndexLine {-1};
		std::size_t prevTextLen {0};
		std::size_t offset {0};
		char c {'\0'};
	
		while (true) {
			++_line;
			_column = 0;
			_offset += prevTextLen;
			offset = _offset;

			// Check for binary data
			c = (char) _in.peek();
			if (!_in.good()) {
				ErrorType code;
				if (_in.eof()) {
					code = ErrorEOF;
				} else {
					code = ErrorRead;
				}
				_err = this->formatError(std::make_shared<Error>(code));
				this->eof();
				_out.close();
				return;
			} else if (c == Token::DataSep) {
				this->read(); // Skip binary separator token
				auto column = _column;
				offset = _offset;
			
				while (true) {
					c = this->read();
					if (_err != nullptr) {
						if (_err->type() == ErrorEOF) {
							_out.send(std::make_shared<Token>(TokenData, offset, _line, column, _buf));
							this->eof();
						}
						_out.close();
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
			text = this->rtrim(text, '\r');

			// All numbers are indexes in 88a, and link elements contain an index
			if (this->isNumber(text) && (beforeCompatSep || _line == expectedIndexLine)) {
				_out.send(std::make_shared<Token>(
					TokenIndex, _offset, _line, 0, this->ltrim(text, '0')));
				expectedIndexLine = -1;
				continue;
			}

			// Check for exact line matches
			if (text == Token::CompatSep) {
				beforeCompatSep = false;
				_out.send(std::make_shared<Token>(TokenCompatSep, offset, _line));
			} else if (text == Token::CreditSep) {
				_out.send(std::make_shared<Token>(TokenCreditSep, offset, _line));
			} else if (text == Token::NestedElementSep) {
				_out.send(std::make_shared<Token>(TokenNestedElementSep, offset, _line));
			} else if (text == Token::NestedTextSep) {
				_out.send(std::make_shared<Token>(TokenNestedTextSep, offset, _line));
			} else if (text == Token::ParagraphSep) {
				_out.send(std::make_shared<Token>(TokenParagraphSep, offset, _line));
			} else if (text == Token::Signature) {
				_out.send(std::make_shared<Token>(TokenSignature, offset, _line));
			} else {
				// Check for line match patterns
				std::smatch matches;
				if (std::regex_match(text, matches, _descriptorRegex)) {
					IdentType ident = this->scanDescriptor(text, matches, offset);
					if (ident == IdentLink) {
						expectedIndexLine = _line + 2;
					}
				} else if (std::regex_match(text, matches, _dataAddressRegex)) {
					this->scanDataAddress(text, matches, offset);
				} else if (std::regex_match(text, matches, _overlayRegionRegex)) {
					this->scanOverlayRegion(text, matches, offset);
				} else if (std::regex_match(text, matches, _overlayAddressRegex)) {
					this->scanOverlayAddress(text, matches, offset);
				} else {
					_out.send(std::make_shared<Token>(TokenString, offset, _line, 0, text));
				}
			}
		}
	} catch (const std::exception& e) {
		_err = std::make_shared<Error>(ErrorRead, e.what());
	}
	_out.close();
}

bool Scanner::hasNext() {
	return _out.ok();
}

std::shared_ptr<Token> Scanner::next() {
	return _out.receive();
}

std::shared_ptr<Error> Scanner::err() {
	if (_err->type() == ErrorEOF) {
		return nullptr;
	}
	return _err;
}

IdentType Scanner::scanDescriptor(std::string s, std::smatch m, std::size_t offset) {
	_out.send(std::make_shared<Token>(
		TokenLength, offset, _line, 0, this->ltrim(m[1].str(), '0')));
	std::string ident {m[2].str()};
	_out.send(std::make_shared<Token>(
		TokenIdent, offset, _line, -1, ident));
	return Token::identType(ident);
}

void Scanner::scanDataAddress(std::string s, std::smatch m, std::size_t offset) {
	_out.send(std::make_shared<Token>(
		TokenDataOffset, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenDataLength, offset, _line, -1, this->ltrim(m[5].str(), '0')));
}

void Scanner::scanOverlayRegion(std::string s, std::smatch m, std::size_t offset) {
	_out.send(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[1].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[2].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[4].str(), '0')));
}

void Scanner::scanOverlayAddress(std::string s, std::smatch m, std::size_t offset) {
	_out.send(std::make_shared<Token>(
		TokenDataOffset, offset, _line, -1, this->ltrim(m[1].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenDataLength, offset, _line, -1, this->ltrim(m[2].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenRegionX, offset, _line, -1, this->ltrim(m[3].str(), '0')));
	_out.send(std::make_shared<Token>(
		TokenRegionY, offset, _line, -1, this->ltrim(m[4].str(), '0')));
}

void Scanner::eof() {
	_out.send(std::make_shared<Token>(TokenEOF, _offset, _line, _column));
}

char Scanner::read() {
	char c = (char) _in.get();
	if (!_in.good()) {
		ErrorType code;
		if (_in.eof()) {
			code = ErrorEOF;
		} else {
			code = ErrorRead;
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

const std::string Scanner::rtrim(std::string s, char c) const {
	auto pos = s.find_last_not_of(c);
	if (pos == std::string::npos) {
		return s;
	} else {
		return s.erase(pos + 1);
	}
}

}

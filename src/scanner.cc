#include <chrono>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::istream& in) : _in {in} {}

Scanner::~Scanner() {}

std::shared_ptr<Error> Scanner::error() {
	if (_err == nullptr) {
		return nullptr;
	}
	if (_err->type() == ErrorEOF) {
		return nullptr;
	}
	return _err;
}

void Scanner::scan() {
	bool beforeCompatSep = true;
	int expectedIndexLine = -1;
	int expectedStringLine = -1;
	std::size_t prevTextLen = 0;
	std::size_t offset = 0;

	while (true) {
		++_line;
		_column = 0;
		_offset += prevTextLen;
		offset = _offset;

		// Check for binary data
		char c = this->peek();
		if (_err != nullptr) {
			if (_err->type() == ErrorEOF) {
				this->eof();
			}
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
		_in.getline(raw, LineLen, '\n');
		std::string text {raw};

		// UHS uses DOS-style line endings
		text = Strings::rtrim(text, '\r');

		// Number of bytes processed (for our offset)
		prevTextLen = text.length() + strlen(EOL);

		// Empty lines may be safely ignored
		if (text.length() == 0) {
			continue;
		}

		// All numbers are indexes in 88a, and link elements contain an index
		if (Strings::isInt(text) && (beforeCompatSep || _line == expectedIndexLine)) {
			_out.send(std::make_shared<Token>(
				TokenIndex, _offset, _line, 0, Strings::ltrim(text, '0')));
			expectedIndexLine = -1;
			continue;
		}

		// All descriptors are immediately followed by a string label
		if (_line == expectedStringLine) {
			_out.send(std::make_shared<Token>(TokenString, offset, _line, 0, text));
			expectedStringLine = -1;
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
				ElementType elementType = this->scanDescriptor(matches, offset);
				if (elementType == ElementLink) {
					expectedIndexLine = _line + 2;
				}
				expectedStringLine = _line + 1;
			} else if (std::regex_match(text, matches, _dataAddressRegex)) {
				this->scanDataAddress(matches, offset);
			} else if (std::regex_match(text, matches, _hyperpngRegionRegex)) {
				this->scanHyperpngRegion(matches, offset);
			} else if (std::regex_match(text, matches, _overlayAddressRegex)) {
				this->scanOverlayAddress(matches, offset);
			} else {
				_out.send(std::make_shared<Token>(TokenString, offset, _line, 0, text));
			}
		}
	}
	_out.close();
}

bool Scanner::hasNext() {
	return _out.ok();
}

std::shared_ptr<Token> Scanner::next() {
	return _out.receive();
}

ElementType Scanner::scanDescriptor(std::smatch m, std::size_t offset) {
	_out.send(std::make_shared<Token>(
		TokenLength, offset, _line, 0, Strings::ltrim(m[1].str(), '0')));
	std::string ident {m[2].str()};
	_out.send(std::make_shared<Token>(TokenIdent, offset, _line, 0, ident));
	return Element::elementType(ident);
}

void Scanner::scanData(std::smatch m, std::size_t offset, std::vector<TokenType> tokens) {
	for (std::vector<TokenType>::size_type i = 0; i < tokens.size(); ++i) {
		if (m[i+1].length() > 0) {
			_out.send(std::make_shared<Token>(
				tokens[i], offset, _line, m.position(i+1), Strings::ltrim(m[i+1].str(), '0')));
		}
	}
}

void Scanner::scanDataAddress(std::smatch m, std::size_t offset) {
	std::vector<TokenType> tokens {TokenDataType, TokenDataOffset, TokenDataLength};
	scanData(m, offset, tokens);
}

void Scanner::scanHyperpngRegion(std::smatch m, std::size_t offset) {
	std::vector<TokenType> tokens {TokenCoordX, TokenCoordY, TokenCoordX, TokenCoordY};
	scanData(m, offset, tokens);
}

void Scanner::scanOverlayAddress(std::smatch m, std::size_t offset) {
	std::vector<TokenType> tokens {TokenDataOffset, TokenDataLength, TokenCoordX, TokenCoordY};
	scanData(m, offset, tokens);
}

void Scanner::eof() {
	_out.send(std::make_shared<Token>(TokenEOF, _offset, _line, _column));
}

char Scanner::peek() {
	char c = (char) _in.peek();
	if (! _in.good()) {
		this->handleReadError();
	}
	return c;
}

char Scanner::read() {
	char c = (char) _in.get();

	if (! _in.good()) {
		this->handleReadError();
		return c;
	}
	++_column;
	++_offset;

	return c;
}

void Scanner::handleReadError() {
	if (_in.eof()) {
		_err = std::make_shared<Error>(ErrorEOF);
	} else {
		_err = std::make_shared<Error>(ErrorRead, "could not read 1 byte");
		_err->finalize(_line, _column);
	}
}

Scanner::TokenQueue::TokenQueue() : _open(true) {}

Scanner::TokenQueue::~TokenQueue() {}

bool Scanner::TokenQueue::send(std::shared_ptr<Token> t) {
	if (! _open) {
		return false;
	}
	std::lock_guard<std::mutex> m {_mutex};
	_queue.push(t);

	return true;
}

std::shared_ptr<Token> Scanner::TokenQueue::receive() {
	while (this->empty()) { // Unlikely
		if (! this->ok()) {
			return nullptr;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> m {_mutex};
	std::shared_ptr<Token> t {_queue.front()};
	_queue.pop();
	
	return t;
}

bool Scanner::TokenQueue::empty() {
	std::lock_guard<std::mutex> m {_mutex};
	return _queue.empty();
}

bool Scanner::TokenQueue::ok() {
	std::lock_guard<std::mutex> m {_mutex};
	return _open || ! _queue.empty();
}

void Scanner::TokenQueue::close() {
	_open = false;
}

}

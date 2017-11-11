#include <chrono>
#include <iostream>
#include <thread>
#include "uhs.h"

namespace UHS {

Scanner::Scanner(std::shared_ptr<Pipe> p) : _pipe {p} {
	_pipe->addHandler([=](const char* s, std::streamsize n) {
		this->scan(s, n);
	});
}

Scanner::~Scanner() {}

void Scanner::scan(const char* buf, std::streamsize n) {
	std::size_t len = n;
	std::string s {buf, len};

	for (std::size_t i = 0; i < len; ++i) {
		char c = s[i];

		if (! _binaryMode) {
			switch (c) {
			case Token::DataSep:
				_binaryMode = true;
				break; // This is safe; it always occurs at column 0
			case '\n':
				this->scanLine();
				_offset += _buf.length() + 1;
				_buf.clear();
				++_line;
				continue;
			}
		}
		_buf += c;
	}

	std::size_t column = _buf.length();

	if (_pipe->eof()) {
		this->scanData();
		_offset += column;
		this->sendEOF(column);
		return;
	}
	if (! _pipe->good()) {
		_err = _pipe->error();
		if (_err == nullptr) {
			_err = std::make_shared<Error>(ErrorRead, "could not read file");
		}
		_err->finalize(_line, column);
	}
}

bool Scanner::hasNext() {
	return _out.ok();
}

std::shared_ptr<Token> Scanner::next() {
	return _out.receive();
}

std::shared_ptr<Error> Scanner::error() {
	if (_err == nullptr) {
		return nullptr;
	}
	if (_err->type() == ErrorEOF) {
		return nullptr;
	}
	return _err;
}

void Scanner::scanLine() {
	// UHS uses DOS-style line endings
	auto s = Strings::rtrim(_buf, '\r');
	// Empty lines may be safely ignored
	if (s.length() == 0) {
		return;
	}

	// All numbers are indexes in 88a, and link elements contain an index
	if (Strings::isInt(s) && (_beforeCompatSep || _line == _expectedIndexLine)) {
		_out.send(std::make_shared<Token>(
			TokenIndex, _offset, _line, 0, Strings::ltrim(s, '0')));
		_expectedIndexLine = -1;
		return;
	}

	// All descriptors are immediately followed by a string label
	if (_line == _expectedStringLine) {
		_out.send(std::make_shared<Token>(TokenString, _offset, _line, 0, s));
		_expectedStringLine = -1;
		return;
	}

	// Check for exact line matches
	if (s == Token::CompatSep) {
		_beforeCompatSep = false;
		_out.send(std::make_shared<Token>(TokenCompatSep, _offset, _line));
	} else if (s == Token::CreditSep) {
		_out.send(std::make_shared<Token>(TokenCreditSep, _offset, _line));
	} else if (s == Token::NestedElementSep) {
		_out.send(std::make_shared<Token>(TokenNestedElementSep, _offset, _line));
	} else if (s == Token::NestedTextSep) {
		_out.send(std::make_shared<Token>(TokenNestedTextSep, _offset, _line));
	} else if (s == Token::ParagraphSep) {
		_out.send(std::make_shared<Token>(TokenParagraphSep, _offset, _line));
	} else if (s == Token::Signature) {
		_out.send(std::make_shared<Token>(TokenSignature, _offset, _line));
	} else {
		// Check for line match patterns
		std::smatch matches;
		if (std::regex_match(s, matches, _descriptorRegex)) {
			ElementType elementType = this->scanDescriptor(matches);
			if (elementType == ElementLink) {
				_expectedIndexLine = _line + 2;
			}
			_expectedStringLine = _line + 1;
		} else if (std::regex_match(s, matches, _dataAddressRegex)) {
			this->scanDataAddress(matches);
		} else if (std::regex_match(s, matches, _hyperpngRegionRegex)) {
			this->scanHyperpngRegion(matches);
		} else if (std::regex_match(s, matches, _overlayAddressRegex)) {
			this->scanOverlayAddress(matches);
		} else {
			_out.send(std::make_shared<Token>(TokenString, _offset, _line, 0, s));
		}
	}
}

void Scanner::scanData() {
	_out.send(std::make_shared<Token>(TokenData, _offset + 1, _line, 1, _buf));
}

ElementType Scanner::scanDescriptor(std::smatch m) {
	_out.send(std::make_shared<Token>(
		TokenLength, _offset, _line, 0, Strings::ltrim(m[1].str(), '0')));
	std::string ident {m[2].str()};
	auto column = m.position(2);
	_out.send(std::make_shared<Token>(TokenIdent, _offset + column, _line, column, ident));
	return Element::elementType(ident);
}

void Scanner::scanDataAddress(std::smatch m) {
	std::vector<TokenType> tokens {TokenDataType, TokenDataOffset, TokenDataLength};
	this->scanMatches(m, tokens);
}

void Scanner::scanHyperpngRegion(std::smatch m) {
	std::vector<TokenType> tokens {TokenCoordX, TokenCoordY, TokenCoordX, TokenCoordY};
	this->scanMatches(m, tokens);
}

void Scanner::scanOverlayAddress(std::smatch m) {
	std::vector<TokenType> tokens {TokenDataOffset, TokenDataLength, TokenCoordX, TokenCoordY};
	this->scanMatches(m, tokens);
}

void Scanner::scanMatches(std::smatch m, std::vector<TokenType> tokens) {
	for (std::vector<TokenType>::size_type i = 0; i < tokens.size(); ++i) {
		if (m[i+1].length() > 0) {
			_out.send(std::make_shared<Token>(
				tokens[i], _offset, _line, m.position(i+1), Strings::ltrim(m[i+1].str(), '0')));
		}
	}
}

void Scanner::sendEOF(std::size_t column) {
	_out.send(std::make_shared<Token>(TokenEOF, _offset, _line, column));
}

Scanner::TokenChannel::TokenChannel() : _open(true) {}

Scanner::TokenChannel::~TokenChannel() {}

bool Scanner::TokenChannel::send(std::shared_ptr<Token> t) {
	if (! _open) {
		return false;
	}
	std::lock_guard<std::mutex> m {_mutex};
	_queue.push(t);

	return true;
}

std::shared_ptr<Token> Scanner::TokenChannel::receive() {
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

bool Scanner::TokenChannel::empty() {
	std::lock_guard<std::mutex> m {_mutex};
	return _queue.empty();
}

bool Scanner::TokenChannel::ok() {
	std::lock_guard<std::mutex> m {_mutex};
	return _open || ! _queue.empty();
}

void Scanner::TokenChannel::close() {
	_open = false;
}

}

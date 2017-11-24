#include <chrono>
#include <thread>
#include "uhs.h"

namespace UHS {

Tokenizer::Tokenizer(Pipe& p) : _pipe {p}, _out {p} {
	_pipe.addHandler([=](const char* s, std::streamsize n) {
		this->tokenize(s, n);
	});
}

std::unique_ptr<Error> Tokenizer::error() {
	if (_err == nullptr) {
		return nullptr;
	}
	if (_err->type() == ErrorEOF) {
		return nullptr;
	}
	return std::move(_err);
}

void Tokenizer::tokenize(const char* buf, std::streamsize n) {
	std::size_t len = n;
	std::string s {buf, len};
	std::size_t i = 0;
	std::size_t column = 0;
	std::size_t lineLen;

	while (i < len) {
		if (_binaryMode) {
			_buf += s.substr(i);
			i = s.length();
			break;
		}

		auto pos = s.find_first_of("\x1A\n", i);
		if (pos == std::string::npos) {
			_buf += s.substr(i);
			i = s.length();
		} else {
			_buf += s.substr(i, pos - i);
			i = pos;
		}

		if (s[i] == Token::DataSep || s[i] == '\n') {
			this->tokenizeLine();
			lineLen = _buf.length() + 1;
			_buf.clear();
			_offset += lineLen;

			switch (s[i]) {
			case Token::DataSep:
				_binaryMode = true;
				column = lineLen;
				break;
			case '\n':
				++_line;
				column = 0;
				break;
			}
			++i;
		}
	}

	if (_pipe.eof()) {
		auto eofColumn = column + _buf.length();

		if (! _beforeHeaderSep) {
			auto crcColumn = eofColumn - CRC::ByteSize;
			auto dataLen = crcColumn - column;
			auto data = _buf.substr(0, dataLen);
			this->tokenizeData(data, column);

			column = crcColumn;
			_offset += column;
			auto crc = _buf.substr(dataLen);
			this->tokenizeCRC(crc, column);

			_offset += CRC::ByteSize;
		}
		this->tokenizeEOF(eofColumn);
		return;
	}
}

bool Tokenizer::hasNext() {
	return _out.ok();
}

std::unique_ptr<const Token> Tokenizer::next() {
	auto t = _out.receive();
	if (t == nullptr) {
		auto err = _out.error();
		if (err != nullptr) {
			_err = std::move(err);
		}
	}
	return t;
}

void Tokenizer::tokenizeLine() {
	// UHS uses DOS-style line endings
	auto s = Strings::rtrim(_buf, '\r');

	// Empty lines may be safely ignored
	if (s.length() == 0) {
		return;
	}

	// All numbers are indexes in 88a, and link elements contain an index
	if (Strings::isInt(s) && (_beforeHeaderSep || _line == _expectedIndexLine)) {
		_out.send({TokenIndex, _offset, _line, 0, Strings::ltrim(s, '0')});
		_expectedIndexLine = -1;
		return;
	}

	// All descriptors are immediately followed by a string title
	if (_line == _expectedStringLine) {
		_out.send({TokenString, _offset, _line, 0, s});
		_expectedStringLine = -1;
		return;
	}

	// Check for exact line matches
	if (s == Token::HeaderSep) {
		_beforeHeaderSep = false;
		_out.send({TokenHeaderSep, _offset, _line});
	} else if (s == Token::CreditSep) {
		_out.send({TokenCreditSep, _offset, _line});
	} else if (s == Token::NestedElementSep) {
		_out.send({TokenNestedElementSep, _offset, _line});
	} else if (s == Token::NestedTextSep) {
		_out.send({TokenNestedTextSep, _offset, _line});
	} else if (s == Token::ParagraphSep) {
		_out.send({TokenParagraphSep, _offset, _line});
	} else if (s == Token::Signature) {
		_out.send({TokenSignature, _offset, _line});
	} else {
		// Check for line match patterns
		std::smatch matches;
		if (std::regex_match(s, matches, _descriptorRegex)) {
			ElementType elementType = this->tokenizeDescriptor(matches);
			if (elementType == ElementLink) {
				_expectedIndexLine = _line + 2;
			}
			_expectedStringLine = _line + 1;
		} else if (std::regex_match(s, matches, _dataAddressRegex)) {
			this->tokenizeDataAddress(matches);
		} else if (std::regex_match(s, matches, _hyperpngRegionRegex)) {
			this->tokenizeHyperpngRegion(matches);
		} else if (std::regex_match(s, matches, _overlayAddressRegex)) {
			this->tokenizeOverlayAddress(matches);
		} else {
			_out.send({TokenString, _offset, _line, 0, s});
		}
	}
}

ElementType Tokenizer::tokenizeDescriptor(const std::smatch& m) {
	_out.send({TokenLength, _offset, _line, 0, Strings::ltrim(m[1].str(), '0')});
	std::string ident {m[2].str()};
	auto column = static_cast<std::size_t>(m.position(2));
	_out.send({TokenIdent, _offset + column, _line, column, ident});
	return Element::elementType(ident);
}

void Tokenizer::tokenizeDataAddress(const std::smatch& m) {
	this->tokenizeMatches(m, {TokenTextFormat, TokenDataOffset, TokenDataLength});
}

void Tokenizer::tokenizeHyperpngRegion(const std::smatch& m) {
	this->tokenizeMatches(m, {TokenCoordX, TokenCoordY, TokenCoordX, TokenCoordY});
}

void Tokenizer::tokenizeOverlayAddress(const std::smatch& m) {
	this->tokenizeMatches(m, {TokenDataOffset, TokenDataLength, TokenCoordX, TokenCoordY});
}

void Tokenizer::tokenizeMatches(const std::smatch& m, const std::vector<TokenType>&& tokens) {
	for (std::vector<TokenType>::size_type i = 0; i < tokens.size(); ++i) {
		if (m[i + 1].length() > 0) {
			auto column = static_cast<std::size_t>(m.position(i + 1));
			_out.send({tokens[i], _offset, _line, column,
				Strings::ltrim(m[i + 1].str(), '0')});
		}
	}
}

void Tokenizer::tokenizeData(const std::string& data, std::size_t column) {
	_out.send({TokenData, _offset, _line, column, data});
}

void Tokenizer::tokenizeCRC(const std::string& crc, std::size_t column) {
	_out.send({TokenCRC, _offset, _line, column, crc});
}

void Tokenizer::tokenizeEOF(std::size_t column) {
	_out.send({TokenEOF, _offset, _line, column});
}

Tokenizer::TokenChannel::TokenChannel(Pipe& p) : _pipe {p} {}

std::unique_ptr<Error> Tokenizer::TokenChannel::error() {
	return std::move(_err);
}

bool Tokenizer::TokenChannel::send(const Token&& t) {
	if (! _open) {
		return false;
	}
	std::lock_guard<std::mutex> m {_mutex};
	_queue.push(t);

	return true;
}

std::unique_ptr<const Token> Tokenizer::TokenChannel::receive() {
	while (this->empty()) { // Unlikely
		if (! this->ok()) {
			auto err = _pipe.error();
			if (err != nullptr) {
				_err = std::move(err);
			}
			return nullptr;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> m {_mutex};
	auto t = std::make_unique<const Token>(_queue.front());
	_queue.pop();
	
	return t;
}

bool Tokenizer::TokenChannel::empty() const {
	std::lock_guard<std::mutex> m {_mutex};
	return _queue.empty();
}

bool Tokenizer::TokenChannel::ok() const {
	std::lock_guard<std::mutex> m {_mutex};

	if (_pipe.error() != nullptr) {
		return false;
	}
	return _open || ! _queue.empty();
}

void Tokenizer::TokenChannel::close() {
	_open = false;
}

}

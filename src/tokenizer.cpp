#include "uhs.h"
#include <chrono>
#include <thread>

namespace UHS {

Tokenizer::Tokenizer(Pipe& p) : _pipe{p}, _out{p} {
	_pipe.addHandler([=](const char* s, std::streamsize n) { this->tokenize(s, n); });
}

void Tokenizer::tokenize(const char* buf, std::streamsize n) {
	std::size_t len = n;
	std::string s{buf, len};
	std::size_t i = 0;
	std::size_t column = 0;
	std::size_t lineLen;

	while (i < len) {
		if (_binaryMode) {
			_buf += s.substr(i);
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

		if (!_beforeHeaderSep) {
			auto crcColumn = eofColumn - CRC::ByteLen;
			auto dataLen = crcColumn - column;
			auto data = _buf.substr(0, dataLen);
			this->tokenizeData(data, column);

			column = crcColumn;
			_offset += column;
			auto crc = _buf.substr(dataLen);
			this->tokenizeCRC(crc, column);

			_offset += CRC::ByteLen;
		}

		this->tokenizeEOF(eofColumn);
		_out.close();
		return;
	}
}

bool Tokenizer::hasNext() {
	return _out.ok();
}

std::unique_ptr<const Token> Tokenizer::next() {
	return _out.receive();
}

void Tokenizer::tokenizeLine() {
	// UHS uses DOS-style line endings
	auto s = Strings::rtrim(_buf, '\r');

	// Empty lines may be safely ignored
	if (s.length() == 0) {
		return;
	}

	// All numbers are line numbers in 88a, and link elements contain a line number
	if (Strings::isInt(s) && (_beforeHeaderSep || _line == _expectedLineTokenLine)) {
		_out.send({TokenType::Line, _offset, _line, 0, Strings::ltrim(s, '0')});
		_expectedLineTokenLine = -1;
		return;
	}

	// All descriptors are immediately followed by a string title
	if (_line == _expectedStringTokenLine) {
		_out.send({TokenType::String, _offset, _line, 0, s});
		_expectedStringTokenLine = -1;
		return;
	}

	// Check for exact line matches
	if (s == Token::HeaderSep && _beforeHeaderSep) {
		_beforeHeaderSep = false;
		_out.send({TokenType::HeaderSep, _offset, _line});
	} else if (s == Token::CreditSep && _beforeHeaderSep) {
		_out.send({TokenType::CreditSep, _offset, _line});
	} else if (s == Token::NestedElementSep && !_beforeHeaderSep) {
		_out.send({TokenType::NestedElementSep, _offset, _line});
	} else if (s == Token::NestedTextSep && !_beforeHeaderSep) {
		_out.send({TokenType::NestedTextSep, _offset, _line});
	} else if (s == Token::ParagraphSep && !_beforeHeaderSep) {
		_out.send({TokenType::NestedParagraphSep, _offset, _line});
	} else if (s == Token::Signature && _line == 1) {
		_out.send({TokenType::Signature, _offset, _line});
	} else {
		// Check for line match patterns
		std::smatch matches;
		if (std::regex_match(s, matches, Regex::Descriptor)) {
			ElementType elementType = this->tokenizeDescriptor(matches);
			if (elementType == ElementType::Link) {
				_expectedLineTokenLine = _line + 2;
			}
			_expectedStringTokenLine = _line + 1;
		} else if (std::regex_match(s, matches, Regex::DataAddress)) {
			this->tokenizeDataAddress(matches);
		} else if (std::regex_match(s, matches, Regex::HyperpngRegion)) {
			this->tokenizeHyperpngRegion(matches);
		} else if (std::regex_match(s, matches, Regex::OverlayAddress)) {
			this->tokenizeOverlayAddress(matches);
		} else {
			_out.send({TokenType::String, _offset, _line, 0, s});
		}
	}
}

ElementType Tokenizer::tokenizeDescriptor(const std::smatch& m) {
	_out.send({TokenType::Length, _offset, _line, 0, Strings::ltrim(m[1].str(), '0')});
	std::string ident{m[2].str()};
	auto column = static_cast<std::size_t>(m.position(2));
	_out.send({TokenType::Ident, _offset + column, _line, column, ident});
	return Element::elementType(ident);
}

void Tokenizer::tokenizeDataAddress(const std::smatch& m) {
	this->tokenizeMatches(m,
	    {
	        TokenType::TextFormat,
	        TokenType::DataOffset,
	        TokenType::DataLength,
	    });
}

void Tokenizer::tokenizeHyperpngRegion(const std::smatch& m) {
	this->tokenizeMatches(m,
	    {
	        TokenType::CoordX,
	        TokenType::CoordY,
	        TokenType::CoordX,
	        TokenType::CoordY,
	    });
}

void Tokenizer::tokenizeOverlayAddress(const std::smatch& m) {
	this->tokenizeMatches(m,
	    {
	        TokenType::DataOffset,
	        TokenType::DataLength,
	        TokenType::CoordX,
	        TokenType::CoordY,
	    });
}

void Tokenizer::tokenizeMatches(
    const std::smatch& m, const std::vector<TokenType>&& tokens) {
	for (std::vector<TokenType>::size_type i = 0; i < tokens.size(); ++i) {
		if (m[i + 1].length() > 0) {
			auto column = static_cast<std::size_t>(m.position(i + 1));
			auto value = Strings::ltrim(m[i + 1].str(), '0');
			_out.send({tokens[i], _offset, _line, column, value});
		}
	}
}

void Tokenizer::tokenizeData(const std::string& data, std::size_t column) {
	_out.send({TokenType::Data, _offset, _line, column, data});
}

void Tokenizer::tokenizeCRC(const std::string& crc, std::size_t column) {
	_out.send({TokenType::CRC, _offset, _line, column, crc});
}

void Tokenizer::tokenizeEOF(std::size_t column) {
	_out.send({TokenType::FileEnd, _offset, _line, column});
}

Tokenizer::TokenChannel::TokenChannel(Pipe& p) : _pipe{p} {}

void Tokenizer::TokenChannel::send(const Token&& t) {
	std::lock_guard<std::mutex> m{_mutex};
	assert(_open);
	_queue.push(t);
}

std::unique_ptr<const Token> Tokenizer::TokenChannel::receive() {
	if (auto err = _pipe.error(); err != nullptr) {
		std::rethrow_exception(err);
	}
	while (this->empty()) { // Uncommon
		assert(this->ok());
		std::this_thread::sleep_for(std::chrono::milliseconds(1));\
		
		if (auto err = _pipe.error(); err != nullptr) {
			std::rethrow_exception(err);
		}
	}
	std::lock_guard<std::mutex> m{_mutex};
	auto t = std::make_unique<const Token>(_queue.front());
	_queue.pop();

	return t;
}

bool Tokenizer::TokenChannel::empty() const {
	std::lock_guard<std::mutex> m{_mutex};
	return _queue.empty();
}

bool Tokenizer::TokenChannel::ok() const {
	std::lock_guard<std::mutex> m{_mutex};
	return _open || !_queue.empty();
}

void Tokenizer::TokenChannel::close() {
	_open = false;
}

} // namespace UHS

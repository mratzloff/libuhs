#include "uhs.h"
#include <chrono>
#include <thread>

namespace UHS {

Tokenizer::Tokenizer(Pipe& p) : pipe_{p}, out_{p} {
	pipe_.addHandler([=](const char* s, std::streamsize n) { this->tokenize(s, n); });
}

void Tokenizer::tokenize(const char* buf, std::streamsize n) {
	std::size_t len = n;
	std::string s{buf, len};
	std::size_t i = 0;
	std::size_t column = 0;
	std::size_t lineLen;

	while (i < len) {
		if (binaryMode_) {
			buf_ += s.substr(i);
			break;
		}

		auto pos = s.find_first_of("\x1A\n", i);
		if (pos == std::string::npos) {
			buf_ += s.substr(i);
			i = s.length();
		} else {
			buf_ += s.substr(i, pos - i);
			i = pos;
		}

		if (s[i] == Token::DataSep || s[i] == '\n') {
			this->tokenizeLine();
			lineLen = buf_.length() + 1;
			buf_.clear();
			offset_ += lineLen;

			switch (s[i]) {
			case Token::DataSep:
				binaryMode_ = true;
				column = lineLen;
				break;
			case '\n':
				++line_;
				column = 0;
				break;
			}
			++i;
		}
	}

	if (pipe_.eof()) {
		auto eofColumn = column + buf_.length();

		if (!beforeHeaderSep_) {
			auto crcColumn = eofColumn - CRC::ByteLen;
			auto dataLen = crcColumn - column;
			auto data = buf_.substr(0, dataLen);
			this->tokenizeData(data, column);

			column = crcColumn;
			offset_ += column;
			auto crc = buf_.substr(dataLen);
			this->tokenizeCRC(crc, column);

			offset_ += CRC::ByteLen;
		}

		this->tokenizeEOF(eofColumn);
		out_.close();
		return;
	}
}

bool Tokenizer::hasNext() {
	return out_.ok();
}

std::unique_ptr<const Token> Tokenizer::next() {
	return out_.receive();
}

void Tokenizer::tokenizeLine() {
	// UHS uses DOS-style line endings
	auto s = Strings::rtrim(buf_, '\r');

	// Empty lines may be safely ignored
	if (s.length() == 0) {
		return;
	}

	// All numbers are line numbers in 88a, and link elements contain a line number
	if (Strings::isInt(s) && (beforeHeaderSep_ || line_ == expectedLineTokenLine_)) {
		out_.send({TokenType::Line, offset_, line_, 0, Strings::ltrim(s, '0')});
		expectedLineTokenLine_ = -1;
		return;
	}

	// All descriptors are immediately followed by a string title
	if (line_ == expectedStringTokenLine_) {
		out_.send({TokenType::String, offset_, line_, 0, s});
		expectedStringTokenLine_ = -1;
		return;
	}

	// Check for exact line matches
	if (s == Token::HeaderSep && beforeHeaderSep_) {
		beforeHeaderSep_ = false;
		out_.send({TokenType::HeaderSep, offset_, line_});
	} else if (s == Token::CreditSep && beforeHeaderSep_) {
		out_.send({TokenType::CreditSep, offset_, line_});
	} else if (s == Token::NestedElementSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedElementSep, offset_, line_});
	} else if (s == Token::NestedTextSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedTextSep, offset_, line_});
	} else if (s == Token::ParagraphSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedParagraphSep, offset_, line_});
	} else if (s == Token::Signature && line_ == 1) {
		out_.send({TokenType::Signature, offset_, line_});
	} else {
		// Check for line match patterns
		std::smatch matches;
		if (std::regex_match(s, matches, Regex::Descriptor)) {
			ElementType elementType = this->tokenizeDescriptor(matches);
			if (elementType == ElementType::Link) {
				expectedLineTokenLine_ = line_ + 2;
			}
			expectedStringTokenLine_ = line_ + 1;
		} else if (std::regex_match(s, matches, Regex::DataAddress)) {
			this->tokenizeDataAddress(matches);
		} else if (std::regex_match(s, matches, Regex::HyperpngRegion)) {
			this->tokenizeHyperpngRegion(matches);
		} else if (std::regex_match(s, matches, Regex::OverlayAddress)) {
			this->tokenizeOverlayAddress(matches);
		} else {
			out_.send({TokenType::String, offset_, line_, 0, s});
		}
	}
}

ElementType Tokenizer::tokenizeDescriptor(const std::smatch& m) {
	out_.send({TokenType::Length, offset_, line_, 0, Strings::ltrim(m[1].str(), '0')});
	std::string ident{m[2].str()};
	auto column = static_cast<std::size_t>(m.position(2));
	out_.send({TokenType::Ident, offset_ + column, line_, column, ident});
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
			out_.send({tokens[i], offset_, line_, column, value});
		}
	}
}

void Tokenizer::tokenizeData(const std::string& data, std::size_t column) {
	out_.send({TokenType::Data, offset_, line_, column, data});
}

void Tokenizer::tokenizeCRC(const std::string& crc, std::size_t column) {
	out_.send({TokenType::CRC, offset_, line_, column, crc});
}

void Tokenizer::tokenizeEOF(std::size_t column) {
	out_.send({TokenType::FileEnd, offset_, line_, column});
}

Tokenizer::TokenChannel::TokenChannel(Pipe& p) : pipe_{p} {}

void Tokenizer::TokenChannel::send(const Token&& t) {
	std::lock_guard<std::mutex> m{mutex_};
	assert(open_);
	queue_.push(t);
}

std::unique_ptr<const Token> Tokenizer::TokenChannel::receive() {
	if (auto err = pipe_.error()) {
		std::rethrow_exception(err);
	}
	while (this->empty()) { // Uncommon
		assert(this->ok());
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (auto err = pipe_.error()) {
			std::rethrow_exception(err);
		}
	}
	std::lock_guard<std::mutex> m{mutex_};
	auto t = std::make_unique<const Token>(queue_.front());
	queue_.pop();

	return t;
}

bool Tokenizer::TokenChannel::empty() const {
	std::lock_guard<std::mutex> m{mutex_};
	return queue_.empty();
}

bool Tokenizer::TokenChannel::ok() const {
	std::lock_guard<std::mutex> m{mutex_};
	return open_ || !queue_.empty();
}

void Tokenizer::TokenChannel::close() {
	open_ = false;
}

} // namespace UHS

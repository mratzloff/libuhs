#include <regex>
#include <stdexcept>

#include "uhs/crc.h"
#include "uhs/element.h"
#include "uhs/regex.h"
#include "uhs/strings.h"
#include "uhs/tokenizer.h"

namespace UHS {

Tokenizer::Tokenizer(Pipe& pipe) : out_{pipe}, pipe_{pipe} {
	pipe_.addHandler([=, this](char const* buffer, std::streamsize length) {
		this->tokenize(buffer, length);
	});
}

bool Tokenizer::hasNext() {
	return out_.ok();
}

std::unique_ptr<Token const> Tokenizer::next() {
	return out_.receive();
}

void Tokenizer::tokenize(char const* buffer, std::streamsize length) {
	std::string localBuffer{buffer, static_cast<std::size_t>(length)};
	std::size_t column = 0;

	for (auto i = 0; i < length;) {
		if (binaryMode_) {
			buffer_ += localBuffer.substr(i);
			break;
		}

		auto position = localBuffer.find_first_of("\x1A\n", i);
		if (position == std::string::npos) {
			buffer_ += localBuffer.substr(i);
			break;
		}
		buffer_ += localBuffer.substr(i, position - i);
		i = position;

		if (localBuffer[i] == Token::DataSep || localBuffer[i] == '\n') {
			this->tokenizeLine();
			auto lineLength = buffer_.length() + 1;
			buffer_.clear();
			offset_ += lineLength;

			switch (localBuffer[i]) {
			case Token::DataSep:
				binaryMode_ = true;
				column = lineLength;
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
		auto eofColumn = column + buffer_.length();

		if (!beforeHeaderSep_) {
			auto crcColumn = eofColumn - CRC::ByteLength;
			auto dataLength = crcColumn - column;
			auto data = buffer_.substr(0, dataLength);
			this->tokenizeData(data, column);

			column = crcColumn;
			offset_ += column;
			auto crc = buffer_.substr(dataLength);
			this->tokenizeCRC(crc, column);

			offset_ += CRC::ByteLength;
		}

		this->tokenizeEOF(eofColumn);
		out_.close();
		return;
	}
}

void Tokenizer::tokenizeCRC(std::string const& crc, std::size_t column) {
	out_.send({TokenType::CRC, offset_, line_, column, crc});
}

void Tokenizer::tokenizeData(std::string const& data, std::size_t column) {
	out_.send({TokenType::Data, offset_, line_, column, data});
}

void Tokenizer::tokenizeDataAddress(std::smatch const& matches) {
	this->tokenizeMatches(matches,
	    {
	        TokenType::TextFormat,
	        TokenType::DataOffset,
	        TokenType::DataLength,
	    });
}

ElementType Tokenizer::tokenizeDescriptor(std::smatch const& matches) {
	out_.send(
	    {TokenType::Length, offset_, line_, 0, Strings::ltrim(matches[1].str(), '0')});
	std::string ident{matches[2].str()};
	auto column = static_cast<std::size_t>(matches.position(2));
	out_.send({TokenType::Ident, offset_ + column, line_, column, ident});
	return Element::elementType(ident);
}

void Tokenizer::tokenizeEOF(std::size_t column) {
	out_.send({TokenType::FileEnd, offset_, line_, column});
}

void Tokenizer::tokenizeHyperpngRegion(std::smatch const& matches) {
	this->tokenizeMatches(matches,
	    {
	        TokenType::CoordX,
	        TokenType::CoordY,
	        TokenType::CoordX,
	        TokenType::CoordY,
	    });
}

void Tokenizer::tokenizeLine() {
	// UHS uses DOS-style line endings
	auto localBuffer = Strings::rtrim(buffer_, '\r');

	// Empty lines may be safely ignored
	if (localBuffer.length() == 0) {
		return;
	}

	// All numbers are line numbers in 88a, and link elements contain a line number
	if (Strings::isInt(localBuffer)
	    && (beforeHeaderSep_ || line_ == expectedLineTokenLine_)) {
		out_.send({TokenType::Line, offset_, line_, 0, Strings::ltrim(localBuffer, '0')});
		expectedLineTokenLine_ = -1;
		return;
	}

	// All descriptors are immediately followed by a string title
	if (line_ == expectedStringTokenLine_) {
		out_.send({TokenType::String, offset_, line_, 0, localBuffer});
		expectedStringTokenLine_ = -1;
		return;
	}

	// Check for exact line matches
	if (localBuffer == Token::HeaderSep && beforeHeaderSep_) {
		beforeHeaderSep_ = false;
		out_.send({TokenType::HeaderSep, offset_, line_});
	} else if (localBuffer == Token::CreditSep && beforeHeaderSep_) {
		out_.send({TokenType::CreditSep, offset_, line_});
	} else if (localBuffer == Token::NestedElementSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedElementSep, offset_, line_});
	} else if (localBuffer == Token::NestedTextSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedTextSep, offset_, line_});
	} else if (localBuffer == Token::ParagraphSep && !beforeHeaderSep_) {
		out_.send({TokenType::NestedParagraphSep, offset_, line_});
	} else if (localBuffer == Token::Signature && line_ == 1) {
		out_.send({TokenType::Signature, offset_, line_});
	} else {
		// Check for line match patterns
		std::smatch matches;
		if (std::regex_match(localBuffer, matches, Regex::Descriptor)) {
			ElementType elementType = this->tokenizeDescriptor(matches);
			if (elementType == ElementType::Link) {
				expectedLineTokenLine_ = line_ + 2;
			}
			expectedStringTokenLine_ = line_ + 1;
		} else if (std::regex_match(localBuffer, matches, Regex::DataAddress)) {
			this->tokenizeDataAddress(matches);
		} else if (std::regex_match(localBuffer, matches, Regex::HyperpngRegion)) {
			this->tokenizeHyperpngRegion(matches);
		} else if (std::regex_match(localBuffer, matches, Regex::OverlayAddress)) {
			this->tokenizeOverlayAddress(matches);
		} else {
			out_.send({TokenType::String, offset_, line_, 0, localBuffer});
		}
	}
}

void Tokenizer::tokenizeMatches(
    std::smatch const& matches, std::vector<TokenType> const&& tokens) {

	for (std::vector<TokenType>::size_type i = 0; i < tokens.size(); ++i) {
		if (matches[i + 1].length() > 0) {
			auto column = static_cast<std::size_t>(matches.position(i + 1));
			auto value = Strings::ltrim(matches[i + 1].str(), '0');
			out_.send({tokens[i], offset_, line_, column, value});
		}
	}
}

void Tokenizer::tokenizeOverlayAddress(std::smatch const& matches) {
	this->tokenizeMatches(matches,
	    {
	        TokenType::DataOffset,
	        TokenType::DataLength,
	        TokenType::CoordX,
	        TokenType::CoordY,
	    });
}

Tokenizer::TokenChannel::TokenChannel(Pipe& pipe) : pipe_{pipe} {}

bool Tokenizer::TokenChannel::ok() const {
	std::lock_guard<std::mutex> m{mutex_};
	return open_ || !queue_.empty();
}

void Tokenizer::TokenChannel::close() {
	{
		std::lock_guard<std::mutex> m{mutex_};
		open_ = false;
	}
	ready_.notify_one();
}

std::unique_ptr<Token const> Tokenizer::TokenChannel::receive() {
	std::unique_lock<std::mutex> lock{mutex_};
	ready_.wait(lock, [this] { return !queue_.empty() || !open_ || pipe_.error(); });

	if (auto err = pipe_.error()) {
		std::rethrow_exception(err);
	}

	if (queue_.empty()) {
		throw std::runtime_error("token channel closed with no pending tokens");
	}
	auto token = std::make_unique<Token const>(queue_.front());
	queue_.pop();

	return token;
}

void Tokenizer::TokenChannel::send(Token const&& token) {
	{
		std::lock_guard<std::mutex> m{mutex_};
		if (!open_) {
			throw std::runtime_error("cannot send on a closed token channel");
		}
		queue_.push(token);
	}
	ready_.notify_one();
}

} // namespace UHS

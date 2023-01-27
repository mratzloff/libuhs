#include "tinyutf8.h"
#include "uhs.h"

namespace UHS {

Codec::Codec(const Options options) : options_{options} {}

const std::string Codec::createKey(std::string secret) const {
	return this->encode96a(secret, KeySeed, false);
}

const std::string Codec::decode88a(std::string encoded) const {
	std::string& decoded = encoded;
	const std::size_t length = encoded.length();

	for (std::size_t i = 0; i < length; ++i) {
		char c = encoded[i];
		if (Strings::isPrintable(c)) {
			int offset = (c < 80) ? Strings::AsciiStart : Strings::AsciiEnd;
			decoded[i] = static_cast<char>(static_cast<int>(c) * 2 - offset);
		} else {
			decoded[i] = '?';
		}
	}
	return decoded;
}

const std::string Codec::decode96a(
    std::string encoded, std::string key, bool isTextElement) const {

	std::string& decoded = encoded;
	const std::size_t length = encoded.length();
	const std::size_t keyLength = key.length();

	for (std::size_t i = 0; i < length; ++i) {
		auto keystream = this->keystream(key, keyLength, i, isTextElement);
		decoded[i] = this->toPrintable(encoded[i] - keystream);
	}
	return decoded;
}

const std::string Codec::decodeSpecialChars(const std::string& encoded) const {
	std::string segment;
	auto length = encoded.length();

	for (std::size_t i = 0; i < length; ++i) {
		if (i + 3 > length || encoded.substr(i, 2) != "#a") {
			segment += encoded[i];
			continue;
		}

		if (encoded[i + 3] == '-') {
			if (!options_.quiet) {
				logger_.warn("unexpected sequence: #a-");
			}
			segment += encoded[i];
			continue;
		}

		const auto offset = i + 3;
		auto pos = encoded.find("#a-", offset);

		if (pos == std::string::npos) {
			if (!options_.quiet) {
				logger_.warn("unexpected sequence: expected #a-");
			}
			segment += encoded[i];
			continue;
		}

		const auto length = pos - offset;
		const auto value = encoded.substr(offset, length);

		try {
			segment += this->toChars_.at(value);
		} catch (const std::out_of_range& err) {
			if (!options_.quiet) {
				logger_.warn("unexpected sequence: %s", value);
			}
			segment += encoded[i];
			break;
		}

		i += length + 5; // Advance to "-" of "#a-" (will increment next loop)
	}

	return segment;
}

const std::string Codec::encode88a(std::string decoded) const {
	std::string& encoded = decoded;
	const std::size_t length = encoded.length();

	for (std::size_t i = 0; i < length; ++i) {
		char c = decoded[i];
		if (Strings::isPrintable(c)) {
			int offset = (c % 2 == 0) ? Strings::AsciiStart : Strings::AsciiEnd;
			encoded[i] = static_cast<char>((static_cast<int>(c) + offset) / 2);
		}
	}
	return encoded;
}

const std::string Codec::encode96a(
    std::string decoded, std::string key, bool isTextElement) const {

	std::string& encoded = decoded;
	const std::size_t length = decoded.length();
	const std::size_t keyLength = key.length();

	for (std::size_t i = 0; i < length; ++i) {
		auto keystream = this->keystream(key, keyLength, i, isTextElement);
		encoded[i] = this->toPrintable(decoded[i] + keystream);
	}
	return encoded;
}

const std::string Codec::encodeSpecialChars(const std::string& decoded) const {
	std::string segment;
	tiny_utf8::string utf8String = decoded;
	auto length = utf8String.length();

	for (std::size_t i = 0; i < length; ++i) {
		std::string sequence;
		try {
			const auto c = (char32_t) utf8String[i];
			sequence = this->fromChars_.at(c);
			segment += Token::AsciiEncBegin;
			segment += sequence;
			segment += Token::AsciiEncEnd;
		} catch (const std::out_of_range& err) {
			segment += utf8String[i];
		}
	}

	return segment;
}

int Codec::keystream(
    std::string key, std::size_t keyLength, std::size_t line, bool isTextElement) const {

	const int intIndex = static_cast<int>(line); // Guarantee signedness
	const int offset = intIndex % keyLength;
	return int(key[offset]) ^ ((isTextElement ? offset : intIndex) + 40);
}

char Codec::toPrintable(int c) const {
	const int step = Strings::AsciiEnd - Strings::AsciiStart + 1;

	while (!Strings::isPrintable(c)) {
		if (c < Strings::AsciiStart) {
			c += step;
		} else if (c > Strings::AsciiEnd) {
			c -= step;
		} else {
			break;
		}
	}
	return char(c);
}

} // namespace UHS

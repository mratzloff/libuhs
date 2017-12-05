#include "uhs.h"

namespace UHS {

const std::string Codec::decode88a(std::string encoded) const {
	std::string& decoded = encoded;
	const std::size_t len = encoded.length();

	for (std::size_t i = 0; i < len; ++i) {
		char c = encoded[i];
		if (this->isPrintable(c)) {
			int offset = (c < 80) ? AsciiStart : AsciiEnd;
			decoded[i] = static_cast<char>(static_cast<int>(c) * 2 - offset);
		} else {
			decoded[i] = '?';
		}
	}
	return decoded;
}

const std::string Codec::encode88a(std::string decoded) const {
	std::string& encoded = decoded;
	const std::size_t len = encoded.length();

	for (std::size_t i = 0; i < len; ++i) {
		char c = decoded[i];
		if (this->isPrintable(c)) {
			int offset = (c % 2 == 0) ? AsciiStart : AsciiEnd;
			encoded[i] = static_cast<char>((static_cast<int>(c) + offset) / 2);
		}
	}
	return encoded;
}

const std::string Codec::decode96a(
    std::string encoded, std::string key, bool isTextElement) const {
	std::string& decoded = encoded;
	const std::size_t len = encoded.length();
	const std::size_t keyLen = key.length();

	for (std::size_t i = 0; i < len; ++i) {
		auto keystream = this->keystream(key, keyLen, i, isTextElement);
		decoded[i] = this->toPrintable(encoded[i] - keystream);
	}
	return decoded;
}

const std::string Codec::encode96a(
    std::string decoded, std::string key, bool isTextElement) const {
	std::string& encoded = decoded;
	const std::size_t len = decoded.length();
	const std::size_t keyLen = key.length();

	for (std::size_t i = 0; i < len; ++i) {
		auto keystream = this->keystream(key, keyLen, i, isTextElement);
		encoded[i] = this->toPrintable(decoded[i] + keystream);
	}
	return encoded;
}

const std::string Codec::createKey(std::string secret) const {
	return this->encode96a(secret, KeySeed, false);
}

int Codec::keystream(
    std::string key, std::size_t keyLen, std::size_t line, bool isTextElement) const {
	const int intIndex = static_cast<int>(line); // Guarantee signedness
	const int offset = intIndex % keyLen;
	return int(key[offset]) ^ ((isTextElement ? offset : intIndex) + 40);
}

bool Codec::isPrintable(int c) const {
	return c >= AsciiStart && c <= AsciiEnd;
}

char Codec::toPrintable(int c) const {
	const int step = AsciiEnd - AsciiStart + 1;

	while (!this->isPrintable(c)) {
		if (c < AsciiStart) {
			c += step;
		} else if (c > AsciiEnd) {
			c -= step;
		} else {
			break;
		}
	}
	return char(c);
}

} // namespace UHS

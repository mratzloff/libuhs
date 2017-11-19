#include "uhs.h"

namespace UHS {

const std::string Codec::decode88a(std::string encoded) const {
	std::string& decoded = encoded;

	std::size_t len = encoded.length();
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

	std::size_t len = encoded.length();
	for (std::size_t i = 0; i < len; ++i) {
		char c = decoded[i];
		if (this->isPrintable(c)) {
			int offset = (c % 2 == 0) ? AsciiStart : AsciiEnd;
			encoded[i] = static_cast<char>((static_cast<int>(c) + offset) / 2);
		} else {
			encoded[i] = '?';
		}
	}
	return encoded;
}

const std::string Codec::decode96a(std::string encoded, std::string key, bool isTextElement, bool createKey) const {
	std::string& decoded = encoded;
	std::size_t len = encoded.length();
	std::size_t keyLen = key.length();
	int keystream;
	int c;

	for (std::size_t i = 0; i < len; ++i) {
		keystream = this->keystream(key, keyLen, i, isTextElement);
		if (createKey) {
			c = encoded[i] + keystream;
		} else {
			c = encoded[i] - keystream;
		}
		decoded[i] = this->toPrintable(c);
	}
	return decoded;
}

const std::string Codec::createKey(std::string secret) const {
	return this->decode96a(secret, KeySeed, false, true);
}

int Codec::keystream(std::string key, std::size_t keyLen, std::size_t index, bool isTextElement) const {
	int intIndex = static_cast<int>(index); // Guarantee signedness
	int offset = intIndex % keyLen;
	return int(key[offset]) ^ ((isTextElement ? offset : intIndex) + 40);
}

bool Codec::isPrintable(int c) const {
	return c >= AsciiStart && c <= AsciiEnd;
}

char Codec::toPrintable(int c) const {
	int step = AsciiEnd - AsciiStart + 1;

	while (! this->isPrintable(c)) {
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

}

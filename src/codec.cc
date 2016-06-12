#include "uhs.h"

namespace UHS {

Codec::Codec() : _keyLen {0} {}

Codec::~Codec() {}

const std::string Codec::decode88a(std::string encoded) {
	std::string& decoded = encoded;

	std::size_t len {encoded.length()};
	for (std::size_t i = 0; i < len; ++i) {
		char c {encoded[i]};
		if (this->isPrintable(c)) {
			int offset = AsciiEnd;
			if (c < 80) {
				offset = AsciiStart;
			}
			decoded[i] = (char) ((int) c) * 2 - offset;
		} else {
			decoded[i] = '?';
		}
	}
	return decoded;
}

bool Codec::isPrintable(char c) {
	return c >= AsciiStart && c <= AsciiEnd;
}

}

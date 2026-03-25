#include "tinyutf8/tinyutf8.h"

#include "uhs/codec.h"
#include "uhs/strings.h"
#include "uhs/token.h"

namespace UHS {

// Windows-1252 characters
Codec::UnicodeToAsciiMap Codec::fromChars_{
    {U'©', "(C)"},
    {U'®', "(R)"},
    {U'À', "A`"},
    {U'Á', "A'"},
    {U'Â', "A^"},
    {U'Ã', "A~"},
    {U'Ä', "A:"},
    {U'Å', "Ao"},
    {U'Æ', "AE"},
    {U'Ç', "C,"},
    {U'Ð', "D-"},
    {U'È', "E`"},
    {U'É', "E'"},
    {U'Ê', "E^"},
    {U'Ë', "E:"},
    {U'Ì', "I`"},
    {U'Í', "I'"},
    {U'Î', "I^"},
    {U'Ï', "I:"},
    {U'Ñ', "N~"},
    {U'Œ', "OE"},
    {U'Ò', "O`"},
    {U'Ó', "O'"},
    {U'Ô', "O^"},
    {U'Õ', "O~"},
    {U'Ö', "O:"},
    {U'Ø', "O/"},
    {U'Š', "S^"},
    {U'™', "TM"},
    {U'Ù', "U`"},
    {U'Ú', "U'"},
    {U'Û', "U^"},
    {U'Ü', "U:"},
    {U'Ý', "Y'"},
    {U'Ÿ', "Y:"},
    {U'à', "a`"},
    {U'á', "a'"},
    {U'â', "a^"},
    {U'ã', "a~"},
    {U'ä', "a:"},
    {U'å', "ao"},
    {U'æ', "ae"},
    {U'ç', "c,"},
    {U'ð', "d-"},
    {U'è', "e`"},
    {U'é', "e'"},
    {U'ê', "e^"},
    {U'ë', "e:"},
    {U'ì', "i`"},
    {U'í', "i'"},
    {U'î', "i^"},
    {U'ï', "i:"},
    {U'ñ', "n~"},
    {U'ò', "o`"},
    {U'ó', "o'"},
    {U'ô', "o^"},
    {U'õ', "o~"},
    {U'ö', "o:"},
    {U'ø', "o/"},
    {U'œ', "oe"},
    {U'š', "s^"},
    {U'ß', "ss"},
    {U'ù', "u`"},
    {U'ú', "u'"},
    {U'û', "u^"},
    {U'ü', "u:"},
    {U'ý', "y'"},
    {U'ÿ', "y:"},
    {U'–', "-"},
    {U'—', "--"},
};

// Windows-1252 characters
Codec::AsciiToUnicodeMap Codec::toChars_{
    {"A'", "Á"},
    {"A:", "Ä"},
    {"A^", "Â"},
    {"A`", "À"},
    {"AE", "Æ"},
    {"Ao", "Å"},
    {"A~", "Ã"},
    {"C,", "Ç"},
    {"D-", "Ð"},
    {"E'", "É"},
    {"E:", "Ë"},
    {"E^", "Ê"},
    {"E`", "È"},
    {"I'", "Í"},
    {"I:", "Ï"},
    {"I^", "Î"},
    {"I`", "Ì"},
    {"N~", "Ñ"},
    {"O'", "Ó"},
    {"O/", "Ø"},
    {"O:", "Ö"},
    {"OE", "Œ"},
    {"O^", "Ô"},
    {"O`", "Ò"},
    {"O~", "Õ"},
    {"S^", "Š"},
    {"TM", "™"},
    {"U'", "Ú"},
    {"U:", "Ü"},
    {"U^", "Û"},
    {"U`", "Ù"},
    {"Y'", "Ý"},
    {"Y:", "Ÿ"},
    {"(C)", "©"},
    {"(R)", "®"},
    {"-", "–"},
    {"--", "—"},
    {"a'", "á"},
    {"a:", "ä"},
    {"a^", "â"},
    {"a`", "à"},
    {"ae", "æ"},
    {"ao", "å"},
    {"a~", "ã"},
    {"c,", "ç"},
    {"d-", "ð"},
    {"e'", "é"},
    {"e:", "ë"},
    {"e^", "ê"},
    {"e`", "è"},
    {"i'", "í"},
    {"i:", "ï"},
    {"i^", "î"},
    {"i`", "ì"},
    {"n~", "ñ"},
    {"o'", "ó"},
    {"o/", "ø"},
    {"o:", "ö"},
    {"o^", "ô"},
    {"o`", "ò"},
    {"oe", "œ"},
    {"o~", "õ"},
    {"s^", "š"},
    {"ss", "ß"},
    {"u'", "ú"},
    {"u:", "ü"},
    {"u^", "û"},
    {"u`", "ù"},
    {"y'", "ý"},
    {"y:", "ÿ"},
};

std::string const Codec::createKey(std::string secret) const {
	return this->encode96a(secret, KeySeed, false);
}

std::string const Codec::decode88a(std::string encoded) const {
	std::string& decoded = encoded;
	std::size_t const length = encoded.length();

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

std::string const Codec::decode96a(
    std::string encoded, std::string const& key, bool isTextElement) const {

	std::string& decoded = encoded;
	std::size_t const length = encoded.length();
	std::size_t const keyLength = key.length();

	for (std::size_t i = 0; i < length; ++i) {
		auto keystream = this->keystream(key, keyLength, i, isTextElement);
		decoded[i] = this->toPrintable(encoded[i] - keystream);
	}
	return decoded;
}

std::string const Codec::decodeSpecialChars(std::string const& encoded) const {
	std::string segment;
	auto length = encoded.length();

	for (std::size_t i = 0; i < length; ++i) {
		if (i + 3 > length || encoded.substr(i, 2) != "#a") {
			segment += encoded[i];
			continue;
		}

		auto const offset = i + 3;
		auto pos = encoded.find("#a-", offset);

		if (pos == std::string::npos) {
			logger_.warn("unexpected sequence: expected #a-");
			segment += encoded[i];
			continue;
		}

		auto const length = pos - offset;
		auto const value = encoded.substr(offset, length);

		try {
			segment += toChars_.at(value);
		} catch (std::out_of_range const& err) {
			logger_.warn("unexpected sequence: %s", value);
			segment += encoded[i];
			break;
		}

		i += length + 5; // Advance to "-" of "#a-" (will increment next loop)
	}

	return segment;
}

std::string const Codec::encode88a(std::string decoded) const {
	std::string& encoded = decoded;
	std::size_t const length = encoded.length();

	for (std::size_t i = 0; i < length; ++i) {
		char c = decoded[i];
		if (Strings::isPrintable(c)) {
			int offset = (c % 2 == 0) ? Strings::AsciiStart : Strings::AsciiEnd;
			encoded[i] = static_cast<char>((static_cast<int>(c) + offset) / 2);
		}
	}
	return encoded;
}

std::string const Codec::encode96a(
    std::string decoded, std::string const& key, bool isTextElement) const {

	std::string& encoded = decoded;
	std::size_t const length = decoded.length();
	std::size_t const keyLength = key.length();

	for (std::size_t i = 0; i < length; ++i) {
		auto keystream = this->keystream(key, keyLength, i, isTextElement);
		encoded[i] = this->toPrintable(decoded[i] + keystream);
	}
	return encoded;
}

std::string const Codec::encodeSpecialChars(std::string const& decoded) const {
	std::string segment;
	tiny_utf8::string utf8String = decoded;
	auto length = utf8String.length();

	for (std::size_t i = 0; i < length; ++i) {
		std::string sequence;
		try {
			auto const c = (char32_t) utf8String[i];
			sequence = fromChars_.at(c);
			segment += Token::AsciiEncBegin;
			segment += sequence;
			segment += Token::AsciiEncEnd;
		} catch (std::out_of_range const& err) {
			segment += utf8String[i];
		}
	}

	return segment;
}

int Codec::keystream(std::string const& key, std::size_t keyLength, std::size_t line,
    bool isTextElement) const {

	int const intIndex = static_cast<int>(line); // Guarantee signedness
	int const offset = intIndex % keyLength;
	return int(key[offset]) ^ ((isTextElement ? offset : intIndex) + 40);
}

char Codec::toPrintable(int c) const {
	int const step = Strings::AsciiEnd - Strings::AsciiStart + 1;

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

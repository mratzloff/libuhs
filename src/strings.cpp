#include <iomanip>

#include "uhs.h"

namespace UHS::Strings {

// const std::unordered_map<const std::string, const std::string> specialChars = {
//     {"S^", u8"Š"},
//     {"OE", u8"Œ"},
//     {"-", u8"–"},
//     {"--", u8"—"},
//     {"TM", u8"™"},
//     {"s^", u8"š"},
//     {"oe", u8"œ"},
//     {"Y:", u8"Ÿ"},
//     {"(C)", u8"©"},
//     {"(R)", u8"®"},
//     {"A`", u8"À"},
//     {"A'", u8"Á"},
//     {"A^", u8"Â"},
//     {"A~", u8"Ã"},
//     {"A:", u8"Ä"},
//     {"Ao", u8"Å"},
//     {"AE", u8"Æ"},
//     {"C,", u8"Ç"},
//     {"E`", u8"È"},
//     {"E'", u8"É"},
//     {"E^", u8"Ê"},
//     {"E:", u8"Ë"},
//     {"I`", u8"Ì"},
//     {"I'", u8"Í"},
//     {"I^", u8"Î"},
//     {"I:", u8"Ï"},
//     {"D-", u8"Ð"},
//     {"N~", u8"Ñ"},
//     {"O`", u8"Ò"},
//     {"O'", u8"Ó"},
//     {"O^", u8"Ô"},
//     {"O~", u8"Õ"},
//     {"O:", u8"Ö"},
//     {"O/", u8"Ø"},
//     {"U`", u8"Ù"},
//     {"U'", u8"Ú"},
//     {"U^", u8"Û"},
//     {"U:", u8"Ü"},
//     {"Y'", u8"Ý"},
//     {"ss", u8"ß"},
//     {"a`", u8"à"},
//     {"a'", u8"á"},
//     {"a^", u8"â"},
//     {"a~", u8"ã"},
//     {"a:", u8"ä"},
//     {"ao", u8"å"},
//     {"ae", u8"æ"},
//     {"c,", u8"ç"},
//     {"e`", u8"è"},
//     {"e'", u8"é"},
//     {"e^", u8"ê"},
//     {"e:", u8"ë"},
//     {"i`", u8"ì"},
//     {"i'", u8"í"},
//     {"i^", u8"î"},
//     {"i:", u8"ï"},
//     {"d-", u8"ð"},
//     {"n~", u8"ñ"},
//     {"o`", u8"ò"},
//     {"o'", u8"ó"},
//     {"o^", u8"ô"},
//     {"o~", u8"õ"},
//     {"o:", u8"ö"},
//     {"o/", u8"ø"},
//     {"u`", u8"ù"},
//     {"u'", u8"ú"},
//     {"u^", u8"û"},
//     {"u:", u8"ü"},
//     {"y'", u8"ý"},
//     {"y:", u8"ÿ"},
// };

bool beginsWithAttachedPunctuation(const std::string& s) {
	auto c = s.front();
	return c == '\'' || c == '"' || c == ')' || c == ' ' || c == '?' || c == '!'
	       || c == '.' || c == ',' || c == ';' || c == ':';
}

std::string& chomp(std::string& s, char c) {
	if (s.back() == c) {
		s.pop_back();
	}
	return s;
}

bool endsWithAttachedPunctuation(const std::string& s) {
	auto c = s.back();
	return c == '\'' || c == '"' || c == '(' || c == ' ';
}

bool endsWithFinalPunctuation(const std::string& s) {
	auto c = s.back();
	return c == '?' || c == '!' || c == '.';
}

const std::string hex(const std::string& s) {
	std::ostringstream out;

	out << std::hex << std::setfill('0') << std::uppercase;
	auto i = 0;
	for (auto c : s) {
		if (i > 0) {
			out << ' ';
		}
		out << std::setw(2) << (int(c) & ~0xFFFFFF00);
		++i;
	}
	return out.str();
}

const std::string hex(char c) {
	std::ostringstream out;

	out << std::hex << std::setfill('0') << std::uppercase;
	out << std::setw(2) << (int(c) & ~0xFFFFFF00);
	return out.str();
}

bool isInt(const std::string& s) {
	if (s.length() == 0) {
		return false;
	}
	for (char c : s) {
		if (c < '0' || '9' < c) {
			return false;
		}
	}
	return true;
}

std::string join(const std::vector<std::string>& items, const std::string& sep) {
	std::string s;
	auto i = 0;

	for (const auto& item : items) {
		if (i > 0) {
			s += sep;
		}
		s += item;
		++i;
	}
	return s;
}

std::string ltrim(const std::string& s, char c) {
	auto pos = s.find_first_not_of(c);
	if (pos != std::string::npos) {
		return s.substr(pos);
	}
	return s;
}

std::string rtrim(const std::string& s, char c) {
	auto pos = s.find_last_not_of(c);
	if (pos != std::string::npos) {
		return s.substr(0, pos + 1);
	}
	return s;
}

std::vector<std::string> split(const std::string& s, const std::string& sep, int n) {
	std::vector<std::string> items;
	std::size_t from = 0;
	std::size_t to;

	if (s.length() == 0) {
		return items;
	}

	auto i = 0;
	for (;;) {
		if (n > 0 && i >= n) {
			break;
		}
		to = s.find(sep, from);
		items.emplace_back(s.substr(from, to - from));
		if (to == std::string::npos) {
			break;
		}
		from = to + sep.length();
		++i;
	}

	return items;
}

std::string toBase64(const std::string& s) {
	static const std::string charset =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t end = 0;
	size_t len = s.length();
	std::stringstream out;

	for (std::size_t i = 0, end = len - len % 3; i < end; i += 3) {
		out << charset[(s[i] & 0xFC) >> 2];
		out << charset[((s[i] & 0x03) << 4) + ((s[i + 1] & 0xF0) >> 4)];
		out << charset[((s[i + 1] & 0x0F) << 2) + ((s[i + 2] & 0xC0) >> 6)];
		out << charset[s[i + 2] & 0x3F];
	}

	if (end < len) {
		out << charset[(s[end] & 0xFC) >> 2];
		out << charset[((s[end] & 0x03) << 4)
		               + (end + 1 < len ? (s[end + 1] & 0xF0) >> 4 : 0)];
		out << (end + 1 < len ? charset[((s[end + 1] & 0x0F) << 2)] : '=');
		out << '=';
	}

	return out.str();
}

int toInt(const std::string& s) {
	int intVal;
	std::string::size_type idx;
	try {
		intVal = std::stoi(s, &idx);
		if (idx != s.length()) {
			throw std::invalid_argument("");
		}
	} catch (const std::invalid_argument& e) {
		std::throw_with_nested(Error("invalid integer: %s", s));
	}
	return intVal;
}

std::string wrap(const std::string& s, const std::string& sep, std::size_t width) {
	auto numLines = 0;
	return wrap(s, sep, width, numLines);
}

std::string wrap(const std::string& s, const std::string& sep, std::size_t width,
    int& numLines, const std::string prefix) {

	std::string lines;
	std::size_t cutpoint = 0;
	auto cutting = false;
	auto end = s.length();
	auto foundCutpoint = false;
	std::size_t start = 0;

	width -= prefix.length();

	auto cut = [&](std::size_t endpoint) {
		if (start > 0) {
			lines += sep;
		}
		lines += prefix;
		lines += s.substr(start, endpoint - start);
		++numLines;
	};

	for (auto cursor = start; cursor <= end; ++cursor) {
		switch (s[cursor]) {
		case '\n':
			cutting = true; // Always cut on newline
			[[fallthrough]];
		case ' ':
			foundCutpoint = true;
			cutpoint = cursor;
		}
		if (auto limit = start + width; cursor > limit) {
			if (!foundCutpoint) { // Hard cut
				cutpoint = limit;
			}
			cutting = true;
		}

		if (cutting) {
			cut(cutpoint);
			start = (foundCutpoint) ? cutpoint + 1 : cutpoint;
			foundCutpoint = false;
			cutting = false;
		}
	}
	if (cutpoint <= end) {
		cut(end);
	}

	return lines;
}

} // namespace UHS::Strings

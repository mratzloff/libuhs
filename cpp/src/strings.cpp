#include <iomanip>

#include "uhs.h"

namespace UHS::Strings {

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

bool isPrintable(int c) {
	return c >= AsciiStart && c <= AsciiEnd;
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
	int intVal = 0;
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

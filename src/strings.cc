#include "uhs.h"

namespace UHS {

namespace Strings {

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

int toInt(const std::string& s) {
	int intVal;
	std::string::size_type idx;
	try {
		intVal = std::stoi(s, &idx);
		if (idx != s.length()) {
			intVal = -1;
		}
	} catch (const std::invalid_argument& e) {
		// Return value must be checked by callers
		intVal = -1;
	}
	return intVal;
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
		return s.substr(0, pos+1);
	}
	return s;
}

std::vector<std::string> split(const std::string& s, const std::string sep, int n) {
	std::vector<std::string> items;
	std::size_t from = 0;
	std::size_t to;

	int i = 0;
	while (true) {
		if (n > 0 && i >= n) {
			break;
		}
		to = s.find(sep, from);
		items.push_back(s.substr(from, to - from));
		if (to == std::string::npos) {
			break;
		}
		from = to + sep.length();
		++i;
	}
	return items;
}

std::string join(const std::vector<std::string>& items, const std::string sep) {
	std::string s;
	int i = 0;

	for (const auto& item : items) {
		if (i > 0) {
			s.append(sep);
		}
		s.append(item);
		++i;
	}
	return s;
}

}

}

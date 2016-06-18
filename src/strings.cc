#include "uhs.h"

namespace UHS {

namespace Strings {

bool isInt(const std::string& s) {
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
		return s.substr(0, pos + 1);
	}
	return s;
}

}

}

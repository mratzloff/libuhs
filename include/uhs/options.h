#if !defined UHS_OPTIONS_H
#define UHS_OPTIONS_H

#include <string>

#include "uhs/constants.h"

namespace UHS {

struct Options {
	bool debug = false;
	std::string mediaDir;
	ModeType mode = ModeType::Auto;
	bool preserve = false;
	bool quiet = false;
};

} // namespace UHS

#endif

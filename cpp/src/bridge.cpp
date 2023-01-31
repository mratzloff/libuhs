#include "uhs.h"

int uhs_write(char const* format, char const* infile, char const* outfile) {
	UHS::Logger logger{UHS::LogLevel::Error};
	auto ok = write(logger, format, infile, outfile);
	if (!ok) {
		return -1;
	}
	return 0;
}

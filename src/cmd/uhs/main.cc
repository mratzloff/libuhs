#include <cstring>
#include <fstream>
#include <iostream>
#include "uhs.h"

enum Status {
	Err = -1,
	OK  = 0,
};

void printVersion() {
	std::cout << UHS::Version << std::endl;
}

void printHelp() {
	std::cout
		<< "uhs " << UHS::Version << "\n\n"
		<< "Usage: uhs [options] <file>\n"
		<< "-d <dir>\tDirectory to write embedded files to\n"
		<< "    --88a\tRead in 88a mode\n"
		<< "-v, --version\tPrint the version\n"
		<< "-h, --help\tPrint this help statement"
		<< std::endl;
}

int main(int argc, const char* argv[]) {
	std::string file;
	std::string dir;
	bool version88a = false;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') { // Parse options
			switch (argv[i][1]) {
			case 'd':
				++i;
				if (i >= argc) {
					std::cerr << "uhs: error: -d requires a parameter\n";
					return Err;
				}
				dir = argv[i];
				break;
			case 'v':
				printVersion();
				return OK;
			case 'h':
				printHelp();
				return OK;
			case '-':
				if (std::strncmp("--88a", argv[i], 5) == 0) {
					version88a = true;
					break;
				}
				if (std::strncmp("--version", argv[i], 9) == 0) {
					printVersion();
					return OK;
				}
				if (std::strncmp("--help", argv[i], 6) == 0) {
					printHelp();
					return OK;
				}
			default:
				printHelp();
				return Err;
			}
		} else { // Parse filename (required)
			if (i != argc-1) {
				printHelp();
				return Err;
			}
			file = argv[argc-1];
		}
	}

	if (file.empty()) {
		std::cerr << "uhs: error: no input file\n";
		return Err;
	}

	std::ifstream in {file, std::ifstream::in};
	UHS::ParserOptions opt;
	opt.debug = true;
	UHS::Parser p {in, opt};
	auto document = p.parse();

	return OK;
}

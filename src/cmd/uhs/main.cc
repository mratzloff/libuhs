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
		<< "-d <dir>\tDirectory to write media files to\n"
		<< "    --88a\tRead in 88a mode\n"
		<< "    --unregistered\tRead in unregistered mode\n"
		<< "    --debug\tPrint debugging statements\n"
		<< "-v, --version\tPrint the version\n"
		<< "-h, --help\tPrint this help statement"
		<< std::endl;
}

int main(int argc, const char* argv[]) {
	std::string file;
	std::string dir;
	UHS::ParserOptions parserOpt;
	UHS::WriterOptions writerOpt;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') { // Parse options
			switch (argv[i][1]) {
			case 'd':
				++i;
				if (i >= argc) {
					std::cerr << "uhs: error: -d requires a parameter\n";
					return Err;
				}
				writerOpt.mediaDir = argv[i];
				break;
			case 'v':
				printVersion();
				return OK;
			case 'h':
				printHelp();
				return OK;
			case '-':
				if (std::strncmp("--88a", argv[i], 5) == 0) {
					parserOpt.version = UHS::Version88a;
					break;
				}
				if (std::strncmp("--unregistered", argv[i], 14) == 0) {
					writerOpt.registered = false;
					break;
				}
				if (std::strncmp("--debug", argv[i], 7) == 0) {
					parserOpt.debug = true;
					break;
				}
				if (std::strncmp("--help", argv[i], 6) == 0) {
					printHelp();
					return OK;
				}
				if (std::strncmp("--version", argv[i], 9) == 0) {
					printVersion();
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

	std::ifstream in {file, std::ifstream::in | std::ifstream::binary};
	UHS::Parser p {in, parserOpt};
	auto document = p.parse();

	auto err = p.error();
	if (err != nullptr) {
		std::cerr << "uhs: " << err->message() << std::endl;
		return Err;
	}

	UHS::JSONWriter w {std::cout, writerOpt};
	w.write(document);

	return OK;
}

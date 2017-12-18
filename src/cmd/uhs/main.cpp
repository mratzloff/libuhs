#include "uhs.h"
#include <cstring>
#include <fstream>
#include <iostream>

using namespace UHS;

enum Status {
	Err = -1,
	OK = 0,
};

void printVersion() {
	std::cout << Version << std::endl;
}

void printHelp() {
	std::cout << "uhs " << Version << "\n\n"
	          << "Usage: uhs -f <fmt> [options] <file>\n"
	          << "-f <fmt>\t\tOutput format (json, tree, uhs)\n"
	          << "-o <file>\t\tOutput file\n"
	          << "-m <dir>\t\tMedia directory\n"
	          << "    --88a\t\tForce 88a mode (read and write)\n"
	          << "    --unregistered\tUnregistered mode (write only)\n"
	          << "    --debug\t\tPrint debugging statements\n"
	          << "-v, --version\t\tPrint the version\n"
	          << "-h, --help\t\tPrint this help statement" << std::endl;
}

int main(int argc, const char* argv[]) {
	std::string format;
	std::string infile;
	std::string outfile;
	std::string dir;
	ParserOptions parserOptions;
	WriterOptions writerOptions;

	if (argc == 1) {
		printHelp();
		return OK;
	}

	for (auto i = 1; i < argc; ++i) {
		auto arg = std::string(argv[i]);

		if (arg[0] == '-') { // Parse options
			switch (arg[1]) {
			case 'f':
				++i;
				if (i >= argc) {
					std::cerr << "uhs: error: -f requires a parameter" << std::endl;
					return Err;
				}
				format = argv[i];
				break;
			case 'o':
				++i;
				if (i >= argc) {
					std::cerr << "uhs: error: -o requires a parameter" << std::endl;
					return Err;
				}
				outfile = argv[i];
				break;
			case 'm':
				++i;
				if (i >= argc) {
					std::cerr << "uhs: error: -m requires a parameter" << std::endl;
					return Err;
				}
				writerOptions.mediaDir = argv[i];
				break;
			case 'v':
				printVersion();
				return OK;
			case 'h':
				printHelp();
				return OK;
			case '-':
				if (arg == "--88a") {
					parserOptions.force88aMode = true;
					writerOptions.force88aMode = true;
					break;
				} else if (arg == "--unregistered") {
					writerOptions.registered = false;
					break;
				} else if (arg == "--debug") {
					parserOptions.debug = true;
					writerOptions.debug = true;
					break;
				} else if (arg == "--help") {
					printHelp();
					return OK;
				} else if (arg == "--version") {
					printVersion();
					return OK;
				} else {
					std::cerr << "uhs: unknown option: " << arg << std::endl;
					return Err;
				}
			default:
				std::cerr << "uhs: unknown option: " << arg << std::endl;
				return Err;
			}
		} else { // Parse filename (required)
			if (i != argc - 1) {
				printHelp();
				return Err;
			}
			infile = argv[argc - 1];
		}
	}

	if (infile.empty()) {
		std::cerr << "uhs: error: no input file" << std::endl;
		return Err;
	}
	if (format.empty()) {
		std::cerr << "uhs: error: no output format specified" << std::endl;
		return Err;
	}
	if (format != "json" && format != "tree" && format != "uhs") {
		std::cerr << "uhs: error: unknown output format: " << format << std::endl;
		return Err;
	}

	try {
		Parser p{parserOptions};
		std::ifstream in{infile, std::ios::in | std::ios::binary};
		auto document = p.parse(in);

		std::ofstream fout;
		if (!outfile.empty()) {
			fout = std::ofstream(outfile, std::ios::out | std::ios::binary);
		}
		std::ostream& out = (outfile.empty()) ? std::cout : fout;

		if (format == "json") {
			JSONWriter w{out, writerOptions};
			w.write(*document);
		} else if (format == "tree") {
			TreeWriter w{out, writerOptions};
			w.write(*document);
		} else if (format == "uhs") {
			UHSWriter w{out, writerOptions};
			w.write(*document);
		}
	} catch (const Error& err) {
		std::cerr << "uhs: " << err << std::endl;
		return Err;
	} catch (const std::exception& err) {
		std::cerr << "uhs: unexpected error: " << err.what() << std::endl;
		return Err;
	}

	return OK;
}

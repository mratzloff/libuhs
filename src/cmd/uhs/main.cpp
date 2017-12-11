#include "uhs.h"
#include <cstring>
#include <fstream>
#include <iostream>

enum Status {
	Err = -1,
	OK = 0,
};

void printVersion() {
	std::cout << UHS::Version << std::endl;
}

void printHelp() {
	std::cout << "uhs " << UHS::Version << "\n\n"
	          << "Usage: uhs -f <fmt> [options] <file>\n"
	          << "-f <fmt>\t\tOutput format (json, tree, uhs)\n"
	          << "-o <file>\t\tOutput file\n"
	          << "-m <dir>\t\tMedia directory\n"
	          << "    --88a\t\tForce 88a mode for reading and writing\n"
	          << "    --unregistered\tRead in unregistered mode\n"
	          << "    --debug\t\tPrint debugging statements\n"
	          << "-v, --version\t\tPrint the version\n"
	          << "-h, --help\t\tPrint this help statement" << std::endl;
}

int main(int argc, const char* argv[]) {
	std::string format, infile, outfile, dir;
	UHS::ParserOptions parserOpt;
	UHS::WriterOptions writerOpt;

	if (argc == 1) {
		printHelp();
		return OK;
	}

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') { // Parse options
			switch (argv[i][1]) {
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
					parserOpt.force88aMode = true;
					writerOpt.force88aMode = true;
					break;
				} else if (std::strncmp("--unregistered", argv[i], 14) == 0) {
					writerOpt.registered = false;
					break;
				} else if (std::strncmp("--debug", argv[i], 7) == 0) {
					parserOpt.debug = true;
					writerOpt.debug = true;
					break;
				} else if (std::strncmp("--help", argv[i], 6) == 0) {
					printHelp();
					return OK;
				} else if (std::strncmp("--version", argv[i], 9) == 0) {
					printVersion();
					return OK;
				} else {
					std::cerr << "uhs: unknown option: " << argv[i] << std::endl;
					return Err;
				}
			default:
				std::cerr << "uhs: unknown option: " << argv[i] << std::endl;
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
	if (format.length() == 0) {
		std::cerr << "uhs: error: no output format specified" << std::endl;
		return Err;
	}
	if (format != "json" && format != "tree" && format != "uhs") {
		std::cerr << "uhs: error: unknown output format: " << format << std::endl;
		return Err;
	}

	try {
		UHS::Parser p{parserOpt};
		std::ifstream in{infile, std::ios::in | std::ios::binary};
		auto document = p.parse(in);

		std::ofstream fout;
		if (!outfile.empty()) {
			fout = std::ofstream(outfile, std::ios::out | std::ios::binary);
		}
		std::ostream& out = (outfile.empty()) ? std::cout : fout;

		if (format == "json") {
			UHS::JSONWriter w{out, writerOpt};
			w.write(*document);
		} else if (format == "tree") {
			UHS::TreeWriter w{out, writerOpt};
			w.write(*document);
		} else if (format == "uhs") {
			UHS::UHSWriter w{out, writerOpt};
			w.write(*document);
		}
	} catch (const UHS::Error& err) {
		std::cerr << "uhs: " << err << std::endl;
		return Err;
	} catch (const std::exception& err) {
		std::cerr << "uhs: unexpected error: " << err.what() << std::endl;
		return Err;
	}

	return OK;
}

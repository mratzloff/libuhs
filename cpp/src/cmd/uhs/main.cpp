#include "argh.h"
#include "uhs.h"

using namespace UHS;

enum Status {
	Err = -1,
	OK = 0,
};

void printHelp() {
	auto help =
	    "uhs %s\n"
	    "A utility to read and write Universal Hint System game hint files\n\n"
	    "\033[1mUSAGE\033[0m\n"
	    "  $ uhs -f <fmt> [options] <file>\n\n"
	    "\033[1mEXAMPLE\033[0m\n"
	    "  $ uhs -f html -o example.html example.uhs\n\n"
	    "\033[1mOPTIONS\033[0m\n"
	    "  -d,        --debug          Print debugging statements\n"
	    "  -f <fmt>,  --format=<fmt>   Output format\n"
	    "                              ●  uhs   UHS file (default)\n"
	    "                              ○  html  HTML file\n"
	    "                              ○  json  JSON file\n"
	    "                              ○  tree  Tree representation of file\n\n"
	    "  -h,        --help           Print this help statement\n"
	    "  -m <dir>,  --media=<dir>    Image and audio write directory (JSON output only)\n"
	    "             --mode=<mode>    Read and write mode\n"
	    "                              ●  auto  Choose mode based on input file (default)\n"
	    "                              ○  96a   2000s-era reader and writer\n"
	    "                              ○  88a   1980s-era reader and writer\n\n"
	    "  -o <file>, --output=<file>  Output file\n"
	    "             --preserve       Preserve unregistered content restrictions\n"
	    "  -q,        --quiet          Suppress errors and warnings\n"
	    "  -v,        --version        Print the version\n";

	tfm::printf(help, Version);
}

void printVersion() {
	std::cout << Version << std::endl;
}

int main(int const argc, char* argv[]) {
	Options options;

	argh::parser args({"-f", "--format", "-o", "--output", "-m", "--media", "--mode"});
	args.parse(argv);

	// Help
	if (args[{"-h", "--help"}] || argc == 1) {
		printHelp();
		return OK;
	}

	// Version
	if (args[{"-v", "--version"}]) {
		printVersion();
		return OK;
	}

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];
	Logger logger{options.quiet ? LogLevel::None : LogLevel::Warn};

	// Debug
	options.debug = args[{"-d", "--debug"}];

	// Output format
	std::string format;

	if (!(args({"-f", "--format"}, "uhs") >> format)) {
		logger.error("--format (-f) is required");
		return Err;
	};

	// Media directory
	std::string mediaDir;
	args({"-m", "--media"}) >> mediaDir;

	// Read and write mode
	std::string mode;
	args({"--mode"}, "auto") >> mode;

	if (mode == "auto") {
		options.mode = ModeType::Auto;
	} else if (mode == "88a") {
		options.mode = ModeType::Version88a;
	} else if (mode == "96a") {
		options.mode = ModeType::Version96a;
	} else {
		logger.error("unknown mode: %s", mode);
		return Err;
	}

	// Output file
	std::string outfile;
	args({"-o", "--output"}) >> outfile;

	// Preserve unregistered content restrictions
	options.preserve = args[{"--preserve"}];

	// Input file
	if (argc < 2) {
		logger.error("no input file specified");
		return Err;
	}
	std::string infile = argv[argc - 1];

	auto ok = write(logger, format, infile, outfile, options);
	if (!ok) {
		return Err;
	}

	return OK;
}

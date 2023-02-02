#include "argh.h"
#include "uhs.h"

using namespace UHS;

enum Status {
	Err = -1,
	OK = 0,
};

int convert(int const argc, char* argv[]) {
	Options options;

	argh::parser args({"-f", "--format", "-o", "--output", "-m", "--media", "--mode"});
	args.parse(argc - 1, argv + 1);

	// Debug
	options.debug = args[{"-d", "--debug"}];

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];
	Logger logger{options.quiet ? LogLevel::None : LogLevel::Info};

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

int download(int const argc, char* argv[]) {
	Options options;

	argh::parser args;
	args.parse(argc - 1, argv + 1);

	// Debug
	options.debug = args[{"-d", "--debug"}];

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];
	Logger logger{options.quiet ? LogLevel::None : LogLevel::Info};

	Downloader downloader{logger, options};

	try {
		downloader.fetchIndex();
	} catch (HTTPError const& err) {
		logger.error(err);
		return Err;
	}

	return OK;
}

void printHelp() {
	auto help =
	    "uhs %s\n"
	    "A utility to read and write Universal Hint System game hint files\n\n"
	    "\033[1mUSAGE\033[0m\n"
	    "  $ uhs convert -f <fmt> [options] <file>\n"
	    "  $ uhs download [options]\n\n"
	    "\033[1mEXAMPLES\033[0m\n"
	    "  $ uhs convert -f html -o example.html example.uhs\n"
	    "  $ uhs download -q\n\n"
	    "\033[1mOPTIONS\033[0m\n"
	    "  -d,        --debug          Print debugging statements\n"
	    "  -h,        --help           Print this help statement\n"
	    "  -q,        --quiet          Suppress errors and warnings\n"
	    "  -v,        --version        Print the version\n\n"
	    "convert:\n"
	    "  -f <fmt>,  --format=<fmt>   Output format\n"
	    "                              ●  uhs   UHS file (default)\n"
	    "                              ○  html  HTML file\n"
	    "                              ○  json  JSON file\n"
	    "                              ○  tree  Tree representation of file\n\n"
	    "  -m <dir>,  --media=<dir>    Image and audio write directory (JSON output only)\n"
	    "             --mode=<mode>    Read and write mode\n"
	    "                              ●  auto  Choose mode based on input file (default)\n"
	    "                              ○  96a   2000s-era reader and writer\n"
	    "                              ○  88a   1980s-era reader and writer\n\n"
	    "  -o <file>, --output=<file>  Output file\n"
	    "             --preserve       Preserve unregistered content restrictions\n";

	tfm::printf(help, Version);
}

void printVersion() {
	std::cout << Version << std::endl;
}

int main(int const argc, char* argv[]) {
	std::string command;
	if (argc >= 2) {
		command = argv[1];
	}

	if (command == "convert") {
		return convert(argc - 1, argv + 1);
	}

	if (command == "download") {
		return download(argc - 1, argv + 1);
	}

	argh::parser args;
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
}

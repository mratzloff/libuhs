#include "argh.h"
#include "uhs.h"

using namespace UHS;

enum Status {
	Err = -1,
	OK = 0,
};

int convert(int argc, char const* argv[]) {
	Options options;

	argh::parser args({"-f", "--format", "-o", "--output", "-m", "--media", "--mode"});
	args.parse(argc - 1, argv + 1);

	// Print debugging statements
	options.debug = args[{"--debug"}];
	Logger logger{options.debug ? LogLevel::Debug : LogLevel::Info};

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];
	logger.level(options.quiet ? LogLevel::None : LogLevel::Info);

	// Output format
	std::string format;
	args({"-f", "--format"}, "html") >> format;

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

int download(int argc, char const* argv[]) {
	Options options;

	argh::parser args({"-d", "--dir"});
	args.parse(argc - 1, argv + 1);

	// All files
	auto all = args[{"--all"}];

	// Output directory
	std::string dir;
	args({"-d", "--dir"}) >> dir;

	// Print debugging statements
	options.debug = args[{"--debug"}];
	Logger logger{options.debug ? LogLevel::Debug : LogLevel::Info};

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];
	logger.level(options.quiet ? LogLevel::None : LogLevel::Info);

	// Download file
	if (argc < 2 && !all) {
		logger.error("no download file specified");
		return Err;
	}
	std::string file = argv[argc - 1];

	Downloader downloader{logger};

	try {
		std::vector<std::string const> files;
		if (all) {
			auto index = downloader.fileIndex();
			for (auto const& [file, _] : index) {
				files.push_back(file);
			}
		} else {
			files.push_back(file);
		}
		downloader.download(dir, files);
	} catch (FileError const& err) {
		logger.error(err);
		return Err;
	} catch (HTTPError const& err) {
		logger.error(err);
		return Err;
	} catch (Error const& err) {
		logger.error(err);
		return Err;
	} catch (std::exception const& err) {
		logger.error(err.what());
		return false;
	}

	return OK;
}

void printHelp() {
	auto help =
	    "uhs %s\n"
	    "A utility to read and write Universal Hint System game hint files\n\n"
	    "\033[1mUSAGE\033[0m\n"
	    "  $ uhs convert [options] <file>\n"
	    "  $ uhs download [options] [file]\n\n"
	    "\033[1mEXAMPLES\033[0m\n"
	    "  $ uhs convert -q -o example.html example.uhs\n"
	    "  $ uhs download -d ./hints example.uhs\n"
	    "  $ uhs download --all\n\n"
	    "\033[1mOPTIONS\033[0m\n"
	    "             --debug          Print debugging statements\n"
	    "  -h,        --help           Print this help statement\n"
	    "  -q,        --quiet          Suppress errors and warnings\n"
	    "  -v,        --version        Print the version\n\n"
	    "convert:\n"
	    "  -f <fmt>,  --format=<fmt>   Output format\n"
	    "                              ●  html  HTML application (default)\n"
	    "                              ○  json  JSON\n"
	    "                              ○  tree  Tree representation\n"
	    "                              ○  uhs   UHS\n"
	    "  -m <dir>,  --media=<dir>    Image and audio write directory (JSON output only)\n"
	    "             --mode=<mode>    Read and write mode\n"
	    "                              ●  auto  Choose mode based on input file (default)\n"
	    "                              ○  96a   2000s-era reader and writer\n"
	    "                              ○  88a   1980s-era reader and writer\n"
	    "  -o <file>, --output=<file>  Output file\n"
	    "             --preserve       Preserve unregistered content restrictions\n\n"
	    "download:\n"
	    "            --all             All files\n"
	    "  -d <dir>, --dir=<dir>       Output directory\n";

	tfm::printf(help, Version);
}

void printVersion() {
	std::cout << Version << std::endl;
}

int main(int argc, char const* argv[]) {
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

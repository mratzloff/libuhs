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
	Logger logger;
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

	// Debug
	options.debug = args[{"-d", "--debug"}];

	// Output format
	std::string format;

	if (!(args({"-f", "--format"}, "uhs") >> format)) {
		logger.error("--format (-f) is required");
		return Err;
	};
	if (format != "uhs" && format != "html" && format != "json" && format != "tree") {
		logger.error("unknown option for --format: %s", format);
		return Err;
	}

	// Media directory
	std::string mediaDir;
	args({"-m", "--media"}) >> mediaDir;

	if (mediaDir.length() > 0) {
		std::filesystem::path path{mediaDir};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {
			logger.error("invalid media directory: %s", mediaDir);
			return Err;
		}
		options.mediaDir = mediaDir;
	}

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
		logger.error("unknown option for --mode: %s", mode);
		return Err;
	}

	// Output file
	std::string outfile;
	args({"-o", "--output"}) >> outfile;

	if (outfile.length() > 0) {
		std::filesystem::path path{outfile};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {
			logger.error("invalid output file: %s", outfile);
			return Err;
		}
	}

	// Preserve unregistered content restrictions
	options.preserve = args[{"--preserve"}];

	// Suppress errors and warnings
	options.quiet = args[{"-q", "--quiet"}];

	// Input file
	if (argc < 2) {
		logger.error("no input file specified");
		return Err;
	}

	std::string infile = argv[argc - 1];
	std::filesystem::path infilePath{infile};

	if (!std::filesystem::exists(infilePath)) {
		logger.error("invalid file: %s", infile);
		return Err;
	}

	try {
		Parser p{logger, options};
		auto const document = p.parseFile(infile);

		std::ofstream fout;
		if (!outfile.empty()) {
			fout = std::ofstream(outfile, std::ios::out | std::ios::binary);
		}
		std::ostream& out = (outfile.empty()) ? std::cout : fout;

		if (format == "uhs") {
			UHSWriter w{logger, out, options};
			w.write(document);
		} else if (format == "html") {
			HTMLWriter w{logger, out, options};
			w.write(document);
		} else if (format == "json") {
			JSONWriter w{logger, out, options};
			w.write(document);
		} else if (format == "tree") {
			TreeWriter w{logger, out, options};
			w.write(document);
		}
	} catch (ReadError const& err) {
		if (!options.quiet) {
			logger.error(err);
		}
		return Err;
	} catch (ParseError const& err) {
		if (!options.quiet) {
			logger.error(err);
		}
		return Err;
	} catch (WriteError const& err) {
		if (!options.quiet) {
			logger.error(err);
		}
		return Err;
	} catch (Error const& err) {
		if (!options.quiet) {
			logger.error(err);
		}
		return Err;
	} catch (std::exception const& err) {
		if (!options.quiet) {
			logger.error(err.what());
		}
		return Err;
	}

	return OK;
}

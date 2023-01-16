#include "argh.h"
#include "uhs.h"

using namespace UHS;

enum Status {
	Err = -1,
	OK = 0,
};

void printError(const std::string message) {
	tfm::format(std::cerr, "uhs: error: %s\n", message);
}

void printHelp() {
	auto help =
	    "uhs %s\n"
	    "A utility to read and write Universal Hint System game hint files\n\n"
	    "\033[1mUSAGE\033[0m\n"
	    "  $ uhs -f <fmt> [options] <file>\n\n"
	    "\033[1mOPTIONS\033[0m\n"
	    "  -f <fmt>,  --format=<fmt>   Output format\n"
	    "                              ●  uhs   UHS file (default)\n"
	    "                              ○  html  HTML file\n"
	    "                              ○  json  JSON file\n"
	    "                              ○  tree  Tree representation of file\n\n"
	    "  -o <file>, --output=<file>  Output file\n"
	    "  -m <dir>,  --media=<dir>    Image and audio directory (JSON output only)\n"
	    "             --mode=<mode>    Read and write mode\n"
	    "                              ●  auto  Choose mode based on input file (default)\n"
	    "                              ○  96a   2000s-era reader and writer\n"
	    "                              ○  88a   1980s-era reader and writer\n\n"
	    "             --preserve       Preserve unregistered content restrictions\n"
	    "  -d,        --debug          Print debugging statements\n"
	    "  -v,        --version        Print the version\n"
	    "  -h,        --help           Print this help statement\n";

	tfm::printf(help, Version);
}

void printVersion() {
	std::cout << Version << std::endl;
}

int main(const int argc, char* argv[]) {
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

	// Output format
	std::string format;

	if (!(args({"-f", "--format"}, "uhs") >> format)) {
		printError("--format (-f) is required");
		return Err;
	};
	if (format != "uhs" && format != "html" && format != "json" && format != "tree") {
		printError("unknown option for --format: " + format);
		return Err;
	}

	// Output file
	std::string outfile;
	args({"-o", "--output"}) >> outfile;

	if (outfile.length() > 0) {
		std::filesystem::path path{outfile};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {

			printError("invalid output file: " + outfile);
			return Err;
		}
	}

	// Media directory
	std::string mediaDir;
	args({"-m", "--media"}) >> mediaDir;

	if (mediaDir.length() > 0) {
		std::filesystem::path path{mediaDir};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {

			printError("invalid media directory: " + mediaDir);
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
		printError("unknown option for --mode: " + mode);
		return Err;
	}

	// Preserve unregistered content restrictions
	options.preserve = args[{"--preserve"}];

	// Debug
	options.debug = args[{"-d", "--debug"}];

	// Input file
	if (argc < 2) {
		printError("no input file specified");
		return Err;
	}

	std::string infile = argv[argc - 1];
	std::filesystem::path infilePath{infile};

	if (!std::filesystem::exists(infilePath)) {
		printError("invalid file: " + infile);
		return Err;
	}

	try {
		Parser p{options};
		const auto document = p.parseFile(infile);

		std::ofstream fout;
		if (!outfile.empty()) {
			fout = std::ofstream(outfile, std::ios::out | std::ios::binary);
		}
		std::ostream& out = (outfile.empty()) ? std::cout : fout;

		if (format == "uhs") {
			UHSWriter w{out, options};
			w.write(document);
		} else if (format == "html") {
			HTMLWriter w{out, options};
			w.write(document);
		} else if (format == "json") {
			JSONWriter w{out, options};
			w.write(document);
		} else if (format == "tree") {
			TreeWriter w{out, options};
			w.write(document);
		}
	} catch (const ReadError& err) {
		printError(err.string());
		return Err;
	} catch (const ParseError& err) {
		printError(err.string());
		return Err;
	} catch (const WriteError& err) {
		printError(err.string());
		return Err;
	} catch (const Error& err) {
		printError(err.string());
		return Err;
	} catch (const std::exception& err) {
		std::cerr << "uhs: unexpected error: " << err.what() << std::endl;
		return Err;
	}

	return OK;
}

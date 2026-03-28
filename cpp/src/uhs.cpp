#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "uhs/error/data_error.h"
#include "uhs/error/file_error.h"
#include "uhs/error/parse_error.h"
#include "uhs/parser.h"
#include "uhs/uhs_write.h"
#include "uhs/writer/html_writer.h"
#include "uhs/writer/json_writer.h"
#include "uhs/writer/tree_writer.h"
#include "uhs/writer/uhs_writer.h"

namespace UHS {

static bool validateFormat(Logger const& logger, std::string const& format) {
	if (format != "html" && format != "json" && format != "tree" && format != "uhs") {
		logger.error("unknown format: %s", format);
		return false;
	}
	return true;
}

static bool validateFile(
    Logger const& logger, std::string const& infile, Options const& options) {

	if (!options.mediaDir.empty()) {
		std::filesystem::path path{options.mediaDir};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {
			logger.error("invalid media directory: %s", options.mediaDir);
			return false;
		}
	}

	std::filesystem::path infilePath{infile};
	if (!std::filesystem::exists(infilePath)) {
		logger.error("invalid file: %s", infile);
		return false;
	}

	return true;
}

static void writeToStream(Logger const& logger, std::string const& format,
    std::shared_ptr<Document> const& document, std::ostream& out,
    Options const& options) {

	if (format == "html") {
		HTMLWriter w{logger, out, options};
		w.write(document);
	} else if (format == "json") {
		JSONWriter w{logger, out, options};
		w.write(document);
	} else if (format == "tree") {
		TreeWriter w{logger, out, options};
		w.write(document);
	} else if (format == "uhs") {
		UHSWriter w{logger, out, options};
		w.write(document);
	}
}

static std::shared_ptr<Document> parseFile(
    Logger const& logger, std::string const& infile, Options const& options) {

	Parser p{logger, options};
	return p.parseFile(infile);
}

static std::shared_ptr<Document> parseBuffer(
    Logger const& logger, char const* data, std::size_t length, Options const& options) {

	Parser p{logger, options};
	std::istringstream in{std::string{data, length}};
	return p.parse(in);
}

static bool safeWrite(Logger const& logger, std::function<void()> const& func) {
	try {
		func();
	} catch (DataError const& err) {
		logger.error(err);
		return false;
	} catch (FileError const& err) {
		logger.error(err);
		return false;
	} catch (ParseError const& err) {
		logger.error(err);
		return false;
	} catch (Error const& err) {
		logger.error(err);
		return false;
	} catch (std::exception const& err) {
		logger.error(err.what());
		return false;
	}
	return true;
}

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    Options const& options) {

	if (!validateFormat(logger, format) || !validateFile(logger, infile, options)) {
		return false;
	}
	return safeWrite(logger, [&] {
		auto const document = parseFile(logger, infile, options);
		writeToStream(logger, format, document, std::cout, options);
	});
}

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    std::filesystem::path const& outfile, Options const& options) {

	if (!validateFormat(logger, format) || !validateFile(logger, infile, options)) {
		return false;
	}
	if (!outfile.has_parent_path() || !std::filesystem::exists(outfile.parent_path())) {
		logger.error("invalid output file: %s", outfile.string());
		return false;
	}
	return safeWrite(logger, [&] {
		auto const document = parseFile(logger, infile, options);
		std::ofstream out{outfile, std::ios::out | std::ios::binary | std::ios::trunc};
		writeToStream(logger, format, document, out, options);
	});
}

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    std::string& output, Options const& options) {

	if (!validateFormat(logger, format) || !validateFile(logger, infile, options)) {
		return false;
	}
	return safeWrite(logger, [&] {
		auto const document = parseFile(logger, infile, options);
		std::ostringstream out;
		writeToStream(logger, format, document, out, options);
		output = out.str();
	});
}

bool write(Logger const& logger, std::string const& format, char const* data,
    std::size_t length, std::string& output, Options const& options) {

	if (!validateFormat(logger, format)) {
		return false;
	}
	return safeWrite(logger, [&] {
		auto const document = parseBuffer(logger, data, length, options);
		std::ostringstream out;
		writeToStream(logger, format, document, out, options);
		output = out.str();
	});
}

} // namespace UHS

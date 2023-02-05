#include "uhs.h"

namespace UHS {

bool write(Logger const logger, std::string const format, std::string const infile,
    std::string const outfile, Options const options) {

	if (format != "html" && format != "json" && format != "tree" && format != "uhs") {
		logger.error("unknown format: %s", format);
		return false;
	}

	if (options.mediaDir.length() > 0) {
		std::filesystem::path path{options.mediaDir};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {
			logger.error("invalid media directory: %s", options.mediaDir);
			return false;
		}
	}

	if (outfile.length() > 0) {
		std::filesystem::path path{outfile};
		if (!path.has_parent_path() || !std::filesystem::exists(path.parent_path())) {
			logger.error("invalid output file: %s", outfile);
			return false;
		}
	}

	std::filesystem::path infilePath{infile};
	if (!std::filesystem::exists(infilePath)) {
		logger.error("invalid file: %s", infile);
		return false;
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
		logger.error(err);
		return false;
	} catch (ParseError const& err) {
		logger.error(err);
		return false;
	} catch (WriteError const& err) {
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

} // namespace UHS

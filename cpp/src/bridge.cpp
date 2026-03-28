#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {
#include "uhs_bridge.h"
}

#include "uhs/downloader.h"
#include "uhs/error/error.h"
#include "uhs/error/file_error.h"
#include "uhs/error/http_error.h"
#include "uhs/uhs_write.h"

int uhs_write_buffer(char const* format, char const* data, size_t length, char** output) {
	UHS::Logger logger{UHS::LogLevel::Error};
	std::string result;
	if (!UHS::write(logger, format, data, length, result)) {
		return -1;
	}
	*output = static_cast<char*>(std::malloc(result.size() + 1));
	if (*output == nullptr) {
		return -1;
	}
	std::memcpy(*output, result.c_str(), result.size() + 1);
	return 0;
}

int uhs_write_file(char const* format, char const* infile, char const* outfile) {
	UHS::Logger logger{UHS::LogLevel::Error};
	if (!UHS::write(logger, format, infile, std::filesystem::path{outfile})) {
		return -1;
	}
	return 0;
}

int uhs_write_string(char const* format, char const* infile, char** output) {
	UHS::Logger logger{UHS::LogLevel::Error};
	std::string result;
	if (!UHS::write(logger, format, infile, result)) {
		return -1;
	}
	*output = static_cast<char*>(std::malloc(result.size() + 1));
	if (*output == nullptr) {
		return -1;
	}
	std::memcpy(*output, result.c_str(), result.size() + 1);
	return 0;
}

int uhs_download(char const* dir, int numFiles, char const** files) {
	UHS::Logger logger{UHS::LogLevel::Error};
	auto downloader = UHS::Downloader{logger};

	std::vector<std::string> filesObj;
	for (auto i = 0; i < numFiles; ++i) {
		filesObj.push_back(std::string{files[i]});
	}

	try {
		downloader.download(dir, filesObj);
	} catch (UHS::FileError const& err) {
		logger.error(err);
		return -1;
	} catch (UHS::HTTPError const& err) {
		logger.error(err);
		return -1;
	} catch (UHS::Error const& err) {
		logger.error(err);
		return -1;
	} catch (std::exception const& err) {
		logger.error(err.what());
		return -1;
	}

	return 0;
}

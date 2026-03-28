#if !defined UHS_WRITE_H
#define UHS_WRITE_H

#include <filesystem>
#include <string>

#include "uhs/logger.h"
#include "uhs/options.h"

namespace UHS {

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    Options const& options = {});

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    std::filesystem::path const& outfile, Options const& options = {});

bool write(Logger const& logger, std::string const& format, std::string const& infile,
    std::string& output, Options const& options = {});

bool write(Logger const& logger, std::string const& format, char const* data,
    std::size_t length, std::string& output, Options const& options = {});

} // namespace UHS

#endif

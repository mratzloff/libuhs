#if !defined UHS_WRITE_H
#define UHS_WRITE_H

#include <string>

#include "uhs/logger.h"
#include "uhs/options.h"

namespace UHS {

bool write(Logger const& logger, std::string const format, std::string const infile,
    std::string const outfile = "", Options const& options = {});

} // namespace UHS

#endif

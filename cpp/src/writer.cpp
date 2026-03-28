#include "uhs/writer.h"

namespace UHS {

Writer::Writer(Logger const& logger, std::ostream& out, Options const& options)
    : logger_{logger}, options_{options}, out_{out} {}

void Writer::reset() {}

} // namespace UHS

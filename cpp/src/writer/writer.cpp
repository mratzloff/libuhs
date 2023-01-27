#include "uhs.h"

namespace UHS {

Writer::Writer(const Logger logger, std::ostream& out, const Options options)
    : codec_{options}, logger_{logger}, out_{out}, options_{options} {}

void Writer::reset() {}

} // namespace UHS

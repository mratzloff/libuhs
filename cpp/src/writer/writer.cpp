#include "uhs.h"

namespace UHS {

Writer::Writer(Logger const logger, std::ostream& out, Options const options)
    : codec_{options}, logger_{logger}, out_{out}, options_{options} {}

void Writer::reset() {}

} // namespace UHS

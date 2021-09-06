#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const Options options) : out_{out}, options_{options} {}

void Writer::reset() {}

} // namespace UHS

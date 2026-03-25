#include "uhs/regex.h"

namespace UHS { namespace Regex {

std::regex const Descriptor{"([0-9]+) ([a-z]{4,})"};
std::regex const DataAddress{"0{6} ?([0-3])? ([0-9]{6,}) ([0-9]{6,})"};
std::regex const EmailAddress{".+@.+\\..+"};
std::regex const HorizontalLine{"(?:-{3,} *)+"};
std::regex const HyperpngRegion{
    "(-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
std::regex const OverlayAddress{
    "0{6} ([0-9]{6,}) ([0-9]{6,}) (-?[0-9]{3,}) (-?[0-9]{3,})"};
std::regex const URL{"https?://.+\\..+"};

}} // namespace UHS::Regex

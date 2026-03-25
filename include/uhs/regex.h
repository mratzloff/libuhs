#if !defined UHS_REGEX_H
#define UHS_REGEX_H

#include <regex>

namespace UHS {
namespace Regex {

extern std::regex const Descriptor;
extern std::regex const DataAddress;
extern std::regex const EmailAddress;
extern std::regex const HorizontalLine;
extern std::regex const HyperpngRegion;
extern std::regex const OverlayAddress;
extern std::regex const URL;

} // namespace Regex
} // namespace UHS

#endif

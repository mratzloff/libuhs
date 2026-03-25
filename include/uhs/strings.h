#if !defined UHS_STRINGS_H
#define UHS_STRINGS_H

#include <regex>
#include <string>
#include <vector>

namespace UHS {
namespace Strings {

constexpr char AsciiStart = 0x20;
constexpr char AsciiEnd = 0x7F;

bool beginsWithAttachedPunctuation(std::string const& s);
std::string& chomp(std::string& s, char c);
bool endsWithAttachedPunctuation(std::string const& s);
bool endsWithFinalPunctuation(std::string const& s);
std::string const hex(std::string const& s);
std::string const hex(char s);
bool isInt(std::string const& s);
bool isPrintable(int c);
std::string join(std::vector<std::string> const& s, std::string const& sep);
std::string ltrim(std::string const& s, char c);
std::string rtrim(std::string const& s, char c);
std::vector<std::string> split(std::string const& s, std::string const& sep, int n = 0);
std::vector<std::string> split(std::string const& s, std::regex const& sep);
std::string toBase64(std::string const& s);
int toInt(std::string const& s);
void toLower(std::string& s);
std::string trim(std::string s, char c);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width);
std::string wrap(std::string const& s, std::string const& sep, std::size_t width,
    int& numLines, std::string const prefix = "");

} // namespace Strings
} // namespace UHS

#endif

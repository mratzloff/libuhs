#if !defined UHS_CODEC_H
#define UHS_CODEC_H

#include <string>

#include "tsl/hopscotch_map.h"

#include "uhs/logger.h"

namespace UHS {

class Codec {
public:
	std::string createKey(std::string const& secret) const;
	std::string decode88a(std::string encoded) const;
	std::string decode96a(
	    std::string encoded, std::string const& key, bool isTextElement) const;
	std::string decodeSpecialChars(std::string const& encoded) const;
	std::string encode88a(std::string decoded) const;
	std::string encode96a(
	    std::string encoded, std::string const& key, bool isTextElement) const;
	std::string encodeSpecialChars(std::string const& decoded) const;

private:
	using AsciiToUnicodeMap = tsl::hopscotch_map<std::string, std::string> const;
	using UnicodeToAsciiMap = tsl::hopscotch_map<char32_t, std::string> const;

	static constexpr auto KeySeed = "key";

	static UnicodeToAsciiMap fromChars_;
	static AsciiToUnicodeMap toChars_;

	Logger const logger_;

	int keystream(std::string const& key, std::size_t keyLength, std::size_t line,
	    bool isText) const;
	char toPrintable(int c) const;
};

} // namespace UHS

#endif

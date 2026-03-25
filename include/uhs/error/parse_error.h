#if !defined UHS_PARSE_ERROR_H
#define UHS_PARSE_ERROR_H

#include <string>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
#include "tinyformat.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "uhs/constants.h"
#include "uhs/error/error.h"

namespace UHS {

class ParseError : public Error {
public:
	enum ValueType {
		Uint,
		Int,
		String,
		Date,
		Time,
	};

	ParseError(int line, int column, std::string const& message);
	ParseError(int line, int column, char const* message);

	template<typename... Args>
	ParseError(int line, int column, char const* format, Args... args) : Error() {
		// TODO: Review for slice
		static_cast<Error&>(*this) = Error(this->format(format, line, column, args...));
	}

	static ParseError badLine(int line, int column, int targetLine);
	static ParseError badToken(int line, int column, TokenType type);
	static ParseError badToken(int line, int column, TokenType expected, TokenType found);
	static ParseError badValue(
	    int line, int column, ValueType expectedType, std::string found);
	static ParseError badValue(
	    int line, int column, std::string expected, std::string found);

private:
	template<typename... Args>
	std::string format(char const* format, int line, int column, Args... args) {
		auto fmt = "parse error at line %d, column %d: "s + format;
		return tfm::format(fmt.data(), line, column, args...);
	}
};

} // namespace UHS

#endif

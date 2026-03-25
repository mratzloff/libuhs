#if !defined UHS_ERROR_H
#define UHS_ERROR_H

#include <ostream>
#include <stdexcept>
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

namespace UHS {

class Error : public std::runtime_error {
public:
	Error();
	Error(std::string const& message);
	Error(char const* message);

	template<typename... Args>
	Error(char const* format, Args... args) : Error() {
		auto message = tfm::format(format, args...);
		// TODO: Review for slice
		static_cast<std::runtime_error&>(*this) = std::runtime_error(message);
	}

	std::string const string() const;
};

std::ostream& operator<<(std::ostream& out, Error const& err);

} // namespace UHS

#endif

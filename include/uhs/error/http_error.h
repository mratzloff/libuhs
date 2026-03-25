#if !defined UHS_HTTP_ERROR_H
#define UHS_HTTP_ERROR_H

#include <string>

#include "httplib.h"

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

class HTTPError : public Error {
public:
	template<typename... Args>
	HTTPError(httplib::Result const& res, char const* format, Args... args)
	    : Error(formatMessage(res.value().status, format, args...))
	    , response_{res.value()} {}

	httplib::Response getResponse() const;

private:
	httplib::Response response_;

	template<typename... Args>
	static std::string formatMessage(int status, char const* format, Args... args) {
		auto fmt = "HTTP %d: "s + format;
		return tfm::format(fmt.data(), status, args...);
	}
};

} // namespace UHS

#endif

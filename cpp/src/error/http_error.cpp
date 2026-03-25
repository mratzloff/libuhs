#include "uhs/error/http_error.h"

namespace UHS {

httplib::Response const HTTPError::getResponse() const {
	return response_;
}

} // namespace UHS

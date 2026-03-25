#include "uhs/error/http_error.h"

namespace UHS {

httplib::Response HTTPError::getResponse() const {
	return response_;
}

} // namespace UHS

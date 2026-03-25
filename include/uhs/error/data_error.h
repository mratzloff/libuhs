#if !defined UHS_DATA_ERROR_H
#define UHS_DATA_ERROR_H

#include "uhs/error/error.h"

namespace UHS {

class DataError : public Error {
	using Error::Error;
};

} // namespace UHS

#endif

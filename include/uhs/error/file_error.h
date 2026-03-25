#if !defined UHS_FILE_ERROR_H
#define UHS_FILE_ERROR_H

#include "uhs/error/error.h"

namespace UHS {

class FileError : public Error {
	using Error::Error;
};

} // namespace UHS

#endif

#if !defined UHS_WRITER_H
#define UHS_WRITER_H

#include <memory>
#include <ostream>

#include "uhs/codec.h"
#include "uhs/document.h"
#include "uhs/logger.h"
#include "uhs/options.h"

namespace UHS {

class Writer {
public:
	Writer(Logger const& logger, std::ostream& out, Options const& options = {});
	virtual ~Writer() = default;
	virtual void write(std::shared_ptr<Document> const document) = 0;
	virtual void reset();

protected:
	Codec const codec_;
	Logger const logger_;
	std::ostream& out_;
	Options const& options_;
};

} // namespace UHS

#endif

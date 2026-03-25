#if !defined UHS_LOGGER_H
#define UHS_LOGGER_H

#include <iostream>
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

class Logger {
public:
	Logger(LogLevel level = LogLevel::Error) : level_{level} {}
	void level(LogLevel level) { level_ = level; }

	// Error
	void error(char const* message) const { this->error("%s", message); }
	void error(Error const& err) const { this->error("%s", err.string().c_str()); }
	template<typename... Args>
	void error(char const* format, Args... args) const {
		if (level_ != LogLevel::None) {
			this->log("error: ", format, args...);
		}
	}

	// Warn
	void warn(char const* message) const { this->warn("%s", message); }
	void warn(Error const& err) const { this->warn("%s", err.string().c_str()); }
	template<typename... Args>
	void warn(char const* format, Args... args) const {
		if (level_ == LogLevel::Warn || level_ == LogLevel::Info
		    || level_ == LogLevel::Debug) {

			this->log("warning: ", format, args...);
		}
	}

	// Info
	void info(char const* message) const { this->info("%s", message); }
	template<typename... Args>
	void info(char const* format, Args... args) const {
		if (level_ == LogLevel::Info || level_ == LogLevel::Debug) {
			this->log("info: ", format, args...);
		}
	}

	// Debug
	void debug(char const* message) const { this->debug("%s", message); }
	template<typename... Args>
	void debug(char const* format, Args... args) const {
		if (level_ == LogLevel::Debug) {
			this->log("debug: ", format, args...);
		}
	}

private:
	LogLevel level_;

	template<typename... Args>
	void log(char const* prefix, char const* format, Args... args) const {
		std::string buffer;
		buffer += "uhs: ";
		buffer += prefix;
		buffer += format;
		buffer += '\n';

		tfm::format(std::cerr, buffer.c_str(), args...);
	}
};

} // namespace UHS

#endif

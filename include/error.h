#ifndef UHS_ERROR_H
#define UHS_ERROR_H

#include <string>

namespace UHS {

enum ErrorCode {
	ErrUnknown,
	ErrEOF,
	ErrRead,
	ErrValue,
	ErrToken,
};

class Error {
public:
	Error();
	Error(int code);
	Error(int code, std::string message);
	int code() const;
	void code(int code);
	const std::string message() const;
	void message(const std::string message);

protected:
	int _code;
	std::string _message;
};

}

#endif

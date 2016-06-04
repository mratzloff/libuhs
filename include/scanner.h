#ifndef UHS_SCANNER_H
#define UHS_SCANNER_H

#include <istream>
#include <string>
#include <regex>
#include "error.h"
#include "token.h"
#include "token_queue.h"

namespace UHS {

class Scanner {
public:
	Scanner(std::istream& in);
	virtual ~Scanner();
	void scan();
	bool hasNext();
	std::shared_ptr<Token> next();
	std::shared_ptr<Error> err();

protected:
	static const int LineLen = 80;

	const std::regex _descriptorRegex {"^([0-9]+) ([a-z]{4,})$"};
	const std::regex _dataAddressRegex {"^0{6}(?: [0-9])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _overlayRegionRegex {"^([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) ([0-9]{4,}) ([0-9]{4,})$"};

	std::istream& _in;
	TokenQueue _out;
	std::shared_ptr<Error> _err;
	int _line;
	std::size_t _column;
	std::size_t _offset;
	std::string _buf;

	IdentType scanDescriptor(std::string s, std::smatch m, std::size_t offset);
	void scanDataAddress(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayRegion(std::string s, std::smatch m, std::size_t offset);
	void scanOverlayAddress(std::string s, std::smatch m, std::size_t offset);
	void eof();
	char read();
	std::shared_ptr<Error> formatError(std::shared_ptr<Error> err) const;
	bool isNumber(std::string s) const;
	const std::string ltrim(std::string s, char c) const;
	const std::string rtrim(std::string s, char c) const;
};

}

#endif

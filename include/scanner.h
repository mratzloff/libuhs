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
	void scan();
	Token* next();
	Error* err();
	
	virtual ~Scanner(); // todo: De-virtualize this

protected:
	const std::regex _descriptorRegex {"^([0-9]+) ([a-z]{4,})$"};
	const std::regex _dataAddressRegex {"^0{6}(?: [0-9])? ([0-9]{6,}) ([0-9]{6,})$"};
	const std::regex _overlayRegionRegex {"^([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,}) ([0-9]{4,})$"};
	const std::regex _overlayAddressRegex {"^0{6} ([0-9]{6,}) ([0-9]{6,}) ([0-9]{4,}) ([0-9]{4,})$"};

	std::istream& _in;
	TokenQueue* _out;
	Error* _err;
	int _line;
	int _column;
	int _offset;
	std::string _buf;
	
	void asyncScan();
	std::string scanDescriptor(std::string s, int offset);
	void scanDataAddress(std::string s, int offset);
	void scanOverlayRegion(std::string s, int offset);
	void scanOverlayAddress(std::string s, int offset);
	void eof();
	char read();
	Error* formatError(Error* err) const;
	bool isNumber(std::string s) const;
	const std::string ltrimZeroes(std::string s) const;
};

}

#endif

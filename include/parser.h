#ifndef UHS_PARSER_H
#define UHS_PARSER_H

#include <memory>
#include "scanner.h"

namespace UHS {

class Parser {
public:
	Parser(std::istream& in);
	virtual ~Parser();
	void parse();

protected:
	// static int HeaderLen = 4;
	// static int FormatTokenLen = 3;
	// static constexpr const char* InlineStartToken = "#w+";
	// static constexpr const char* InlineEndToken = "#w.";
	// static constexpr const char* PreformattedStartToken = "#p-";
	// static constexpr const char* PreformattedEndToken = "#p+";

	std::unique_ptr<Scanner> _scanner;
};

}

#endif

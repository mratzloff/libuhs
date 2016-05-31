#ifndef UHS_PARSER_H
#define UHS_PARSER_H

namespace UHS {

class Parser {
public:
	Parser();
	virtual ~Parser();

protected:
	static int HeaderLen = 4;
	static int FormatTokenLen = 3;
	static constexpr const char* InlineStartToken = "#w+";
	static constexpr const char* InlineEndToken = "#w.";
	static constexpr const char* PreformattedStartToken = "#p-";
	static constexpr const char* PreformattedEndToken = "#p+";
};

}

#endif

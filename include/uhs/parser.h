#if !defined UHS_PARSER_H
#define UHS_PARSER_H

#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "uhs/codec.h"
#include "uhs/constants.h"
#include "uhs/crc.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node.h"
#include "uhs/node/container_node.h"
#include "uhs/options.h"
#include "uhs/token.h"
#include "uhs/tokenizer.h"

namespace UHS {

class Parser {
public:
	explicit Parser(Logger const& logger, Options const& options = {});

	std::shared_ptr<Document> parse(std::istream& in);
	std::shared_ptr<Document> parseFile(std::string const& filename);
	void reset();

private:
	using NodeMap = std::map<int const, Node*>;
	using DataCallback = std::function<void(std::string)>;

	struct NodeRange {
		NodeRange(Node& node, int const min, int const max);

		int max = 0;
		int min = 0;
		Node& node;
	};

	struct NodeRangeList {
		std::vector<NodeRange> data = {};

		void add(Node& node, int const min, int const max);
		void clear();
		Node* find(int const min, int const max);
	};

	struct LinkData {
		LinkData(Element& link, int const line, int const column);

		int column = 0;
		int line = 0;
		Element& link;
	};

	struct VisibilityData {
		VisibilityData(int targetLine, VisibilityType visibility);

		int targetLine = 0;
		VisibilityType visibility;
	};

	struct DataHandler {
		DataHandler(int line, int column, std::size_t offset, std::size_t length,
		    DataCallback func);

		int column = 0;
		DataCallback func;
		std::size_t length = 0;
		int line = 0;
		std::size_t offset = 0;
	};

	static constexpr auto FormatTokenLength = 3;
	static constexpr auto HeaderLength = 4;

	// Fix for bad 88a backwards-compatibility header found in 401 files
	static constexpr auto BadHeaderTitle = "Why aren't there any more hints here?";
	static constexpr auto BadHeaderFirstChildLine = HeaderLength + 16;
	static constexpr auto GoodHeaderFirstChildLine = HeaderLength + 15;

	Codec const codec_;
	std::unique_ptr<CRC> crc_ = nullptr;
	std::vector<DataHandler> dataHandlers_;
	std::vector<LinkData> deferredLinks_;
	std::vector<VisibilityData> deferredVisibilities_;
	std::shared_ptr<Document> document_ = nullptr;
	bool done_ = false;
	bool isTitleSet_ = false;
	std::string key_;
	int lineOffset_ = 0;
	Logger const logger_;
	Options const options_;
	NodeRangeList parents_;
	std::unique_ptr<Tokenizer> tokenizer_ = nullptr;

	void addDataCallback(
	    int line, int column, std::size_t offset, std::size_t length, DataCallback func);
	void addNodeToParentIndex(ContainerNode& node);
	void appendText(std::string& text, TextFormat& format, ContainerNode& node);
	void checkCRC();
	std::unique_ptr<Token const> expect(TokenType expected);
	Node* findParent(ContainerNode& node);
	int fixHeaderFirstChildLine(std::string title, int firstChildLine) const;
	std::unique_ptr<Token const> next();
	int offsetLine(int line);
	void parse88a();
	void parse88aCredits(std::unique_ptr<Token const> token);
	void parse88aElements(int firstHintTextLine, NodeMap& parents);
	void parse88aTextNodes(int lastHintTextLine, NodeMap& parents);
	void parse96a();
	void parseCommentElement(Element& element);
	void parseData(std::unique_ptr<Token const> token);
	void parseDataElement(Element& element);
	void parseDate(std::string const& date, std::tm& tm) const;
	std::shared_ptr<Element> parseElement(std::unique_ptr<Token const> token);
	void parseHeaderSep(std::unique_ptr<Token const> token);
	void parseHintElement(Element& element);
	void parseHyperpngElement(Element& element);
	void parseIncentiveElement(Element& element);
	void parseInfoElement(Element& element);
	void parseLinkElement(Element& element);
	void parseOverlayElement(Element& element);
	void parseSubjectElement(Element& element);
	void parseTextElement(Element& element);
	void parseTime(std::string const& time, std::tm& tm) const;
	void parseVersionElement(Element& element);
	void parseWithFormat(std::string const& text, TextFormat& format, ContainerNode& node,
	    ElementType elementType);
	void processLinks();
	void processVisibility();
};

} // namespace UHS

#endif

#if !defined UHS_HTML_WRITER_H
#define UHS_HTML_WRITER_H

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "pugixml.hpp"
#include "tsl/hopscotch_map.h"

#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/element_dispatcher.h"
#include "uhs/node/text_node.h"
#include "uhs/traits/visibility.h"
#include "uhs/writer.h"

namespace UHS {

class HTMLWriter : public Writer {
public:
	struct TableAccessor;

	HTMLWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

	class Table {
		friend struct HTMLWriter::TableAccessor;

	public:
		static constexpr double HeaderlessConsensusThreshold = 0.6;

		Table(std::vector<std::string> const& lines);

		std::size_t endLine() const;
		bool hasPrecedingText() const;
		void parse();
		void serialize(pugi::xml_node& xmlNode) const;
		std::size_t startLine() const;
		bool valid() const;

	private:
		using Boundaries = std::vector<std::pair<std::size_t, std::size_t>>;

		bool charGrid_ = false;
		int demarcationLine_ = 0;
		std::size_t endLine_ = 0;
		bool headerless_ = false;
		std::vector<std::string> const lines_;
		bool pipeDelimited_ = false;
		std::vector<std::vector<std::string>> rows_;
		std::size_t startLine_ = 0;
		bool valid_ = false;

		void addHeaderRows(Boundaries const& columnBoundaries);
		Boundaries detectBoundariesFromLine(std::string const& line) const;
		bool detectCharGrid(Boundaries const& columnBoundaries) const;
		Boundaries detectColumnBoundaries() const;
		Boundaries detectHeaderlessColumnBoundaries() const;
		std::vector<std::string> extractCellsByBoundaries(
		    std::string const& line, Boundaries const& boundaries) const;
		std::vector<std::string> extractDataRowCells(std::string const& line,
		    Boundaries const& naturalBounds, Boundaries const& columnBoundaries,
		    int expectedNumColumns) const;
		int findDemarcationLine() const;
		bool isContinuationLine(std::string const& line, Boundaries const& naturalBounds,
		    Boundaries const& columnBoundaries, std::size_t tableWidth,
		    bool afterSeparator) const;
		bool isNextTableBoundary(std::vector<std::string>::const_iterator start) const;
		void mergeContinuationLine(std::string const& line,
		    Boundaries const& naturalBounds, Boundaries const& columnBoundaries);
		bool parsePipeDelimitedTable();
		void serializeBody(pugi::xml_node& table) const;
		void serializeHeader(pugi::xml_node& table) const;
		void serializeTextFallback(pugi::xml_node& parent) const;
	};

private:
	using NodeMap = tsl::hopscotch_map<Node const*, pugi::xml_node const>;

	using SerializeFunc = void (HTMLWriter::*)(
	    Element const& element, pugi::xml_node xmlNode) const;
	using Dispatcher = ElementDispatcher<SerializeFunc>;

	static Dispatcher dispatcher_;

	static void appendLinesToNode(
	    pugi::xml_node node, std::vector<std::string> const& lines);
	static std::vector<std::vector<std::string>> splitIntoParagraphs(
	    std::vector<std::string> const& segment);

	std::string css_;
	std::string js_;
	std::map<ElementType, std::string> mediaContentTypes_;
	std::map<ElementType, std::string> mediaTagTypes_;

	std::shared_ptr<Document> addEntryPointTo88aDocument(Document const& document) const;
	std::optional<pugi::xml_node> appendBody(
	    Element const& element, pugi::xml_node xmlNode) const;
	void appendClassNames(
	    pugi::xml_node xmlNode, std::vector<std::string> classNames) const;
	pugi::xml_node appendMedia(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node appendTitle(Element const& element, pugi::xml_node xmlNode) const;
	void appendVisibility(Traits::Visibility const& node, pugi::xml_node xmlNode) const;
	void applyHyperlinkFormat(TextNode const& textNode, pugi::xml_node xmlNode) const;
	void cleanUpTableContainerBreaks(pugi::xml_node root) const;
	pugi::xml_node createHTMLDocument(
	    Document const& document, pugi::xml_document& xml) const;
	std::optional<pugi::xml_node> findHyperpngContainer(
	    Element const& element, pugi::xml_node xmlNode) const;
	std::pair<std::vector<std::string>, std::vector<std::string>::const_iterator>
	    findNextSegment(std::vector<std::string>::const_iterator it,
	        std::vector<std::string>::const_iterator end) const;
	pugi::xml_node findOrCreateMap(Element const& element, pugi::xml_node xmlNode) const;
	pugi::xml_node findXMLParent(Node const& node, pugi::xml_node const parent,
	    NodeMap const parents, int const depth) const;
	std::string getDataURI(std::string const& contentType, std::string const& data) const;
	std::tuple<int, int> getImageSize(Element const& element) const;
	Element const* getParentElement(Element const& element) const;
	std::tuple<int, int, int, int> getRegionCoordinates(Element const& element) const;
	void populateArea(Element const& element, pugi::xml_node area) const;
	void removeEmptyParagraphs(pugi::xml_node root) const;
	void removeTrailingBreaks(pugi::xml_node xmlNode) const;
	std::pair<int, bool> scanForTables(
	    std::vector<std::string> const& lines, bool isMonoOrOverflow) const;
	void serialize(Document const& document, pugi::xml_document& xml) const;
	void serializeBlankElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeCommentElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeDataElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeDocument(Document const& document, pugi::xml_node root) const;
	void serializeElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeGifaElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeHintElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeHyperpngElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeIncentiveElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeInfoElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeLinkElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeOverlayElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeSoundElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeSubjectElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeTextElement(Element const& element, pugi::xml_node xmlNode) const;
	void serializeTextNode(TextNode const& textNode, pugi::xml_node xmlNode) const;
	void serializeTextNodeToContainer(TextNode const& textNode, pugi::xml_node xmlNode,
	    std::vector<std::string> const& lines, bool hasTrailingBreak) const;
	void splitMonospaceOverflowSpans(pugi::xml_node root) const;
	void wrapInlineRuns(pugi::xml_node root) const;
};

} // namespace UHS

#endif

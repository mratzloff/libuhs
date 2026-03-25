#if !defined UHS_UHS_WRITER_H
#define UHS_UHS_WRITER_H

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "uhs/constants.h"
#include "uhs/crc.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/element_dispatcher.h"
#include "uhs/node/text_node.h"
#include "uhs/writer.h"

namespace UHS {

class UHSWriter : public Writer {
public:
	// The official readers work with lines longer than 76 characters, but
	// lines were capped at 76 so that DOS readers would display correctly
	// within the standard 80-character window (with border and padding).
	static std::size_t const LineLength = 76;

	UHSWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;
	void reset() override;

private:
	using SerializeFunc = int (UHSWriter::*)(Element&, std::string&);
	using Dispatcher = ElementDispatcher<SerializeFunc>;
	using DataQueue = std::queue<std::pair<ElementType, std::string const>>;

	static constexpr auto DataAddressMarker = "000000";
	static auto const InitialElementLength = 2;  // Element descriptor and title
	static std::size_t const FileSizeLength = 7; // Up to 9,999,999 bytes per document
	static constexpr auto InfoLengthMarker = "length=0000000";
	static std::size_t const InitialBufferLength = 204'800; // 200 KiB
	static constexpr auto LinkMarker = "** LINK **";
	static std::size_t const MediaSizeLength = 6;  // Up to 999,999 bytes per media file
	static std::size_t const RegionSizeLength = 4; // Up to 9,999 × 9,999 px per region

	static Dispatcher dispatcher_;

	CRC crc_;
	int currentLine_ = 1;
	DataQueue data_;
	std::vector<Node*> deferredLinks_;
	std::shared_ptr<Document> document_ = nullptr;
	std::string key_;

	void addData(Element const& element);
	void convertTo96a();
	std::string createDataAddress(
	    std::size_t bodyLength, std::string textFormat = "") const;
	std::string createOverlayAddress(std::size_t bodyLength, int x, int y) const;
	std::string createRegion(int x1, int y1, int x2, int y2) const;
	std::string encodeText(std::string const& text, ElementType const parentType);
	std::string formatText(TextNode const& textNode, TextFormat const previousFormat);
	void serialize88a(Document const& document, std::string& out);
	void serialize96a(std::string& out);
	int serializeBlankElement(Element& element, std::string& out);
	int serializeCommentElement(Element& element, std::string& out);
	void serializeCRC(std::string& out);
	void serializeData(std::string& out);
	int serializeDataElement(Element& element, std::string& out);
	int serializeElement(Element& element, std::string& out);
	void serializeElementHeader(Element& element, std::string& out) const;
	int serializeHintChild(Node& node, Element& parentElement,
	    std::map<int const, TextFormat>& formats, std::string& textBuffer,
	    std::string& out);
	int serializeHintElement(Element& element, std::string& out);
	int serializeHyperpngElement(Element& element, std::string& out);
	int serializeIncentiveElement(Element& element, std::string& out);
	int serializeInfoElement(Element& element, std::string& out);
	int serializeLinkElement(Element& element, std::string& out);
	int serializeOverlayElement(Element& element, std::string& out);
	int serializeSubjectElement(Element& element, std::string& out);
	int serializeTextElement(Element& element, std::string& out);
	void updateLinkTargets(std::string& out) const;
};

} // namespace UHS

#endif

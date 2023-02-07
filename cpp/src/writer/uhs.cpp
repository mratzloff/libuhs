#include <iomanip>

#include "uhs.h"

namespace UHS {

UHSWriter::Serializer UHSWriter::serializer_;

UHSWriter::UHSWriter(Logger const& logger, std::ostream& out, Options const& options)
    : Writer(logger, out, options) {}

void UHSWriter::write(std::shared_ptr<Document> const document) {
	std::string buffer;
	buffer.reserve(InitialBufferLength);
	document_ = document;
	auto const is88a = document_->isVersion(VersionType::Version88a);

	switch (options_.mode) {
	case ModeType::Auto:
		if (is88a) {
			this->serialize88a(*document, buffer);
		} else {
			this->serialize96a(buffer);
		}
		break;
	case ModeType::Version88a:
		this->serialize88a(*document, buffer);
		break;
	case ModeType::Version96a:
		if (is88a) {
			throw DataError("conversion from 88a to 96a is unimplemented");
		}
		this->serialize96a(buffer);
		break;
	}

	out_ << buffer;
	std::flush(out_);
}

void UHSWriter::reset() {
	Writer::reset();

	crc_.reset();
	document_.reset();
	key_.clear();

	while (!data_.empty()) {
		data_.pop();
	}
}

void UHSWriter::serialize88a(Document const& document, std::string& out) {
	std::queue<Node const*> queue;
	auto previousType = ElementType::Unknown;
	auto line = 1;
	auto numPreviousChildren = 0;
	auto firstHintTextLine = 0;
	auto lastHintTextLine = 0;

	// Traverse tree, breadth first
	queue.push(&document);
	std::string buffer;

	while (!queue.empty()) {
		auto const node = queue.front();
		queue.pop();

		switch (node->nodeType()) {
		case NodeType::Document:
			numPreviousChildren = node->numChildren();
			break;
		case NodeType::Element: {
			auto const element = static_cast<Element const*>(node);
			auto const elementType = element->elementType();
			std::string title;

			switch (elementType) {
			case ElementType::Subject:
				line += numPreviousChildren * 2;
				title = element->title();
				break;
			case ElementType::Hint:
				if (previousType == ElementType::Subject) {
					line += numPreviousChildren * 2;
				} else {
					line += numPreviousChildren;
				}
				title = Strings::rtrim(element->title(), '?');
				break;
			default:
				throw DataError(
				    "unexpected element type: %s", Element::typeString(elementType));
			}

			if (!options_.debug) {
				title = codec_.encode88a(title);
			}
			buffer += title;
			buffer += EOL;
			buffer += std::to_string(line);
			buffer += EOL;

			previousType = elementType;
			numPreviousChildren = node->numChildren();
			break;
		}
		case NodeType::Group:
			line += numPreviousChildren - 1; // Simplifies to -1 after first pass
			if (lastHintTextLine == 0) {
				lastHintTextLine = line;
			}
			numPreviousChildren = node->numChildren();
			break;
		case NodeType::Text: {
			auto const& textNode = static_cast<TextNode const*>(node);
			auto body = textNode->body();

			if (!options_.debug) {
				body = codec_.encode88a(body);
			}
			buffer += body;
			buffer += EOL;

			firstHintTextLine = line;
			--line;
			break;
		}
		default:
			break; // Ignore
		}

		if (auto n = node->firstChild()) {
			queue.push(n.get());

			while ((n = n->nextSibling())) {
				queue.push(n.get());
			}
		}
	}

	out += Token::Signature;
	out += EOL;
	out += document.title() + EOL;
	out += std::to_string(firstHintTextLine) + EOL;
	out += std::to_string(lastHintTextLine) + EOL;
	out += buffer;

	if (auto credit = document.attr("notice")) {
		out += Token::CreditSep;
		out += EOL;
		out += Strings::wrap(*credit, EOL, LineLength) + EOL;
	}
}

void UHSWriter::serialize96a(std::string& out) {
	// TODO: Fix hint separators in convertTo96a() before enabling
	// if (document_->isVersion(VersionType::Version88a)) {
	// 	this->convertTo96a();
	// }

	key_ = codec_.createKey(document_->title());

	for (auto node = document_->firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Break:
			throw DataError("unexpected break node");
		case NodeType::Document: {
			auto const document = static_pointer_cast<Document const>(node);
			if (!document->isVersion(VersionType::Version88a)) {
				continue;
			}
			this->serialize88a(*document, out);
			out += Token::HeaderSep;
			out += EOL;
			break;
		}
		case NodeType::Element: {
			auto const element = static_pointer_cast<Element>(node);
			this->serializeElement(*element, out);
			break;
		}
		case NodeType::Group:
			throw DataError("unexpected group node");
		case NodeType::Text: {
			auto const textNode = static_pointer_cast<TextNode>(node);
			throw DataError("unexpected text node: %s", textNode->body());
		}
		}
	}

	this->updateLinkTargets(out);

	out += Token::DataSep;

	this->serializeData(out);
	this->serializeCRC(out);
}

int UHSWriter::serializeElement(Element& element, std::string& out) {
	element.line(currentLine_);
	return serializer_.invoke(*this, element, out);
}

int UHSWriter::serializeBlankElement(Element& element, std::string& out) {
	auto length = InitialElementLength;

	currentLine_ += length;
	element.length(length);
	this->serializeElementHeader(element, out);

	return length;
}

int UHSWriter::serializeCommentElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	auto const& body = codec_.encodeSpecialChars(element.body());
	if (!body.empty()) {
		buffer += Strings::wrap(body, EOL, LineLength, length) + EOL;
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

// GIFs don't appear to be displayed by the official reader any longer
int UHSWriter::serializeDataElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	auto const& body = element.body();

	auto dataAddress = this->createDataAddress(body.length());
	this->addData(element);
	++length;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += dataAddress;

	return length;
}

int UHSWriter::serializeHintChild(Node& node, Element& parentElement,
    std::map<int const, TextFormat>& formats, std::string& textBuffer, std::string& out) {

	auto length = 0;
	auto depth = parentElement.depth();
	auto const elementType = parentElement.elementType();

	if (node.isElement() && elementType == ElementType::Nesthint) {
		// Outputs to text buffer
		auto& element = static_cast<Element&>(node);
		auto followsTextNode = false;

		if (node.hasPreviousSibling()) {
			auto previousNode = node.previousSibling();
			followsTextNode = previousNode->isText();
		}
		if (!followsTextNode && element.inlined()) {
			textBuffer += Token::InlineBegin;
		}
	}

	if (!node.isText() && !textBuffer.empty()) {
		// Outputs directly to main buffer
		auto const scEncoded = codec_.encodeSpecialChars(textBuffer);
		auto childLength = 0;
		auto const wrapped = Strings::wrap(scEncoded, "\n", LineLength, childLength);
		out += this->encodeText(wrapped, elementType);
		length += childLength;
		currentLine_ += childLength;
		textBuffer.clear();
	}

	if (node.isElement() && elementType == ElementType::Nesthint) {
		// Outputs directly to main buffer
		auto& element = static_cast<Element&>(node);
		depth = parentElement.depth();

		out += Token::NestedElementSep;
		out += EOL;
		++length;
		++currentLine_;

		length += this->serializeElement(element, out);
	} else if (node.isBreak()) {
		// Outputs directly to main buffer
		out += Token::NestedTextSep;
		out += EOL;
		++length;
		++currentLine_;
	} else if (node.isGroup()) {
		// Outputs directly to main buffer
		for (auto child = node.firstChild(); child; child = child->nextSibling()) {
			length +=
			    this->serializeHintChild(*child, parentElement, formats, textBuffer, out);
		}
	} else if (node.isText()) {
		// Outputs to text buffer
		auto const& textNode = static_cast<TextNode const&>(node);
		textBuffer += this->formatText(textNode, formats[depth]);
		formats[depth] = textNode.format();
	} else {
		throw DataError("unexpected node type: %s", node.nodeTypeString());
	}

	return length;
}

int UHSWriter::serializeHintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;
	std::map<int const, TextFormat> formats;
	std::string textBuffer;

	currentLine_ += length;
	auto const elementType = element.elementType();

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		length += this->serializeHintChild(*node, element, formats, textBuffer, buffer);
	}

	if (!textBuffer.empty()) {
		auto const scEncoded = codec_.encodeSpecialChars(textBuffer);
		auto childLength = 0;
		auto const wrapped = Strings::wrap(scEncoded, "\n", LineLength, childLength);
		buffer += this->encodeText(wrapped, elementType);
		length += childLength;
		currentLine_ += childLength;
	}

	element.length(length);
	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeHyperpngElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	auto const& body = element.body();

	auto buffer = this->createDataAddress(body.length());
	this->addData(element);
	++length;
	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw DataError("unexpected node type: %s", node->nodeTypeString());
		}

		auto child = static_pointer_cast<Element>(node);

		std::vector<std::pair<std::string, int>> coords = {
		    {"region-top-left-x", 0},
		    {"region-top-left-y", 0},
		    {"region-bottom-right-x", 0},
		    {"region-bottom-right-y", 0},
		};
		for (auto& [attr, coord] : coords) {
			if (auto value = child->attr(attr)) {
				try {
					coord = Strings::toInt(*value);
				} catch (Error const& err) {
					throw DataError("invalid coordinate: %s", *value);
				}
			}
		}

		auto x1 = coords[0].second;
		auto y1 = coords[1].second;
		auto x2 = coords[2].second;
		auto y2 = coords[3].second;

		buffer += this->createRegion(x1, y1, x2, y2);
		++length;
		++currentLine_;

		length += this->serializeElement(*child, buffer);
	}

	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeIncentiveElement(Element& element, std::string& out) {
	if (!options_.preserve) {
		return 0;
	}

	auto length = InitialElementLength;
	std::string buffer;

	std::map<ElementType const, bool> excluded = {
	    {ElementType::Incentive, true},
	    {ElementType::Info, true},
	    {ElementType::Version, true},
	};

	std::vector<std::string> instructions;

	for (auto& node : *document_) {
		if (node.nodeType() != NodeType::Element) {
			continue;
		}

		auto const& candidate = static_cast<Element const&>(node);
		if (excluded[candidate.elementType()]) {
			continue;
		}

		std::string flag;
		switch (candidate.visibility()) {
		case VisibilityType::RegisteredOnly:
			flag = Token::RegisteredOnly;
			break;
		case VisibilityType::UnregisteredOnly:
			flag = Token::UnregisteredOnly;
			break;
		default:
			continue; // Ignore
		}

		instructions.push_back(std::to_string(candidate.line()) + flag);
	}

	if (instructions.empty()) {
		return 0; // Elide element if empty
	}

	auto decoded = Strings::join(instructions, " ");
	auto wrapped = Strings::wrap(decoded, "\n", LineLength, length);
	auto lines = Strings::split(wrapped, "\n");
	if (!options_.debug) {
		for (auto& line : lines) {
			line = codec_.encode96a(line, key_, false);
		}
	}
	buffer += Strings::join(lines, EOL);
	buffer += EOL;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;
	return length;
}

int UHSWriter::serializeInfoElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	buffer += InfoLengthMarker;
	buffer += EOL;
	++length;

	auto const now = std::time(nullptr);
	auto const tm = std::localtime(&now);

	char formatted[10];
	auto const dateLength = std::strftime(formatted, 10, "%d-%b-%y", tm);
	buffer += "date=" + std::string(formatted, dateLength) + EOL;
	++length;

	auto const timeLength = std::strftime(formatted, 9, "%H:%M:%S", tm);
	buffer += "time=" + std::string(formatted, timeLength) + EOL;
	++length;

	std::map<std::string const, bool> whitelisted = {
	    {"author", true},
	    {"author-note", true},
	    {"copyright", true},
	    {"game-note", true},
	    {"publisher", true},
	};

	for (auto const& [key, value] : document_->attrs()) {
		if (whitelisted[key] && !value.empty()) {
			auto const scEncoded = codec_.encodeSpecialChars(value);
			buffer += Strings::wrap(scEncoded, EOL, LineLength, length, key + "=");
			buffer += EOL;
		}
	}

	if (auto notice = document_->attr("notice")) {
		auto const scEncoded = codec_.encodeSpecialChars(*notice);
		buffer += Strings::wrap(scEncoded, EOL, LineLength, length, Token::NoticePrefix);
		buffer += EOL;
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeLinkElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	buffer += LinkMarker;
	buffer += EOL;
	++length;

	int targetID = 0;
	try {
		targetID = Strings::toInt(element.body());
		if (targetID < 0) {
			throw Error();
		}
	} catch (Error const& err) {
		throw DataError("expected positive integer, found %s", element.body());
	}

	if (auto target = document_->find(targetID)) {
		deferredLinks_.push_back(target);
	} else {
		throw DataError("target element not found for id %d", targetID);
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeOverlayElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	auto const& body = element.body();

	std::vector<std::pair<std::string, int>> coords = {
	    {"image-x", 0},
	    {"image-y", 0},
	};
	for (auto& [attr, coord] : coords) {
		if (auto value = element.attr(attr)) {
			try {
				coord = Strings::toInt(*value);
			} catch (Error const& err) {
				throw DataError("invalid coordinate: %s", *value);
			}
		}
	}

	auto x = coords[0].second;
	auto y = coords[1].second;

	auto buffer = this->createOverlayAddress(body.length(), x, y);
	this->addData(element);
	++length;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeSubjectElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw DataError("unexpected node type: %s", node->nodeTypeString());
		}
		auto const child = static_pointer_cast<Element>(node);
		length += this->serializeElement(*child, buffer);
	}
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeTextElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	auto const elementType = element.elementType();
	std::string textBuffer;
	std::string buffer;
	auto previousFormat = TextFormat::None;
	auto const message = "unexpected node type: %s";

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->isGroup()) {
			for (auto child = node->firstChild(); child; child = child->nextSibling()) {
				if (child->isText()) {
					auto const textNode = static_pointer_cast<TextNode const>(child);
					textBuffer += this->formatText(*textNode, previousFormat);
					previousFormat = textNode->format();
				} else {
					throw DataError(message, child->nodeTypeString());
				}
			}
		} else {
			throw DataError(message, node->nodeTypeString());
		}
	}

	auto const scEncoded = codec_.encodeSpecialChars(textBuffer);
	auto body = this->encodeText(scEncoded, elementType);

	auto const format = element.attr("format");
	buffer += this->createDataAddress(body.length(), format ? *format : "0");
	++length;

	data_.emplace(std::make_pair(elementType, body));

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

void UHSWriter::serializeElementHeader(Element& element, std::string& out) const {
	out += std::to_string(element.length());
	out += ' ';
	out += element.elementTypeString();
	out += EOL;

	auto const title = codec_.encodeSpecialChars(element.title());

	if (title.empty()) {
		out += '-';
	} else {
		out += title;
	}
	out += EOL;
}

void UHSWriter::updateLinkTargets(std::string& out) const {
	// Because we're shifting characters, we move backwards through the file in
	// order to reduce the total number of bytes copied within the output buffer.
	auto const end = deferredLinks_.crend();
	for (auto it = deferredLinks_.crbegin(); it != end; ++it) {
		auto const target = *it;
		assert(target);

		if (!target->isElement()) {
			logger_.warn("link target is not an element");
			continue;
		}

		auto targetElement = static_cast<Element*>(target);
		auto const pos = out.rfind(LinkMarker);
		if (pos == std::string::npos) {
			throw DataError("could not find link marker");
		}

		auto const targetLine = std::to_string(targetElement->line());
		auto const length = targetLine.length();
		out.replace(pos, length, targetLine);
		out.erase(pos + length, strlen(LinkMarker) - length);
	}
}

void UHSWriter::serializeData(std::string& out) {
	// Add data and replace data addresses
	auto dataOffset = out.length();
	auto searchEnd = out.cend();
	auto offset = 0;

	while (!data_.empty()) {
		auto const& pair = data_.front();
		auto const elementType = pair.first;
		auto const& data = pair.second;

		std::smatch matches;
		auto isOverlay = (elementType == ElementType::Overlay);
		auto const& regex = (isOverlay) ? Regex::OverlayAddress : Regex::DataAddress;
		if (!std::regex_search(out.cbegin() + offset, searchEnd, matches, regex)) {
			throw DataError(
			    "could not find address offset for %s element data (%d bytes)",
			    Element::typeString(elementType),
			    data.length());
		}

		auto const& matchNumber = (isOverlay) ? 1 : 2;
		auto const position = static_cast<std::size_t>(matches.position(matchNumber));
		auto const dataAddress = std::to_string(dataOffset);
		auto const dataAddressLength = dataAddress.length();
		auto const dataAddressOffset =
		    offset + position + FileSizeLength - dataAddressLength;

		// Replace data address
		out.replace(dataAddressOffset, dataAddressLength, dataAddress);

		// Advance search position
		offset += position;

		// Add data
		out += data;
		dataOffset += data.length();

		data_.pop();
	}

	// Find length attribute
	auto const position = out.find(InfoLengthMarker);
	if (position == std::string::npos) {
		return; // No info node present
	}
	auto const fileLength = out.length() + CRC::ByteLength;
	auto const fileLengthString = std::to_string(fileLength);
	auto const fileLengthStringLength = fileLengthString.length();
	auto const infoLengthOffset =
	    position + strlen(InfoLengthMarker) - fileLengthStringLength;

	// Replace length attribute
	out.replace(infoLengthOffset, fileLengthStringLength, fileLengthString);
}

void UHSWriter::serializeCRC(std::string& out) {
	// Calculate
	crc_.calculate(out.data(), out.length());

	// Write checksum
	std::vector<char> checksum;
	checksum.reserve(2);
	crc_.result(checksum);
	out.push_back(checksum[0]);
	out.push_back(checksum[1]);
}

std::string UHSWriter::formatText(
    TextNode const& textNode, TextFormat const previousFormat) {

	auto body = textNode.body();

	if (textNode.hasFormat(TextFormat::Hyperlink)) {
		body = Token::HyperlinkBegin + body + Token::HyperlinkEnd;
	}

	if (textNode.hasFormat(TextFormat::Monospace)
	    && !hasFormat(previousFormat, TextFormat::Monospace)) {

		body = Token::MonospaceBegin + body;
	} else if (!textNode.hasFormat(TextFormat::Monospace)
	           && hasFormat(previousFormat, TextFormat::Monospace)) {

		body = Token::MonospaceEnd + body;
	}

	if (textNode.hasFormat(TextFormat::Overflow)
	    && !hasFormat(previousFormat, TextFormat::Overflow)) {

		body = Token::OverflowBegin + body;
	} else if (!textNode.hasFormat(TextFormat::Overflow)
	           && hasFormat(previousFormat, TextFormat::Overflow)) {

		body = Token::OverflowEnd + body;
	}

	if (textNode.hasPreviousSibling()) {
		if (auto node = textNode.previousSibling(); node->isElement()) {
			auto const previousElement = static_cast<Element*>(node);
			if (previousElement->inlined()) {
				body = Token::InlineEnd + body;
			}
		}
	}

	if (textNode.hasNextSibling()) {
		if (auto node = textNode.nextSibling(); node->isElement()) {
			auto const nextElement = static_pointer_cast<Element>(node);
			if (nextElement->inlined()) {
				body += Token::InlineBegin;
			}
		}
	}

	return body;
}

std::string UHSWriter::encodeText(std::string const& text, ElementType const parentType) {
	std::string buffer;

	auto lines = Strings::split(text, "\n");
	auto numLines = lines.size();

	for (std::size_t i = 0; i < numLines; ++i) {
		auto& line = lines[i];

		if (line == "") {
			line = Token::ParagraphSep;
		}

		// Account for weird edge case
		if (i + 1 < numLines) {
			if (lines[i + 1].starts_with(Token::HyperlinkBegin)
			    && !Strings::endsWithAttachedPunctuation(lines[i])) {
				lines[i] += ' ';
			}
			if (lines[i + 1].starts_with(Token::InlineBegin)
			    && !Strings::endsWithAttachedPunctuation(lines[i])) {
				lines[i] += ' ';
			}
		}

		if (options_.debug) {
			continue; // Don't encode text
		}

		if (parentType == ElementType::Hint) {
			line = codec_.encode88a(line);
		} else if (parentType == ElementType::Nesthint) {
			line = codec_.encode96a(line, key_, false);
		} else if (parentType == ElementType::Text) {
			line = codec_.encode96a(line, key_, true);
		}
	}

	buffer += Strings::join(lines, EOL);
	buffer += EOL; // TODO: Remove this and replace with better system

	return buffer;
}

void UHSWriter::addData(Element const& element) {
	data_.emplace(std::make_pair(element.elementType(), element.body()));
}

std::string UHSWriter::createDataAddress(
    std::size_t bodyLength, std::string textFormat) const {

	std::string out;

	out += DataAddressMarker;
	out += ' ';
	out += (textFormat.empty() ? "" : textFormat + ' ');
	out += std::string(FileSizeLength, '0');
	out += ' ';
	std::ostringstream buffer;
	buffer << std::setfill('0') << std::setw(MediaSizeLength) << bodyLength;
	out += buffer.str();
	out += EOL;

	return out;
}

std::string UHSWriter::createOverlayAddress(std::size_t bodyLength, int x, int y) const {
	std::string out;

	out += DataAddressMarker;
	out += ' ';
	out += std::string(FileSizeLength, '0');
	out += ' ';
	std::ostringstream buffer;
	buffer << std::setfill('0') << std::setw(MediaSizeLength) << bodyLength;
	buffer << ' ' << std::setfill('0') << std::setw(RegionSizeLength) << x;
	buffer << ' ' << std::setfill('0') << std::setw(RegionSizeLength) << y;
	out += buffer.str();
	out += EOL;

	return out;
}

std::string UHSWriter::createRegion(int x1, int y1, int x2, int y2) const {
	std::ostringstream buffer;
	std::vector<int const> coords = {x1, y1, x2, y2};
	auto continuation = false;

	for (auto coord : coords) {
		if (continuation) {
			buffer << ' ';
		}
		auto width = RegionSizeLength;
		if (coord < 0) {
			buffer << '-';
			width -= 1;
			coord = std::abs(coord);
		}
		buffer << std::setfill('0') << std::setw(width) << coord;
		continuation = true;
	}
	buffer << EOL;

	return buffer.str();
}

void UHSWriter::convertTo96a() {
	// Re-parent under subject node
	auto container = Element::create(ElementType::Subject);
	container->title(document_->title());
	for (auto node = document_->firstChild(); node; node = document_->firstChild()) {
		if (node->nodeType() != NodeType::Element) {
			throw DataError("expected element, found %s node", node->nodeTypeString());
		}
		document_->removeChild(node);
		container->appendChild(node);
	}
	document_->appendChild(container);

	// Prepend minimal header
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	auto subject = Element::create(ElementType::Subject);
	subject->title(codec_.decode88a("-"));
	auto hint = Element::create(ElementType::Hint);
	hint->title(codec_.decode88a("-"));
	hint->appendChild(TextNode::create(codec_.decode88a("-")));
	subject->appendChild(hint);
	header->appendChild(subject);
	document_->insertBefore(header, document_->firstChild().get());

	// Set version to 96a
	document_->version(VersionType::Version96a);
	auto version = Element::create(ElementType::Version);
	version->title("96a");
	if (auto notice = document_->attr("notice")) {
		version->body(*notice);
	}
	document_->appendChild(version);
}

UHSWriter::Serializer::Serializer() {
	map_.try_emplace(ElementType::Blank, &UHSWriter::serializeBlankElement);
	map_.try_emplace(ElementType::Comment, &UHSWriter::serializeCommentElement);
	map_.try_emplace(ElementType::Credit, &UHSWriter::serializeCommentElement);
	map_.try_emplace(ElementType::Gifa, &UHSWriter::serializeDataElement);
	map_.try_emplace(ElementType::Hint, &UHSWriter::serializeHintElement);
	map_.try_emplace(ElementType::Hyperpng, &UHSWriter::serializeHyperpngElement);
	map_.try_emplace(ElementType::Incentive, &UHSWriter::serializeIncentiveElement);
	map_.try_emplace(ElementType::Info, &UHSWriter::serializeInfoElement);
	map_.try_emplace(ElementType::Link, &UHSWriter::serializeLinkElement);
	map_.try_emplace(ElementType::Nesthint, &UHSWriter::serializeHintElement);
	map_.try_emplace(ElementType::Overlay, &UHSWriter::serializeOverlayElement);
	map_.try_emplace(ElementType::Sound, &UHSWriter::serializeDataElement);
	map_.try_emplace(ElementType::Subject, &UHSWriter::serializeSubjectElement);
	map_.try_emplace(ElementType::Text, &UHSWriter::serializeTextElement);
	map_.try_emplace(ElementType::Version, &UHSWriter::serializeCommentElement);
}

int UHSWriter::Serializer::invoke(UHSWriter& writer, Element& element, std::string& out) {
	auto func = map_.at(element.elementType());
	return (writer.*func)(element, out);
}

} // namespace UHS
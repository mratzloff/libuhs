#include <iomanip>

#include "uhs.h"

namespace UHS {

UHSWriter::Serializer UHSWriter::serializer_;

UHSWriter::UHSWriter(std::ostream& out, const Options options) : Writer(out, options) {}

void UHSWriter::write(const std::shared_ptr<Document> document) {
	std::string buffer;
	buffer.reserve(InitialBufferLength);
	document_ = document;
	const auto is88a = document_->isVersion(VersionType::Version88a);

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
			throw WriteError("conversion from 88a to 96a is unimplemented");
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

void UHSWriter::serialize88a(const Document& document, std::string& out) {
	std::queue<const Node*> queue;
	auto previousType = ElementType::Unknown;
	auto line = 1;
	auto numPreviousChildren = 0;
	auto firstHintTextLine = 0;
	auto lastHintTextLine = 0;

	// Traverse tree, breadth first
	queue.push(&document);
	std::string buffer;

	while (!queue.empty()) {
		const auto node = queue.front();
		queue.pop();

		switch (node->nodeType()) {
		case NodeType::Document:
			numPreviousChildren = node->numChildren();
			break;
		case NodeType::Element: {
			const auto element = static_cast<const Element*>(node);
			const auto elementType = element->elementType();
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
				throw WriteError(
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
			const auto& textNode = static_cast<const TextNode*>(node);
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
			queue.push(n);

			while ((n = n->nextSibling())) {
				queue.push(n);
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
			throw WriteError("unexpected break node");
		case NodeType::Document: {
			const auto document = static_cast<const Document*>(node);
			if (!document->isVersion(VersionType::Version88a)) {
				continue;
			}
			this->serialize88a(*document, out);
			out += Token::HeaderSep;
			out += EOL;
			break;
		}
		case NodeType::Element: {
			const auto element = static_cast<Element*>(node);
			this->serializeElement(*element, out);
			break;
		}
		case NodeType::Group:
			throw WriteError("unexpected group node");
		case NodeType::Text: {
			const auto textNode = static_cast<const TextNode*>(node);
			throw WriteError("unexpected text node: %s", textNode->body());
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

	const auto& body = codec_.encodeSpecialChars(element.body());
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
	const auto& body = element.body();

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
    std::map<const int, TextFormat>& formats, std::string& textBuffer, std::string& out) {

	auto length = 0;
	auto depth = parentElement.depth();
	const auto elementType = parentElement.elementType();

	if (node.isElement() && elementType == ElementType::Nesthint) {
		auto& child = static_cast<Element&>(node);
		auto followsTextNode = false;

		if (node.hasPreviousSibling()) {
			auto previousNode = node.previousSibling();
			followsTextNode = previousNode->isText();
		}
		if (!followsTextNode && child.inlined()) {
			textBuffer += Token::InlineBegin;
		}
	}

	if (!node.isText() && !textBuffer.empty()) {
		const auto scEncoded = codec_.encodeSpecialChars(textBuffer);
		auto childLength = 0;
		const auto wrapped = Strings::wrap(scEncoded, "\n", LineLength, childLength);
		out += this->encodeText(wrapped, elementType);
		length += childLength;
		currentLine_ += childLength;
		textBuffer.clear();
	}

	if (node.isElement() && elementType == ElementType::Nesthint) {
		auto& child = static_cast<Element&>(node);
		depth = parentElement.depth();

		out += Token::NestedElementSep;
		out += EOL;
		++length;
		++currentLine_;

		auto childLength = this->serializeElement(child, out);
		length += childLength;
	} else if (node.isBreak()) {
		out += Token::NestedTextSep;
		out += EOL;
		++length;
		++currentLine_;
	} else if (node.isGroup()) {
		for (auto child = node.firstChild(); child; child = child->nextSibling()) {
			length +=
			    this->serializeHintChild(*child, parentElement, formats, textBuffer, out);
		}
	} else if (node.isText()) {
		const auto& textNode = static_cast<const TextNode&>(node);
		textBuffer += this->formatText(textNode, formats[depth]);
		formats[depth] = textNode.format();
	} else {
		throw WriteError("unexpected node type: %s", node.nodeTypeString());
	}

	return length;
}

int UHSWriter::serializeHintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;
	std::map<const int, TextFormat> formats;
	std::string textBuffer;

	currentLine_ += length;
	const auto elementType = element.elementType();

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		length += this->serializeHintChild(*node, element, formats, textBuffer, buffer);
	}

	if (!textBuffer.empty()) {
		const auto scEncoded = codec_.encodeSpecialChars(textBuffer);
		auto childLength = 0;
		const auto wrapped = Strings::wrap(scEncoded, "\n", LineLength, childLength);
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
	const auto& body = element.body();

	auto buffer = this->createDataAddress(body.length());
	this->addData(element);
	++length;
	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}

		auto child = static_cast<Element*>(node);

		std::vector<std::pair<std::string, int>> coords{
		    {"region-top-left-x", 0},
		    {"region-top-left-y", 0},
		    {"region-bottom-right-x", 0},
		    {"region-bottom-right-y", 0},
		};
		for (auto& [attr, coord] : coords) {
			if (auto value = child->attr(attr)) {
				try {
					coord = Strings::toInt(*value);
				} catch (const Error& err) {
					throw WriteError("invalid coordinate: %s", *value);
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

	std::map<const ElementType, bool> excluded = {
	    {ElementType::Incentive, true},
	    {ElementType::Info, true},
	    {ElementType::Version, true},
	};

	std::vector<std::string> instructions;

	for (auto& node : *document_) {
		if (node.nodeType() != NodeType::Element) {
			continue;
		}

		const auto& candidate = static_cast<const Element&>(node);
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

	const auto now = std::time(nullptr);
	const auto tm = std::localtime(&now);

	char formatted[10];
	const auto dateLength = std::strftime(formatted, 10, "%d-%b-%y", tm);
	buffer += "date=" + std::string(formatted, dateLength) + EOL;
	++length;

	const auto timeLength = std::strftime(formatted, 9, "%H:%M:%S", tm);
	buffer += "time=" + std::string(formatted, timeLength) + EOL;
	++length;

	std::map<const std::string, bool> whitelisted = {
	    {"author", true},
	    {"author-note", true},
	    {"copyright", true},
	    {"game-note", true},
	    {"publisher", true},
	};

	for (const auto& [key, value] : document_->attrs()) {
		if (whitelisted[key] && !value.empty()) {
			const auto scEncoded = codec_.encodeSpecialChars(value);
			buffer += Strings::wrap(scEncoded, EOL, LineLength, length, key + "=");
			buffer += EOL;
		}
	}

	if (auto notice = document_->attr("notice")) {
		const auto scEncoded = codec_.encodeSpecialChars(*notice);
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

	int targetID;
	try {
		targetID = Strings::toInt(element.body());
		if (targetID < 0) {
			throw Error();
		}
	} catch (const Error& err) {
		throw WriteError("expected positive integer, found %s", element.body());
	}

	if (auto target = document_->find(targetID)) {
		deferredLinks_.push_back(target);
	} else {
		throw WriteError("target element not found for id %d", targetID);
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeOverlayElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto& body = element.body();

	std::vector<std::pair<std::string, int>> coords = {
	    {"image-x", 0},
	    {"image-y", 0},
	};
	for (auto& [attr, coord] : coords) {
		if (auto value = element.attr(attr)) {
			try {
				coord = Strings::toInt(*value);
			} catch (const Error& err) {
				throw WriteError("invalid coordinate: %s", *value);
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
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}
		const auto child = static_cast<Element*>(node);
		length += this->serializeElement(*child, buffer);
	}
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeTextElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto elementType = element.elementType();
	std::string textBuffer;
	std::string buffer;
	auto previousFormat = TextFormat::None;
	const auto message = "unexpected node type: %s";

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->isGroup()) {
			for (auto child = node->firstChild(); child; child = child->nextSibling()) {
				if (child->isText()) {
					const auto textNode = static_cast<const TextNode*>(child);
					textBuffer += this->formatText(*textNode, previousFormat);
					previousFormat = textNode->format();
				} else {
					throw WriteError(message, child->nodeTypeString());
				}
			}
		} else {
			throw WriteError(message, node->nodeTypeString());
		}
	}

	const auto scEncoded = codec_.encodeSpecialChars(textBuffer);
	auto body = this->encodeText(scEncoded, elementType);

	const auto format = element.attr("format");
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

	const auto title = codec_.encodeSpecialChars(element.title());

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
	// TODO: I don't think this is actually true
	const auto end = deferredLinks_.crend();
	for (auto it = deferredLinks_.crbegin(); it != end; ++it) {
		const auto target = *it;
		assert(target);

		if (!target->isElement()) {
			// TODO: Warn
			continue;
		}

		auto targetElement = static_cast<Element*>(target);
		const auto pos = out.rfind(LinkMarker);
		const auto targetLine = std::to_string(targetElement->line());
		const auto length = targetLine.length();
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
		const auto& pair = data_.front();
		const auto elementType = pair.first;
		const auto& data = pair.second;

		std::smatch matches;
		auto isOverlay = (elementType == ElementType::Overlay);
		const auto& regex = (isOverlay) ? Regex::OverlayAddress : Regex::DataAddress;
		if (!std::regex_search(out.cbegin() + offset, searchEnd, matches, regex)) {
			throw WriteError(
			    "could not find address offset for %s element data (%d bytes)",
			    Element::typeString(elementType),
			    data.length());
		}

		const auto& matchNumber = (isOverlay) ? 1 : 2;
		const auto position = static_cast<std::size_t>(matches.position(matchNumber));
		const auto dataAddress = std::to_string(dataOffset);
		const auto dataAddressLength = dataAddress.length();
		const auto dataAddressOffset =
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
	const auto position = out.find(InfoLengthMarker);
	if (position == std::string::npos) {
		return; // No info node present
	}
	const auto fileLength = out.length() + CRC::ByteLength;
	const auto fileLengthString = std::to_string(fileLength);
	const auto fileLengthStringLength = fileLengthString.length();
	const auto infoLengthOffset =
	    position + strlen(InfoLengthMarker) - fileLengthStringLength;

	// Replace length attribute
	out.replace(infoLengthOffset, fileLengthStringLength, fileLengthString);
}

void UHSWriter::serializeCRC(std::string& out) {
	// Calculate
	crc_.calculate(out.data(), out.length());

	// Write checksum
	std::vector<char> checksum;
	crc_.result(checksum);
	out.push_back(checksum[0]);
	out.push_back(checksum[1]);
}

std::string UHSWriter::formatText(
    const TextNode& textNode, const TextFormat previousFormat) {

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

	if (textNode.hasNextSibling()) {
		if (auto node = textNode.nextSibling(); node->isElement()) {
			const auto nextElement = static_cast<Element*>(node);
			if (nextElement->inlined()) {
				if (body.length() > 0 && !Strings::endsWithAttachedPunctuation(body)) {
					body += ' ';
				}
				body += Token::InlineBegin;
			}
		}
	}

	if (textNode.hasPreviousSibling()) {
		if (auto node = textNode.previousSibling(); node->isElement()) {
			const auto previousElement = static_cast<Element*>(node);
			if (previousElement->inlined()) {
				std::string temp;
				temp += Token::InlineEnd;

				if (body.length() > 0 && !Strings::beginsWithAttachedPunctuation(body)) {
					temp += ' ';
				}
				temp += body;
				body = temp;
			}
		}
	}

	return body;
}

std::string UHSWriter::encodeText(const std::string text, const ElementType parentType) {
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

void UHSWriter::addData(const Element& element) {
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
	std::vector<const int> coords = {x1, y1, x2, y2};
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
			throw WriteError("expected element, found %s node", node->nodeTypeString());
		}
		container->appendChild(document_->removeChild(node));
	}
	document_->appendChild(container);

	// Prepend minimal header
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	auto subject = Element::create(ElementType::Subject);
	subject->title(codec_.decode88a("-"));
	auto hint = Element::create(ElementType::Hint);
	hint->title(codec_.decode88a("-"));
	hint->appendChild(codec_.decode88a("-"));
	subject->appendChild(hint);
	header->appendChild(subject);
	document_->insertBefore(header, document_->firstChild());

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
	map_.emplace(ElementType::Blank, &UHSWriter::serializeBlankElement);
	map_.emplace(ElementType::Comment, &UHSWriter::serializeCommentElement);
	map_.emplace(ElementType::Credit, &UHSWriter::serializeCommentElement);
	map_.emplace(ElementType::Gifa, &UHSWriter::serializeDataElement);
	map_.emplace(ElementType::Hint, &UHSWriter::serializeHintElement);
	map_.emplace(ElementType::Hyperpng, &UHSWriter::serializeHyperpngElement);
	map_.emplace(ElementType::Incentive, &UHSWriter::serializeIncentiveElement);
	map_.emplace(ElementType::Info, &UHSWriter::serializeInfoElement);
	map_.emplace(ElementType::Link, &UHSWriter::serializeLinkElement);
	map_.emplace(ElementType::Nesthint, &UHSWriter::serializeHintElement);
	map_.emplace(ElementType::Overlay, &UHSWriter::serializeOverlayElement);
	map_.emplace(ElementType::Sound, &UHSWriter::serializeDataElement);
	map_.emplace(ElementType::Subject, &UHSWriter::serializeSubjectElement);
	map_.emplace(ElementType::Text, &UHSWriter::serializeTextElement);
	map_.emplace(ElementType::Version, &UHSWriter::serializeCommentElement);
}

int UHSWriter::Serializer::invoke(UHSWriter& writer, Element& element, std::string& out) {
	auto func = map_.at(element.elementType());
	return (writer.*func)(element, out);
}

} // namespace UHS
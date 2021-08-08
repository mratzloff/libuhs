#include "uhs.h"
#include <cmath>
#include <fstream>
#include <iomanip>

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions options)
    : out_{out}, options_{options} {}

void Writer::reset() {}

TreeWriter::TreeWriter(std::ostream& out, const WriterOptions options)
    : Writer(out, options) {}

void TreeWriter::write(const Document& document) {
	for (const auto& node : document) {
		switch (node.nodeType()) {
		case NodeType::Document: {
			const auto& d = static_cast<const Document&>(node);
			this->draw(d);
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			this->draw(element);
			break;
		}
		default:
			break; // Ignore
		}
	}
}

void TreeWriter::draw(const Document& document) {
	this->drawScaffold(document);
	out_ << "[" << document.nodeTypeString() << "] \"" << document.title() << "\""
	     << std::endl;
}

void TreeWriter::draw(const Element& element) {
	this->drawScaffold(element);
	out_ << "[" << element.elementTypeString() << "] \"" << element.title() << "\""
	     << std::endl;
}

void TreeWriter::drawScaffold(const Node& node) {
	const auto depth = node.depth();

	std::string line;
	auto parent = node.parent();
	if (!parent) {
		return;
	}

	for (auto i = depth; i > 0 && parent; --i) {
		auto grandparent = parent->parent();
		if (grandparent) {
			if (parent == grandparent->lastChild()) {
				line = "    " + line;
			} else {
				line = "│   " + line;
			}
		}
		parent = grandparent;
	}
	out_ << line;

	if (&node == node.parent()->lastChild()) {
		out_ << "└───";
	} else {
		out_ << "├───";
	}
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions options)
    : Writer(out, options) {}

void JSONWriter::write(const Document& document) {
	Json::Value root{Json::objectValue};

	this->serialize(document, root);
	out_ << root << std::endl;
}

Json::Value JSONWriter::serialize(const Document& document, Json::Value& root) const {
	Json::Value* parents[MaxDepth];
	Json::Value* parent = &root;

	auto depth = 0;
	parents[depth] = &root;

	for (const auto& node : document) {
		// Manage JSON arrays as we descend and ascend
		auto nodeDepth = node.depth();
		if (nodeDepth > depth) { // Down
			parents[depth] = parent;
			if ((*parent)["children"].empty()) {
				Json::Value a = {Json::arrayValue};
				(*parent)["children"] = a;
			} else {
				parent = &((*parent)["children"][(*parent)["children"].size() - 1]);
			}
		} else if (nodeDepth < depth) { // Up
			if (nodeDepth >= 0) {
				parent = parents[nodeDepth];
			}
		}

		// Serialize node
		Json::Value object = {Json::objectValue};

		switch (node.nodeType()) {
		case NodeType::Break:
			(*parent)["children"].append("-");
			break;
		case NodeType::Document: {
			if (nodeDepth == 0) {
				object = root;
			}
			const auto& d = static_cast<const Document&>(node);
			this->serializeDocument(d, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			this->serializeElement(element, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			this->serializeTextNode(textNode, object);
			(*parent)["children"].append(object);
			break;
		}
		default:
			throw WriteError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}

	return root;
}

void JSONWriter::serializeDocument(const Document& document, Json::Value& object) const {
	object["title"] = document.title();
	object["version"] = document.versionString();

	if (document.version() > VersionType::Version88a) {
		object["registered"] = options_.registered;
		object["validChecksum"] = document.validChecksum();
	}
	if (!document.visible(options_.registered)) {
		object["visible"] = false;
	}
	object["type"] = document.nodeTypeString();

	// Build attributes map
	const auto& attrs = document.attrs();
	if (!attrs.empty()) {
		Json::Value map = {Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
}

void JSONWriter::serializeElement(const Element& element, Json::Value& object) const {
	std::string fname;
	std::ofstream fout;

	object["title"] = element.title();
	if (const auto id = element.id(); id > 0) {
		object["id"] = id;
	}

	if (element.isMedia() && !options_.mediaDir.empty()) {
		// TODO: Do something about how lazy this is
		fname = options_.mediaDir + "/" + std::to_string(element.line()) + "."
		        + element.mediaExt();
		fout.open(fname, std::ios::out | std::ios::binary);
		fout << element.body();
		fout.close();
		object["body"] = fname;
	} else {
		const auto& body = element.body();
		if (!body.empty()) {
			object["body"] = body;
		}
	}

	if (!element.visible(options_.registered)) {
		object["visible"] = false;
	}
	object["type"] = Element::typeString(element.elementType());

	// Build attributes map
	const auto& attrs = element.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
}

void JSONWriter::serializeTextNode(const TextNode& textNode, Json::Value& object) const {
	object["body"] = textNode.body();

	auto attributes = Json::Value(Json::objectValue);
	attributes["word-wrap"] = textNode.hasFormat(TextFormat::WordWrap);
	attributes["proportional"] = textNode.hasFormat(TextFormat::Proportional);
	attributes["hyperlink"] = textNode.hasFormat(TextFormat::Hyperlink);
	object["attributes"] = attributes;
}

void JSONWriter::serializeMap(
    const Traits::Attributes::Type& attrs, Json::Value& object) const {

	for (const auto& [k, v] : attrs) {
		if (v == "true") {
			object[k] = true;
		} else if (v == "false") {
			object[k] = false;
		} else if (Strings::isInt(v)) {
			try {
				object[k] = Strings::toInt(v);
			} catch (const Error& err) {
				object[k] = v;
			}
		} else {
			object[k] = v; // String value
		}
	}
}

//-------------------------------- UHSWriter --------------------------------//

UHSWriter::Serializer UHSWriter::serializer_;

UHSWriter::UHSWriter(std::ostream& out, const WriterOptions options)
    : Writer(out, options) {}

void UHSWriter::write(const Document& document) {
	std::string buffer;
	buffer.reserve(InitialBufferLength);

	if (options_.force88aMode) {
		this->serialize88a(document, buffer);
	} else {
		document_ = document.clone();
		this->serialize96a(buffer);
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
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(*node);
			const auto elementType = element.elementType();
			std::string title;

			switch (elementType) {
			case ElementType::Subject:
				line += numPreviousChildren * 2;
				title = element.title();
				break;
			case ElementType::Hint:
				if (previousType == ElementType::Subject) {
					line += numPreviousChildren * 2;
				} else {
					line += numPreviousChildren;
				}
				title = Strings::rtrim(element.title(), '?');
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
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(*node);
			auto body = textNode.body();

			if (!options_.debug) {
				body = codec_.encode88a(body);
			}
			buffer += body;
			buffer += EOL;

			line += numPreviousChildren - 1; // Simplifies to line -= 1 after first pass

			if (lastHintTextLine == 0) {
				lastHintTextLine = line;
			}
			firstHintTextLine = line; // Work backwards from last
			break;
		}
		default:
			break; // Ignore
		}

		numPreviousChildren = node->numChildren();

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
	if (document_->version() == VersionType::Version88a) {
		this->convertTo91a();
	}

	key_ = codec_.createKey(document_->title());

	for (auto node = document_->firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Break:
			throw WriteError("unexpected break node");
		case NodeType::Document: {
			const auto& document = static_cast<const Document&>(*node);
			if (document.version() != VersionType::Version88a) {
				continue;
			}
			this->serialize88a(document, out);
			out += Token::HeaderSep;
			out += EOL;
			break;
		}
		case NodeType::Element: {
			auto& element = static_cast<Element&>(*node);
			this->serializeElement(element, out);
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(*node);
			throw WriteError("unexpected text node: %s", textNode.body());
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

	if (const auto& body = element.body(); !body.empty()) {
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

int UHSWriter::serializeHintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;
	std::map<const int, TextFormat> formats;
	auto depth = element.depth();
	// formats[depth] = TextFormat::Proportional | TextFormat::WordWrap;

	currentLine_ += length;
	const auto elementType = element.elementType();

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		auto childLength = 0;

		if (node->isElement() && elementType == ElementType::Nesthint) {
			depth = element.depth();
			buffer += Token::NestedElementSep;
			buffer += EOL;
			++length;
			++currentLine_;
			auto& child = static_cast<Element&>(*node);
			childLength += this->serializeElement(child, buffer);
		} else if (node->isBreak()) {
			buffer += Token::NestedTextSep;
			buffer += EOL;
			++length;
			++currentLine_;
		} else if (node->isText()) {
			const auto& textNode = static_cast<const TextNode&>(*node);

			// tfm::format(std::cerr, "line = %d, depth = %d\n", element.line(), depth);
			// ::UHS::printFormat(formats[depth]);

			buffer += this->serializeTextNode(
			    elementType, textNode, formats[depth], childLength);
			formats[depth] = textNode.format();
			currentLine_ += childLength;
		} else {
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}

		length += childLength;
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

		auto& child = static_cast<Element&>(*node);

		std::vector<std::pair<std::string, int>> coords{
		    {"region-top-left-x", 0},
		    {"region-top-left-y", 0},
		    {"region-bottom-right-x", 0},
		    {"region-bottom-right-y", 0},
		};
		for (auto& [attr, coord] : coords) {
			if (auto value = child.attr(attr)) {
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

		length += this->serializeElement(child, buffer);
	}

	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeIncentiveElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	std::map<const ElementType, bool> blacklisted = {
	    {ElementType::Incentive, true},
	    {ElementType::Info, true},
	    {ElementType::Version, true},
	};

	std::vector<std::string> instructions;

	for (const auto& node : *document_) {
		if (node.nodeType() != NodeType::Element) {
			continue;
		}
		const auto& candidate = static_cast<const Element&>(node);
		if (blacklisted[candidate.elementType()]) {
			continue;
		}

		std::string flag;
		switch (candidate.visibility()) {
		case VisibilityType::Registered:
			flag = (options_.registered) ? Token::Registered : Token::Unregistered;
			break;
		case VisibilityType::Unregistered:
			flag = (options_.registered) ? Token::Unregistered : Token::Registered;
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
	auto wrapped = Strings::wrap(decoded, EOL, LineLength, length);
	auto lines = Strings::split(wrapped, EOL);
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
			buffer += Strings::wrap(value, EOL, LineLength, length, key + "=");
			buffer += EOL;
		}
	}

	if (auto notice = document_->attr("notice")) {
		buffer += Strings::wrap(*notice, EOL, LineLength, length, Token::NoticePrefix);
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
		auto& child = static_cast<Element&>(*node);
		length += this->serializeElement(child, buffer);
	}
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeTextElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto elementType = element.elementType();
	const auto& body = element.body();
	std::string buffer;

	const auto textFormat = ((*element.attr("typeface") == "monospace") ? "1 " : "0 ");
	buffer += this->createDataAddress(body.length(), textFormat);
	++length;

	auto lines = Strings::split(body, "\n");
	for (auto& line : lines) {
		if (line == "") {
			line = Token::ParagraphSep;
		}
		if (!options_.debug) {
			line = codec_.encode96a(line, key_, true);
		}
	}
	data_.emplace(std::make_pair(elementType, Strings::join(lines, EOL) + EOL));

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

	if (const auto& title = element.title(); title.empty()) {
		out += '-';
	} else {
		out += title;
	}
	out += EOL;
}

void UHSWriter::updateLinkTargets(std::string& out) const {
	// Because we're shifting characters, we move backwards through the file in order to
	// reduce the total number of bytes copied within the output buffer.
	// TODO: I don't think this is actually true
	const auto end = deferredLinks_.crend();
	for (auto it = deferredLinks_.crbegin(); it != end; ++it) {
		const auto target = *it;
		assert(target);
		const auto pos = out.rfind(LinkMarker);
		const auto targetLine = std::to_string(target->line());
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

std::string UHSWriter::serializeTextNode(const ElementType parentType,
    const TextNode& textNode, const TextFormat previousFormat, int& length) {

	std::string buffer;
	auto body = textNode.body();

	if (body == "") {
		body = Token::ParagraphSep;
	}
	if (textNode.hasFormat(TextFormat::Hyperlink)) {
		// body = Token::HyperlinkStart + body + Token::HyperlinkEnd;
	}
	if (textNode.hasFormat(TextFormat::Proportional)
	    && !hasFormat(previousFormat, TextFormat::Proportional)) {

		// body = Token::ProportionalStart + body;
	}
	if (!textNode.hasFormat(TextFormat::Proportional)
	    && hasFormat(previousFormat, TextFormat::Proportional)) {

		// body = Token::ProportionalEnd + body;
	}
	if (textNode.hasFormat(TextFormat::WordWrap)
	    && !hasFormat(previousFormat, TextFormat::WordWrap)) {

		// body = Token::WordWrapStart + body;
	}
	if (!textNode.hasFormat(TextFormat::WordWrap)
	    && hasFormat(previousFormat, TextFormat::WordWrap)) {

		// body = Token::WordWrapEnd + body;
	}

	auto wrapped = Strings::wrap(body, EOL, LineLength, length);
	auto lines = Strings::split(wrapped, EOL);

	for (auto& line : lines) {
		if (line == "") {
			line = Token::ParagraphSep;
		}
		if (options_.debug) {
			continue; // Don't encode text
		}
		if (parentType == ElementType::Nesthint) {
			line = codec_.encode96a(line, key_, false);
		} else if (parentType == ElementType::Hint) {
			line = codec_.encode88a(line);
		}
	}
	buffer += Strings::join(lines, EOL);
	buffer += EOL;

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
	out += textFormat;
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

void UHSWriter::convertTo91a() {
	// Re-parent under subject node
	auto container = Element::create(ElementType::Subject);
	container->title(document_->title());
	for (auto node = document_->firstChild(); node; node = document_->firstChild()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("expected element, found %s node", node->nodeTypeString());
		}
		container->appendChild(document_->removeChild(node));
	}
	document_->appendChild(std::move(container));

	// Prepend minimal 88a header
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	auto subject = Element::create(ElementType::Subject);
	subject->title(codec_.decode88a("-"));
	auto hint = Element::create(ElementType::Hint);
	hint->title(codec_.decode88a("-"));
	hint->appendChild(codec_.decode88a("-"));
	subject->appendChild(std::move(hint));
	header->appendChild(std::move(subject));
	document_->insertBefore(std::move(header), document_->firstChild());

	// Set version to 91a
	document_->version(VersionType::Version91a);
	auto version = Element::create(ElementType::Version);
	version->title("91a");
	if (auto notice = document_->attr("notice")) {
		version->body(*notice);
	}
	document_->appendChild(std::move(version));
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
	return (writer.*map_[element.elementType()])(element, out);
}

} // namespace UHS

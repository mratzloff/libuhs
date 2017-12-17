#include "uhs.h"
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
				Json::Value a{Json::arrayValue};
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
		Json::Value object{Json::objectValue};

		switch (node.nodeType()) {
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			(*parent)["children"].append(textNode.body());
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			this->serializeElement(element, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Document: {
			const auto& d = static_cast<const Document&>(node);
			if (nodeDepth > 0) {
				this->serializeDocument(d, object);
				(*parent)["children"].append(object);
			} else {
				this->serializeDocument(d, root);
			}
			break;
		}
		}

		depth = nodeDepth;
	}

	return root;
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
	object["type"] = Node::typeString(document.nodeType());

	// Build attributes map
	const auto& attrs = document.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
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
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(*node);
			buffer += codec_.encode88a(textNode.body()) + EOL;

			line += numPreviousChildren - 1; // Simplifies to line -= 1 after first pass

			if (lastHintTextLine == 0) {
				lastHintTextLine = line;
			}
			firstHintTextLine = line; // Work backwards from last
			break;
		}

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
				    "unexpected element: %s", Element::typeString(elementType).data());
			}

			buffer += codec_.encode88a(title) + EOL;
			buffer += std::to_string(line) + EOL;

			previousType = elementType;
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
			throw WriteError("unexpected text node: %s", textNode.body().data());
		}
		}
	}

	this->updateLinkTargets(out);

	out += Token::DataSep;

	this->serializeData(out);
	this->serializeCRC(out);
}

int UHSWriter::serializeElement(Element& element, std::string& out) {
	auto length = 0;
	std::string buffer;

	element.line(currentLine_); // For links and incentives

	switch (element.elementType()) {
	case ElementType::Unknown: /* No further processing required */
		return length;
	case ElementType::Blank: /* No further processing required */
		length = this->serializeBlankElement(element, buffer);
		break;
	case ElementType::Comment:
		length = this->serializeCommentElement(element, buffer);
		break;
	case ElementType::Credit:
		length = this->serializeCommentElement(element, buffer);
		break;
	case ElementType::Gifa:
		length = this->serializeDataElement(element, buffer);
		break;
	case ElementType::Hint:
		length = this->serializeHintElement(element, buffer);
		break;
	case ElementType::Hyperpng: /* TODO 2 */
		length = this->serializeHyperpngElement(element, buffer);
		break;
	case ElementType::Incentive: /* TODO 3 */
		length = this->serializeIncentiveElement(element, buffer);
		break;
	case ElementType::Info:
		length = this->serializeInfoElement(buffer);
		break;
	case ElementType::Link:
		length = this->serializeLinkElement(element, buffer);
		break;
	case ElementType::Nesthint: /* TODO 1 */
		length = this->serializeNesthintElement(element, buffer);
		break;
	case ElementType::Overlay: /* TODO 2 */
		length = this->serializeOverlayElement(element, buffer);
		break;
	case ElementType::Sound:
		length = this->serializeDataElement(element, buffer);
		break;
	case ElementType::Subject:
		length = this->serializeSubjectElement(element, buffer);
		break;
	case ElementType::Text:
		length = this->serializeTextElement(element, buffer);
		break;
	case ElementType::Version:
		length = this->serializeCommentElement(element, buffer);
		break;
	}

	out += std::to_string(length);
	out += ' ';
	out += element.elementTypeString();
	out += EOL;

	const auto& title = element.title();
	if (title.length() == 0) {
		out += '-';
	} else {
		out += title;
	}
	out += EOL;
	out += buffer;

	return length;
}

int UHSWriter::serializeBlankElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;
	return length;
}

int UHSWriter::serializeCommentElement(Element& element, std::string& out) {
	auto length = InitialElementLength;

	if (const auto& body = element.body(); !body.empty()) {
		out += Strings::wrap(body, EOL, LineLength, length) + EOL;
	}
	currentLine_ += length;

	return length;
}

// GIFs don't appear to be displayed by the official reader any longer
int UHSWriter::serializeDataElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto elementType = element.elementType();
	const auto& body = element.body();

	out += this->createDataAddress(body.length());
	++length;
	data_.emplace(std::make_pair(elementType, body));

	currentLine_ += length;

	return length;
}

int UHSWriter::serializeHintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	auto continuation = false;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (continuation) {
			out += Token::NestedTextSep;
			out += EOL;
			++length;
		}
		if (node->nodeType() != NodeType::Text) {
			// Hint should only contain text nodes at this point
			throw WriteError("unexpected node type: %s", node->nodeTypeString().data());
		}
		const auto& textNode = static_cast<const TextNode&>(*node);
		const auto& body = textNode.body();
		out += codec_.encode88a(Strings::wrap(body, EOL, LineLength, length)) + EOL;

		continuation = true;
	}

	currentLine_ += length;

	return length;
}

int UHSWriter::serializeHyperpngElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;
	return length;
}

int UHSWriter::serializeIncentiveElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;
	return length;
}

int UHSWriter::serializeInfoElement(std::string& out) {
	auto length = InitialElementLength;
	out += InfoLengthMarker;
	out += EOL;
	++length;

	const auto now = std::time(nullptr);
	const auto tm = std::localtime(&now);

	char buffer[10];
	const auto dateLength = std::strftime(buffer, 10, "%d-%b-%y", tm);
	out += "date=" + std::string(buffer, dateLength) + EOL;
	++length;

	const auto timeLength = std::strftime(buffer, 9, "%H:%M:%S", tm);
	out += "time=" + std::string(buffer, timeLength) + EOL;
	++length;

	std::map<const std::string, bool> whitelisted = {
	    {"author", true},
	    {"author-note", true},
	    {"copyright", true},
	    {"game-note", true},
	    {"publisher", true},
	};

	for (const auto& [key, value] : document_->attrs()) {
		if (whitelisted[key]) {
			out += Strings::wrap(value, EOL, LineLength, length, key + "=") + EOL;
		}
	}

	if (auto notice = document_->attr("notice")) {
		out += Strings::wrap(*notice, EOL, LineLength, length, Token::NoticePrefix) + EOL;
	}

	currentLine_ += length;

	return length;
}

int UHSWriter::serializeLinkElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	out += LinkMarker;
	out += EOL;

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
		++length;
	} else {
		throw WriteError("target element not found for id %d", targetID);
	}

	currentLine_ += length;

	return length;
}

int UHSWriter::serializeNesthintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;
	return length;
}

int UHSWriter::serializeOverlayElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;
	return length;
}

int UHSWriter::serializeSubjectElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("unexpected node type: %s", node->nodeTypeString().data());
		}
		auto& child = static_cast<Element&>(*node);
		length += this->serializeElement(child, out);
	}

	return length;
}

int UHSWriter::serializeTextElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto elementType = element.elementType();
	const auto& body = element.body();

	const auto textFormat = ((*element.attr("typeface") == "monospace") ? "1 " : "0 ");
	out += this->createDataAddress(body.length(), textFormat);
	++length;

	auto lines = Strings::split(body, "\n");
	for (auto& line : lines) {
		if (line == "") {
			line = Token::ParagraphSep;
		}
		line = codec_.encode96a(line, key_, true);
	}
	data_.emplace(std::make_pair(elementType, Strings::join(lines, EOL) + EOL));

	currentLine_ += length;

	return length;
}

void UHSWriter::updateLinkTargets(std::string& out) {
	// Because we're shifting characters, we move backwards through the file in order to
	// reduce the total number of bytes copied within the output buffer.
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

std::string UHSWriter::createDataAddress(std::size_t bodyLength, std::string textFormat) {
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

void UHSWriter::convertTo91a() {
	// Re-parent under subject node
	auto container = Element::create(ElementType::Subject);
	container->title(document_->title());
	for (auto node = document_->firstChild(); node; node = document_->firstChild()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError(
			    "expected element, found %s node", node->nodeTypeString().data());
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

	// Set version to 96a
	document_->version(VersionType::Version91a);
	auto version = Element::create(ElementType::Version);
	version->title("91a");
	if (auto notice = document_->attr("notice")) {
		version->body(*notice);
	}
	document_->appendChild(std::move(version));
}

} // namespace UHS

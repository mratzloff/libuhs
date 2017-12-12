#include "uhs.h"
#include <fstream>
#include <iomanip>
#include <iostream>

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions opt) : _out{out}, _opt{opt} {}

void Writer::reset() {}

TreeWriter::TreeWriter(std::ostream& out, const WriterOptions opt) : Writer(out, opt) {}

void TreeWriter::write(const Document& d) {
	for (const auto& n : d) {
		switch (n.nodeType()) {
		case NodeType::Document: {
			const auto& doc = static_cast<const Document&>(n);
			this->draw(doc);
			break;
		}
		case NodeType::Element: {
			const auto& e = static_cast<const Element&>(n);
			this->draw(e);
			break;
		}
		default:
			break; // Ignore
		}
	}
}

void TreeWriter::draw(const Document& d) {
	this->drawScaffold(d);
	_out << "[" << d.nodeTypeString() << "] \"" << d.title() << "\"" << std::endl;
}

void TreeWriter::draw(const Element& e) {
	this->drawScaffold(e);
	_out << "[" << e.elementTypeString() << "] \"" << e.title() << "\"" << std::endl;
}

void TreeWriter::drawScaffold(const Node& n) {
	const int depth = n.depth();

	std::string s;
	auto p = n.parent();
	if (!p) {
		return;
	}

	for (int i = depth; i > 0 && p; --i) {
		auto gp = p->parent();
		if (gp) {
			if (p == gp->lastChild()) {
				s = "    " + s;
			} else {
				s = "│   " + s;
			}
		}
		p = gp;
	}
	_out << s;

	if (&n == n.parent()->lastChild()) {
		_out << "└───";
	} else {
		_out << "├───";
	}
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions opt) : Writer(out, opt) {}

void JSONWriter::write(const Document& d) {
	Json::Value root{Json::objectValue};

	this->serialize(d, root);
	_out << root << std::endl;
}

Json::Value JSONWriter::serialize(const Document& d, Json::Value& root) const {
	Json::Value* parents[MaxDepth];
	Json::Value* parent = &root;

	int depth = 0;
	parents[depth] = &root;

	for (const auto& n : d) {
		// Manage JSON arrays as we descend and ascend
		int nodeDepth = n.depth();
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
		Json::Value obj{Json::objectValue};

		switch (n.nodeType()) {
		case NodeType::Text: {
			const auto& tn = static_cast<const TextNode&>(n);
			(*parent)["children"].append(tn.body());
			break;
		}
		case NodeType::Element: {
			const auto& e = static_cast<const Element&>(n);
			this->serializeElement(e, obj);
			(*parent)["children"].append(obj);
			break;
		}
		case NodeType::Document: {
			const auto& doc = static_cast<const Document&>(n);
			if (nodeDepth > 0) {
				this->serializeDocument(doc, obj);
				(*parent)["children"].append(obj);
			} else {
				this->serializeDocument(doc, root);
			}
			break;
		}
		}

		depth = nodeDepth;
	}

	return root;
}

void JSONWriter::serializeElement(const Element& e, Json::Value& obj) const {
	std::string fname;
	std::ofstream fout;

	obj["title"] = e.title();
	if (const auto id = e.id(); id > 0) {
		obj["id"] = id;
	}

	if (e.isMedia() && !_opt.mediaDir.empty()) {
		// TODO: Do something about how lazy this is
		fname = _opt.mediaDir + "/" + std::to_string(e.line()) + "." + e.mediaExt();
		fout.open(fname, std::ios::out | std::ios::binary);
		fout << e.body();
		fout.close();
		obj["body"] = fname;
	} else {
		const auto& body = e.body();
		if (!body.empty()) {
			obj["body"] = body;
		}
	}

	if (!e.visible(_opt.registered)) {
		obj["visible"] = false;
	}
	obj["type"] = Element::typeString(e.elementType());

	// Build attributes map
	const auto& attrs = e.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		obj["attributes"] = map;
	}
}

void JSONWriter::serializeDocument(const Document& d, Json::Value& obj) const {
	obj["title"] = d.title();
	obj["version"] = d.versionString();

	if (d.version() > VersionType::Version88a) {
		obj["registered"] = _opt.registered;
		obj["validChecksum"] = d.validChecksum();
	}
	if (!d.visible(_opt.registered)) {
		obj["visible"] = false;
	}
	obj["type"] = Node::typeString(d.nodeType());

	// Build attributes map
	const auto& attrs = d.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		obj["attributes"] = map;
	}
}

void JSONWriter::serializeMap(
    const Traits::Attributes::Type& attrs, Json::Value& obj) const {
	for (const auto& [k, v] : attrs) {
		if (v == "true") {
			obj[k] = true;
		} else if (v == "false") {
			obj[k] = false;
		} else if (Strings::isInt(v)) {
			try {
				obj[k] = Strings::toInt(v);
			} catch (const Error& err) {
				obj[k] = v;
			}
		} else {
			obj[k] = v; // String value
		}
	}
}

//-------------------------------- UHSWriter --------------------------------//

UHSWriter::UHSWriter(std::ostream& out, const WriterOptions opt) : Writer(out, opt) {}

void UHSWriter::write(const Document& d) {
	std::string buf;
	buf.reserve(InitialBufferLen);

	if (_opt.force88aMode) {
		this->serialize88a(d, buf);
	} else {
		_document = d.clone();
		this->serialize96a(buf);
	}

	_out << buf;
	std::flush(_out);
}

void UHSWriter::reset() {
	Writer::reset();

	_crc.reset();
	_document.reset();
	_key.clear();

	while (!_data.empty()) {
		_data.pop();
	}
}

void UHSWriter::serialize88a(const Document& d, std::string& out) {
	std::queue<const Node*> queue;
	auto prevType = ElementType::Unknown;
	int line = 1;
	int numPrevChildren = 0;
	int firstHintTextLine = 0;
	int lastHintTextLine = 0;

	// Traverse tree, breadth first
	queue.push(&d);
	std::string buf;

	while (!queue.empty()) {
		const auto n = queue.front();
		queue.pop();

		switch (n->nodeType()) {
		case NodeType::Text: {
			const auto& tn = static_cast<const TextNode&>(*n);
			buf += _codec.encode88a(tn.body()) + EOL;

			line += numPrevChildren - 1; // Simplifies to line -= 1 after first pass

			if (lastHintTextLine == 0) {
				lastHintTextLine = line;
			}
			firstHintTextLine = line; // Work backwards from last
			break;
		}

		case NodeType::Element: {
			const auto& e = static_cast<const Element&>(*n);
			const auto elementType = e.elementType();
			std::string title;

			switch (elementType) {
			case ElementType::Subject:
				line += numPrevChildren * 2;
				title = e.title();
				break;
			case ElementType::Hint:
				if (prevType == ElementType::Subject) {
					line += numPrevChildren * 2;
				} else {
					line += numPrevChildren;
				}
				title = Strings::rtrim(e.title(), '?');
				break;
			default:
				throw WriteError(
				    "unexpected element: %s", Element::typeString(elementType).data());
			}

			buf += _codec.encode88a(title) + EOL;
			buf += std::to_string(line) + EOL;

			prevType = elementType;
			break;
		}

		default:
			break; // Ignore
		}

		numPrevChildren = n->numChildren();

		if (auto child = n->firstChild()) {
			if (!Node::isElementOfType(*child, ElementType::Credit)) {
				queue.push(child);
			}
			while ((child = child->nextSibling())) {
				if (!Node::isElementOfType(*child, ElementType::Credit)) {
					queue.push(child);
				}
			}
		}
	}

	out += Token::Signature;
	out += EOL;
	out += d.title() + EOL;
	out += std::to_string(firstHintTextLine) + EOL;
	out += std::to_string(lastHintTextLine) + EOL;
	out += buf;

	if (auto credit = d.attr("notice")) {
		out += Token::CreditSep;
		out += EOL;
		out += Strings::wrap(*credit, EOL, LineLen) + EOL;
	}
}

void UHSWriter::serialize96a(std::string& out) {
	if (_document->version() == VersionType::Version88a) {
		this->convertTo91a();
	}

	_key = _codec.createKey(_document->title());

	for (Node* n = _document->firstChild(); n; n = n->nextSibling()) {
		switch (n->nodeType()) {
		case NodeType::Document: {
			const auto& child = static_cast<const Document&>(*n);
			if (child.version() != VersionType::Version88a) {
				continue;
			}
			this->serialize88a(child, out);
			out += Token::HeaderSep;
			out += EOL;
			break;
		}
		case NodeType::Element: {
			int len = 0;
			const auto& child = static_cast<const Element&>(*n);
			this->serializeElement(child, out, len);
			break;
		}
		case NodeType::Text: {
			const auto& tn = static_cast<const TextNode&>(*n);
			throw WriteError("unexpected text node: %s", tn.body().data());
		}

		default:
			// Not handled
			break;
		}
	}

	out += Token::DataSep;

	this->serializeData(out);
	this->serializeCRC(out);
}

void UHSWriter::serializeElement(const Element& e, std::string& out, int& len) {
	std::string buf;
	int childLen = 0;

	switch (e.elementType()) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		this->serializeCommentElement(e, buf, childLen);
		break;
	case ElementType::Credit:
		this->serializeCommentElement(e, buf, childLen);
		break;
	case ElementType::Gifa:
		this->serializeDataElement(e, buf, childLen);
		break;
	case ElementType::Hint:
		this->serializeHintElement(e, buf, childLen);
		break;
	case ElementType::Hyperpng: /* TODO 2 */
		break;
	case ElementType::Incentive: /* TODO 3 */
		break;
	case ElementType::Info:
		this->serializeInfoElement(buf, childLen);
		break;
	case ElementType::Link: /* TODO 1 */
		break;
	case ElementType::Nesthint: /* TODO 1 */
		break;
	case ElementType::Overlay: /* TODO 2 */
		break;
	case ElementType::Sound:
		this->serializeDataElement(e, buf, childLen);
		break;
	case ElementType::Subject:
		this->serializeSubjectElement(e, buf, childLen);
		break;
	case ElementType::Text:
		this->serializeTextElement(e, buf, childLen);
		break;
	case ElementType::Version:
		this->serializeCommentElement(e, buf, childLen);
		break;
	}

	childLen += 2; // Include descriptor and title in length

	out += std::to_string(childLen);
	out += ' ';
	out += e.elementTypeString();
	out += EOL;

	const auto& title = e.title();
	if (title.length() == 0) {
		out += '-';
	} else {
		out += title;
	}
	out += EOL;

	out += buf;
	len += childLen;
}

void UHSWriter::serializeCommentElement(const Element& e, std::string& out, int& len) {
	const auto& body = e.body();
	if (!body.empty()) {
		out += Strings::wrap(body, EOL, LineLen, len) + EOL;
	}
}

// GIFs don't appear to be displayed by the official reader any longer
void UHSWriter::serializeDataElement(const Element& e, std::string& out, int& len) {
	const auto elementType = e.elementType();
	const auto& body = e.body();

	out += this->createDataAddress(body.length());
	++len;
	_data.emplace(std::make_pair(elementType, body));
}

void UHSWriter::serializeHintElement(const Element& e, std::string& out, int& len) {
	bool continuation = false;

	for (Node* n = e.firstChild(); n; n = n->nextSibling()) {
		if (continuation) {
			out += Token::NestedTextSep;
			out += EOL;
			++len;
		}
		if (n->nodeType() != NodeType::Text) {
			// Hint should only contain text nodes at this point
			throw WriteError("unexpected node type: %s", n->nodeTypeString().data());
		}
		const auto& tn = static_cast<const TextNode&>(*n);
		const auto& body = tn.body();
		out += _codec.encode88a(Strings::wrap(body, EOL, LineLen, len)) + EOL;

		continuation = true;
	}
}

void UHSWriter::serializeInfoElement(std::string& out, int& len) {
	out += InfoLengthMarker;
	out += EOL;
	++len;

	const auto now = std::time(nullptr);
	const auto tm = std::localtime(&now);

	char buf[10];
	const auto dateLen = std::strftime(buf, 10, "%d-%b-%y", tm);
	out += "date=" + std::string(buf, dateLen) + EOL;
	++len;

	const auto timeLen = std::strftime(buf, 9, "%H:%M:%S", tm);
	out += "time=" + std::string(buf, timeLen) + EOL;
	++len;

	for (const auto& [k, v] : _document->attrs()) {
		if (k == "length" || k == "date" || k == "time" || k == "notice") {
			continue;
		}
		out += Strings::wrap(v, EOL, LineLen, len, k + "=") + EOL;
	}

	if (auto notice = _document->attr("notice")) {
		out += Strings::wrap(*notice, EOL, LineLen, len, Token::NoticePrefix) + EOL;
	}
}

void UHSWriter::serializeSubjectElement(const Element& e, std::string& out, int& len) {
	for (Node* n = e.firstChild(); n; n = n->nextSibling()) {
		if (n->nodeType() != NodeType::Element) {
			throw WriteError("unexpected node type: %s", n->nodeTypeString().data());
		}
		const auto& child = static_cast<const Element&>(*n);
		this->serializeElement(child, out, len);
	}
}

void UHSWriter::serializeTextElement(const Element& e, std::string& out, int& len) {
	const auto elementType = e.elementType();
	const auto& body = e.body();

	const auto textFormat = ((*e.attr("typeface") == "monospace") ? "1 " : "0 ");
	out += this->createDataAddress(body.length(), textFormat);
	++len;

	auto lines = Strings::split(body, "\n");
	for (auto& line : lines) {
		if (line == "") {
			line = Token::ParagraphSep;
		}
		line = _codec.encode96a(line, _key, true);
	}
	_data.emplace(std::make_pair(elementType, Strings::join(lines, EOL) + EOL));
}

void UHSWriter::serializeData(std::string& out) {
	// Add data and replace data addresses
	auto dataOffset = out.length();
	auto searchEnd = out.cend();
	int offset = 0;

	while (!_data.empty()) {
		const auto& pair = _data.front();
		const auto elementType = pair.first;
		const auto& data = pair.second;

		std::smatch matches;
		const auto& regex = (elementType == ElementType::Overlay) ? Regex::OverlayAddress
		                                                          : Regex::DataAddress;
		if (!std::regex_search(out.cbegin() + offset, searchEnd, matches, regex)) {
			throw WriteError(
			    "could not find address offset for %s element data (%d bytes)",
			    Element::typeString(elementType),
			    data.length());
		}

		const auto& matchNumber = (elementType == ElementType::Overlay) ? 1 : 2;
		const auto pos = static_cast<std::size_t>(matches.position(matchNumber));
		const auto dataAddress = std::to_string(dataOffset);
		const auto dataAddressLen = dataAddress.length();
		const auto dataAddressOffset = offset + pos + FileSizeLen - dataAddressLen;

		// Replace data address
		out.replace(dataAddressOffset, dataAddressLen, dataAddress);

		// Advance search position
		offset += pos;

		// Add data
		out += data;
		dataOffset += data.length();

		_data.pop();
	}

	// Find length attribute
	const auto pos = out.find(InfoLengthMarker);
	if (pos == std::string::npos) {
		return; // No info node present
	}
	const auto fileLen = out.length() + CRC::ByteLen;
	const auto fileLenStr = std::to_string(fileLen);
	const auto fileLenStrLen = fileLenStr.length();
	const auto infoLengthOffset = pos + strlen(InfoLengthMarker) - fileLenStrLen;

	// Replace length attribute
	out.replace(infoLengthOffset, fileLenStrLen, fileLenStr);
}

void UHSWriter::serializeCRC(std::string& out) {
	// Calculate
	_crc.calculate(out.data(), out.length());

	// Write checksum
	std::vector<char> checksum;
	_crc.result(checksum);
	out.push_back(checksum[0]);
	out.push_back(checksum[1]);
}

std::string UHSWriter::createDataAddress(std::size_t bodyLen, std::string textFormat) {
	std::string out;

	out += DataAddressMarker;
	out += ' ';
	out += textFormat;
	out += std::string(FileSizeLen, '0');
	out += ' ';
	std::ostringstream buf;
	buf << std::setfill('0') << std::setw(MediaSizeLen) << bodyLen;
	out += buf.str();
	out += EOL;

	return out;
}

void UHSWriter::convertTo91a() {
	// Re-parent under subject node
	auto container = Element::create(ElementType::Subject);
	container->title(_document->title());
	for (Node* n = _document->firstChild(); n; n = _document->firstChild()) {
		if (n->nodeType() != NodeType::Element) {
			throw WriteError(
			    "expected element, found %s node", n->nodeTypeString().data());
		}
		container->appendChild(_document->removeChild(n));
	}
	_document->appendChild(std::move(container));

	// Prepend minimal 88a header
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	auto subject = Element::create(ElementType::Subject);
	subject->title(_codec.decode88a("-"));
	auto hint = Element::create(ElementType::Hint);
	hint->title(_codec.decode88a("-"));
	hint->appendChild(_codec.decode88a("-"));
	subject->appendChild(std::move(hint));
	header->appendChild(std::move(subject));
	_document->insertBefore(std::move(header), _document->firstChild());

	// Set version to 96a
	_document->version(VersionType::Version91a);
	auto version = Element::create(ElementType::Version);
	version->title("91a");
	if (auto notice = _document->attr("notice")) {
		version->body(*notice);
	}
	_document->appendChild(std::move(version));
}

} // namespace UHS

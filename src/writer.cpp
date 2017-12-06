#include "uhs.h"
#include <fstream>
#include <iomanip>
#include <iostream>

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions opt) : _out{out}, _opt{opt} {}

std::unique_ptr<Error> Writer::error() {
	return std::move(_err);
}

void Writer::reset() {
	_err = nullptr;
}

TreeWriter::TreeWriter(std::ostream& out, const WriterOptions opt) : Writer(out, opt) {}

bool TreeWriter::write(const Document& d) {
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
			// Ignore
			break;
		}
	}
	return true;
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
	if (p == nullptr) {
		return;
	}

	for (int i = depth; i > 0; --i) {
		if (p == nullptr) {
			break;
		}
		auto gp = p->parent();

		if (gp != nullptr) {
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

bool JSONWriter::write(const Document& d) {
	Json::Value root{Json::objectValue};

	this->serialize(d, root);
	_out << root << std::endl;

	return true;
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
		int intVal = Strings::toInt(v);
		if (v == "true") {
			obj[k] = true;
		} else if (v == "false") {
			obj[k] = false;
		} else if (intVal != Strings::NaN) {
			obj[k] = intVal;
		} else {
			obj[k] = v;
		}
	}
}

//-------------------------------- UHSWriter --------------------------------//

UHSWriter::UHSWriter(std::ostream& out, const WriterOptions opt) : Writer(out, opt) {}

bool UHSWriter::write(const Document& d) {
	bool ok = false;
	std::string buf;
	buf.reserve(InitialBufferLen);

	if (_opt.force88aMode) {
		ok = this->serialize88a(d, buf);
	} else {
		_document = d.clone();
		ok = this->serialize96a(buf);
	}
	if (!ok) {
		return false;
	}

	_out << buf;
	std::flush(_out);

	return ok;
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

bool UHSWriter::serialize88a(const Document& d, std::string& out) {
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

			// if (_opt.debug) {
			// 	this->debug(e);
			// }

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
				_err = std::make_unique<Error>(ErrorType::Value);
				_err->messagef(
				    "unexpected element: %s", Element::typeString(elementType).data());
				_err->finalize();
				return false;
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

		if (n->hasFirstChild()) {
			auto child = n->firstChild();
			if (!Node::isElementOfType(*child, ElementType::Credit)) {
				queue.push(child);
			}
			while (child->hasNextSibling()) {
				child = child->nextSibling();
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

	const auto credit = d.attr("notice");
	if (!credit.empty()) {
		out += Token::CreditSep;
		out += EOL;
		out += Strings::wrap(credit, EOL, LineLen) + EOL;
	}

	return true;
}

bool UHSWriter::serialize96a(std::string& out) {
	bool ok = false;

	if (_document->version() == VersionType::Version88a) {
		ok = this->convertTo91a();
		if (!ok) {
			return false;
		}
	}

	_key = _codec.createKey(_document->title());

	for (Node* n = _document->firstChild(); n != nullptr; n = n->nextSibling()) {
		switch (n->nodeType()) {
		case NodeType::Document: {
			const auto& child = static_cast<const Document&>(*n);
			if (child.version() != VersionType::Version88a) {
				continue;
			}

			ok = this->serialize88a(child, out);
			if (!ok) {
				return false;
			}

			out += Token::HeaderSep;
			out += EOL;
			break;
		}

		case NodeType::Element: {
			int len = 0;
			const auto& child = static_cast<const Element&>(*n);

			ok = this->serializeElement(child, out, len);
			if (!ok) {
				return false;
			}
			break;
		}

		case NodeType::Text: {
			const auto& tn = static_cast<const TextNode&>(*n);
			_err = std::make_unique<Error>(ErrorType::Value);
			_err->messagef("unexpected text node: %s", tn.body().data());
			_err->finalize();
			return false;
		}

		default:
			// Not handled
			break;
		}
	}

	out += Token::DataSep;

	ok = this->serializeData(out);
	if (!ok) {
		return false;
	}

	this->serializeCRC(out);

	return true;
}

bool UHSWriter::serializeElement(const Element& e, std::string& out, int& len) {
	bool ok = true;
	std::string buf;
	int childLen = 0;

	switch (e.elementType()) {
	case ElementType::Unknown: /* No further processing required */
		break;
	case ElementType::Blank: /* No further processing required */
		break;
	case ElementType::Comment:
		ok = this->serializeCommentElement(e, buf, childLen);
		break;
	case ElementType::Credit:
		ok = this->serializeCommentElement(e, buf, childLen);
		break;
	case ElementType::Gifa:
		ok = this->serializeDataElement(e, buf, childLen);
		break;
	case ElementType::Hint:
		ok = this->serializeHintElement(e, buf, childLen);
		break;
	case ElementType::Hyperpng: /* TODO 2 */
		break;
	case ElementType::Incentive: /* TODO 3 */
		break;
	case ElementType::Info:
		ok = this->serializeInfoElement(buf, childLen);
		break;
	case ElementType::Link: /* TODO 1 */
		break;
	case ElementType::Nesthint: /* TODO 1 */
		break;
	case ElementType::Overlay: /* TODO 2 */
		break;
	case ElementType::Sound:
		ok = this->serializeDataElement(e, buf, childLen);
		break;
	case ElementType::Subject:
		ok = this->serializeSubjectElement(e, buf, childLen);
		break;
	case ElementType::Text:
		ok = this->serializeTextElement(e, buf, childLen);
		break;
	case ElementType::Version:
		ok = this->serializeCommentElement(e, buf, childLen);
		break;
	}

	if (!ok) {
		return false;
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

	return true;
}

bool UHSWriter::serializeCommentElement(const Element& e, std::string& out, int& len) {
	const auto& body = e.body();
	if (!body.empty()) {
		out += Strings::wrap(body, EOL, LineLen, len) + EOL;
	}
	return true;
}

// GIFs don't appear to be displayed by the official reader any longer
bool UHSWriter::serializeDataElement(const Element& e, std::string& out, int& len) {
	const auto elementType = e.elementType();
	const auto& body = e.body();

	out += this->createDataAddress(body.length());
	++len;
	_data.emplace(std::make_pair(elementType, body));

	return true;
}

bool UHSWriter::serializeHintElement(const Element& e, std::string& out, int& len) {
	bool continuation = false;

	for (Node* n = e.firstChild(); n != nullptr; n = n->nextSibling()) {
		if (continuation) {
			out += Token::NestedTextSep;
			out += EOL;
			++len;
		}
		if (n->nodeType() != NodeType::Text) {
			// Hint should only contain text nodes at this point
			_err = std::make_unique<Error>(ErrorType::Value);
			_err->messagef("unexpected node type: %s", n->nodeTypeString().data());
			_err->finalize();
			return false;
		}
		const auto& tn = static_cast<const TextNode&>(*n);
		const auto& body = tn.body();
		out += _codec.encode88a(Strings::wrap(body, EOL, LineLen, len)) + EOL;

		continuation = true;
	}

	return true;
}

bool UHSWriter::serializeInfoElement(std::string& out, int& len) {
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

	const auto notice = _document->attr("notice");
	if (!notice.empty()) {
		out += Strings::wrap(notice, EOL, LineLen, len, Token::NoticePrefix) + EOL;
	}

	return true;
}

bool UHSWriter::serializeSubjectElement(const Element& e, std::string& out, int& len) {
	bool ok = false;

	for (Node* n = e.firstChild(); n != nullptr; n = n->nextSibling()) {
		if (n->nodeType() != NodeType::Element) {
			_err = std::make_unique<Error>(ErrorType::Value);
			_err->messagef("unexpected node type: %s", n->nodeTypeString().data());
			_err->finalize();
			return false;
		}
		const auto& child = static_cast<const Element&>(*n);
		ok = this->serializeElement(child, out, len);
		if (!ok) {
			return false;
		}
	}

	return true;
}

bool UHSWriter::serializeTextElement(const Element& e, std::string& out, int& len) {
	const auto elementType = e.elementType();
	const auto& body = e.body();

	const auto textFormat = ((e.attr("typeface") == "monospace") ? "1 " : "0 ");
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

	return true;
}

bool UHSWriter::serializeData(std::string& out) {
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
			_err = std::make_unique<Error>(ErrorType::Value);
			_err->messagef("could not find address offset for %s element data (%d bytes)",
			    Element::typeString(elementType).data(),
			    data.length());
			_err->finalize();
			return false;
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
		// No info node present
		return true;
	}
	const auto fileLen = out.length() + CRC::ByteLen;
	const auto fileLenStr = std::to_string(fileLen);
	const auto fileLenStrLen = fileLenStr.length();
	const auto infoLengthOffset = pos + strlen(InfoLengthMarker) - fileLenStrLen;

	// Replace length attribute
	out.replace(infoLengthOffset, fileLenStrLen, fileLenStr);

	return true;
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
	std::ostringstream buf;
	buf << std::setfill('0') << std::setw(MediaSizeLen) << bodyLen;

	std::string out;
	out += DataAddressMarker;
	out += ' ';
	out += textFormat;
	out += std::string(FileSizeLen, '0');
	out += ' ';
	out += buf.str();
	out += EOL;

	return out;
}

bool UHSWriter::convertTo91a() {
	// Re-parent under subject node
	auto container = std::make_unique<Element>(ElementType::Subject);
	container->title(_document->title());
	for (Node* n = _document->firstChild(); n != nullptr; n = _document->firstChild()) {
		if (n->nodeType() != NodeType::Element) {
			_err = std::make_unique<Error>(ErrorType::Value);
			_err->messagef("expected element, found %s node", n->nodeTypeString().data());
			_err->finalize();
			return false;
		}
		container->appendChild(_document->removeChild(n));
	}
	_document->appendChild(std::move(container));

	// Prepend minimal 88a header
	auto header = std::make_unique<Document>(VersionType::Version88a, "-");
	auto subject = std::make_unique<Element>(ElementType::Subject, _codec.decode88a("-"));
	auto hint = std::make_unique<UHS::Element>(ElementType::Hint, _codec.decode88a("-"));
	hint->appendChild(std::make_unique<TextNode>(_codec.decode88a("-")));
	subject->appendChild(std::move(hint));
	header->appendChild(std::move(subject));
	_document->insertBefore(std::move(header), _document->firstChild());

	// Set version to 96a
	_document->version(VersionType::Version91a);
	auto version = std::make_unique<Element>(ElementType::Version);
	version->title("91a");
	version->body(_document->attr("notice"));
	_document->appendChild(std::move(version));

	return true;
}

} // namespace UHS

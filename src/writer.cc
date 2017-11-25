#include <fstream>
#include <iostream>
#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions opt)
	: _out {out}, _opt {opt} {}

std::unique_ptr<Error> Writer::error() {
	return std::move(_err);
}

void Writer::reset() {
	_err = nullptr;
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions opt)
	: Writer(out, opt) {}

bool JSONWriter::write(Document& d) {
	Json::Value root {Json::objectValue};

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
				Json::Value a {Json::arrayValue};
				(*parent)["children"] = a;
			} else {
				parent = &((*parent)["children"][(*parent)["children"].size()-1]);
			}
		} else if (nodeDepth < depth) { // Up
			if (nodeDepth >= 0) {
				parent = parents[nodeDepth];
			}
		}

		// Serialize node
		Json::Value obj {Json::objectValue};

		switch (n.nodeType()) {
		case NodeText:
			{
				const auto& tn = dynamic_cast<const TextNode&>(n);
				(*parent)["children"].append(tn.body());
			}
			break;
		case NodeElement:
			{
				const auto& e = dynamic_cast<const Element&>(n);
				this->serializeElement(e, obj);
				(*parent)["children"].append(obj);
			}
			break;
		case NodeDocument:
			{
				const auto& doc = dynamic_cast<const Document&>(n);
				if (nodeDepth > 0) {
					this->serializeDocument(doc, obj);
					(*parent)["children"].append(obj);
				} else {
					this->serializeDocument(doc, root);
				}
			}
			break;
		}

		depth = nodeDepth;
	}

	return root;
}

void JSONWriter::serializeElement(const Element& e, Json::Value& obj) const {
	std::string fname;
	std::ofstream fout;

	obj["title"] = e.title();
	
	if (e.isMedia() && ! _opt.mediaDir.empty()) {
		// TODO: Do something about how lazy this is
		fname = _opt.mediaDir + "/" + std::to_string(e.index()) + "." + e.mediaExt();
		fout.open(fname, std::ios::out | std::ios::binary);
		fout << e.body();
		fout.close();
		obj["body"] = fname;
	} else {
		const auto& body = e.body();
		if (! body.empty()) {
			obj["body"] = body;
		}
	}

	if (! e.visible(_opt.registered)) {
		obj["visible"] = false;
	}
	obj["type"] = Element::typeString(e.elementType());

	// Build attributes map
	const auto& attrs = e.attrs();
	if (! attrs.empty()) {
		Json::Value map {Json::objectValue};
		this->serializeMap(attrs, map);
		obj["attributes"] = map;
	}
}

void JSONWriter::serializeDocument(const Document& d, Json::Value& obj) const {
	obj["title"] = d.title();
	obj["version"] = d.versionString();

	if (d.version() > Version88a) {
		obj["registered"] = _opt.registered;
		obj["validChecksum"] = d.validChecksum();
	}
	if (! d.visible(_opt.registered)) {
		obj["visible"] = false;
	}
	obj["type"] = Node::typeString(d.nodeType());

	// Build attributes map
	const auto& attrs = d.attrs();
	if (! attrs.empty()) {
		Json::Value map {Json::objectValue};
		this->serializeMap(attrs, map);
		obj["attributes"] = map;
	}
}

void JSONWriter::serializeMap(const Traits::Attributes::Type& attrs,
		Json::Value& obj) const {

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

UHSWriter::UHSWriter(std::ostream& out, const WriterOptions opt)
	: Writer(out, opt) {}

bool UHSWriter::write(Document& d) {
	bool ok = false;
	std::ostringstream buf;
	
	if (_opt.force88aMode) {
		ok = this->serialize88a(d, buf);
	} else {
		ok = this->serialize96a(d, buf);
	}
	_out << buf.str();
	std::flush(_out);

	return ok;
}

bool UHSWriter::serialize88a(const Document& d, std::ostream& out) {
	std::queue<const Node*> queue;
	auto prevType = ElementUnknown;
	int index = 1;
	int numPrevChildren = 0;
	int firstHintTextIndex = 0;
	int lastHintTextIndex = 0;

	// Traverse tree, breadth first
	queue.push(&d);
	std::ostringstream buf;

	while (! queue.empty()) {
		const auto n = queue.front();
		queue.pop();

		switch (n->nodeType()) {
		case NodeText:
			{
				const auto& tn = dynamic_cast<const TextNode&>(*n);
				buf << _codec.encode88a(tn.body()) << EOL;
			}
			index += numPrevChildren - 1; // Simplifies to index -= 1 after first pass

			if (lastHintTextIndex == 0) {
				lastHintTextIndex = index;
			}
			firstHintTextIndex = index; // Work backwards from last
			break;
		
		case NodeElement:
			{
				const auto& e = dynamic_cast<const Element&>(*n);
				ElementType elementType = e.elementType();
				std::string title;

				switch (elementType) {
				case ElementSubject:
					index += numPrevChildren * 2;
					title = e.title();
					break;
				case ElementHint:
					if (prevType == ElementSubject) {
						index += numPrevChildren * 2;
					} else {
						index += numPrevChildren;
					}
					title = Strings::rtrim(e.title(), '?');
					break;
				default:
					_err = std::make_unique<Error>(ErrorValue);
	 				_err->messagef("unexpected element: %s", Element::typeString(elementType).data());
	 				_err->finalize();
	 				return false;
				}

				buf << _codec.encode88a(title) << EOL;
				buf << index << EOL;

				prevType = elementType;
			}
			break;

		default:
			break; // Ignore
		}

		numPrevChildren = n->numChildren();

		if (n->hasFirstChild()) {
			auto child = n->firstChild();
			if (! Node::isElementOfType(*child, ElementCredit)) {
				queue.push(child);
			}
			while (child->hasNextSibling()) {
				child = child->nextSibling();
				if (! Node::isElementOfType(*child, ElementCredit)) {
					queue.push(child);
				}
			}
		}
	}

	out << Token::Signature << EOL;
	out << d.title() << EOL;
	out << firstHintTextIndex << EOL;
	out << lastHintTextIndex << EOL;
	out << buf.str();

	auto credit = d.attr("notice");
	if (! credit.empty()) {
		out << Token::CreditSep << EOL;
		out << Strings::wrap(credit, EOL, LineLen) << EOL;
	}

	return true;
}

bool UHSWriter::serialize96a(Document& d, std::ostringstream& out) {
	bool ok = false;

	if (d.version() == Version88a) {
		ok = this->convertTo96a(d);
		if (! ok) {
			return false;
		}
	}

	for (Node* n = d.firstChild(); n != nullptr; n = n->nextSibling()) {
		switch (n->nodeType()) {
		case NodeDocument:
			{
				const auto& child = dynamic_cast<const Document&>(*n);
				if (child.version() != Version88a) {
					continue;
				}
				ok = this->serialize88a(child, out);
				if (! ok) {
					return false;
				}
			}
			out << Token::HeaderSep << EOL;
			break;

		case NodeElement:
			{
				int len = 0;
				const auto& child = dynamic_cast<const Element&>(*n);
				ok = this->serializeElement(d, child, out, len);
				if (! ok) {
					return false;
				}
			}
			break;

		default:
			// TODO: Create error; no text nodes should be at this level
			return false;
		}
	}

	out << Token::DataSep;

	// TODO: Add data
	// TODO: Update data addresses
	// TODO: Update length=0000000

	this->serializeCRC(out);

	return true;
}

bool UHSWriter::serializeElement(const Document& d, const Element& e, std::ostream& out, int& len) {
	bool ok = false;
	std::ostringstream buf;

	int childLen = 0;

	switch (e.elementType()) {
	case ElementUnknown:   /* No further processing required */                     break;
	case ElementBlank:     /* No further processing required */                     break;
	case ElementComment:   ok = this->serializeCommentElement(e, buf, childLen);    break;
	case ElementCredit:    ok = this->serializeCommentElement(e, buf, childLen);    break;
	case ElementGifa:      /* TODO */                                               break;
	case ElementHint:      ok = this->serializeHintElement(e, buf, childLen);       break;
	case ElementHyperpng:  /* TODO */                                               break;
	case ElementIncentive: /* TODO */                                               break;
	case ElementInfo:      ok = this->serializeInfoElement(d, buf, childLen);       break;
	case ElementLink:      /* TODO */                                               break;
	case ElementNesthint:  /* TODO */                                               break;
	case ElementOverlay:   /* TODO */                                               break;
	case ElementSound:     /* TODO */                                               break;
	case ElementSubject:   ok = this->serializeSubjectElement(d, e, buf, childLen); break;
	case ElementText:      /* TODO */                                               break;
	case ElementVersion:   ok = this->serializeCommentElement(e, buf, childLen);    break;
	}

	childLen += 2; // Include descriptor and title in length
	out << childLen << ' ' << e.elementTypeString() << EOL;

	auto title = e.title();
	if (title.length() == 0) {
		title = '-';
	}
	out << title << EOL;

	if (! ok) {
		return false;
	}

	out << buf.str();
	len += childLen;

	return true;
}

bool UHSWriter::serializeCommentElement(const Element& e, std::ostream& out, int& len) {
	const auto& body = e.body();
	if (! body.empty()) {
		out << Strings::wrap(body, EOL, LineLen, len) << EOL;
	}
	return true;
}

bool UHSWriter::serializeHintElement(const Element& e, std::ostream& out, int& len) {
	bool continuation = false;

	for (Node* n = e.firstChild(); n != nullptr; n = n->nextSibling()) {
		if (continuation) {
			out << Token::NestedTextSep << EOL;
			++len;
		}
		if (n->nodeType() != NodeText) {
			// TODO: Create error, hint should only contain text nodes at this point
			return false;
		}
		const auto& tn = dynamic_cast<const TextNode&>(*n);
		auto body = tn.body();
		out << _codec.encode88a(Strings::wrap(body, EOL, LineLen, len)) << EOL;

		continuation = true;
	}

	return true;
}

bool UHSWriter::serializeInfoElement(const Document& d, std::ostream& out, int& len) {
	out << "length=0000000" << EOL;
	++len;

	const auto now = std::time(nullptr);
	const auto tm = std::localtime(&now);
	char buf[10];

	auto bufLen = std::strftime(buf, 10, "%d-%b-%y", tm);
	out << "date=" << std::string(buf, bufLen) << EOL;;
	++len;

	bufLen = std::strftime(buf, 9, "%H:%M:%S", tm);
	out << "time=" << std::string(buf, bufLen) << EOL;;
	++len;

	for (const auto& [k, v] : d.attrs()) {
		if (k == "length" || k == "date" || k == "time" || k == "notice") {
			continue;
		}
		out << Strings::wrap(v, EOL, LineLen, len, k + "=") << EOL;
	}

	auto notice = d.attr("notice");
	if (! notice.empty()) {
		out << Strings::wrap(notice, EOL, LineLen, len, ">") << EOL;
	}

	return true;
}

bool UHSWriter::serializeSubjectElement(const Document& d, const Element& e, std::ostream& out, int& len) {
	bool ok = false;

	for (Node* n = e.firstChild(); n != nullptr; n = n->nextSibling()) {
		if (n->nodeType() != NodeElement) {
			// TODO: Create error, subject should only contain elements
			return false;
		}
		const auto& child = dynamic_cast<const Element&>(*n);
		ok = this->serializeElement(d, child, out, len);
		if (! ok) {
			return false;
		}
	}

	return true;
}

void UHSWriter::serializeCRC(std::ostringstream& out) {
	// Calculate
	auto buf = out.str();
	_crc.calculate(buf.data(), buf.length());
	_crc.finalize();

	// Write checksum
	std::vector<char> checksum;
	_crc.checksum(checksum);
	out.put(checksum[0]); // Ensure that 0x00 will be written (to a file)
	out.put(checksum[1]);
}

bool UHSWriter::convertTo96a(Document& d) {
	auto info = std::make_unique<Element>(ElementInfo);

	// Re-parent under subject node
	auto container = std::make_unique<Element>(ElementSubject);
	container->title(d.title());
	for (Node* n = d.firstChild(); n != nullptr; n = d.firstChild()) {
		if (n->nodeType() != NodeElement) {
			// TODO: Create error
			return false;
		}
		container->appendChild(d.removeChild(n));
	}
	d.appendChild(std::move(container));

	// Prepend minimal 88a header
	auto header = std::make_unique<Document>(Version88a, "-");
	auto subject = std::make_unique<Element>(ElementSubject, _codec.decode88a("-"));
	auto hint = std::make_unique<UHS::Element>(ElementHint, _codec.decode88a("-"));
	hint->appendChild(std::make_unique<TextNode>(_codec.decode88a("-")));
	subject->appendChild(std::move(hint));
	header->appendChild(std::move(subject));
	d.insertBefore(std::move(header), d.firstChild());

	// Set version to 96a
	d.version(Version96a);
	auto version = std::make_unique<Element>(ElementVersion);
	version->title("96a");
	d.appendChild(std::move(version));

	// Add info
	d.appendChild(std::move(info));

	return true;
}

}

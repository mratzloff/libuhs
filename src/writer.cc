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

bool JSONWriter::write(const Document& d) {
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
	
	if (e.isMedia()) { // TODO: Do something about how lazy this is
		fname = _opt.mediaDir + "/" + std::to_string(e.index()) + "." + e.mediaExt();
		fout.open(fname, std::ofstream::out | std::ofstream::binary);
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

bool UHSWriter::write(const Document& d) {
	bool ok = false;
	std::ostringstream buf;
	
	if (d.version() == Version88a) {
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
	int firstHintIndex = 0;
	int lastHintIndex = 0;

	// Find credit node, if any
	const Element* credit = nullptr;
	for (const auto& n : d) {
		if (n.nodeType() == NodeElement) {
			const auto& e = dynamic_cast<const Element&>(n);
			if (e.elementType() == ElementCredit) {
				credit = &e;
				break;
			}
		}
	}

	// Traverse tree, breadth first
	queue.push(&d);
	std::ostringstream buf;

	while (! queue.empty()) {
		const auto n = queue.front();
		queue.pop();

		switch (n->nodeType()) {
		case NodeText:
			if (lastHintIndex == 0) {
				lastHintIndex = index + numPrevChildren - 1;
			}
			{
				const auto& tn = dynamic_cast<const TextNode&>(*n);
				buf << _codec.encode88a(tn.body()) << EOL;
			}
			break;
		
		case NodeElement:
			{
				const auto& e = dynamic_cast<const Element&>(*n);
				ElementType elementType = e.elementType();

				std::string title;
				if (elementType == ElementHint) {
					title = Strings::rtrim(e.title(), '?');
				} else {
					title = e.title();
				}

				if (elementType == ElementSubject || prevType == ElementSubject) {
					index += numPrevChildren * 2;
				} else if (elementType == ElementHint) {
					if (firstHintIndex == 0) {
						firstHintIndex = index;
					}
					index += numPrevChildren;
				} else {
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
		if (n->nodeType() == NodeDocument && credit != nullptr) {
			--numPrevChildren; // Don't count credits for indexing purposes
		}

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
	out << firstHintIndex << EOL;
	out << lastHintIndex << EOL;
	out << buf.str();

	if (credit != nullptr) {
		out << Token::CreditSep << EOL;
		out << Strings::wrap(credit->body(), EOL, LineLen) << EOL;
	}

	return true;
}

bool UHSWriter::serialize96a(const Document& d, std::ostringstream& out) {
	for (Node* n = d.firstChild(); n != nullptr; n = n->nextSibling()) {
		switch (n->nodeType()) {
		case NodeDocument:
			{
				const auto& child = dynamic_cast<const Document&>(*n);
				if (child.version() != Version88a) {
					continue;
				}
				this->serialize88a(child, out);
			}
			out << Token::HeaderSep << EOL;
			break;

		case NodeElement:
			{
				int len = 0;
				const auto& child = dynamic_cast<const Element&>(*n);
				this->serializeElement(child, out, len);
			}
			break;

		default:
			// TODO: Create error; no text nodes should be at this level
			return false;
		}
	}
	out << Token::DataSep;

	// TODO: Add data

	auto buf = out.str();
	_crc.calculate(buf.data(), buf.length());
	_crc.finalize();
	out << _crc.string();

	return true;
}

bool UHSWriter::serializeElement(const Element& e, std::ostream& out, int& len) {
	bool ok = false;
	std::ostringstream buf;

	int childLen = 0;

	switch (e.elementType()) {
	case ElementUnknown:   /* No further processing required */                  break;
	case ElementBlank:     /* No further processing required */                  break;
	case ElementComment:   ok = this->serializeCommentElement(e, buf, childLen); break;
	case ElementCredit:    ok = this->serializeCommentElement(e, buf, childLen); break;
	case ElementGifa:      /* TODO */                                            break;
	case ElementHint:      ok = this->serializeHintElement(e, buf, childLen);    break;
	case ElementHyperpng:  /* TODO */                                            break;
	case ElementIncentive: /* TODO */                                            break;
	case ElementInfo:      /* TODO */                                            break;
	case ElementLink:      /* TODO */                                            break;
	case ElementNesthint:  /* TODO */                                            break;
	case ElementOverlay:   /* TODO */                                            break;
	case ElementSound:     /* TODO */                                            break;
	case ElementSubject:   ok = this->serializeSubjectElement(e, buf, childLen); break;
	case ElementText:      /* TODO */                                            break;
	case ElementVersion:   ok = this->serializeCommentElement(e, buf, childLen); break;
	}

	childLen += 2; // Include descriptor and title in length
	out << childLen << ' ' << e.elementTypeString() << EOL;
	out << e.title() << EOL;

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

bool UHSWriter::serializeSubjectElement(const Element& e, std::ostream& out, int& len) {
	bool ok = false;

	for (Node* n = e.firstChild(); n != nullptr; n = n->nextSibling()) {
		if (n->nodeType() != NodeElement) {
			// TODO: Create error, subject should only contain elements
			return false;
		}
		const auto& child = dynamic_cast<const Element&>(*n);
		ok = this->serializeElement(child, out, len);
		if (! ok) {
			return false;
		}
	}

	return true;
}

}

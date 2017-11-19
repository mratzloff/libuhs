#include <fstream>
#include <iostream>
#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions opt)
	: _out {out}
	, _mediaDir {opt.mediaDir}
	, _registered {opt.registered}
{}

const std::shared_ptr<Error> Writer::error() {
	return _err;
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions opt)
	: Writer(out, opt) {}

bool JSONWriter::write(const std::shared_ptr<const Document> d) {
	Json::Value root {Json::objectValue};

	if (d == nullptr) {
		return false;
	}
	this->serialize(*d, root);
	_out << root;
	std::flush(_out);

	return true;
}

Json::Value JSONWriter::serialize(const Document& d, Json::Value& root) const {
	Json::Value* parents[UHS_MAX_DEPTH];
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
		auto nodeType = n.nodeType();
		Json::Value obj {Json::objectValue};

		if (nodeType == NodeText) {
			const auto& tn = dynamic_cast<const TextNode&>(n);
			(*parent)["children"].append(tn.body());

		} else if (nodeType == NodeElement) {
			const auto& e = dynamic_cast<const Element&>(n);
			this->serializeElement(e, obj);
			(*parent)["children"].append(obj);

		} else if (nodeType == NodeDocument) {
			const auto& doc = dynamic_cast<const Document&>(n);

			if (nodeDepth > 0) {
				this->serializeDocument(doc, obj);
				(*parent)["children"].append(obj);
			} else {
				this->serializeDocument(doc, root);
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
	
	if (e.isMedia()) { // TODO: Do something about how lazy this is
		fname = _mediaDir + "/" + std::to_string(e.index()) + "." + e.mediaExt();
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

	if (! e.visible(_registered)) {
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
		obj["registered"] = _registered;
		obj["validChecksum"] = d.validChecksum();
	}
	if (! d.visible(_registered)) {
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

bool UHSWriter::write(const std::shared_ptr<const Document> d) {
	if (d == nullptr) {
		return false;
	}
	auto version = d->version();

	if (version == Version88a) {
		this->write88a(d);
	} else {
		this->write96a(*d);
	}
	std::flush(_out);

	return true;
}

bool UHSWriter::write88a(std::shared_ptr<const Document> d) {
	std::queue<std::shared_ptr<const Node>> queue;
	std::shared_ptr<const Node> n;
	auto prevType = ElementUnknown;
	int index = 1;
	int numPrevChildren = 0;
	int firstHintIndex = 0;
	int lastHintIndex = 0;

	// Find credit node, if any
	std::shared_ptr<const Element> credit;
	for (const auto& n : *d) {
		if (n.nodeType() == NodeElement) {
			const auto& e = dynamic_cast<const Element&>(n);
			if (e.elementType() == ElementCredit) {
				credit = std::make_shared<const Element>(e);
				break;
			}
		}
	}

	// Traverse tree, breadth first
	queue.push(d);
	std::ostringstream ss;

	while (! queue.empty()) {
		n = queue.front();
		queue.pop();

		auto nodeType = n->nodeType();
		if (nodeType == NodeText) {
			if (lastHintIndex == 0) {
				lastHintIndex = index + numPrevChildren - 1;
			}
			const auto& tn = dynamic_cast<const TextNode&>(*n);
			ss << _codec.encode88a(tn.body()) << EOL;

		} else if (nodeType == NodeElement) {
			const auto& e = dynamic_cast<const Element&>(*n);
			auto elementType = e.elementType();

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
				_err = std::make_shared<Error>(ErrorValue);
 				_err->messagef("unexpected element: %s", Element::typeString(elementType).data());
 				_err->finalize();
 				return false;
			}

			ss << _codec.encode88a(title) << EOL;
			ss << index << EOL;

			prevType = elementType;
		}

		numPrevChildren = n->numChildren();
		if (n == d && credit != nullptr) {
			--numPrevChildren; // Don't count credits for indexing purposes
		}

		if (n->hasFirstChild()) {
			n = n->firstChild();
			if (! Node::isElementOfType(*n, ElementCredit)) {
				queue.push(n);
			}
			while (n->hasNextSibling()) {
				n = n->nextSibling();
				if (! Node::isElementOfType(*n, ElementCredit)) {
					queue.push(n);
				}
			}
		}
	}

	_out << Token::Signature << EOL;
	_out << d->title() << EOL;
	_out << firstHintIndex << EOL;
	_out << lastHintIndex << EOL;
	_out << ss.str();

	this->write88aCreditElement(credit);

	return true;
}

void UHSWriter::write88aCreditElement(const std::shared_ptr<const Element> e) {
	// TODO: Note that credit nodes support the "\r\n \r\n" paragraph idiom
	if (e == nullptr) {
		return;
	}

	const auto& n = *(e->firstChild());
	if (n.nodeType() != NodeText) {
		return;
	}

	const auto& tn = dynamic_cast<const TextNode&>(n);
	_out << Token::CreditSep << EOL;
	_out << Strings::wrap(tn.body(), EOL, LineLen - strlen(EOL)) << EOL;
}

bool UHSWriter::write96a(const Document& d) {
	_err = std::make_shared<Error>(ErrorWrite, "not implemented");
	_err->finalize();
	return false;
}

}

#include <fstream>
#include <iostream>
#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions opt)
	: _out {out}
	, _mediaDir {opt.mediaDir}
	, _registered {opt.registered}
{}

std::shared_ptr<Error> Writer::error() {
	return _err;
}

bool Writer::write(const std::shared_ptr<const Document>) const {
	return false;
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions opt)
	: Writer(out, opt) {}

bool JSONWriter::write(const std::shared_ptr<const Document> d) const {
	Json::Value root {Json::objectValue};

	if (d == nullptr) {
		return false;
	}
	this->serialize(*d, root);
	_out << root;

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

}

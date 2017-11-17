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

bool Writer::write(std::shared_ptr<Document>) const {
	return false;
}

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions opt)
	: Writer(out, opt) {}

bool JSONWriter::write(std::shared_ptr<Document> d) const {
	_out << this->serialize(d);
	return true;
}

Json::Value JSONWriter::serialize(std::shared_ptr<Document> d) const {
	Json::Value root {Json::objectValue};
	Json::Value map {Json::objectValue};
	Json::Value* parents[UHS_MAX_DEPTH];
	Json::Value* j;
	std::map<std::string, std::string> attrs;

	if (d == nullptr) {
		return root;
	}

	root["title"] = d->title();
	root["version"] = d->versionString();

	if (d->version() > Version88a) {
		root["header"] = this->serialize(d->header());
		root["registered"] = _registered;
		root["validChecksum"] = d->validChecksum();
	}

	for (const auto& [k, v] : d->attrs()) {
		map[k] = v;
	}
	root["attributes"] = map;

	int depth = 0;
	parents[depth] = &root;
	j = &root;

	for (const auto& n : *d) {
		// Create JSON arrays when we descend
		int newDepth = n.depth();
		if (newDepth > depth) { // Down
			parents[depth] = j;
			if ((*j)["children"].empty()) {
				Json::Value a {Json::arrayValue};
				(*j)["children"] = a;
			} else {
				j = &((*j)["children"][(*j)["children"].size()-1]);
			}
		} else if (newDepth < depth) { // Up
			if (newDepth >= 0) {
				j = parents[newDepth];
			}
		}

		Json::Value object {Json::objectValue};
		std::string fname;
		std::ofstream fout;

		auto nodeType = n.nodeType();

		if (nodeType == NodeText) {
			const auto& tn = dynamic_cast<const TextNode&>(n);
			(*j)["children"].append(tn.body());

		} else if (nodeType == NodeElement) {
			const auto& e = dynamic_cast<const Element&>(n);
			object["label"] = e.label();

			if (e.isMedia()) { // TODO: Do something about how lazy this is
				fname = _mediaDir + "/" + std::to_string(e.index()) + "." + e.mediaExt();
				fout.open(fname, std::ofstream::out | std::ofstream::binary);
				fout << e.body();
				fout.close();
				object["body"] = fname;
			} else {
				object["body"] = e.body();
			}

			attrs = e.attrs();
			map.clear();

			for (const auto& [k, v] : attrs) {
				if (v == "true") {
					map[k] = true;
				} else if (v == "false") {
					map[k] = false;
				} else {
					map[k] = v;
				}
			}
			if (! e.visible(_registered)) {
				map["visible"] = false;
			}

			map["type"] = Element::typeString(e.elementType());
			object["attributes"] = map;
			(*j)["children"].append(object);

		} else if (nodeType == NodeContainer) {
			// Ignore
		}

		depth = newDepth;
	}

	return root;
}

}

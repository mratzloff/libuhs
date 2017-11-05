#include <iostream>
#include "json.h"
#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const WriterOptions& opt)
	: _out {out}
	, _registered {opt.registered}
{}

Writer::~Writer() {}

std::shared_ptr<Error> Writer::error() {
	return _err;
}

bool Writer::write(std::shared_ptr<Document>) const {
	return false;
}

JSONWriter::JSONWriter(std::ostream& out, const WriterOptions& opt)
	: Writer(out, opt) {}

JSONWriter::~JSONWriter() {}

bool JSONWriter::write(std::shared_ptr<Document> d) const {
	std::shared_ptr<Node> n;
	std::shared_ptr<TextNode> tn;
	std::shared_ptr<Element> e;
	std::map<std::string, std::string> attrs;
	bool visited = false;
	int depth = 0;
	Json::Value root(Json::objectValue);
	Json::Value* j;
	Json::Value* parents[UHS_MAX_DEPTH];

	if (d == nullptr) {
		return false;
	}

	n = d->root();
	parents[depth] = &root;
	j = &root;

	root["title"] = d->title();
	root["version"] = d->versionString();

	if (d->version() > Version91a) {
		root["length"] = int(d->length());
		root["timestamp"] = d->timestampString();
	}

	for (const auto& [k, v] : *d->meta()) {
		root[k] = v;
	}

	while (n != nullptr) {
		// Assemble JSON object
		if (!visited) {
			Json::Value map(Json::objectValue), object(Json::objectValue);

			switch (n->nodeType()) {
			case NodeText:
				tn = std::static_pointer_cast<TextNode>(n);
				(*j)["children"].append(tn->value());
				break;
			case NodeElement:
				e = std::static_pointer_cast<Element>(n);

				if (! e->visible(_registered)) {
					break;
				}

				if (false) { // TODO: If binary, write to output directory.
					object["value"] = "/path/to/file";
				} else {
					object["value"] = e->value();
				}

				attrs = e->attrs();
				for (const auto& [k, v] : attrs) {
					if (v == "true") {
						map[k] = true;
					} else if (v == "false") {
						map[k] = false;
					} else {
						map[k] = v;
					}
				}

				map["type"] = Element::typeString(e->elementType());
				object["attributes"] = map;
				(*j)["children"].append(object);

				break;
			case NodeContainer:
				// Ignore
				break;
			}
		}

		// Navigate
		if (n->hasFirstChild() && !visited) { // Down
			n = n->firstChild();
			visited = false;
			parents[depth] = j;
			++depth;
			if ((*j)["children"].empty()) {
				Json::Value a(Json::arrayValue);
				(*j)["children"] = a;
			} else {
				j = &((*j)["children"][(*j)["children"].size()-1]);
			}
		} else if (n->hasNextSibling()) { // Next
			n = n->nextSibling();
			visited = false;
		} else { // Up
			n = n->parent();
			visited = true;
			--depth;
			if (depth >= 0) {
				j = parents[depth];
			}
		}
	}

	_out << root;

	return true;
}

}

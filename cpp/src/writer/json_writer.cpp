#include "uhs.h"

namespace UHS {

JSONWriter::JSONWriter(Logger const& logger, std::ostream& out, Options const& options)
    : Writer(logger, out, options) {}

void JSONWriter::write(std::shared_ptr<Document> const document) {
	Json::Value root{Json::objectValue};
	this->serialize(*document, root);
	out_ << root << std::endl;
}

void JSONWriter::serialize(Document const& document, Json::Value& root) const {
	Json::Value* parents[MaxDepth];
	Json::Value* parent = &root;

	auto depth = 0;
	parents[depth] = &root;

	for (auto const& node : document) {
		// Manage JSON arrays as we descend and ascend
		auto nodeDepth = node.depth();

		if (nodeDepth > depth) { // Down
			parents[depth] = parent;
			auto& children = (*parent)["children"];

			if (children.empty()) {
				(*parent)["children"] = Json::arrayValue;
			} else {
				parent = &(children[children.size() - 1]);
			}
		} else if (nodeDepth < depth) { // Up
			if (nodeDepth >= 0) {
				parent = parents[nodeDepth];
			}
		}

		// Serialize node
		Json::Value object = Json::objectValue;

		switch (node.nodeType()) {
		case NodeType::Break:
			(*parent)["children"].append("-");
			break;
		case NodeType::Document: {
			if (nodeDepth == 0) {
				object = root;
			}
			auto const& d = static_cast<Document const&>(node);
			this->serializeDocument(d, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Element: {
			auto const& element = static_cast<Element const&>(node);
			this->serializeElement(element, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Group: {
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Text: {
			auto const& textNode = static_cast<TextNode const&>(node);
			this->serializeTextNode(textNode, object);
			(*parent)["children"].append(object);
			break;
		}
		default:
			throw DataError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}
}

void JSONWriter::serializeDocument(Document const& document, Json::Value& object) const {
	object["title"] = document.title();
	object["version"] = document.versionString();

	if (document.version() > VersionType::Version88a) {
		object["validChecksum"] = document.validChecksum();
	}
	object["type"] = document.nodeTypeString();

	// Build attributes map
	auto const& attrs = document.attrs();
	if (!attrs.empty()) {
		Json::Value map = {Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
}

void JSONWriter::serializeElement(Element const& element, Json::Value& object) const {
	std::string fname;
	std::ofstream fout;

	object["title"] = element.title();
	if (auto const id = element.id(); id > 0) {
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
		auto const& body = element.body();
		if (!body.empty()) {
			object["body"] = body;
		}
	}

	object["inline"] = element.inlined();
	object["visibility"] = element.visibilityString();
	object["type"] = Element::typeString(element.elementType());

	// Build attributes map
	auto const& attrs = element.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
}

void JSONWriter::serializeMap(
    Traits::Attributes::Type const& attrs, Json::Value& object) const {

	for (auto const& [k, v] : attrs) {
		if (v == "true") {
			object[k] = true;
		} else if (v == "false") {
			object[k] = false;
		} else if (Strings::isInt(v)) {
			try {
				object[k] = Strings::toInt(v);
			} catch (Error const& err) {
				object[k] = v;
			}
		} else {
			object[k] = v; // String value
		}
	}
}

void JSONWriter::serializeTextNode(TextNode const& textNode, Json::Value& object) const {
	object["body"] = textNode.body();

	auto attributes = Json::Value(Json::objectValue);
	attributes["overflow"] = textNode.hasFormat(TextFormat::Overflow);
	attributes["monospace"] = textNode.hasFormat(TextFormat::Monospace);
	attributes["hyperlink"] = textNode.hasFormat(TextFormat::Hyperlink);
	object["attributes"] = attributes;
}

} // namespace UHS

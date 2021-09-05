#include <cmath>
#include <fstream>
#include <iomanip>

#include "uhs.h"

namespace UHS {

Writer::Writer(std::ostream& out, const Options options) : out_{out}, options_{options} {}

void Writer::reset() {}

TreeWriter::TreeWriter(std::ostream& out, const Options options) : Writer(out, options) {}

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
		case NodeType::Group: {
			const auto& groupNode = static_cast<const GroupNode&>(node);
			this->draw(groupNode);
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			this->draw(textNode);
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

void TreeWriter::draw(const GroupNode& groupNode) {
	this->drawScaffold(groupNode);
	out_ << "[" << groupNode.nodeTypeString() << "]" << std::endl;
}

void TreeWriter::draw(const TextNode& textNode) {
	this->drawScaffold(textNode);
	auto body = textNode.body();
	std::replace(body.begin(), body.end(), '\n', ' ');
	if (body.length() > 76) {
		body = body.substr(0, 73) + "...";
	}
	out_ << "[" << textNode.nodeTypeString() << "] \"" << body << "\"" << std::endl;
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

//------------------------------- HTMLWriter --------------------------------//

HTMLWriter::Serializer HTMLWriter::serializer_;

HTMLWriter::HTMLWriter(std::ostream& out, const Options options) : Writer(out, options) {}

void HTMLWriter::write(const Document& document) {
	pugi::xml_document xml;
	this->serialize(document, xml);
	out_ << "<!DOCTYPE html>" << std::endl;
	xml.save(out_, "", pugi::format_raw | pugi::format_no_declaration);
}

void HTMLWriter::serialize(const Document& document, pugi::xml_document& xml) {
	auto html = xml.append_child("html");
	html.append_attribute("lang") = "en";
	auto head = html.append_child("head");
	auto title = head.append_child("title");
	title.append_child(pugi::node_pcdata).set_value(document.title().c_str());
	auto meta1 = head.append_child("meta");
	meta1.append_attribute("charset") = "utf-8";

	if (auto author = document.attr("author")) {
		auto meta2 = head.append_child("meta");
		meta2.append_attribute("name") = "author";
		meta2.append_attribute("content") = author.value().c_str();
	}

	auto style = head.append_child("style");
	style.append_child(pugi::node_pcdata)
	    .set_value(
	        ".overflow { white-space: nowrap; }"
	        ".overflow.monospace { white-space: pre; }"
	        ".monospace { font-family: monospace; white-space: pre; }"
	        ".visibility-none { display: none; }\n");

	auto body = html.append_child("body");
	auto root = body.append_child("main");

	pugi::xml_node parents[MaxDepth];
	pugi::xml_node parent = root;
	auto depth = 0;
	parents[depth] = root;

	for (const auto& node : document) {
		auto nodeDepth = node.depth();
		if (nodeDepth > depth) { // Down
			parents[depth] = parent;
			if (!parent.children().empty()) {
				parent = parent.last_child();
			}
		} else if (nodeDepth < depth) { // Up
			if (nodeDepth >= 0) {
				parent = parents[nodeDepth];
			}
		}

		// Serialize node
		switch (node.nodeType()) {
		case NodeType::Break:
			break;
		case NodeType::Document: {
			const auto& d = static_cast<const Document&>(node);
			auto xmlNode = (nodeDepth == 0) ? root : parent.append_child("section");
			this->serializeDocument(d, xmlNode);
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			auto xmlNode = parent.append_child("section");
			this->serializeElement(element, xmlNode);
			break;
		}
		case NodeType::Group:
			parent.append_child("p");
			break;
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			assert(textNode.hasParent());
			auto childOfGroup = textNode.parent()->nodeType() == NodeType::Group;
			auto xmlNode = parent.append_child(childOfGroup ? "span" : "p");
			this->serializeTextNode(textNode, xmlNode);
			break;
		}
		default:
			throw WriteError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}
}

void HTMLWriter::serializeDocument(
    const Document& document, pugi::xml_node xmlNode) const {

	xmlNode.append_attribute("data-type") = document.nodeTypeString().c_str();
	xmlNode.append_attribute("data-version") = document.versionString().c_str();
	this->appendVisibility(document, xmlNode);
}

void HTMLWriter::serializeElement(const Element& element, pugi::xml_node xmlNode) {
	if (const auto id = element.id(); id > 0) {
		xmlNode.append_attribute("id") = id;
	}

	xmlNode.append_attribute("data-type") =
	    Element::typeString(element.elementType()).c_str();

	this->appendVisibility(element, xmlNode);

	for (const auto& [k, v] : element.attrs()) {
		xmlNode.append_attribute(("data-" + k).c_str()) = v.c_str();
	}

	serializer_.invoke(*this, element, xmlNode);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void HTMLWriter::serializeBlankElement(const Element& element, pugi::xml_node xmlNode) {
	xmlNode.set_name("hr");
}
#pragma clang diagnostic pop

void HTMLWriter::serializeCommentElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeader(element, xmlNode);
	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeDataElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeHintElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeader(element, xmlNode);
}

void HTMLWriter::serializeHyperpngElement(
    const Element& element, pugi::xml_node xmlNode) {

	this->appendHeader(element, xmlNode);
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeIncentiveElement(
    const Element& element, pugi::xml_node xmlNode) {

	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeInfoElement(const Element& element, pugi::xml_node xmlNode) {
	auto document = element.findDocument();
	if (document && document->attrs().size() == 0) {
		return;
	}

	xmlNode.append_attribute("id") = "info";
	auto aboutTitle = xmlNode.append_child("h1");
	aboutTitle.append_child(pugi::node_pcdata).set_value("File Information");
	auto dl = xmlNode.append_child("dl");

	if (auto author = document->attr("author")) {
		auto authorKey = dl.append_child("dt");
		authorKey.append_child(pugi::node_pcdata).set_value("Author");
		auto authorValue = dl.append_child("dd");
		authorValue.append_child(pugi::node_pcdata).set_value(author.value().c_str());
	}

	if (auto publisher = document->attr("publisher")) {
		auto publisherKey = dl.append_child("dt");
		publisherKey.append_child(pugi::node_pcdata).set_value("Game Publisher");
		auto publisherValue = dl.append_child("dd");
		publisherValue.append_child(pugi::node_pcdata)
		    .set_value(publisher.value().c_str());
	}

	if (auto timestamp = document->attr("timestamp")) {
		auto timestampKey = dl.append_child("dt");
		timestampKey.append_child(pugi::node_pcdata).set_value("Date Created");
		auto timestampValue = dl.append_child("dd");
		timestampValue.append_child(pugi::node_pcdata)
		    .set_value(timestamp.value().c_str());
	}

	if (auto copyright = document->attr("copyright")) {
		auto copyrightKey = dl.append_child("dt");
		copyrightKey.append_child(pugi::node_pcdata).set_value("Copyright");
		auto copyrightValue = dl.append_child("dd");
		copyrightValue.append_child(pugi::node_pcdata)
		    .set_value(copyright.value().c_str());
	}
}

void HTMLWriter::serializeLinkElement(const Element& element, pugi::xml_node xmlNode) {
	auto isLink = false;
	auto isText = false;
	pugi::xml_node map;

	if (auto parentNode = element.parent(); parentNode->isElement()) {
		auto parentElement = static_cast<Element&>(*parentNode);

		if (parentElement.elementType() == ElementType::Hyperpng) {
			auto parent = xmlNode.parent();
			auto id = parentElement.id();

			map =
			    parent.find_child_by_attribute("map", "name", std::to_string(id).c_str());

			if (!map) {
				map = parent.append_child("map");
				map.append_attribute("name") = id;
			}
		}
	}

	if (map) {
		auto x1 = element.attr("region-top-left-x");
		auto y1 = element.attr("region-top-left-y");
		auto x2 = element.attr("region-bottom-right-x");
		auto y2 = element.attr("region-bottom-right-y");

		if (!x1 || !y1 || !x2 || !y2) {
			throw WriteError("expected coordinates for hyperpng link");
		}
		auto coords = tfm::format("%s,%s,%s,%s", *x1, *y1, *x2, *y2);

		map.append_move(xmlNode);
		xmlNode.set_name("area");
		xmlNode.append_attribute("shape") = "rect";
		xmlNode.append_attribute("coords") = coords.c_str();
		xmlNode.append_attribute("href") = ("#" + element.body()).c_str();
		xmlNode.append_attribute("alt") = element.title().c_str();
	} else {
		if (element.hasPreviousSibling()) {
			auto previousNode = element.previousSibling();
			if (previousNode->isText()) {
				isText = true;
			} else {
				auto previousElement = static_cast<Element*>(previousNode);
				isLink = (previousElement->elementType() == ElementType::Link);
			}
		}

		if (!element.inlined() && (isText || isLink)) {
			xmlNode.parent().insert_child_before("br", xmlNode);
		}

		xmlNode.set_name("a");
		xmlNode.append_attribute("href") = ("#" + element.body()).c_str();
		xmlNode.append_attribute("inline") = element.inlined();
		xmlNode.append_child(pugi::node_pcdata).set_value(element.title().c_str());
	}
}

void HTMLWriter::serializeOverlayElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeSubjectElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeader(element, xmlNode);
}

void HTMLWriter::serializeTextElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeader(element, xmlNode);
}

void HTMLWriter::serializeTextNode(
    const TextNode& textNode, pugi::xml_node xmlNode) const {

	if (textNode.hasPreviousSibling()) {
		auto previousNode = textNode.previousSibling();
		if (previousNode->isElement()) {
			auto previousElement = static_cast<Element&>(*previousNode);
			auto isLink = (previousElement.elementType() == ElementType::Link);

			if (!previousElement.inlined() && isLink) {
				xmlNode.parent().insert_child_before("br", xmlNode);
			}
		}
	}

	auto body = textNode.body();
	auto lines = Strings::split(body, "\n");

	for (std::size_t i = 0; i < lines.size(); ++i) {
		if (i > 0) {
			xmlNode.append_child("br");
		}
		xmlNode.append_child(pugi::node_pcdata).set_value(lines[i].c_str());
	}

	if (textNode.hasFormat(TextFormat::Hyperlink)) {
		xmlNode.set_name("a");
		std::smatch matches;
		if (std::regex_match(body, matches, Regex::EmailAddress)) {
			body = "mailto:" + body;
		} else if (!std::regex_match(body, matches, Regex::URL)) {
			body = "http://" + body;
		}
		xmlNode.append_attribute("href") = body.c_str();
	}

	std::vector<std::string> classes;

	if (textNode.hasFormat(TextFormat::Overflow)) {
		classes.push_back("overflow");
	}
	if (textNode.hasFormat(TextFormat::Monospace)) {
		classes.push_back("monospace");
	}
	if (!classes.empty()) {
		xmlNode.append_attribute("class") = Strings::join(classes, " ").c_str();
	}
}

void HTMLWriter::appendBody(const Element& element, pugi::xml_node xmlNode) const {
	const auto& body = element.body();
	if (body.empty()) {
		return;
	}
	auto p = xmlNode.append_child("p");
	p.append_child(pugi::node_pcdata).set_value(body.c_str());
}

void HTMLWriter::appendHeader(const Element& element, pugi::xml_node xmlNode) const {
	auto depth = element.depth();
	auto headerDepth = std::to_string((depth <= 6) ? depth : 6);
	auto title = xmlNode.append_child(("h" + headerDepth).c_str());
	title.append_child(pugi::node_pcdata).set_value(element.title().c_str());
}

void HTMLWriter::appendMedia(const Element& element, pugi::xml_node xmlNode) const {
	std::string name;
	std::string contentType;

	switch (element.elementType()) {
	case ElementType::Gifa:
		name = "img";
		contentType = "image/gif";
		break;
	case ElementType::Hyperpng:
		name = "img";
		contentType = "image/png";
		break;
	case ElementType::Overlay:
		name = "img";
		contentType = "image/png";
		break;
	case ElementType::Sound:
		name = "audio";
		contentType = "audio/wave";
		break;
	default:
		throw WriteError("unexpected media type: %s", element.elementTypeString());
	}

	auto container = xmlNode.append_child("div");
	auto media = container.append_child(name.c_str());
	auto base64 = Strings::toBase64(element.body());
	auto dataURI = tfm::format("data:%s;base64,%s", contentType, base64);
	media.append_attribute("src") = dataURI.c_str();

	if (element.elementType() == ElementType::Hyperpng && element.hasFirstChild()) {
		media.append_attribute("usemap") = tfm::format("#%d", element.id()).c_str();
	}

	if (element.elementType() == ElementType::Sound) {
		media.append_attribute("controls");
	}
}

void HTMLWriter::appendVisibility(
    const Traits::Visibility& node, pugi::xml_node xmlNode) const {

	if (node.visibility() == VisibilityType::All) {
		return;
	}
	xmlNode.append_attribute("class") = ("visibility-" + node.visibilityString()).c_str();
}

HTMLWriter::Serializer::Serializer() {
	map_.emplace(ElementType::Blank, &HTMLWriter::serializeBlankElement);
	map_.emplace(ElementType::Comment, &HTMLWriter::serializeCommentElement);
	map_.emplace(ElementType::Credit, &HTMLWriter::serializeCommentElement);
	map_.emplace(ElementType::Gifa, &HTMLWriter::serializeDataElement);
	map_.emplace(ElementType::Hint, &HTMLWriter::serializeHintElement);
	map_.emplace(ElementType::Hyperpng, &HTMLWriter::serializeHyperpngElement);
	map_.emplace(ElementType::Incentive, &HTMLWriter::serializeIncentiveElement);
	map_.emplace(ElementType::Info, &HTMLWriter::serializeInfoElement);
	map_.emplace(ElementType::Link, &HTMLWriter::serializeLinkElement);
	map_.emplace(ElementType::Nesthint, &HTMLWriter::serializeHintElement);
	map_.emplace(ElementType::Overlay, &HTMLWriter::serializeOverlayElement);
	map_.emplace(ElementType::Sound, &HTMLWriter::serializeDataElement);
	map_.emplace(ElementType::Subject, &HTMLWriter::serializeSubjectElement);
	map_.emplace(ElementType::Text, &HTMLWriter::serializeTextElement);
	map_.emplace(ElementType::Version, &HTMLWriter::serializeCommentElement);
}

void HTMLWriter::Serializer::invoke(
    HTMLWriter& writer, const Element& element, pugi::xml_node xmlNode) {

	auto func = map_.at(element.elementType());
	(writer.*func)(element, xmlNode);
}

//------------------------------- JSONWriter --------------------------------//

JSONWriter::JSONWriter(std::ostream& out, const Options options) : Writer(out, options) {}

void JSONWriter::write(const Document& document) {
	Json::Value root{Json::objectValue};
	this->serialize(document, root);
	out_ << root << std::endl;
}

void JSONWriter::serialize(const Document& document, Json::Value& root) const {
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
		case NodeType::Break:
			(*parent)["children"].append("-");
			break;
		case NodeType::Document: {
			if (nodeDepth == 0) {
				object = root;
			}
			const auto& d = static_cast<const Document&>(node);
			this->serializeDocument(d, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			this->serializeElement(element, object);
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Group: {
			(*parent)["children"].append(object);
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			this->serializeTextNode(textNode, object);
			(*parent)["children"].append(object);
			break;
		}
		default:
			throw WriteError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}
}

void JSONWriter::serializeDocument(const Document& document, Json::Value& object) const {
	object["title"] = document.title();
	object["version"] = document.versionString();

	if (document.version() > VersionType::Version88a) {
		object["validChecksum"] = document.validChecksum();
	}
	object["type"] = document.nodeTypeString();

	// Build attributes map
	const auto& attrs = document.attrs();
	if (!attrs.empty()) {
		Json::Value map = {Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
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

	object["inline"] = element.inlined();
	object["visibility"] = element.visibilityString();
	object["type"] = Element::typeString(element.elementType());

	// Build attributes map
	const auto& attrs = element.attrs();
	if (!attrs.empty()) {
		Json::Value map{Json::objectValue};
		this->serializeMap(attrs, map);
		object["attributes"] = map;
	}
}

void JSONWriter::serializeTextNode(const TextNode& textNode, Json::Value& object) const {
	object["body"] = textNode.body();

	auto attributes = Json::Value(Json::objectValue);
	attributes["overflow"] = textNode.hasFormat(TextFormat::Overflow);
	attributes["monospace"] = textNode.hasFormat(TextFormat::Monospace);
	attributes["hyperlink"] = textNode.hasFormat(TextFormat::Hyperlink);
	object["attributes"] = attributes;
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

UHSWriter::Serializer UHSWriter::serializer_;

UHSWriter::UHSWriter(std::ostream& out, const Options options) : Writer(out, options) {}

void UHSWriter::write(const Document& document) {
	std::string buffer;
	buffer.reserve(InitialBufferLength);

	if (options_.mode == VersionType::Version88a) {
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
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(*node);
			const auto elementType = element.elementType();
			std::string title;

			switch (elementType) {
			case ElementType::Hint:
				if (previousType == ElementType::Subject) {
					line += numPreviousChildren * 2;
				} else {
					line += numPreviousChildren;
				}
				title = Strings::rtrim(element.title(), '?');
				break;
			case ElementType::Subject:
				line += numPreviousChildren * 2;
				title = element.title();
				break;
			default:
				throw WriteError(
				    "unexpected element type: %s", Element::typeString(elementType));
			}

			if (!options_.debug) {
				title = codec_.encode88a(title);
			}
			buffer += title;
			buffer += EOL;
			buffer += std::to_string(line);
			buffer += EOL;

			previousType = elementType;
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(*node);
			auto body = textNode.body();

			if (!options_.debug) {
				body = codec_.encode88a(body);
			}
			buffer += body;
			buffer += EOL;

			line += numPreviousChildren - 1; // Simplifies to line -= 1 after first pass

			if (lastHintTextLine == 0) {
				lastHintTextLine = line;
			}
			firstHintTextLine = line; // Work backwards from last
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
	if (document_->isVersion(VersionType::Version88a)) {
		this->convertTo91a();
	}

	key_ = codec_.createKey(document_->title());

	for (auto node = document_->firstChild(); node; node = node->nextSibling()) {
		switch (node->nodeType()) {
		case NodeType::Break:
			throw WriteError("unexpected break node");
		case NodeType::Document: {
			const auto& document = static_cast<const Document&>(*node);
			if (!document.isVersion(VersionType::Version88a)) {
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
		case NodeType::Group:
			throw WriteError("unexpected group node");
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(*node);
			throw WriteError("unexpected text node: %s", textNode.body());
		}
		}
	}

	this->updateLinkTargets(out);

	out += Token::DataSep;

	this->serializeData(out);
	this->serializeCRC(out);
}

int UHSWriter::serializeElement(Element& element, std::string& out) {
	element.line(currentLine_);
	return serializer_.invoke(*this, element, out);
}

int UHSWriter::serializeBlankElement(Element& element, std::string& out) {
	auto length = InitialElementLength;

	currentLine_ += length;
	element.length(length);
	this->serializeElementHeader(element, out);

	return length;
}

int UHSWriter::serializeCommentElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	if (const auto& body = element.body(); !body.empty()) {
		buffer += Strings::wrap(body, EOL, LineLength, length) + EOL;
	}
	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

// GIFs don't appear to be displayed by the official reader any longer
int UHSWriter::serializeDataElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto& body = element.body();

	auto dataAddress = this->createDataAddress(body.length());
	this->addData(element);
	++length;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += dataAddress;

	return length;
}

int UHSWriter::serializeHintChild(Node& node, Element& parentElement,
    std::map<const int, TextFormat>& formats, std::string& textBuffer, std::string& out) {

	auto length = 0;
	auto depth = parentElement.depth();
	const auto elementType = parentElement.elementType();

	if (node.isElement() && elementType == ElementType::Nesthint) {
		auto& child = static_cast<Element&>(node);
		auto followsTextNode = false;

		if (node.hasPreviousSibling()) {
			auto previousNode = node.previousSibling();
			followsTextNode = previousNode->isText();
		}
		if (!followsTextNode && child.inlined()) {
			textBuffer += Token::InlineBegin;
		}
	}

	if (!node.isText() && !textBuffer.empty()) {
		auto childLength = 0;
		auto wrapped = Strings::wrap(textBuffer, "\n", LineLength, childLength);
		out += this->encodeText(wrapped, elementType);
		length += childLength;
		currentLine_ += childLength;
		textBuffer.clear();
	}

	if (node.isElement() && elementType == ElementType::Nesthint) {
		auto& child = static_cast<Element&>(node);
		depth = parentElement.depth();

		out += Token::NestedElementSep;
		out += EOL;
		++length;
		++currentLine_;

		auto childLength = this->serializeElement(child, out);
		length += childLength;
	} else if (node.isBreak()) {
		out += Token::NestedTextSep;
		out += EOL;
		++length;
		++currentLine_;
	} else if (node.isGroup()) {
		for (auto child = node.firstChild(); child; child = child->nextSibling()) {
			length +=
			    this->serializeHintChild(*child, parentElement, formats, textBuffer, out);
		}
	} else if (node.isText()) {
		const auto& textNode = static_cast<const TextNode&>(node);
		textBuffer += this->formatText(textNode, formats[depth]);
		formats[depth] = textNode.format();
	} else {
		throw WriteError("unexpected node type: %s", node.nodeTypeString());
	}

	return length;
}

int UHSWriter::serializeHintElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;
	std::map<const int, TextFormat> formats;
	std::string textBuffer;

	currentLine_ += length;
	const auto elementType = element.elementType();

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		length += this->serializeHintChild(*node, element, formats, textBuffer, buffer);
	}

	if (!textBuffer.empty()) {
		auto childLength = 0;
		auto wrapped = Strings::wrap(textBuffer, "\n", LineLength, childLength);
		buffer += this->encodeText(wrapped, elementType);
		length += childLength;
		currentLine_ += childLength;
	}

	element.length(length);
	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

// int UHSWriter::serializeHintElement(Element& element, std::string& out) {
// 	auto length = InitialElementLength;
// 	std::string buffer;
// 	auto childLength = 0;
// 	std::string textBuffer;
// 	std::map<const int, TextFormat> formats;
// 	auto depth = element.depth();

// 	currentLine_ += length;
// 	const auto elementType = element.elementType();

// 	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
// 		if (node->isElement() && elementType == ElementType::Nesthint) {
// 			auto& child = static_cast<Element&>(*node);
// 			auto followsTextNode = false;

// 			if (node->hasPreviousSibling()) {
// 				auto previousNode = node->previousSibling();
// 				followsTextNode = previousNode->isText();
// 			}
// 			if (!followsTextNode && child.inlined()) {
// 				textBuffer += Token::InlineBegin;
// 			}
// 		}

// 		if (!node->isText() && !textBuffer.empty()) {
// 			auto wrapped = Strings::wrap(textBuffer, "\n", LineLength, childLength);
// 			buffer += this->encodeText(wrapped, elementType);
// 			length += childLength;
// 			currentLine_ += childLength;
// 			textBuffer.clear();
// 		}

// 		childLength = 0;

// 		if (node->isElement() && elementType == ElementType::Nesthint) {
// 			auto& child = static_cast<Element&>(*node);
// 			depth = element.depth();

// 			buffer += Token::NestedElementSep;
// 			buffer += EOL;
// 			++length;
// 			++currentLine_;

// 			childLength += this->serializeElement(child, buffer);
// 			length += childLength;
// 		} else if (node->isBreak()) {
// 			buffer += Token::NestedTextSep;
// 			buffer += EOL;
// 			++length;
// 			++currentLine_;
// 		} else if (node->isGroup()) {
// 			// TODO
// 		} else if (node->isText()) {
// 			const auto& textNode = static_cast<const TextNode&>(*node);
// 			textBuffer += this->formatText(textNode, formats[depth]);
// 			formats[depth] = textNode.format();
// 		} else {
// 			throw WriteError("unexpected node type: %s", node->nodeTypeString());
// 		}
// 	}

// 	if (!textBuffer.empty()) {
// 		auto wrapped = Strings::wrap(textBuffer, "\n", LineLength, childLength);
// 		buffer += this->encodeText(wrapped, elementType);
// 		length += childLength;
// 		currentLine_ += childLength;
// 	}

// 	element.length(length);
// 	this->serializeElementHeader(element, out);
// 	out += buffer;

// 	return length;
// }

int UHSWriter::serializeHyperpngElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto& body = element.body();

	auto buffer = this->createDataAddress(body.length());
	this->addData(element);
	++length;
	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}

		auto& child = static_cast<Element&>(*node);

		std::vector<std::pair<std::string, int>> coords{
		    {"region-top-left-x", 0},
		    {"region-top-left-y", 0},
		    {"region-bottom-right-x", 0},
		    {"region-bottom-right-y", 0},
		};
		for (auto& [attr, coord] : coords) {
			if (auto value = child.attr(attr)) {
				try {
					coord = Strings::toInt(*value);
				} catch (const Error& err) {
					throw WriteError("invalid coordinate: %s", *value);
				}
			}
		}

		auto x1 = coords[0].second;
		auto y1 = coords[1].second;
		auto x2 = coords[2].second;
		auto y2 = coords[3].second;

		buffer += this->createRegion(x1, y1, x2, y2);
		++length;
		++currentLine_;

		length += this->serializeElement(child, buffer);
	}

	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeIncentiveElement(Element& element, std::string& out) {
	if (!options_.preserve) {
		return 0;
	}

	auto length = InitialElementLength;
	std::string buffer;

	std::map<const ElementType, bool> excluded = {
	    {ElementType::Incentive, true},
	    {ElementType::Info, true},
	    {ElementType::Version, true},
	};

	std::vector<std::string> instructions;

	for (auto& node : *document_) {
		if (node.nodeType() != NodeType::Element) {
			continue;
		}

		const auto& candidate = static_cast<const Element&>(node);
		if (excluded[candidate.elementType()]) {
			continue;
		}

		std::string flag;
		switch (candidate.visibility()) {
		case VisibilityType::RegisteredOnly:
			flag = Token::RegisteredOnly;
			break;
		case VisibilityType::UnregisteredOnly:
			flag = Token::UnregisteredOnly;
			break;
		default:
			continue; // Ignore
		}

		instructions.push_back(std::to_string(candidate.line()) + flag);
	}

	if (instructions.empty()) {
		return 0; // Elide element if empty
	}

	auto decoded = Strings::join(instructions, " ");
	auto wrapped = Strings::wrap(decoded, "\n", LineLength, length);
	auto lines = Strings::split(wrapped, "\n");
	if (!options_.debug) {
		for (auto& line : lines) {
			line = codec_.encode96a(line, key_, false);
		}
	}
	buffer += Strings::join(lines, EOL);
	buffer += EOL;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;
	return length;
}

int UHSWriter::serializeInfoElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	buffer += InfoLengthMarker;
	buffer += EOL;
	++length;

	const auto now = std::time(nullptr);
	const auto tm = std::localtime(&now);

	char formatted[10];
	const auto dateLength = std::strftime(formatted, 10, "%d-%b-%y", tm);
	buffer += "date=" + std::string(formatted, dateLength) + EOL;
	++length;

	const auto timeLength = std::strftime(formatted, 9, "%H:%M:%S", tm);
	buffer += "time=" + std::string(formatted, timeLength) + EOL;
	++length;

	std::map<const std::string, bool> whitelisted = {
	    {"author", true},
	    {"author-note", true},
	    {"copyright", true},
	    {"game-note", true},
	    {"publisher", true},
	};

	for (const auto& [key, value] : document_->attrs()) {
		if (whitelisted[key] && !value.empty()) {
			buffer += Strings::wrap(value, EOL, LineLength, length, key + "=");
			buffer += EOL;
		}
	}

	if (auto notice = document_->attr("notice")) {
		buffer += Strings::wrap(*notice, EOL, LineLength, length, Token::NoticePrefix);
		buffer += EOL;
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeLinkElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	buffer += LinkMarker;
	buffer += EOL;
	++length;

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
	} else {
		throw WriteError("target element not found for id %d", targetID);
	}

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeOverlayElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto& body = element.body();

	std::vector<std::pair<std::string, int>> coords = {
	    {"image-x", 0},
	    {"image-y", 0},
	};
	for (auto& [attr, coord] : coords) {
		if (auto value = element.attr(attr)) {
			try {
				coord = Strings::toInt(*value);
			} catch (const Error& err) {
				throw WriteError("invalid coordinate: %s", *value);
			}
		}
	}

	auto x = coords[0].second;
	auto y = coords[1].second;

	auto buffer = this->createOverlayAddress(body.length(), x, y);
	this->addData(element);
	++length;

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeSubjectElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	std::string buffer;

	currentLine_ += length;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}
		auto& child = static_cast<Element&>(*node);
		length += this->serializeElement(child, buffer);
	}
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

int UHSWriter::serializeTextElement(Element& element, std::string& out) {
	auto length = InitialElementLength;
	const auto elementType = element.elementType();
	std::string textBuffer;
	std::string buffer;
	auto previousFormat = TextFormat::None;

	for (auto node = element.firstChild(); node; node = node->nextSibling()) {
		if (node->isText()) {
			const auto& textNode = static_cast<const TextNode&>(*node);
			textBuffer += this->formatText(textNode, previousFormat);
			previousFormat = textNode.format();
		} else if (!node->isGroup()) {
			throw WriteError("unexpected node type: %s", node->nodeTypeString());
		}
	}

	auto body = this->encodeText(textBuffer, elementType);

	const auto textFormat = ((*element.attr("typeface") == "monospace") ? "1 " : "0 ");
	buffer += this->createDataAddress(body.length(), textFormat);
	++length;

	data_.emplace(std::make_pair(elementType, body));

	currentLine_ += length;
	element.length(length);

	this->serializeElementHeader(element, out);
	out += buffer;

	return length;
}

void UHSWriter::serializeElementHeader(Element& element, std::string& out) const {
	out += std::to_string(element.length());
	out += ' ';
	out += element.elementTypeString();
	out += EOL;

	if (const auto& title = element.title(); title.empty()) {
		out += '-';
	} else {
		out += title;
	}
	out += EOL;
}

void UHSWriter::updateLinkTargets(std::string& out) const {
	// Because we're shifting characters, we move backwards through the file in order to
	// reduce the total number of bytes copied within the output buffer.
	// TODO: I don't think this is actually true
	const auto end = deferredLinks_.crend();
	for (auto it = deferredLinks_.crbegin(); it != end; ++it) {
		const auto target = *it;
		assert(target);

		if (!target->isElement()) {
			// TODO: Warn
			continue;
		}

		auto targetElement = static_cast<Element&>(*target);
		const auto pos = out.rfind(LinkMarker);
		const auto targetLine = std::to_string(targetElement.line());
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

std::string UHSWriter::formatText(
    const TextNode& textNode, const TextFormat previousFormat) {

	auto body = textNode.body();

	if (textNode.hasFormat(TextFormat::Hyperlink)) {
		body = Token::HyperlinkBegin + body + Token::HyperlinkEnd;
	}

	if (textNode.hasFormat(TextFormat::Monospace)
	    && !hasFormat(previousFormat, TextFormat::Monospace)) {

		body = Token::MonospaceBegin + body;
	} else if (!textNode.hasFormat(TextFormat::Monospace)
	           && hasFormat(previousFormat, TextFormat::Monospace)) {

		body = Token::MonospaceEnd + body;
	}

	if (textNode.hasFormat(TextFormat::Overflow)
	    && !hasFormat(previousFormat, TextFormat::Overflow)) {

		body = Token::OverflowBegin + body;
	} else if (!textNode.hasFormat(TextFormat::Overflow)
	           && hasFormat(previousFormat, TextFormat::Overflow)) {

		body = Token::OverflowEnd + body;
	}

	if (textNode.hasNextSibling()) {
		if (auto node = textNode.nextSibling(); node->isElement()) {
			auto& nextElement = static_cast<Element&>(*node);
			if (nextElement.inlined()) {
				if (body.length() > 0 && !Strings::endsWithAttachedPunctuation(body)) {
					body += ' ';
				}
				body += Token::InlineBegin;
			}
		}
	}

	if (textNode.hasPreviousSibling()) {
		if (auto node = textNode.previousSibling(); node->isElement()) {
			auto& previousElement = static_cast<Element&>(*node);
			if (previousElement.inlined()) {
				std::string temp;
				temp += Token::InlineEnd;

				if (body.length() > 0 && !Strings::beginsWithAttachedPunctuation(body)) {
					temp += ' ';
				}
				temp += body;
				body = temp;
			}
		}
	}

	return body;
}

std::string UHSWriter::encodeText(const std::string text, const ElementType parentType) {
	std::string buffer;

	auto lines = Strings::split(text, "\n");
	auto numLines = lines.size();

	for (std::size_t i = 0; i < numLines; ++i) {
		auto& line = lines[i];

		if (line == "") {
			line = Token::ParagraphSep;
		}

		// Account for weird edge case
		if (i + 1 < numLines) {
			if (lines[i + 1].starts_with(Token::HyperlinkBegin)
			    && !Strings::endsWithAttachedPunctuation(lines[i])) {
				lines[i] += ' ';
			}
			if (lines[i + 1].starts_with(Token::InlineBegin)
			    && !Strings::endsWithAttachedPunctuation(lines[i])) {
				lines[i] += ' ';
			}
		}

		if (options_.debug) {
			continue; // Don't encode text
		}

		if (parentType == ElementType::Hint) {
			line = codec_.encode88a(line);
		} else if (parentType == ElementType::Nesthint) {
			line = codec_.encode96a(line, key_, false);
		} else if (parentType == ElementType::Text) {
			line = codec_.encode96a(line, key_, true);
		}
	}

	buffer += Strings::join(lines, EOL);
	buffer += EOL; // TODO: Remove this and replace with better system

	return buffer;
}

void UHSWriter::addData(const Element& element) {
	data_.emplace(std::make_pair(element.elementType(), element.body()));
}

std::string UHSWriter::createDataAddress(
    std::size_t bodyLength, std::string textFormat) const {

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

std::string UHSWriter::createOverlayAddress(std::size_t bodyLength, int x, int y) const {
	std::string out;

	out += DataAddressMarker;
	out += ' ';
	out += std::string(FileSizeLength, '0');
	out += ' ';
	std::ostringstream buffer;
	buffer << std::setfill('0') << std::setw(MediaSizeLength) << bodyLength;
	buffer << ' ' << std::setfill('0') << std::setw(RegionSizeLength) << x;
	buffer << ' ' << std::setfill('0') << std::setw(RegionSizeLength) << y;
	out += buffer.str();
	out += EOL;

	return out;
}

std::string UHSWriter::createRegion(int x1, int y1, int x2, int y2) const {
	std::ostringstream buffer;
	std::vector<const int> coords = {x1, y1, x2, y2};
	auto continuation = false;

	for (auto coord : coords) {
		if (continuation) {
			buffer << ' ';
		}
		auto width = RegionSizeLength;
		if (coord < 0) {
			buffer << '-';
			width -= 1;
			coord = std::abs(coord);
		}
		buffer << std::setfill('0') << std::setw(width) << coord;
		continuation = true;
	}
	buffer << EOL;

	return buffer.str();
}

void UHSWriter::convertTo91a() {
	// Re-parent under subject node
	auto container = Element::create(ElementType::Subject);
	container->title(document_->title());
	for (auto node = document_->firstChild(); node; node = document_->firstChild()) {
		if (node->nodeType() != NodeType::Element) {
			throw WriteError("expected element, found %s node", node->nodeTypeString());
		}
		container->appendChild(document_->removeChild(node));
	}
	document_->appendChild(container);

	// Prepend minimal 88a header
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	auto subject = Element::create(ElementType::Subject);
	subject->title(codec_.decode88a("-"));
	auto hint = Element::create(ElementType::Hint);
	hint->title(codec_.decode88a("-"));
	hint->appendChild(codec_.decode88a("-"));
	subject->appendChild(hint);
	header->appendChild(subject);
	document_->insertBefore(header, document_->firstChild());

	// Set version to 91a
	document_->version(VersionType::Version91a);
	auto version = Element::create(ElementType::Version);
	version->title("91a");
	if (auto notice = document_->attr("notice")) {
		version->body(*notice);
	}
	document_->appendChild(version);
}

UHSWriter::Serializer::Serializer() {
	map_.emplace(ElementType::Blank, &UHSWriter::serializeBlankElement);
	map_.emplace(ElementType::Comment, &UHSWriter::serializeCommentElement);
	map_.emplace(ElementType::Credit, &UHSWriter::serializeCommentElement);
	map_.emplace(ElementType::Gifa, &UHSWriter::serializeDataElement);
	map_.emplace(ElementType::Hint, &UHSWriter::serializeHintElement);
	map_.emplace(ElementType::Hyperpng, &UHSWriter::serializeHyperpngElement);
	map_.emplace(ElementType::Incentive, &UHSWriter::serializeIncentiveElement);
	map_.emplace(ElementType::Info, &UHSWriter::serializeInfoElement);
	map_.emplace(ElementType::Link, &UHSWriter::serializeLinkElement);
	map_.emplace(ElementType::Nesthint, &UHSWriter::serializeHintElement);
	map_.emplace(ElementType::Overlay, &UHSWriter::serializeOverlayElement);
	map_.emplace(ElementType::Sound, &UHSWriter::serializeDataElement);
	map_.emplace(ElementType::Subject, &UHSWriter::serializeSubjectElement);
	map_.emplace(ElementType::Text, &UHSWriter::serializeTextElement);
	map_.emplace(ElementType::Version, &UHSWriter::serializeCommentElement);
}

int UHSWriter::Serializer::invoke(UHSWriter& writer, Element& element, std::string& out) {
	auto func = map_.at(element.elementType());
	return (writer.*func)(element, out);
}

} // namespace UHS

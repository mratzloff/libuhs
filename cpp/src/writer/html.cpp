#include "uhs.h"

namespace UHS {

HTMLWriter::Serializer HTMLWriter::serializer_;

HTMLWriter::HTMLWriter(std::ostream& out, const Options options) : Writer(out, options) {
	js_ =
#include "js.h"
	    ;

	mediaContentTypes_.emplace(ElementType::Gifa, "image/gif");
	mediaContentTypes_.emplace(ElementType::Hyperpng, "image/png");
	mediaContentTypes_.emplace(ElementType::Overlay, "image/png");
	mediaContentTypes_.emplace(ElementType::Sound, "audio/wave");

	mediaTagTypes_.emplace(ElementType::Gifa, "img");
	mediaTagTypes_.emplace(ElementType::Hyperpng, "img");
	mediaTagTypes_.emplace(ElementType::Overlay, "img");
	mediaTagTypes_.emplace(ElementType::Sound, "audio");
}

void HTMLWriter::write(const Document& document) {
	pugi::xml_document xml;
	this->serialize(document, xml);
	xml.save(out_, "", pugi::format_raw | pugi::format_no_declaration);
}

void HTMLWriter::serialize(const Document& document, pugi::xml_document& xml) {
	xml.append_child(pugi::node_doctype).set_value("html");
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

	auto script = head.append_child("script");
	script.append_child(pugi::node_pcdata).set_value("//");
	script.append_child(pugi::node_cdata).set_value(js_.c_str());

	auto style = head.append_child("style");
	style.append_child(pugi::node_pcdata)
	    .set_value(
	        ".hidden { visibility: hidden; }"
	        ".hyperpng { position: relative; }"
	        ".image-container { position: relative; }"
	        ".overflow { white-space: nowrap; }"
	        ".overflow.monospace { white-space: pre; }"
	        ".overlay { position: absolute; }"
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
	this->appendHeading(element, xmlNode);
	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeDataElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeHintElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeading(element, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeHyperpngElement(
    const Element& element, pugi::xml_node xmlNode) {

	this->appendHeading(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);

	if (element.hasFirstChild()) {
		auto container = xmlNode.append_child("div");
		container.append_attribute("class") = "image-container";
		container.append_move(media);
		media.append_attribute("class") = "hyperpng";
		media.append_attribute("usemap") = tfm::format("#%d", element.id()).c_str();
	}
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

	if (auto container = this->findImageContainer(element, xmlNode)) {
		xmlNode.set_name("area");
		auto map = this->findOrCreateMap(element, *container);
		map.append_move(xmlNode);
		this->populateArea(element, xmlNode);
		xmlNode.append_attribute("href") = ("#" + element.body()).c_str();
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
	static auto jsTemplate =
	    "javascript:document.getElementById('%d').classList.remove('hidden')";

	// Re-parent node
	auto container = findImageContainer(element, xmlNode);
	if (!container) {
		throw WriteError("overlay parent must be a hyperpng element");
	}
	container->append_move(xmlNode);

	// Set node attributes
	auto elementType = element.elementType();
	xmlNode.set_name(mediaTagTypes_.at(elementType).c_str());
	auto dataURI = this->getDataURI(mediaContentTypes_.at(elementType), element.body());
	xmlNode.append_attribute("src") = dataURI.c_str();
	xmlNode.append_attribute("class") = "overlay hidden";

	auto [x, y] = this->getImageSize(element);
	xmlNode.append_attribute("style") =
	    tfm::format("left: %dpx; top: %dpx", x, y).c_str();

	// Create area tag for map
	auto map = this->findOrCreateMap(element, *container);
	auto area = map.append_child("area");
	this->populateArea(element, area);
	area.append_attribute("href") = tfm::format(jsTemplate, element.id()).c_str();
}

void HTMLWriter::serializeSoundElement(const Element& element, pugi::xml_node xmlNode) {
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("controls");
}

void HTMLWriter::serializeSubjectElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeading(element, xmlNode);
}

void HTMLWriter::serializeTextElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendHeading(element, xmlNode);
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

std::optional<pugi::xml_node> HTMLWriter::appendBody(
    const Element& element, pugi::xml_node xmlNode) const {

	const auto& body = element.body();
	if (body.empty()) {
		return std::nullopt;
	}
	auto p = xmlNode.append_child("p");
	p.append_child(pugi::node_pcdata).set_value(body.c_str());

	return p;
}

pugi::xml_node HTMLWriter::appendHeading(
    const Element& element, pugi::xml_node xmlNode) const {

	auto depth = element.depth();
	auto headerDepth = std::to_string((depth <= 6) ? depth : 6);
	auto title = xmlNode.append_child(("h" + headerDepth).c_str());
	title.append_child(pugi::node_pcdata).set_value(element.title().c_str());

	return title;
}

pugi::xml_node HTMLWriter::appendMedia(
    const Element& element, pugi::xml_node xmlNode) const {

	std::string contentType;
	std::string tagName;
	auto elementType = element.elementType();

	try {
		contentType = mediaContentTypes_.at(elementType);
		tagName = mediaTagTypes_.at(elementType);
	} catch (const std::out_of_range&) {
		throw WriteError("unexpected media type: %s", element.elementTypeString());
	}

	auto media = xmlNode.append_child(tagName.c_str());
	auto dataURI = this->getDataURI(contentType, element.body());
	media.append_attribute("src") = dataURI.c_str();

	return media;
}

void HTMLWriter::appendVisibility(
    const Traits::Visibility& node, pugi::xml_node xmlNode) const {

	if (node.visibility() == VisibilityType::All) {
		return;
	}
	xmlNode.append_attribute("class") = ("visibility-" + node.visibilityString()).c_str();
}

std::optional<pugi::xml_node> HTMLWriter::findImageContainer(
    const Element& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		return std::nullopt;
	}

	auto parent = xmlNode.parent();
	auto container = parent.find_child_by_attribute("div", "class", "image-container");

	return container;
}

pugi::xml_node HTMLWriter::findOrCreateMap(
    const Element& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		throw WriteError(
		    "expected hyperpng parent; got %s", parentElement->elementTypeString());
	}

	auto container = this->findImageContainer(element, xmlNode);
	if (!container) {
		throw WriteError(
		    "hyperpng %s must have an image container", element.elementTypeString());
	}

	auto id = parentElement->id();
	auto map =
	    container->find_child_by_attribute("map", "name", std::to_string(id).c_str());

	if (!map) {
		map = container->append_child("map");
		map.append_attribute("name") = id;
	}

	return map;
}

std::string HTMLWriter::getDataURI(
    const std::string& contentType, const std::string& data) const {

	return tfm::format("data:%s;base64,%s", contentType, Strings::toBase64(data));
}

std::tuple<int, int> HTMLWriter::getImageSize(const Element& element) const {
	auto x = element.attr("image-x");
	auto y = element.attr("image-y");

	if (!x || !y) {
		throw WriteError("expected size for %s", element.elementTypeString());
	}

	return std::make_tuple(Strings::toInt(*x), Strings::toInt(*y));
}

Element* HTMLWriter::getParentElement(const Element& element) const {
	auto parentNode = element.parent();
	if (!parentNode || !parentNode->isElement()) {
		return nullptr;
	}
	return static_cast<Element*>(parentNode);
}

std::tuple<int, int, int, int> HTMLWriter::getRegionCoordinates(
    const Element& element) const {
	auto x1 = element.attr("region-top-left-x");
	auto y1 = element.attr("region-top-left-y");
	auto x2 = element.attr("region-bottom-right-x");
	auto y2 = element.attr("region-bottom-right-y");

	if (!x1 || !y1 || !x2 || !y2) {
		throw WriteError(
		    "expected coordinates for hyperpng %s", element.elementTypeString());
	}

	return std::make_tuple(Strings::toInt(*x1),
	    Strings::toInt(*y1),
	    Strings::toInt(*x2),
	    Strings::toInt(*y2));
}

void HTMLWriter::populateArea(const Element& element, pugi::xml_node area) const {
	auto [x1, y1, x2, y2] = this->getRegionCoordinates(element);
	auto coordsString = tfm::format("%d,%d,%d,%d", x1, y1, x2, y2);

	area.append_attribute("alt") = element.title().c_str();
	area.append_attribute("coords") = coordsString.c_str();
	area.append_attribute("shape") = "rect";
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
	map_.emplace(ElementType::Sound, &HTMLWriter::serializeSoundElement);
	map_.emplace(ElementType::Subject, &HTMLWriter::serializeSubjectElement);
	map_.emplace(ElementType::Text, &HTMLWriter::serializeTextElement);
	map_.emplace(ElementType::Version, &HTMLWriter::serializeCommentElement);
}

void HTMLWriter::Serializer::invoke(
    HTMLWriter& writer, const Element& element, pugi::xml_node xmlNode) {

	auto func = map_.at(element.elementType());
	(writer.*func)(element, xmlNode);
}

} // namespace UHS
#include "uhs.h"

namespace UHS {

HTMLWriter::Serializer HTMLWriter::serializer_;

HTMLWriter::HTMLWriter(std::ostream& out, const Options options) : Writer(out, options) {
	css_ =
#include "css.h"
	    ;
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
	auto root = this->createHTMLDocument(document, xml);

	NodeMap parents;
	auto parent = root;
	auto depth = 0;

	for (const auto& node : document) {
		auto nodeDepth = node.depth();
		parent = this->findXMLParent(node, parent, parents, depth);

		// Serialize node
		switch (node.nodeType()) {
		case NodeType::Break:
			break;
		case NodeType::Document: {
			const auto& d = static_cast<const Document&>(node);
			pugi::xml_node xmlNode;

			if (strcmp(parent.name(), "ol") == 0) {
				auto li = parent.append_child("li");
				if (d.visibility() == VisibilityType::None) {
					this->appendClassNames(li, {"visibility-none"});
				}
				xmlNode = li.append_child("section");
			} else {
				xmlNode = parent.append_child("section");
			}

			parents.emplace(&node, xmlNode);
			this->serializeDocument(d, xmlNode);
			break;
		}
		case NodeType::Element: {
			const auto& element = static_cast<const Element&>(node);
			pugi::xml_node xmlNode;

			if (strcmp(parent.name(), "ol") == 0) {
				auto li = parent.append_child("li");
				if (element.visibility() == VisibilityType::None) {
					this->appendClassNames(li, {"visibility-none"});
				}
				xmlNode = li.append_child("div");
			} else {
				xmlNode = parent.append_child("div");
			}

			parents.emplace(&node, xmlNode);
			this->serializeElement(element, xmlNode);
			break;
		}
		case NodeType::Group: {
			pugi::xml_node xmlNode;
			ElementType parentElementType = ElementType::Unknown;
			assert(node.hasParent());
			auto nodeParent = node.parent();

			if (nodeParent->isElement()) {
				auto parentElement = static_cast<const Element&>(*nodeParent);
				parentElementType = parentElement.elementType();
			}

			if (parentElementType == ElementType::Text) {
				xmlNode = parent.append_child("p");
			} else {
				auto li = parent.append_child("li");
				xmlNode = li.append_child("div");
			}

			if (parentElementType == ElementType::Hint
			    || parentElementType == ElementType::Nesthint) {

				this->appendClassNames(xmlNode, {"hint"});
			}

			parents.emplace(&node, xmlNode);
			break;
		}
		case NodeType::Text: {
			const auto& textNode = static_cast<const TextNode&>(node);
			assert(node.hasParent());
			auto childOfGroup = textNode.parent()->nodeType() == NodeType::Group;
			auto xmlNode = parent.append_child(childOfGroup ? "span" : "li");
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
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeElement(const Element& element, pugi::xml_node xmlNode) {
	if (const auto id = element.id(); id > 0) {
		xmlNode.append_attribute("data-id") = id;

		const auto parentDocument = element.findDocument();
		if (!parentDocument) {
			throw new WriteError(
			    "no document found for element with ID %d", element.id());
		}

		auto inHeaderDocument = (parentDocument->findDocument() != nullptr);
		if (!inHeaderDocument) {
			xmlNode.append_attribute("id") = id;
		}
	}

	xmlNode.append_attribute("data-type") =
	    Element::typeString(element.elementType()).c_str();

	this->appendVisibility(element, xmlNode);

	for (const auto& [k, v] : element.attrs()) {
		xmlNode.append_attribute(("data-" + k).c_str()) = v.c_str();
	}

	if (element.inlined()) {
		this->appendClassNames(xmlNode, {"inline"});
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
	this->appendTitle(element, xmlNode);
	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeDataElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeGifaElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("alt") = element.title().c_str();
	this->appendClassNames(media, {"media", "gifa"});
}

void HTMLWriter::serializeHintElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeHyperpngElement(
    const Element& element, pugi::xml_node xmlNode) {

	this->appendTitle(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("alt") = element.title().c_str();
	this->appendClassNames(media, {"media", "hyperpng"});

	if (element.hasFirstChild()) {
		auto container = xmlNode.append_child("div");
		this->appendClassNames(container, {"media", "hyperpng-container"});
		container.append_move(media);
		this->appendClassNames(media, {"media", "hyperpng-background"});
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

	if (auto container = this->findHyperpngContainer(element, xmlNode)) {
		xmlNode.set_name("area");
		auto map = this->findOrCreateMap(element, *container);
		map.append_move(xmlNode);
		this->populateArea(element, xmlNode);
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
		if (element.inlined()) {
			this->appendClassNames(xmlNode, {"inline"});
		}
		xmlNode.append_child(pugi::node_pcdata).set_value(element.title().c_str());
	}

	auto target = element.body();
	xmlNode.append_attribute("data-target") = target.c_str();
	xmlNode.append_attribute("href") = ("#" + target).c_str();
}

void HTMLWriter::serializeOverlayElement(const Element& element, pugi::xml_node xmlNode) {
	// Re-parent node
	auto container = findHyperpngContainer(element, xmlNode);
	if (!container) {
		throw WriteError("overlay parent must be a hyperpng element");
	}
	container->append_move(xmlNode);

	// Set node attributes
	auto elementType = element.elementType();
	xmlNode.set_name(mediaTagTypes_.at(elementType).c_str());
	auto dataURI = this->getDataURI(mediaContentTypes_.at(elementType), element.body());
	xmlNode.append_attribute("src") = dataURI.c_str();
	this->appendClassNames(xmlNode, {"media", "overlay"});
	xmlNode.append_attribute("hidden");

	auto [x, y] = this->getImageSize(element);
	xmlNode.append_attribute("style") =
	    tfm::format("left: %dpx; top: %dpx", x, y).c_str();

	// Create area tag for map
	auto map = this->findOrCreateMap(element, *container);
	auto area = map.append_child("area");
	this->populateArea(element, area);
	area.append_attribute("data-overlay") = element.id();
}

void HTMLWriter::serializeSoundElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("controls");
	this->appendClassNames(media, {"media", "sound"});
}

void HTMLWriter::serializeSubjectElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeTextElement(const Element& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
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
		this->appendClassNames(xmlNode, {"hyperlink"});
	}

	std::vector<std::string> classNames;

	if (textNode.hasFormat(TextFormat::Overflow)) {
		classNames.push_back("overflow");
	}
	if (textNode.hasFormat(TextFormat::Monospace)) {
		classNames.push_back("monospace");
	}
	if (!classNames.empty()) {
		this->appendClassNames(xmlNode, classNames);
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

void HTMLWriter::appendClassNames(
    pugi::xml_node xmlNode, std::vector<std::string> classNames) const {

	auto attr = xmlNode.attribute("class");
	if (!attr) {
		xmlNode.append_attribute("class");
	}

	const auto value = std::string{attr.value()};
	if (!value.empty()) {
		auto oldClassNames = Strings::split(value, " ");
		for (const auto& className : oldClassNames) {
			classNames.emplace_back(className);
		}
		std::sort(classNames.begin(), classNames.end());
		auto duplicates = std::unique(classNames.begin(), classNames.end());
		classNames.erase(duplicates, classNames.end());
	}

	xmlNode.attribute("class") = Strings::join(classNames, " ").c_str();
}

pugi::xml_node HTMLWriter::appendTitle(
    const Element& element, pugi::xml_node xmlNode) const {

	auto div = xmlNode.append_child("div");
	auto title = div.append_child("span");
	this->appendClassNames(title, {"title"});
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

	this->appendClassNames(xmlNode, {"visibility-" + node.visibilityString()});
}

pugi::xml_node HTMLWriter::createHTMLDocument(
    const Document& document, pugi::xml_document& xml) {

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
	style.append_child(pugi::node_pcdata).set_value("/*");
	style.append_child(pugi::node_cdata).set_value(css_.c_str());
	style.append_child(pugi::node_pcdata).set_value("*/");

	auto body = html.append_child("body");
	auto root = body.append_child("main");
	root.append_attribute("id") = "root";
	root.append_attribute("hidden");

	return root;
}

std::optional<pugi::xml_node> HTMLWriter::findHyperpngContainer(
    const Element& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		return std::nullopt;
	}

	auto parent = xmlNode.parent();
	auto container = parent.find_child_by_attribute("div", "class", "hyperpng-container");

	return container;
}

pugi::xml_node HTMLWriter::findOrCreateMap(
    const Element& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		throw WriteError(
		    "expected hyperpng parent; got %s", parentElement->elementTypeString());
	}

	auto container = this->findHyperpngContainer(element, xmlNode);
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

pugi::xml_node HTMLWriter::findXMLParent(const Node& node, const pugi::xml_node parent,
    const NodeMap parents, const int depth) const {

	auto nodeDepth = node.depth();
	auto nodeParent = parent;

	if (nodeDepth == depth) {
		return nodeParent;
	}

	nodeParent = parents.at(node.parent());
	auto ol = nodeParent.first_element_by_path("ol");
	if (nodeDepth < depth) {
		if (ol) {
			nodeParent = ol;
		}
	} else if (ol) {
		if (!ol.children().empty()) {
			nodeParent = ol.last_child();
		} else {
			nodeParent = ol;
		}
	}

	return nodeParent;
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
	map_.emplace(ElementType::Gifa, &HTMLWriter::serializeGifaElement);
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
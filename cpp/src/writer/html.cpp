#include "uhs.h"

namespace UHS {

HTMLWriter::Serializer HTMLWriter::serializer_;

HTMLWriter::HTMLWriter(Logger const& logger, std::ostream& out, Options const& options)
    : Writer(logger, out, options) {

	css_ =
#include "css.h"
	    ;
	js_ =
#include "js.h"
	    ;

	mediaContentTypes_.try_emplace(ElementType::Gifa, "image/gif");
	mediaContentTypes_.try_emplace(ElementType::Hyperpng, "image/png");
	mediaContentTypes_.try_emplace(ElementType::Overlay, "image/png");
	mediaContentTypes_.try_emplace(ElementType::Sound, "audio/wave");

	mediaTagTypes_.try_emplace(ElementType::Gifa, "img");
	mediaTagTypes_.try_emplace(ElementType::Hyperpng, "img");
	mediaTagTypes_.try_emplace(ElementType::Overlay, "img");
	mediaTagTypes_.try_emplace(ElementType::Sound, "audio");
}

void HTMLWriter::write(std::shared_ptr<Document> const document) {
	pugi::xml_document xml;
	this->serialize(*document, xml);
	xml.save(out_, "", pugi::format_raw | pugi::format_no_declaration);
}

void HTMLWriter::serialize(Document const& document, pugi::xml_document& xml) {
	auto depth = 0;
	NodeMap parents;

	auto d = &document;
	std::shared_ptr<Document> copy; // Keep object in scope
	if (document.isVersion(VersionType::Version88a)) {
		copy = this->addEntryPointTo88aDocument(document); // Copy and modify
		d = copy.get();
	}

	auto root = this->createHTMLDocument(*d, xml);
	auto parent = root;

	for (auto const& node : *d) {
		auto nodeDepth = node.depth();

		try {
			parent = this->findXMLParent(node, parent, parents, depth);
		} catch (std::out_of_range const& err) {
			throw DataError("could not find XML parent");
		}

		// Serialize node
		switch (node.nodeType()) {
		case NodeType::Break:
			break;
		case NodeType::Document: {
			auto const& d = static_cast<Document const&>(node);
			pugi::xml_node xmlNode;

			if (strcmp(parent.name(), "ol") == 0) {
				auto li = parent.append_child("li");
				if (d.visibility() == VisibilityType::None) {
					li.append_attribute("hidden");
				}
				xmlNode = li.append_child("section");
			} else {
				xmlNode = parent.append_child("section");
			}

			parents.try_emplace(&node, xmlNode);
			this->serializeDocument(d, xmlNode);
			break;
		}
		case NodeType::Element: {
			auto const& element = static_cast<Element const&>(node);
			pugi::xml_node xmlNode;

			if (strcmp(parent.name(), "ol") == 0) {
				auto li = parent.append_child("li");
				if (element.visibility() == VisibilityType::None) {
					li.append_attribute("hidden");
				}
				xmlNode = li.append_child("div");
			} else {
				xmlNode = parent.append_child("div");
			}

			parents.try_emplace(&node, xmlNode);
			this->serializeElement(element, xmlNode);
			break;
		}
		case NodeType::Group: {
			pugi::xml_node xmlNode;
			Element const* parentElement;
			ElementType parentElementType = ElementType::Unknown;
			assert(node.hasParent());
			auto nodeParent = node.parent();

			if (nodeParent->isElement()) {
				parentElement = static_cast<Element const*>(nodeParent);
				parentElementType = parentElement->elementType();
			}

			if (parentElementType == ElementType::Text) {
				xmlNode = parent.append_child("p");
				if (parentElement->attr("monospace")) {
					this->appendClassNames(xmlNode, {"monospace"});
				}
			} else {
				auto li = parent.append_child("li");
				xmlNode = li.append_child("div");
			}

			if (parentElementType == ElementType::Hint
			    || parentElementType == ElementType::Nesthint) {

				this->appendClassNames(xmlNode, {"hint"});
			}

			parents.try_emplace(&node, xmlNode);
			break;
		}
		case NodeType::Text: {
			auto const& textNode = static_cast<TextNode const&>(node);
			auto xmlNode = parent.append_child("span");
			this->serializeTextNode(textNode, xmlNode);
			break;
		}
		default:
			throw DataError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}
}

void HTMLWriter::serializeDocument(
    Document const& document, pugi::xml_node xmlNode) const {

	xmlNode.append_attribute("data-type") = document.nodeTypeString().c_str();
	xmlNode.append_attribute("data-version") = document.versionString().c_str();
	this->appendVisibility(document, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeElement(Element const& element, pugi::xml_node xmlNode) {
	if (auto const id = element.id(); id > 0) {
		xmlNode.append_attribute("data-id") = id;

		auto const parentDocument = element.findDocument();
		if (!parentDocument) {
			throw DataError("no document found for element with ID %d", element.id());
		}

		auto inHeaderDocument = (parentDocument->findDocument() != nullptr);
		if (!inHeaderDocument) {
			xmlNode.append_attribute("id") = id;
		}
	}

	xmlNode.append_attribute("data-type") =
	    Element::typeString(element.elementType()).c_str();

	this->appendVisibility(element, xmlNode);

	for (auto const& [k, v] : element.attrs()) {
		xmlNode.append_attribute(("data-" + k).c_str()) = v.c_str();
	}

	if (element.inlined()) {
		this->appendClassNames(xmlNode, {"inline"});
	}

	serializer_.invoke(*this, element, xmlNode);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void HTMLWriter::serializeBlankElement(Element const& element, pugi::xml_node xmlNode) {
	xmlNode.set_name("hr");
}
#pragma clang diagnostic pop

void HTMLWriter::serializeCommentElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeDataElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
}

void HTMLWriter::serializeGifaElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("alt") = element.title().c_str();
	this->appendClassNames(media, {"media", "gifa"});
}

void HTMLWriter::serializeHintElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeHyperpngElement(
    Element const& element, pugi::xml_node xmlNode) {

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
    Element const& element, pugi::xml_node xmlNode) {

	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeInfoElement(Element const& element, pugi::xml_node xmlNode) {
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

void HTMLWriter::serializeLinkElement(Element const& element, pugi::xml_node xmlNode) {
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

void HTMLWriter::serializeOverlayElement(Element const& element, pugi::xml_node xmlNode) {
	// Re-parent node
	auto container = findHyperpngContainer(element, xmlNode);
	if (!container) {
		throw DataError("overlay parent must be a hyperpng element");
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

void HTMLWriter::serializeSoundElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	auto media = this->appendMedia(element, xmlNode);
	media.append_attribute("controls");
	this->appendClassNames(media, {"media", "sound"});
}

void HTMLWriter::serializeSubjectElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	xmlNode.append_child("ol");
}

void HTMLWriter::serializeTextElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
}

std::pair<std::vector<std::string>, std::vector<std::string>::const_iterator>
    HTMLWriter::findNextSegment(std::vector<std::string>::const_iterator it,
        std::vector<std::string>::const_iterator end) const {

	std::vector<std::string> lines;
	std::smatch match;

	while (it < end) {
		if (it + 1 < end && *it == "" && *(it + 1) == "") {
			for (; it < end && *it == ""; ++it) {
				tfm::printf("\"%s\" (ignored)\n", *it);
			}
			break;
		} else if (it + 2 < end && *it == ""
		           && std::regex_match(*(it + 2), match, Regex::HorizontalLine)) {
			tfm::printf("\"%s\" (ignored)\n", *it);
			++it;
			break;
		} else {
			tfm::printf("\"%s\"\n", *it);
			lines.emplace_back(*it);
			++it;
		}
	}

	return {lines, it};
}

void HTMLWriter::serializeTextNode(
    TextNode const& textNode, pugi::xml_node xmlNode) const {

	if (textNode.hasPreviousSibling()) {
		auto previousNode = textNode.previousSibling();
		if (previousNode->isElement()) {
			auto previousElement = static_cast<Element*>(previousNode);
			auto isLink = (previousElement->elementType() == ElementType::Link);

			if (!previousElement->inlined() && isLink) {
				xmlNode.parent().insert_child_before("br", xmlNode);
			}
		}
	}

	auto body = textNode.body();
	auto lines = Strings::split(body, "\n");

	auto appendLines = [](pugi::xml_node node, std::vector<std::string> const& src) {
		for (std::size_t i = 0; i < src.size(); ++i) {
			if (i > 0) {
				node.append_child("br");
			}
			node.append_child(pugi::node_pcdata).set_value(src[i].c_str());
		}
	};

	if (textNode.hasFormat(TextFormat::Monospace | TextFormat::Overflow)) {
		xmlNode.set_name("div");
		auto it = lines.cbegin();

		while (it < lines.cend()) {
			auto segmentStart = it;
			auto [segment, next] = this->findNextSegment(it, lines.cend());
			it = next;

			Table table{segment};
			table.parse();

			if (table.valid()) {
				table.serialize(xmlNode);

				if (table.endLine() < segment.size()) {
					it = segmentStart + table.endLine();
				}
				continue;
			}

			auto hasLongLine = std::any_of(segment.begin(),
			    segment.end(),
			    [](auto const& l) { return l.size() > SuspectMonospaceLineLength; });

			if (hasLongLine) {
				auto child = xmlNode.append_child("span");
				appendLines(child, segment);

				// Render trailing blank lines that findNextSegment consumed
				for (auto blankIt = segmentStart + segment.size(); blankIt < it;
				    ++blankIt) {
					child.append_child("br");
				}
			} else {
				auto child = xmlNode.append_child("div");
				child.append_attribute("class");
				child.attribute("class") = "monospace overflow";
				appendLines(child, segment);
			}
		}
	} else {
		appendLines(xmlNode, lines);
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
}

std::shared_ptr<Document> HTMLWriter::addEntryPointTo88aDocument(
    Document const& document) const {

	auto copy = document.clone(); // Expensive, but 88a files are very small
	auto container = Element::create(ElementType::Subject, 1);

	container->title(copy->title());
	for (auto node = copy->firstChild(); node; node = copy->firstChild()) {
		if (node->nodeType() != NodeType::Element) {
			throw DataError("expected element, found %s node", node->nodeTypeString());
		}
		copy->removeChild(node);
		container->appendChild(node);
	}
	copy->appendChild(container);

	return copy;
}

std::optional<pugi::xml_node> HTMLWriter::appendBody(
    Element const& element, pugi::xml_node xmlNode) const {

	auto const& body = element.body();
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

	auto const value = std::string{attr.value()};
	if (!value.empty()) {
		auto oldClassNames = Strings::split(value, " ");
		for (auto const& className : oldClassNames) {
			classNames.push_back(className);
		}
	}

	std::sort(classNames.begin(), classNames.end());
	auto duplicates = std::unique(classNames.begin(), classNames.end());
	classNames.erase(duplicates, classNames.end());

	xmlNode.attribute("class") = Strings::join(classNames, " ").c_str();
}

pugi::xml_node HTMLWriter::appendTitle(
    Element const& element, pugi::xml_node xmlNode) const {

	auto div = xmlNode.append_child("div");
	auto title = div.append_child("span");
	this->appendClassNames(title, {"title"});
	title.append_child(pugi::node_pcdata).set_value(element.title().c_str());

	return title;
}

pugi::xml_node HTMLWriter::appendMedia(
    Element const& element, pugi::xml_node xmlNode) const {

	std::string contentType;
	std::string tagName;
	auto elementType = element.elementType();

	try {
		contentType = mediaContentTypes_.at(elementType);
		tagName = mediaTagTypes_.at(elementType);
	} catch (std::out_of_range const&) {
		throw DataError("unexpected media type: %s", element.elementTypeString());
	}

	auto media = xmlNode.append_child(tagName.c_str());
	auto dataURI = this->getDataURI(contentType, element.body());
	media.append_attribute("src") = dataURI.c_str();

	return media;
}

void HTMLWriter::appendVisibility(
    Traits::Visibility const& node, pugi::xml_node xmlNode) const {

	if (node.visibility() == VisibilityType::All) {
		return;
	}
	xmlNode.append_attribute("data-visibility") = node.visibilityString().c_str();
}

pugi::xml_node HTMLWriter::createHTMLDocument(
    Document const& document, pugi::xml_document& xml) const {

	// <!DOCTYPE html>
	xml.append_child(pugi::node_doctype).set_value("html");

	// <html>
	auto html = xml.append_child("html");
	html.append_attribute("lang") = "en";

	// <head>
	auto head = html.append_child("head");

	// <title>
	auto title = head.append_child("title");
	title.append_child(pugi::node_pcdata).set_value(document.title().c_str());

	// <meta>
	auto charset = head.append_child("meta");
	charset.append_attribute("charset") = "utf-8";

	// <meta>
	auto viewport = head.append_child("meta");
	viewport.append_attribute("name") = "viewport";
	viewport.append_attribute("content") =
	    "initial-scale=1, "
	    "maximum-scale=1, "
	    "width=device-width";

	// <meta>
	if (auto name = document.attr("author")) {
		auto author = head.append_child("meta");
		author.append_attribute("name") = "author";
		author.append_attribute("content") = name.value().c_str();
	}

	// <script>
	auto nccHack = head.append_child("script");
	nccHack.append_child(pugi::node_pcdata).set_value(R"(var __dirname="",module={})");

	// <script>
	auto script = head.append_child("script");
	script.append_child(pugi::node_pcdata).set_value("//");
	script.append_child(pugi::node_cdata).set_value(js_.c_str());

	// <style>
	auto style = head.append_child("style");
	style.append_child(pugi::node_pcdata).set_value("/*");
	style.append_child(pugi::node_cdata).set_value(css_.c_str());
	style.append_child(pugi::node_pcdata).set_value("*/");

	// <body>
	auto body = html.append_child("body");

	// <main>
	auto root = body.append_child("main");
	root.append_attribute("id") = "root";
	root.append_attribute("hidden");

	return root;
}

std::optional<pugi::xml_node> HTMLWriter::findHyperpngContainer(
    Element const& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		return std::nullopt;
	}

	auto parent = xmlNode.parent();
	auto container =
	    parent.find_child_by_attribute("div", "class", "hyperpng-container media");

	return container;
}

pugi::xml_node HTMLWriter::findOrCreateMap(
    Element const& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement || parentElement->elementType() != ElementType::Hyperpng) {
		throw DataError(
		    "expected hyperpng parent; got %s", parentElement->elementTypeString());
	}

	auto container = this->findHyperpngContainer(element, xmlNode);
	if (!container) {
		throw DataError(
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

pugi::xml_node HTMLWriter::findXMLParent(Node const& node, pugi::xml_node const parent,
    NodeMap const parents, int const depth) const {

	auto nodeDepth = node.depth();
	auto nodeParent = parent;

	if (nodeDepth == depth) {
		return nodeParent;
	}

	try {
		nodeParent = parents.at(node.parent());
	} catch (std::out_of_range const& err) {
		throw DataError("could not find HTML parent node");
	}

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
    std::string const& contentType, std::string const& data) const {

	return tfm::format("data:%s;base64,%s", contentType, Strings::toBase64(data));
}

std::tuple<int, int> HTMLWriter::getImageSize(Element const& element) const {
	auto x = element.attr("image-x");
	auto y = element.attr("image-y");

	if (!x || !y) {
		throw DataError("expected size for %s", element.elementTypeString());
	}

	return std::make_tuple(Strings::toInt(*x), Strings::toInt(*y));
}

Element* HTMLWriter::getParentElement(Element const& element) const {
	auto parentNode = element.parent();
	if (!parentNode || !parentNode->isElement()) {
		return nullptr;
	}
	return static_cast<Element*>(parentNode);
}

std::tuple<int, int, int, int> HTMLWriter::getRegionCoordinates(
    Element const& element) const {
	auto x1 = element.attr("region-top-left-x");
	auto y1 = element.attr("region-top-left-y");
	auto x2 = element.attr("region-bottom-right-x");
	auto y2 = element.attr("region-bottom-right-y");

	if (!x1 || !y1 || !x2 || !y2) {
		throw DataError(
		    "expected coordinates for hyperpng %s", element.elementTypeString());
	}

	return std::make_tuple(Strings::toInt(*x1),
	    Strings::toInt(*y1),
	    Strings::toInt(*x2),
	    Strings::toInt(*y2));
}

void HTMLWriter::populateArea(Element const& element, pugi::xml_node area) const {
	auto [x1, y1, x2, y2] = this->getRegionCoordinates(element);
	auto coordsString = tfm::format("%d,%d,%d,%d", x1, y1, x2, y2);

	area.append_attribute("alt") = element.title().c_str();
	area.append_attribute("coords") = coordsString.c_str();
	area.append_attribute("shape") = "rect";
}

HTMLWriter::Serializer::Serializer() {
	map_.try_emplace(ElementType::Blank, &HTMLWriter::serializeBlankElement);
	map_.try_emplace(ElementType::Comment, &HTMLWriter::serializeCommentElement);
	map_.try_emplace(ElementType::Credit, &HTMLWriter::serializeCommentElement);
	map_.try_emplace(ElementType::Gifa, &HTMLWriter::serializeGifaElement);
	map_.try_emplace(ElementType::Hint, &HTMLWriter::serializeHintElement);
	map_.try_emplace(ElementType::Hyperpng, &HTMLWriter::serializeHyperpngElement);
	map_.try_emplace(ElementType::Incentive, &HTMLWriter::serializeIncentiveElement);
	map_.try_emplace(ElementType::Info, &HTMLWriter::serializeInfoElement);
	map_.try_emplace(ElementType::Link, &HTMLWriter::serializeLinkElement);
	map_.try_emplace(ElementType::Nesthint, &HTMLWriter::serializeHintElement);
	map_.try_emplace(ElementType::Overlay, &HTMLWriter::serializeOverlayElement);
	map_.try_emplace(ElementType::Sound, &HTMLWriter::serializeSoundElement);
	map_.try_emplace(ElementType::Subject, &HTMLWriter::serializeSubjectElement);
	map_.try_emplace(ElementType::Text, &HTMLWriter::serializeTextElement);
	map_.try_emplace(ElementType::Version, &HTMLWriter::serializeCommentElement);
}

void HTMLWriter::Serializer::invoke(
    HTMLWriter& writer, Element const& element, pugi::xml_node xmlNode) {

	auto func = map_.at(element.elementType());
	(writer.*func)(element, xmlNode);
}

} // namespace UHS

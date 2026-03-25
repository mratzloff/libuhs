#include <cassert>

#include "uhs/error/data_error.h"
#include "uhs/regex.h"
#include "uhs/strings.h"
#include "uhs/token.h"
#include "uhs/writer/html_writer.h"

namespace UHS {

HTMLWriter::Dispatcher HTMLWriter::dispatcher_ = [] {
	Dispatcher dispatcher;

	dispatcher.add(ElementType::Blank, &HTMLWriter::serializeBlankElement);
	dispatcher.add(ElementType::Comment, &HTMLWriter::serializeCommentElement);
	dispatcher.add(ElementType::Credit, &HTMLWriter::serializeCommentElement);
	dispatcher.add(ElementType::Gifa, &HTMLWriter::serializeGifaElement);
	dispatcher.add(ElementType::Hint, &HTMLWriter::serializeHintElement);
	dispatcher.add(ElementType::Hyperpng, &HTMLWriter::serializeHyperpngElement);
	dispatcher.add(ElementType::Incentive, &HTMLWriter::serializeIncentiveElement);
	dispatcher.add(ElementType::Info, &HTMLWriter::serializeInfoElement);
	dispatcher.add(ElementType::Link, &HTMLWriter::serializeLinkElement);
	dispatcher.add(ElementType::Nesthint, &HTMLWriter::serializeHintElement);
	dispatcher.add(ElementType::Overlay, &HTMLWriter::serializeOverlayElement);
	dispatcher.add(ElementType::Sound, &HTMLWriter::serializeSoundElement);
	dispatcher.add(ElementType::Subject, &HTMLWriter::serializeSubjectElement);
	dispatcher.add(ElementType::Text, &HTMLWriter::serializeTextElement);
	dispatcher.add(ElementType::Version, &HTMLWriter::serializeCommentElement);

	return dispatcher;
}();

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
	document->normalize();

	pugi::xml_document xml;
	this->serialize(*document, xml);
	xml.save(out_, "", pugi::format_raw | pugi::format_no_declaration);
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

void HTMLWriter::appendLinesToNode(
    pugi::xml_node node, std::vector<std::string> const& lines) {

	for (std::size_t i = 0; i < lines.size(); ++i) {
		if (i > 0) {
			node.append_child("br");
		}
		node.append_child(pugi::node_pcdata).set_value(lines[i].c_str());
	}
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

pugi::xml_node HTMLWriter::appendTitle(
    Element const& element, pugi::xml_node xmlNode) const {

	auto div = xmlNode.append_child("div");
	auto title = div.append_child("span");
	this->appendClassNames(title, {"title"});
	title.append_child(pugi::node_pcdata).set_value(element.title().c_str());

	return title;
}

void HTMLWriter::appendVisibility(
    Traits::Visibility const& node, pugi::xml_node xmlNode) const {

	if (node.visibility() == VisibilityType::All) {
		return;
	}
	xmlNode.append_attribute("data-visibility") = node.visibilityString().c_str();
}

void HTMLWriter::applyHyperlinkFormat(
    TextNode const& textNode, pugi::xml_node xmlNode) const {

	if (!textNode.hasFormat(TextFormat::Hyperlink)) {
		return;
	}

	xmlNode.set_name("a");
	auto href = textNode.body();
	std::smatch matches;
	if (std::regex_match(href, matches, Regex::EmailAddress)) {
		href = "mailto:" + href;
	} else if (!std::regex_match(href, matches, Regex::URL)) {
		href = "http://" + href;
	}
	xmlNode.append_attribute("href") = href.c_str();
	this->appendClassNames(xmlNode, {"hyperlink"});
}

void HTMLWriter::cleanUpTableContainerBreaks(pugi::xml_node root) const {
	for (auto node : root.select_nodes("//div[@class='table-container']")) {
		auto previous = node.node().previous_sibling();
		if (previous && std::string(previous.name()) == "p") {
			auto lastChild = previous.last_child();
			if (lastChild) {
				this->removeTrailingBreaks(lastChild);
			}
		}
	}
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

std::pair<std::vector<std::string>, std::vector<std::string>::const_iterator>
    HTMLWriter::findNextSegment(std::vector<std::string>::const_iterator it,
        std::vector<std::string>::const_iterator end) const {

	std::vector<std::string> lines;
	std::smatch match;

	while (it < end) {
		if (it + 1 < end && *it == "" && *(it + 1) == "") {
			for (; it < end && *it == ""; ++it) {
			}
			break;
		} else if (it + 2 < end && *it == ""
		           && std::regex_match(*(it + 2), match, Regex::HorizontalLine)) {
			++it;
			break;
		} else {
			lines.emplace_back(*it);
			++it;
		}
	}

	return {lines, it};
}

pugi::xml_node HTMLWriter::findOrCreateMap(
    Element const& element, pugi::xml_node xmlNode) const {

	auto parentElement = this->getParentElement(element);
	if (!parentElement) {
		throw DataError("expected hyperpng parent; got no parent");
	}
	if (parentElement->elementType() != ElementType::Hyperpng) {
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

Element const* HTMLWriter::getParentElement(Element const& element) const {
	auto parentNode = element.parent();
	if (!parentNode || !parentNode->isElement()) {
		return nullptr;
	}
	return &parentNode->asElement();
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

void HTMLWriter::removeTrailingBreaks(pugi::xml_node xmlNode) const {
	auto lastChild = xmlNode.last_child();
	while (lastChild) {
		auto isBr = (std::string(lastChild.name()) == "br");
		auto isEmptyText = (lastChild.type() == pugi::node_pcdata
		                    && std::string(lastChild.value()).empty());

		if (!isBr && !isEmptyText) {
			break;
		}
		xmlNode.remove_child(lastChild);
		lastChild = xmlNode.last_child();
	}
}

void HTMLWriter::removeEmptyParagraphs(pugi::xml_node root) const {
	for (auto node : root.select_nodes("//p[not(node()) or normalize-space() = '']")) {
		auto p = node.node();
		p.parent().remove_child(p);
	}
}

std::pair<int, bool> HTMLWriter::scanForTables(
    std::vector<std::string> const& lines, bool isMonoOrOverflow) const {

	int segmentCount = 0;
	bool hasTable = false;

	for (auto scanIt = lines.cbegin(); scanIt < lines.cend();) {
		auto [segment, next] = this->findNextSegment(scanIt, lines.cend());
		scanIt = next;
		++segmentCount;

		if (isMonoOrOverflow && !hasTable) {
			Table table{segment};
			table.parse();
			if (table.valid() || table.hasPrecedingText()) {
				hasTable = true;
			}
		}
	}

	return {segmentCount, hasTable};
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
			auto const& document = node.asDocument();
			pugi::xml_node xmlNode;

			if (strcmp(parent.name(), "ol") == 0) {
				auto li = parent.append_child("li");
				if (document.visibility() == VisibilityType::None) {
					li.append_attribute("hidden");
				}
				xmlNode = li.append_child("section");
			} else {
				xmlNode = parent.append_child("section");
			}

			parents.try_emplace(&node, xmlNode);
			this->serializeDocument(document, xmlNode);
			break;
		}
		case NodeType::Element: {
			auto const& element = node.asElement();
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
			Element const* parentElement = nullptr;
			ElementType parentElementType = ElementType::Unknown;
			assert(node.hasParent());
			auto nodeParent = node.parent();

			if (nodeParent->isElement()) {
				parentElement = &nodeParent->asElement();
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
			auto xmlNode = parent.append_child("span");
			this->serializeTextNode(node.asText(), xmlNode);
			break;
		}
		default:
			throw DataError("unexpected node type: %s", node.nodeTypeString());
		}

		depth = nodeDepth;
	}

	this->splitMonospaceOverflowSpans(root);
	this->wrapInlineRuns(root);
	this->removeEmptyParagraphs(root);
	this->cleanUpTableContainerBreaks(root);
}

void HTMLWriter::serializeBlankElement(
    Element const& /* element */, pugi::xml_node xmlNode) {
	xmlNode.set_name("hr");
}

void HTMLWriter::serializeCommentElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendTitle(element, xmlNode);
	this->appendBody(element, xmlNode);
}

void HTMLWriter::serializeDataElement(Element const& element, pugi::xml_node xmlNode) {
	this->appendMedia(element, xmlNode);
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

	dispatcher_.dispatch(*this, element, xmlNode);
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
	if (!document || document->attrs().size() == 0) {
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
				isLink = (previousNode->asElement().elementType() == ElementType::Link);
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

void HTMLWriter::serializeTextNode(
    TextNode const& textNode, pugi::xml_node xmlNode) const {

	// Remove trailing <br> tags from the previous sibling span
	auto previousSibling = xmlNode.previous_sibling();
	if (previousSibling && std::string(previousSibling.name()) == "span") {
		this->removeTrailingBreaks(previousSibling);
	}

	if (textNode.hasPreviousSibling()) {
		auto previousNode = textNode.previousSibling();
		if (previousNode->isElement()) {
			auto const& previousElement = previousNode->asElement();
			auto isLink = (previousElement.elementType() == ElementType::Link);

			if (!previousElement.inlined() && isLink) {
				xmlNode.parent().insert_child_before("br", xmlNode);
			}
		}
	}

	if (textNode.hasFormat(TextFormat::Hyperlink)) {
		this->appendClassNames(xmlNode, {"format-hyperlink"});
	}
	if (textNode.hasFormat(TextFormat::Monospace)) {
		this->appendClassNames(xmlNode, {"format-monospace"});
	}
	if (textNode.hasFormat(TextFormat::Overflow)) {
		this->appendClassNames(xmlNode, {"format-overflow"});
	}

	auto lines = Strings::split(textNode.body(), "\n");
	auto isMonospace = textNode.hasFormat(TextFormat::Monospace);
	auto isOverflow = textNode.hasFormat(TextFormat::Overflow);
	auto isMonoOrOverflow = (isMonospace || isOverflow);
	auto [segmentCount, hasTable] = this->scanForTables(lines, isMonoOrOverflow);

	bool hasParagraphBreak = false;
	for (auto const& line : lines) {
		if (line == Token::ParagraphSep) {
			hasParagraphBreak = true;
			break;
		}
	}

	bool hasTrailingBreak = (!lines.empty() && lines.back() == Token::ParagraphSep);

	auto escapeToContainer =
	    (segmentCount > 1 || hasTable || hasTrailingBreak || hasParagraphBreak);

	if (escapeToContainer) {
		this->serializeTextNodeToContainer(textNode, xmlNode, lines, hasTrailingBreak);
	} else {
		if (isMonospace) {
			this->appendClassNames(xmlNode, {"monospace"});
		}
		if (isOverflow) {
			this->appendClassNames(xmlNode, {"overflow"});
		}
		appendLinesToNode(xmlNode, lines);
		this->applyHyperlinkFormat(textNode, xmlNode);
	}

	// If content was escaped to container level by this or a previous text node,
	// move this node to container level (unwrapped) so the inline-run wrapping
	// post-processing can group it with adjacent spans.
	if (xmlNode.parent() && xmlNode.parent().next_sibling()) {
		auto container = xmlNode.parent().parent();
		container.insert_move_after(xmlNode, container.last_child());
	}
}

void HTMLWriter::serializeTextNodeToContainer(TextNode const& textNode,
    pugi::xml_node xmlNode, std::vector<std::string> const& lines,
    bool hasTrailingBreak) const {

	auto isMonospace = textNode.hasFormat(TextFormat::Monospace);
	auto isOverflow = textNode.hasFormat(TextFormat::Overflow);
	auto isMonoOrOverflow = (isMonospace || isOverflow);

	auto parentNode = xmlNode.parent();
	auto container = parentNode;
	if (strcmp(parentNode.name(), "p") == 0) {
		container = parentNode.parent();
	}

	pugi::xml_node insertAfter;
	if (container == parentNode) {
		// Span is directly in the container — insert after the span
		insertAfter = xmlNode;
	} else {
		insertAfter = container.last_child();

		// Move preceding inline siblings from <p> to container level
		while (parentNode.first_child() != xmlNode) {
			auto sibling = parentNode.first_child();
			insertAfter = container.insert_move_after(sibling, insertAfter);
		}
	}

	auto it = lines.cbegin();
	bool firstSegment = true;

	while (it < lines.cend()) {
		auto segmentStart = it;
		auto [segment, next] = this->findNextSegment(it, lines.cend());
		it = next;

		// Insert an empty <p> between segments to break inline runs.
		// The empty-<p> cleanup pass removes these after wrapping.
		if (!firstSegment) {
			insertAfter = container.insert_child_after("p", insertAfter);
		}
		firstSegment = false;

		// Try table parse for monospace/overflow content
		if (isMonoOrOverflow) {
			Table table{segment};
			table.parse();

			if (table.valid()) {
				auto tableNode = container.insert_child_after("div", insertAfter);
				insertAfter = tableNode;
				table.serialize(tableNode);

				if (table.endLine() < segment.size()) {
					it = segmentStart + table.endLine();
				}
				continue;
			}

			if (table.hasPrecedingText()) {
				it = segmentStart + table.startLine();
				segment.resize(table.startLine());
			}
		}

		auto paragraphs = splitIntoParagraphs(segment);

		for (std::size_t p = 0; p < paragraphs.size(); ++p) {
			if (p > 0) {
				insertAfter = container.insert_child_after("p", insertAfter);
			}
			auto child = container.insert_child_after("span", insertAfter);
			insertAfter = child;
			if (isMonospace) {
				this->appendClassNames(child, {"format-monospace", "monospace"});
			}
			if (isOverflow) {
				this->appendClassNames(child, {"format-overflow", "overflow"});
			}
			appendLinesToNode(child, paragraphs[p]);
			this->applyHyperlinkFormat(textNode, child);
		}
	}

	// Insert trailing separator so the next text node starts a new paragraph
	if (hasTrailingBreak) {
		container.insert_child_after("p", insertAfter);
	}

	xmlNode.parent().remove_child(xmlNode);
}

void HTMLWriter::splitMonospaceOverflowSpans(pugi::xml_node root) const {
	for (auto node : root.select_nodes(
	         "//span[contains(@class,'monospace') and contains(@class,'overflow')]")) {
		auto span = node.node();
		auto parent = span.parent();

		// If the span is the only child, or the parent is already a block
		// container, stay within the parent. Only escape to the grandparent
		// when the parent is a <p> and the span has siblings.
		auto isOnlyChild = (parent.first_child() == span && !span.next_sibling());
		auto shouldEscape = (!isOnlyChild && strcmp(parent.name(), "p") == 0);
		auto insertInto = shouldEscape ? parent.parent() : parent;
		auto insertBefore = shouldEscape ? parent : span;

		// Move preceding siblings into a new <p>
		if (shouldEscape && parent.first_child() != span) {
			auto before = insertInto.insert_child_before("p", insertBefore);
			while (parent.first_child() != span) {
				auto sibling = parent.first_child();
				before.append_copy(sibling);
				parent.remove_child(sibling);
			}
		}

		// Move the span into its own <p>
		auto block = insertInto.insert_child_before("p", insertBefore);
		this->appendClassNames(block, {"monospace", "overflow"});
		while (span.first_child()) {
			auto child = span.first_child();
			block.append_copy(child);
			span.remove_child(child);
		}
		parent.remove_child(span);
	}
}

std::vector<std::vector<std::string>> HTMLWriter::splitIntoParagraphs(
    std::vector<std::string> const& segment) {

	std::vector<std::vector<std::string>> paragraphs;
	std::vector<std::string> currentParagraph;

	for (auto const& line : segment) {
		if (line == Token::ParagraphSep) {
			if (!currentParagraph.empty()) {
				paragraphs.push_back(currentParagraph);
				currentParagraph.clear();
			}
		} else {
			currentParagraph.push_back(line);
		}
	}
	if (!currentParagraph.empty()) {
		paragraphs.push_back(currentParagraph);
	}

	return paragraphs;
}

// Wrap consecutive inline element runs in <p> when not already inside a <p>.
// Select only the first element in each run (no preceding inline sibling).
void HTMLWriter::wrapInlineRuns(pugi::xml_node root) const {
	auto results = root.select_nodes(
	    "//span[not(parent::p) and not(contains(@class,'title'))"
	    " and not(preceding-sibling::*[1][self::a or self::span])]");
	std::vector<pugi::xml_node> runStarts;
	for (auto const& node : results) {
		runStarts.push_back(node.node());
	}

	for (auto current : runStarts) {
		auto wrapper = current.parent().insert_child_before("p", current);
		while (current) {
			auto name = std::string(current.name());
			if (name != "a" && name != "span") {
				break;
			}
			auto next = current.next_sibling();
			wrapper.append_move(current);
			current = next;
		}
		auto lastChild = wrapper.last_child();
		if (lastChild) {
			this->removeTrailingBreaks(lastChild);
		}
	}
}

} // namespace UHS

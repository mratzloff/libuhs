#include "helpers.h"

namespace UHS {

// Get Nth child of a node (0-indexed). Returns nullptr if out of bounds.
std::shared_ptr<Node> child(std::shared_ptr<Node> parent, int index) {
	auto node = parent->firstChild();
	for (int i = 0; i < index && node; ++i) {
		node = node->nextSibling();
	}
	return node;
}

// Create a 96a document with proper structure (88a header, main subject, version
// element, and info element). The returned document has a single subject element
// as the main content container. Add children (hints, text, links, etc.) to the
// returned subject.
//
// NOTE: In 96a, the document title is derived from the first subject title during
// parsing, so the title is used for both.
std::pair<std::shared_ptr<Document>, std::shared_ptr<Element>> createDocument96a(
    std::string const& title, VersionType version) {

	auto document = Document::create(version);
	document->title(title);

	// 88a header (required for 91a/95a/96a formats)
	auto header = Document::create(VersionType::Version88a);
	header->title("-");
	header->visibility(VisibilityType::None);

	auto headerSubject = Element::create(ElementType::Subject);
	headerSubject->title("-");

	auto headerHint = Element::create(ElementType::Hint);
	headerHint->title("-");

	auto headerGroup = GroupNode::create(0, 0);
	headerGroup->appendChild(TextNode::create("-"));

	headerHint->appendChild(headerGroup);
	headerSubject->appendChild(headerHint);
	header->appendChild(headerSubject);
	document->appendChild(header);

	// Main content subject (title must match document title)
	auto subject = Element::create(ElementType::Subject, 1);
	subject->title(title);
	document->appendChild(subject);

	// Metadata elements (version -> info)
	auto versionElement = Element::create(ElementType::Version);
	auto versionString = "96a";
	if (version == VersionType::Version95a) {
		versionString = "95a";
	} else if (version == VersionType::Version91a) {
		versionString = "91a";
	}
	versionElement->title(versionString);
	document->appendChild(versionElement);

	document->appendChild(Element::create(ElementType::Info));

	return {document, subject};
}

// Get the first TextNode body within a hint or nesthint element.
// Traverses element -> first group -> first text node -> body.
std::string firstTextBody(std::shared_ptr<Element> element) {
	auto group = element->firstChild();
	if (!group) {
		return "";
	}
	auto textNode = group->firstChild();
	if (!textNode || !textNode->isText()) {
		return "";
	}
	return textNode->asText().body();
}

std::string readFile(std::string const& path) {
	std::ifstream file(path, std::ios::binary);
	return std::string(
	    std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::shared_ptr<Document> roundTrip(
    std::shared_ptr<Document> document, Options const& options) {

	auto uhsData = writeUHS(document, options);
	Logger logger(LogLevel::None);
	Parser parser(logger, options);
	std::istringstream in(uhsData);

	return parser.parse(in);
}

std::string writeHTML(std::shared_ptr<Document> document) {
	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	HTMLWriter writer(logger, out, options);
	writer.write(document);

	return out.str();
}

std::string writeJSON(std::shared_ptr<Document> document) {
	Logger logger(LogLevel::None);
	Options options;
	std::ostringstream out;
	JSONWriter writer(logger, out, options);
	writer.write(document);

	return out.str();
}

std::string writeUHS(std::shared_ptr<Document> document, Options const& options) {
	Logger logger(LogLevel::None);
	std::ostringstream out;
	UHSWriter writer(logger, out, options);
	writer.write(document);

	return out.str();
}

} // namespace UHS

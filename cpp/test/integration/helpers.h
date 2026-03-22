#pragma once

#include <fstream>
#include <sstream>

#include "uhs.h"

namespace UHS {

static std::string const MediaDir = "cpp/test/media/";

static std::shared_ptr<Element> asElement(std::shared_ptr<Node> node) {
	return std::static_pointer_cast<Element>(node);
}

static std::shared_ptr<TextNode> asTextNode(std::shared_ptr<Node> node) {
	return std::static_pointer_cast<TextNode>(node);
}

std::shared_ptr<Node> child(std::shared_ptr<Node> parent, int index);

std::pair<std::shared_ptr<Document>, std::shared_ptr<Element>> createDocument96a(
    std::string const& title, VersionType version = VersionType::Version96a);

std::string firstTextBody(std::shared_ptr<Element> element);

static std::shared_ptr<Element> firstSubject(std::shared_ptr<Document> document) {
	return asElement(child(document, 1));
}

std::string readFile(std::string const& path);

std::shared_ptr<Document> roundTrip(
    std::shared_ptr<Document> document, Options const& options = {});

std::string writeHTML(std::shared_ptr<Document> document);
std::string writeJSON(std::shared_ptr<Document> document);
std::string writeUHS(std::shared_ptr<Document> document, Options const& options = {});

} // namespace UHS

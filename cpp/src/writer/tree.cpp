#include "uhs.h"

namespace UHS {

TreeWriter::TreeWriter(Logger const logger, std::ostream& out, Options const options)
    : Writer(logger, out, options) {}

void TreeWriter::write(std::shared_ptr<Document> const document) {
	for (auto const& node : *document) {
		switch (node.nodeType()) {
		case NodeType::Document: {
			auto const& d = static_cast<Document const&>(node);
			this->draw(d);
			break;
		}
		case NodeType::Element: {
			auto const& element = static_cast<Element const&>(node);
			this->draw(element);
			break;
		}
		case NodeType::Group: {
			auto const& groupNode = static_cast<GroupNode const&>(node);
			this->draw(groupNode);
			break;
		}
		case NodeType::Text: {
			auto const& textNode = static_cast<TextNode const&>(node);
			this->draw(textNode);
			break;
		}
		default:
			break; // Ignore
		}
	}
}

void TreeWriter::draw(Document const& document) {
	this->drawScaffold(document);
	out_ << "[" << document.nodeTypeString() << "] \"" << document.title() << "\""
	     << std::endl;
}

void TreeWriter::draw(Element const& element) {
	this->drawScaffold(element);
	out_ << "[" << element.elementTypeString() << "] \"" << element.title() << "\""
	     << std::endl;
}

void TreeWriter::draw(GroupNode const& groupNode) {
	this->drawScaffold(groupNode);
	out_ << "[" << groupNode.nodeTypeString() << "]" << std::endl;
}

void TreeWriter::draw(TextNode const& textNode) {
	this->drawScaffold(textNode);
	auto body = textNode.body();
	std::replace(body.begin(), body.end(), '\n', ' ');

	if (body.length() > 76) {
		body = body.substr(0, 73) + "...";
	}
	out_ << "[" << textNode.nodeTypeString() << "] \"" << body << "\"" << std::endl;
}

void TreeWriter::drawScaffold(Node const& node) {
	auto const depth = node.depth();

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

} // namespace UHS
#include "uhs.h"

namespace UHS {

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
	out_ << "[" << document.nodeTypeString() << "] \""
	     << Strings::replaceSpecialChars(document.title()) << "\"" << std::endl;
}

void TreeWriter::draw(const Element& element) {
	this->drawScaffold(element);
	out_ << "[" << element.elementTypeString() << "] \""
	     << Strings::replaceSpecialChars(element.title()) << "\"" << std::endl;
}

void TreeWriter::draw(const GroupNode& groupNode) {
	this->drawScaffold(groupNode);
	out_ << "[" << groupNode.nodeTypeString() << "]" << std::endl;
}

void TreeWriter::draw(const TextNode& textNode) {
	this->drawScaffold(textNode);
	auto body = textNode.body();
	std::replace(body.begin(), body.end(), '\n', ' ');
	body = Strings::replaceSpecialChars(body);

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

} // namespace UHS
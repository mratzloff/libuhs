#include "uhs/writer/tree_writer.h"
#include "uhs/node/break_node.h"

namespace UHS {

// Using 256-color palette
TreeWriter::NodeToStyleMap TreeWriter::styles_{
    {"blank", "\033[1;38;5;242m"},     // dim gray
    {"break", "\033[1;38;5;240m"},     // dark gray
    {"comment", "\033[1;38;5;146m"},   // lavender
    {"credit", "\033[1;38;5;183m"},    // lilac
    {"document", "\033[1;38;5;255m"},  // bright white
    {"gifa", "\033[1;38;5;203m"},      // coral
    {"group", "\033[1;38;5;245m"},     // medium gray
    {"hint", "\033[1;38;5;114m"},      // seafoam
    {"hyperpng", "\033[1;38;5;214m"},  // amber
    {"hyperlink", "\033[4;38;5;33m"},  // dodger blue
    {"incentive", "\033[1;38;5;228m"}, // lemon yellow
    {"monospace", "\033[38;5;179m"},   // wheat
    {"info", "\033[1;38;5;153m"},      // ice blue
    {"link", "\033[1;38;5;111m"},      // periwinkle
    {"nesthint", "\033[1;38;5;116m"},  // teal
    {"overlay", "\033[1;38;5;173m"},   // sienna
    {"sound", "\033[1;38;5;222m"},     // golden
    {"subject", "\033[1;38;5;75m"},    // sky blue
    {"text", "\033[1;38;5;167m"},      // brick
    {"version", "\033[1;38;5;69m"},    // cornflower
};

TreeWriter::TreeWriter(Logger const& logger, std::ostream& out, Options const& options)
    : Writer(logger, out, options) {}

void TreeWriter::draw(Document const& document) {
	this->drawScaffold(document);
	out_ << drawType(document.nodeTypeString()) << " \"" << document.title() << "\""
	     << std::endl;
}

void TreeWriter::draw(Element const& element) {
	this->drawScaffold(element);

	std::string visibilityMarker;
	if (options_.preserve) {
		auto const style = typeStyle("incentive");

		switch (element.visibility()) {
		case VisibilityType::RegisteredOnly:
			visibilityMarker = " " + style + "(registered only)" + StyleReset;
			break;
		case VisibilityType::UnregisteredOnly:
			visibilityMarker = " " + style + "(unregistered only)" + StyleReset;
			break;
		default:
			break;
		}
	}
	out_ << drawType(element.elementTypeString()) << visibilityMarker << " \""
	     << element.title() << "\"" << std::endl;
}

void TreeWriter::draw(GroupNode const& groupNode) {
	this->drawScaffold(groupNode);
	out_ << drawType(groupNode.nodeTypeString()) << std::endl;
}

void TreeWriter::draw(TextNode const& textNode) {
	this->drawScaffold(textNode);
	auto body = textNode.body();
	std::replace(body.begin(), body.end(), '\n', ' ');

	std::string format;
	if (textNode.hasFormat(UHS::TextFormat::Hyperlink)) {
		format = "hyperlink";
	} else if (textNode.hasFormat(UHS::TextFormat::Monospace)) {
		format = "monospace";
	}

	if (body.length() > 76) {
		body = body.substr(0, 73) + "...";
	}
	out_ << drawText(body, format) << std::endl;
}

void TreeWriter::drawScaffold(Node const& node) {
	auto const depth = node.depth();

	std::string line;
	auto ancestor = node.parent();
	if (!ancestor) {
		return;
	}

	for (auto i = depth; i > 0 && ancestor; --i) {
		auto grandparent = ancestor->parent();
		if (grandparent) {
			if (ancestor == grandparent->lastChild()) {
				line = "    " + line;
			} else {
				auto const style = nodeStyle(*grandparent);
				line = style + "│" + StyleReset + "   " + line;
			}
		}
		ancestor = grandparent;
	}
	out_ << line;

	auto parentStyle = nodeStyle(*node.parent());
	if (&node == node.parent()->lastChild()) {
		out_ << parentStyle << "└───" << StyleReset;
	} else {
		out_ << parentStyle << "├───" << StyleReset;
	}
}

std::string TreeWriter::drawText(std::string const& text, std::string const& format) {
	auto const style = typeStyle(format);
	if (!style.empty()) {
		return "\"" + style + text + StyleReset + "\"";
	}
	return "\"" + text + "\"";
}

std::string TreeWriter::drawType(std::string const& name) {
	auto const style = typeStyle(name);
	if (!style.empty()) {
		return style + "[" + name + "]" + StyleReset;
	}
	return "[" + name + "]";
}

std::string TreeWriter::nodeStyle(Node const& node) {
	if (node.nodeType() == NodeType::Element) {
		return typeStyle(node.asElement().elementTypeString());
	}
	return typeStyle(node.nodeTypeString());
}

std::string TreeWriter::typeStyle(std::string const& name) {
	auto it = styles_.find(name);
	if (it != styles_.end()) {
		return it->second;
	}
	return "";
}

void TreeWriter::write(std::shared_ptr<Document> const document) {
	document->normalize();

	for (auto const& node : *document) {
		switch (node.nodeType()) {
		case NodeType::Document:
			this->draw(node.asDocument());
			break;
		case NodeType::Element:
			this->draw(node.asElement());
			break;
		case NodeType::Group:
			this->draw(node.asGroup());
			break;
		case NodeType::Text:
			this->draw(node.asText());
			break;
		case NodeType::Break: {
			auto const& breakNode = node.asBreak();
			this->drawScaffold(breakNode);
			out_ << drawType(breakNode.nodeTypeString()) << std::endl;
			break;
		}
		default:
			break; // Ignore
		}
	}
}

} // namespace UHS

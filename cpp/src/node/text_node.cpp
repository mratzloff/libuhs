#include "uhs/node/text_node.h"
#include "uhs/constants.h"

namespace UHS {

void swap(TextNode& lhs, TextNode& rhs) noexcept {
	using std::swap;

	swap(static_cast<Node&>(lhs), static_cast<Node&>(rhs));
	swap(static_cast<Traits::Body&>(lhs), static_cast<Traits::Body&>(rhs));
	swap(lhs.format_, rhs.format_);
}

TextNode::TextNode(std::string const& body) : Node(NodeType::Text), Body(body) {}

TextNode::TextNode(std::string const& body, TextFormat format)
    : Node(NodeType::Text), Body(body), format_{format} {}

TextNode::TextNode(TextNode const& other)
    : Node(other), Traits::Body(other), format_{other.format_} {}

TextNode::TextNode(TextNode&& other) noexcept
    : Node(std::move(other)), Traits::Body(std::move(other)), format_{other.format_} {

	other.format_ = TextFormat::None;
}

std::shared_ptr<TextNode> TextNode::create(std::string const& body) {
	return std::make_shared<TextNode>(body);
}

std::shared_ptr<TextNode> TextNode::create(std::string const& body, TextFormat format) {
	return std::make_shared<TextNode>(body, format);
}

void TextNode::addFormat(TextFormat format) {
	format_ = ::UHS::withFormat(format_, format);
}

// Copies and returns a detached text node.
std::shared_ptr<TextNode> TextNode::clone() const {
	auto textNode = std::make_shared<TextNode>(*this);
	textNode->detachParent();
	return textNode;
}

TextFormat TextNode::format() const {
	return format_;
}

void TextNode::format(TextFormat format) {
	format_ = format;
}

bool TextNode::hasFormat(TextFormat format) const {
	return ::UHS::hasFormat(format_, format);
}

TextNode& TextNode::operator=(TextNode other) {
	swap(*this, other);
	return *this;
}

void TextNode::removeFormat(TextFormat format) {
	format_ = ::UHS::withoutFormat(format_, format);
}

std::string const& TextNode::string() const {
	return this->body();
}

} // namespace UHS

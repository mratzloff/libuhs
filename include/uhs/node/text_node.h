#if !defined UHS_TEXT_NODE_H
#define UHS_TEXT_NODE_H

#include <memory>
#include <string>

#include "uhs/constants.h"
#include "uhs/node.h"
#include "uhs/traits/body.h"

namespace UHS {

class TextNode
    : public Node
    , public Traits::Body {
public:
	friend void swap(TextNode& lhs, TextNode& rhs) noexcept;

	explicit TextNode(std::string const& body);
	TextNode(std::string const& body, TextFormat format);
	TextNode(TextNode const& other);
	TextNode(TextNode&& other) noexcept;

	static std::shared_ptr<TextNode> create(std::string const& body);
	static std::shared_ptr<TextNode> create(std::string const& body, TextFormat format);

	void addFormat(TextFormat format);
	std::shared_ptr<TextNode> clone() const;
	TextFormat format() const;
	void format(TextFormat format);
	bool hasFormat(TextFormat format) const;
	TextNode& operator=(TextNode other);
	void removeFormat(TextFormat format);
	std::string const& string() const;

private:
	TextFormat format_ = TextFormat::None;
};

} // namespace UHS

#endif

#if !defined UHS_NODE_H
#define UHS_NODE_H

#include <memory>
#include <string>

#include "uhs/constants.h"
#include "uhs/node_iterator.h"

namespace UHS {

class BreakNode;
class Document;
class Element;
class GroupNode;
class TextNode;

class Node : public std::enable_shared_from_this<Node> {
public:
	using iterator = NodeIterator<Node>;
	using const_iterator = NodeIterator<Node const>;

	friend void swap(Node& lhs, Node& rhs) noexcept;

	explicit Node(NodeType type);
	Node(Node const& other);
	Node(Node&& other) noexcept;
	virtual ~Node() = default;

	static bool isElementOfType(Node const& node, ElementType type);
	static std::string typeString(NodeType type);

	void appendChild(std::shared_ptr<Node> node);
	void appendChild(std::shared_ptr<Node> node, bool silenceEvent);
	BreakNode& asBreak();
	BreakNode const& asBreak() const;
	Document& asDocument();
	Document const& asDocument() const;
	Element& asElement();
	Element const& asElement() const;
	GroupNode& asGroup();
	GroupNode const& asGroup() const;
	TextNode& asText();
	TextNode const& asText() const;
	iterator begin();
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator cend() const;
	int depth() const;
	void detachParent();
	iterator end();
	const_iterator end() const;
	Document* findDocument() const;
	std::shared_ptr<Node> firstChild() const;
	bool hasFirstChild() const;
	bool hasLastChild() const;
	bool hasNextSibling() const;
	bool hasParent() const;
	bool hasPreviousSibling() const;
	void insertBefore(std::shared_ptr<Node> node, Node* ref);
	bool isBreak() const;
	bool isDocument() const;
	bool isElement() const;
	bool isGroup() const;
	bool isText() const;
	Node* lastChild() const;
	std::shared_ptr<Node> nextSibling() const;
	NodeType nodeType() const;
	std::string nodeTypeString() const;
	int numChildren() const;
	Node& operator=(Node other);
	Node* parent() const;
	std::shared_ptr<Node> pointer() const;
	Node* previousSibling() const;
	void removeChild(std::shared_ptr<Node> node);

protected:
	void cloneChildren(Node const& node);
	virtual void didAdd();
	virtual void didRemove();

private:
	int depth_ = 0;
	std::shared_ptr<Node> firstChild_ = nullptr;
	Node* lastChild_ = nullptr;
	std::shared_ptr<Node> nextSibling_ = nullptr;
	NodeType nodeType_;
	int numChildren_ = 0;
	Node* parent_ = nullptr;
	Node* previousSibling_ = nullptr;
};

} // namespace UHS

#endif

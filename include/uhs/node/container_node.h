#if !defined UHS_CONTAINER_NODE_H
#define UHS_CONTAINER_NODE_H

#include "uhs/node.h"

namespace UHS {

class ContainerNode : public Node {
public:
	using Node::appendChild;

	friend void swap(ContainerNode& lhs, ContainerNode& rhs) noexcept;

	explicit ContainerNode(NodeType type);
	ContainerNode(ContainerNode const& other);
	ContainerNode(ContainerNode&& other) noexcept;
	virtual ~ContainerNode() = default;

	int length() const;
	void length(int length); // Used by UHSWriter
	int line() const;
	void line(int line);
	ContainerNode& operator=(ContainerNode other);

protected:
	int length_ = 0;
	int line_ = 0;
};

} // namespace UHS

#endif

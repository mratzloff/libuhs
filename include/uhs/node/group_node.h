#if !defined UHS_GROUP_NODE_H
#define UHS_GROUP_NODE_H

#include <memory>

#include "uhs/node/container_node.h"

namespace UHS {

class GroupNode : public ContainerNode {
public:
	friend void swap(GroupNode& lhs, GroupNode& rhs) noexcept;

	GroupNode(int line, int length);
	GroupNode(GroupNode const&);
	GroupNode(GroupNode&&) noexcept;

	static std::shared_ptr<GroupNode> create(int line, int length);

	std::shared_ptr<GroupNode> clone() const;
	GroupNode& operator=(GroupNode);
};

} // namespace UHS

#endif

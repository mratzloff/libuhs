#if !defined UHS_BREAK_NODE_H
#define UHS_BREAK_NODE_H

#include <memory>

#include "uhs/node.h"

namespace UHS {

class BreakNode : public Node {
public:
	BreakNode();
	BreakNode(BreakNode const&);
	BreakNode(BreakNode&&) noexcept;

	static std::shared_ptr<BreakNode> create();

	std::shared_ptr<BreakNode> clone() const;
	BreakNode& operator=(BreakNode);
};

} // namespace UHS

#endif

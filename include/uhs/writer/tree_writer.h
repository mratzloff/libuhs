#if !defined UHS_TREE_WRITER_H
#define UHS_TREE_WRITER_H

#include <string>

#include "tsl/hopscotch_map.h"

#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/writer.h"

namespace UHS {

class TreeWriter : public Writer {
public:
	TreeWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	using NodeToStyleMap = tsl::hopscotch_map<std::string, std::string> const;

	static constexpr auto StyleReset = "\033[0m";

	static NodeToStyleMap styles_;

	void draw(Document const& document);
	void draw(Element const& element);
	void draw(GroupNode const& groupNode);
	void draw(TextNode const& textNode);
	void drawScaffold(Node const& node);
	std::string drawText(std::string const& text, std::string const& name);
	std::string drawType(std::string const& name);
	std::string nodeStyle(Node const& node);
	std::string typeStyle(std::string const& name);
};

} // namespace UHS

#endif

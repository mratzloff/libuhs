#if !defined UHS_JSON_WRITER_H
#define UHS_JSON_WRITER_H

#include "json/json.h"

#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/node/text_node.h"
#include "uhs/traits/attributes.h"
#include "uhs/writer.h"

namespace UHS {

class JSONWriter : public Writer {
public:
	JSONWriter(Logger const& logger, std::ostream& out, Options const& options = {});
	void write(std::shared_ptr<Document> const document) override;

private:
	void serialize(Document const& document, Json::Value& root) const;
	void serializeDocument(Document const& document, Json::Value& object) const;
	void serializeElement(Element const& element, Json::Value& object) const;
	void serializeMap(Traits::Attributes::Type const& attrs, Json::Value& object) const;
	void serializeTextNode(TextNode const& textNode, Json::Value& object) const;
};

} // namespace UHS

#endif

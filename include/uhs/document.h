#if !defined UHS_DOCUMENT_H
#define UHS_DOCUMENT_H

#include <map>
#include <memory>
#include <string>

#include "uhs/constants.h"
#include "uhs/element.h"
#include "uhs/node.h"
#include "uhs/traits/attributes.h"
#include "uhs/traits/title.h"
#include "uhs/traits/visibility.h"

namespace UHS {

class Document
    : public Node
    , public Traits::Attributes
    , public Traits::Title
    , public Traits::Visibility {
public:
	friend void swap(Document& lhs, Document& rhs) noexcept;

	Document(VersionType version);
	Document(Document const& other);
	Document(Document&& other) noexcept;

	static std::shared_ptr<Document> create(VersionType version);

	std::shared_ptr<Document> clone() const;
	void elementAdded(Element& element);
	void elementRemoved(Element& element);
	Node* find(int const id);
	bool isVersion(VersionType v) const;
	void normalize();
	Document& operator=(Document other);
	void reindex();
	bool validChecksum() const;
	void validChecksum(bool value);
	VersionType version() const;
	void version(VersionType v);
	std::string versionString() const;

private:
	std::map<int, Node*> index_;
	bool indexed_ = true;
	bool validChecksum_ = false;
	VersionType version_;
};

} // namespace UHS

#endif

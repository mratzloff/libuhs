#if !defined UHS_ELEMENT_H
#define UHS_ELEMENT_H

#include <map>
#include <memory>
#include <string>

#include "uhs/constants.h"
#include "uhs/node/container_node.h"
#include "uhs/traits/attributes.h"
#include "uhs/traits/body.h"
#include "uhs/traits/inlined.h"
#include "uhs/traits/title.h"
#include "uhs/traits/visibility.h"

namespace UHS {

class Element
    : public ContainerNode
    , public Traits::Attributes
    , public Traits::Body
    , public Traits::Title
    , public Traits::Inlined
    , public Traits::Visibility {
public:
	friend void swap(Element& lhs, Element& rhs) noexcept;

	Element(ElementType type, int id = 0);
	Element(Element const& other);
	Element(Element&& other) noexcept;

	static std::shared_ptr<Element> create(ElementType type, int const id = 0);
	static ElementType elementType(std::string const& typeString);
	static std::string typeString(ElementType type);

	std::shared_ptr<Element> clone() const;
	ElementType elementType() const;
	std::string elementTypeString() const;
	int id() const;
	bool isMedia() const;
	std::string mediaExt() const;
	Element& operator=(Element other);

private:
	class TypeMap {
	public:
		TypeMap();
		std::string findByType(ElementType const type) const;
		ElementType findByString(std::string const& string) const;

	private:
		std::map<ElementType const, std::string const> byType_;
		std::map<std::string const, ElementType const> byString_;
	};

	static Element::TypeMap typeMap_;

	ElementType elementType_;
	int id_ = 0;
};

} // namespace UHS

#endif

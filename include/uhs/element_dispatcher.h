#if !defined UHS_ELEMENT_DISPATCHER_H
#define UHS_ELEMENT_DISPATCHER_H

#include <utility>

#include "tsl/hopscotch_map.h"

#include "uhs/constants.h"

namespace UHS {

template<typename Func>
class ElementDispatcher {
public:
	void add(ElementType type, Func func) { map_.try_emplace(type, func); }

	template<typename Writer, typename Elem, typename... Args>
	auto dispatch(Writer& writer, Elem& element, Args&&... args) const {
		auto func = map_.at(element.elementType());
		return (writer.*func)(element, std::forward<Args>(args)...);
	}

private:
	tsl::hopscotch_map<ElementType, Func> map_;
};

} // namespace UHS

#endif

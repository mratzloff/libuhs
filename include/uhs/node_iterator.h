#if !defined UHS_NODE_ITERATOR_H
#define UHS_NODE_ITERATOR_H

#include <cstddef>
#include <iterator>

namespace UHS {

template<typename T>
class NodeIterator {
public:
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T*;
	using reference = T&;
	using iterator_category = std::forward_iterator_tag;

	NodeIterator() = default;
	explicit NodeIterator(pointer node);
	reference operator*() const;
	pointer operator->() const;
	NodeIterator<T>& operator++();
	NodeIterator<T> operator++(int);
	bool operator==(NodeIterator<T> const& rhs) const;
	bool operator!=(NodeIterator<T> const& rhs) const;

private:
	pointer initial_ = nullptr;
	pointer current_ = nullptr;
	bool visited_ = false;
};

} // namespace UHS

#endif

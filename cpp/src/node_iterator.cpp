#include "uhs/node_iterator.h"
#include "uhs/node.h"

namespace UHS {

template<typename T>
NodeIterator<T>::NodeIterator(NodeIterator<T>::pointer node) {
	initial_ = node;
	current_ = node;
}

template<typename T>
typename NodeIterator<T>::reference NodeIterator<T>::operator*() const {
	return *current_;
}

template<typename T>
typename NodeIterator<T>::pointer NodeIterator<T>::operator->() const {
	return current_;
}

// ++it
template<typename T>
NodeIterator<T>& NodeIterator<T>::operator++() {
	do {
		if (!current_) {
			break;
		}
		if (current_->hasFirstChild() && !visited_) { // Down
			current_ = current_->firstChild().get();
			visited_ = false;
		} else if (current_->hasNextSibling()) { // Next
			current_ = current_->nextSibling().get();
			visited_ = false;
		} else { // Up
			current_ = current_->parent();
			if (current_ == initial_) {
				current_ = nullptr;
				break;
			}
			visited_ = true;
		}
	} while (visited_);

	return *this;
}

// it++
template<typename T>
NodeIterator<T> NodeIterator<T>::operator++(int) {
	auto self = *this;
	++(*this);
	return self;
}

template<typename T>
bool NodeIterator<T>::operator==(NodeIterator<T> const& rhs) const {
	return current_ == rhs.current_;
}

template<typename T>
bool NodeIterator<T>::operator!=(NodeIterator<T> const& rhs) const {
	return !(*this == rhs);
}

template class NodeIterator<Node>;
template class NodeIterator<Node const>;

} // namespace UHS

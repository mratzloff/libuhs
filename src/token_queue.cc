#include "token_queue.h"

namespace UHS {

TokenQueue::TokenQueue() {}

TokenQueue::~TokenQueue() {}

void TokenQueue::push(std::shared_ptr<Token> t) {
	// std::lock_guard<std::mutex> m {_mutex};
	_queue.push(t);
}

std::shared_ptr<Token> TokenQueue::front() {
	// std::lock_guard<std::mutex> m {_mutex};
	if (_queue.empty()) {
		return nullptr;
	}
	std::shared_ptr<Token> t {_queue.front()};
	_queue.pop();
	return t;
}

}

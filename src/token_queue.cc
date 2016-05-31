#include "token_queue.h"

namespace UHS {

TokenQueue::TokenQueue() {}

void TokenQueue::push(std::shared_ptr<Token> t) {
	_mutex.lock();
	_queue.push(t);
	_mutex.unlock();
}

std::shared_ptr<Token> TokenQueue::front() {
	_mutex.lock();
	std::shared_ptr<Token> t {_queue.front()};
	_mutex.unlock();
	return t;
}

}

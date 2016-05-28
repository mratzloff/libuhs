#include "token_queue.h"

namespace UHS {

TokenQueue::TokenQueue() {}

void TokenQueue::push(Token* t) {
	_mutex.lock();
	_queue.push(t);
	_mutex.unlock();
}

Token* TokenQueue::front() {
	_mutex.lock();
	Token* t {_queue.front()};
	_mutex.unlock();
	return t;
}

}

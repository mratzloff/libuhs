#include <chrono>
#include <iostream>
#include <thread>
#include "token_queue.h"

namespace UHS {

TokenQueue::TokenQueue() : _open(true) {}

TokenQueue::~TokenQueue() {}

bool TokenQueue::send(std::shared_ptr<Token> t) {
	if (!_open) {
		return false;
	}
	std::lock_guard<std::mutex> m {_mutex};
	_queue.push(t);
	return true;
}

std::shared_ptr<Token> TokenQueue::receive() {
	while (this->empty()) { // Unlikely
		if (!this->ok()) {
			return nullptr;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> m {_mutex};
	std::shared_ptr<Token> t {_queue.front()};
	_queue.pop();
	return t;
}

bool TokenQueue::empty() {
	std::lock_guard<std::mutex> m {_mutex};
	return _queue.empty();
}

bool TokenQueue::ok() {
	std::lock_guard<std::mutex> m {_mutex};
	return _open || !_queue.empty();
}

void TokenQueue::close() {
	_open = false;
}

}

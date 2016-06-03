#ifndef UHS_TOKEN_QUEUE_H
#define UHS_TOKEN_QUEUE_H

#include <mutex>
#include <queue>
#include "token.h"

namespace UHS {

class TokenQueue {
public:
	TokenQueue();
	virtual ~TokenQueue();
	void push(std::shared_ptr<Token> t);
	std::shared_ptr<Token> front();

protected:
	std::queue<std::shared_ptr<Token>> _queue;
	std::mutex _mutex;
};

}

#endif

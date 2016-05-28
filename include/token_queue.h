#ifndef UHS_TOKEN_QUEUE_H
#define UHS_TOKEN_QUEUE_H

#include <mutex>
#include <queue>
#include "token.h"

namespace UHS {

class TokenQueue {
public:
	TokenQueue();
	void push(Token* t);
	Token* front();

protected:
	std::queue<Token*> _queue;
	std::mutex _mutex;
};

}

#endif

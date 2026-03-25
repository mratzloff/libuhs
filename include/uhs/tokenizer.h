#if !defined UHS_TOKENIZER_H
#define UHS_TOKENIZER_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <regex>
#include <string>
#include <vector>

#include "uhs/constants.h"
#include "uhs/pipe.h"
#include "uhs/token.h"

namespace UHS {

class Tokenizer {
public:
	explicit Tokenizer(Pipe& pipe);

	bool hasNext();
	std::unique_ptr<Token const> next();
	void tokenize(char const* buffer, std::streamsize length);

private:
	class TokenChannel {
	public:
		explicit TokenChannel(Pipe& pipe);

		void close();
		bool ok() const;
		std::unique_ptr<Token const> receive();
		void send(Token const&& token);

	private:
		mutable std::mutex mutex_;
		bool open_ = true;
		Pipe& pipe_; // For exceptions
		std::queue<Token> queue_;
		std::condition_variable ready_;
	};

	bool beforeHeaderSep_ = true;
	bool binaryMode_ = false;
	std::string buffer_;
	int expectedLineTokenLine_ = 0;
	int expectedStringTokenLine_ = 0;
	int line_ = 1;
	std::size_t offset_ = 0;
	TokenChannel out_;
	Pipe& pipe_;

	void tokenizeCRC(std::string const& crc, std::size_t column);
	void tokenizeData(std::string const& data, std::size_t column);
	void tokenizeDataAddress(std::smatch const& matches);
	ElementType tokenizeDescriptor(std::smatch const& matches);
	void tokenizeEOF(std::size_t column);
	void tokenizeHyperpngRegion(std::smatch const& matches);
	void tokenizeLine();
	void tokenizeMatches(
	    std::smatch const& matches, std::vector<TokenType> const&& tokens);
	void tokenizeOverlayAddress(std::smatch const& matches);
};

} // namespace UHS

#endif

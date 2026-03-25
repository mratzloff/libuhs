#if !defined UHS_PIPE_H
#define UHS_PIPE_H

#include <cstdint>
#include <exception>
#include <functional>
#include <istream>
#include <vector>

namespace UHS {

class Pipe {
public:
	using Handler = std::function<void(char const*, std::streamsize n)>;

	explicit Pipe(std::istream& in);

	void addHandler(Handler func);
	bool eof();
	std::exception_ptr error();
	bool good();
	void read();

private:
	static std::size_t const ReadLength = 1024;

	std::exception_ptr err_ = nullptr;
	std::vector<Handler> handlers_;
	std::istream& in_;
	std::size_t offset_ = 0;
};

} // namespace UHS

#endif

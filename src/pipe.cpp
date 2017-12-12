#include "uhs.h"
#include <iostream>

namespace UHS {

Pipe::Pipe(std::ifstream& in) : in_{in} {
	if (!in.is_open()) {
		throw ReadError("could not open input file");
	}
}

void Pipe::addHandler(Pipe::Handler func) {
	handlers_.push_back(func);
}

std::exception_ptr Pipe::error() {
	return err_;
}

void Pipe::read() {
	try {
		std::streamsize n = ReadLen;
		char buf[ReadLen] = {0};

		while (in_.read(buf, n)) {
			for (const auto& func : handlers_) {
				func(buf, n);
			}
			offset_ += n;
		}
		n = in_.gcount();
		for (const auto& func : handlers_) {
			func(buf, n);
		}
		offset_ += n;
		in_.close();
	} catch (const std::exception& err) {
		err_ = std::current_exception();
	}
}

bool Pipe::good() {
	return in_.good();
}

bool Pipe::eof() {
	return in_.eof();
}

} // namespace UHS

#include "uhs.h"
#include <iostream>

namespace UHS {

Pipe::Pipe(std::ifstream& in) : _in{in} {
	if (!in.is_open()) {
		throw ReadError("could not open input file");
	}
}

void Pipe::addHandler(Pipe::Handler func) {
	_handlers.push_back(func);
}

std::exception_ptr Pipe::error() {
	return _err;
}

void Pipe::read() {
	try {
		std::streamsize n = ReadLen;
		char buf[ReadLen] = {0};

		while (_in.read(buf, n)) {
			for (const auto& func : _handlers) {
				func(buf, n);
			}
			_offset += n;
		}
		n = _in.gcount();
		for (const auto& func : _handlers) {
			func(buf, n);
		}
		_offset += n;
		_in.close();
	} catch (const std::exception& err) {
		_err = std::current_exception();
	}
}

bool Pipe::good() {
	return _in.good();
}

bool Pipe::eof() {
	return _in.eof();
}

} // namespace UHS

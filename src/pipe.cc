#include <iostream>
#include "uhs.h"

namespace UHS {

Pipe::Pipe(std::ifstream& in) : _in {in} {
	if (! _in.is_open()) {
		_err = std::make_shared<Error>(ErrorRead, "could not open file");
		_err->finalize();
	}
}

void Pipe::addHandler(Pipe::Handler h) {
	_handlers.push_back(h);
}

void Pipe::read() {
	std::streamsize n = ReadLen;
	char buf[ReadLen] = {0};

	while (_in.read(buf, n)) {
		for (const auto& h : _handlers) {
			h(buf, n);
		}
		_offset += n;
	}
	n = _in.gcount();
	for (const auto& h : _handlers) {
		h(buf, n);
	}
	_offset += n;
	_in.close();
}

bool Pipe::good() {
	return _in.good();
}

bool Pipe::eof() {
	return _in.eof();
}

const std::shared_ptr<Error> Pipe::error() {
	return _err;
}

}

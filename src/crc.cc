#include "uhs.h"

namespace UHS {

// There are two files with a bad checksum.
// 
// The first is phantom.uhs. It was (inexplicably) modified after compilation
// to pad the end with NUL characters to an even multiple of 1024 bytes. The
// checksum preceding the NUL characters is correct for the original file.
// 
// The second is tfc.uhs, which contains a stray CRLF at the end. If removed,
// the checksum is also correct.
CRC::CRC(std::shared_ptr<Pipe> p) : _pipe {p}, _buf {0}, _rem {0} {
	this->createTable();
	_pipe->addHandler([=](const char* s, std::streamsize n) {
		this->update(s, n);
	});
}

CRC::~CRC() {}

void CRC::update(const char* buf, std::streamsize n) {
	switch (n) {
	case 0:
		return;
	case 1:
		switch (_bufLen) {
		case 0:
			_buf[0] = buf[0];
			_bufLen = 1;
			break;
		case 1:
			_buf[1] = buf[1];
			_bufLen = 2;
			break;
		default:
			this->calculate(_buf, 1);
			_buf[0] = _buf[1];
			_buf[1] = buf[0];
			_bufLen = 2;
			return;
		}
	default:
		if (_bufLen > 0) {
			this->calculate(_buf, _bufLen);
		}
		_buf[0] = buf[n-2];
		_buf[1] = buf[n-1];
		_bufLen = 2;
		this->calculate(buf, n - _bufLen);
		break;
	}
}

void CRC::finalize() {
	if (_rem > Polynomial) {
		_rem = (_rem + FinalXor) & CastMask;
	}
}

bool CRC::valid() {
	return _rem == this->checksum();
}

void CRC::createTable() {
	for (int i = 0; i < TableLen; ++i) {
		uint16_t byte = (i << 8) & CastMask;
		for (int bit = 0; bit < 8; ++bit) {
			if ((byte & MSBMask) != 0) {
				byte <<= 1;
				byte ^= Polynomial;
			} else {
				byte <<= 1;
			}
		}
		_table[i] = byte & CastMask;
	}
}

void CRC::calculate(const char* buf, std::streamsize n) {
	for (int i = 0; i < n; ++i) {
		uint8_t byte = this->reflectByte(buf[i] & 0xFF);
		_rem = (_rem ^ (byte << 8)) & CastMask;
		int pos = (_rem >> 8) & 0xFF;
		_rem = (_rem << 8) & CastMask;
		_rem = (_rem ^ _table[pos]) & CastMask;
	}
}

uint8_t CRC::reflectByte(uint8_t byte) {
	uint8_t etyb = 0;

	for (int bit = 0; bit < 8; ++bit) {
		if ((byte & (1 << bit)) != 0) {
			etyb |= ((1 << (7 - bit)) & 0xFF);
		}
	}
	return etyb;
}

uint16_t CRC::checksum() {
	return (static_cast<uint8_t>(_buf[1]) << 8) | static_cast<uint8_t>(_buf[0]);
}

}

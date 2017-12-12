#include "uhs.h"

namespace UHS {

// There are two files with bad checksums.
//
// The first is phantom.uhs. It was (inexplicably) modified after compilation
// to pad the end with NUL characters to an even multiple of 1024 bytes. The
// checksum preceding the NUL characters is correct for the original file.
//
// The second is tfc.uhs, which contains a stray CRLF at the end. If removed,
// the checksum is also correct.
CRC::CRC() {
	this->createTable();
}

void CRC::upstream(Pipe& p) {
	pipe_ = &p;
	p.addHandler([=](const char* s, std::streamsize n) { this->calculate(s, n, true); });
}

void CRC::calculate(const char* buf, std::streamsize n) {
	for (int i = 0; i < n; ++i) {
		auto byte = this->reflectByte(buf[i] & 0xFF);
		rem_ = (rem_ ^ (byte << 8)) & CastMask;
		int pos = (rem_ >> 8) & 0xFF;
		rem_ = (rem_ << 8) & CastMask;
		rem_ = (rem_ ^ table_[pos]) & CastMask;
	}
}

void CRC::calculate(const char* buf, std::streamsize n, bool) {
	switch (n) {
	case 0:
		return;
	case 1:
		switch (bufLen_) {
		case 0:
			buf_[0] = buf[0];
			bufLen_ = 1;
			break;
		case 1:
			buf_[1] = buf[1];
			bufLen_ = 2;
			break;
		default:
			this->calculate(buf_, 1);
			buf_[0] = buf_[1];
			buf_[1] = buf[0];
			bufLen_ = 2;
			return;
		}
	default:
		if (bufLen_ > 0) {
			this->calculate(buf_, bufLen_);
		}
		buf_[0] = buf[n - 2];
		buf_[1] = buf[n - 1];
		bufLen_ = 2;
		this->calculate(buf, n - bufLen_);
	}
}

uint16_t CRC::result() {
	this->finalize();
	return rem_;
}

void CRC::result(std::vector<char>& out) {
	this->finalize();
	out.push_back(rem_ & 0xFF); // low
	out.push_back(rem_ >> 8);   // high
}

bool CRC::valid() {
	return this->result() == this->checksum();
}

void CRC::reset() {
	pipe_ = nullptr;
	std::fill(buf_, buf_ + 2, 0);
	bufLen_ = 0;
	rem_ = 0x0000;
	finalized_ = false;
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
		table_[i] = byte & CastMask;
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

void CRC::finalize() {
	if (!finalized_ && rem_ > Polynomial) {
		rem_ = (rem_ + FinalXOR) & CastMask;
	}
	finalized_ = true;
}

uint16_t CRC::checksum() {
	return (static_cast<uint8_t>(buf_[1]) << 8) | static_cast<uint8_t>(buf_[0]);
}

} // namespace UHS

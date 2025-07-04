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

void CRC::upstream(Pipe& pipe) {
	pipe_ = &pipe;
	pipe.addHandler([=, this](char const* buffer, std::streamsize length) {
		this->calculate(buffer, length, true);
	});
}

void CRC::calculate(char const* buffer, std::streamsize length) {
	for (auto i = 0; i < length; ++i) {
		auto byte = this->reflectByte(buffer[i] & 0xFF);
		remainder_ = (remainder_ ^ (byte << 8)) & CastMask;
		int position = (remainder_ >> 8) & 0xFF;
		remainder_ = (remainder_ << 8) & CastMask;
		remainder_ = (remainder_ ^ table_[position]) & CastMask;
	}
}

void CRC::calculate(char const* buffer, std::streamsize length, bool) {
	switch (length) {
	case 0:
		return;
	case 1:
		switch (checksumLength_) {
		case 0:
			checksum_[0] = buffer[0];
			checksumLength_ = 1;
			break;
		case 1:
			checksum_[1] = buffer[1];
			checksumLength_ = 2;
			break;
		default:
			this->calculate(checksum_, 1);
			checksum_[0] = checksum_[1];
			checksum_[1] = buffer[0];
			checksumLength_ = 2;
			return;
		}
	default:
		if (checksumLength_ > 0) {
			this->calculate(checksum_, checksumLength_);
		}
		checksum_[0] = buffer[length - 2];
		checksum_[1] = buffer[length - 1];
		checksumLength_ = 2;
		this->calculate(buffer, length - checksumLength_);
	}
}

uint16_t CRC::result() {
	this->finalize();
	return remainder_;
}

void CRC::result(std::vector<char>& out) {
	this->finalize();
	out.push_back(remainder_ & 0xFF); // low
	out.push_back(remainder_ >> 8);   // high
}

bool CRC::valid() {
	return this->result() == this->checksum();
}

void CRC::reset() {
	pipe_ = nullptr;
	std::fill(checksum_, checksum_ + 2, 0);
	checksumLength_ = 0;
	remainder_ = 0x0000;
	finalized_ = false;
}

void CRC::createTable() {
	for (auto i = 0; i < TableLength; ++i) {
		uint16_t byte = (i << 8) & CastMask;
		for (auto bit = 0; bit < 8; ++bit) {
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

	for (auto bit = 0; bit < 8; ++bit) {
		if ((byte & (1 << bit)) != 0) {
			etyb |= ((1 << (7 - bit)) & 0xFF);
		}
	}
	return etyb;
}

void CRC::finalize() {
	if (!finalized_ && remainder_ > Polynomial) {
		remainder_ = (remainder_ + FinalXOR) & CastMask;
	}
	finalized_ = true;
}

uint16_t CRC::checksum() {
	return (static_cast<uint8_t>(checksum_[1]) << 8) | static_cast<uint8_t>(checksum_[0]);
}

} // namespace UHS

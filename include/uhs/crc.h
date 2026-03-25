#if !defined UHS_CRC_H
#define UHS_CRC_H

#include <cstdint>
#include <vector>

#include "uhs/pipe.h"

namespace UHS {

class CRC {
public:
	static constexpr auto ByteLength = 2;

	CRC();

	void calculate(char const* buffer, std::streamsize length, bool bufferChecksum);
	void calculate(char const* buffer, std::streamsize length);
	void reset();
	uint16_t result();
	void result(std::vector<char>& out);
	void upstream(Pipe& pipe);
	bool valid();

private:
	static uint16_t const CastMask = 0xFFFF;
	static uint16_t const FinalXOR = 0x0100;
	static uint16_t const MSBMask = 0x8000;
	static uint16_t const Polynomial = 0x8005;
	static constexpr auto TableLength = 256;

	char checksum_[2];
	int checksumLength_ = 0;
	bool finalized_ = false;
	Pipe* pipe_ = nullptr;
	uint16_t remainder_ = 0x0000;
	uint16_t table_[TableLength];

	uint16_t checksum();
	void createTable();
	void finalize();
	uint8_t reflectByte(uint8_t byte);
};

} // namespace UHS

#endif

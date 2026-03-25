#if !defined UHS_ZIP_H
#define UHS_ZIP_H

#include <cstdint>
#include <string>

namespace UHS {

class Zip {
public:
	Zip(std::string const& data);
	bool isZip();
	void unzip(std::string const& dir);

private:
	static int const CompressedSizeOffset = 18;
	static int const ExtraFieldLengthOffset = 28;
	static int const FilenameLengthOffset = 26;
	static int const FilenameOffset = 30;
	static uint32_t const Signature = 0x04034b50;
	static int const SignatureOffset = 0;
	static int const UncompressedSizeOffset = 22;

	std::string const data_;

	uint16_t readUint16LE(int offset);
	uint32_t readUint32LE(int offset);
};

} // namespace UHS

#endif

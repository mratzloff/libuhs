#include <filesystem>
#include <fstream>
#include <vector>

extern "C" {
#include "puff.h"
}

#include "uhs/error/file_error.h"
#include "uhs/strings.h"
#include "uhs/zip.h"

namespace UHS {

Zip::Zip(std::string const& data) : data_{data} {}

bool Zip::isZip() const {
	return this->readUint32LE(SignatureOffset) == Signature;
}

void Zip::unzip(std::string const& dir) {
	if (!this->isZip()) {
		throw FileError("file is not in ZIP format");
	}

	// Read metadata from zip
	auto compressedSize = this->readUint32LE(CompressedSizeOffset);
	auto uncompressedSize = this->readUint32LE(UncompressedSizeOffset);
	auto filenameLength = this->readUint16LE(FilenameLengthOffset);
	auto extraFieldLength = this->readUint16LE(ExtraFieldLengthOffset);

	auto filename = data_.substr(FilenameOffset, filenameLength);
	Strings::toLower(filename);

	// Read DEFLATE data
	auto compressedOffset = FilenameOffset + filenameLength + extraFieldLength;
	auto compressedString = data_.substr(compressedOffset, compressedSize);

	// Decompress DEFLATE data
	std::vector<unsigned char> uncompressed(uncompressedSize);
	unsigned long uncompressedLength = uncompressedSize;
	auto compressed = reinterpret_cast<unsigned char const*>(compressedString.c_str());
	unsigned long compressedLength = compressedString.length();

	auto status =
	    puff(uncompressed.data(), &uncompressedLength, compressed, &compressedLength);
	if (status != 0) {
		throw FileError("invalid DEFLATE data (error code: %d)\n", status);
	}

	// Write to file
	std::filesystem::path path{dir};
	auto outfile = path.append(filename).string();
	auto fout = std::ofstream(outfile, std::ios::out | std::ios::binary);
	if (!fout) {
		throw FileError("could not open file for writing: %s", outfile);
	}
	fout.write(reinterpret_cast<char const*>(uncompressed.data()), uncompressedLength);
	if (!fout) {
		throw FileError("failed to write file: %s", outfile);
	}
}

uint16_t Zip::readUint16LE(int offset) const {
	if (offset < 0
	    || static_cast<std::size_t>(offset) + sizeof(uint16_t) > data_.size()) {
		throw FileError("ZIP read out of bounds at offset %d", offset);
	}
	return static_cast<uint16_t>(data_[offset])
	       | ((static_cast<uint16_t>(data_[offset + 1]) & 0xFF) << 8);
}

uint32_t Zip::readUint32LE(int offset) const {
	if (offset < 0
	    || static_cast<std::size_t>(offset) + sizeof(uint32_t) > data_.size()) {
		throw FileError("ZIP read out of bounds at offset %d", offset);
	}
	return static_cast<uint32_t>(data_[offset])
	       | ((static_cast<uint32_t>(data_[offset + 1]) & 0xFF) << 8)
	       | ((static_cast<uint32_t>(data_[offset + 2]) & 0xFF) << 16)
	       | ((static_cast<uint32_t>(data_[offset + 3]) & 0xFF) << 24);
}

} // namespace UHS

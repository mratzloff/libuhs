#include "uhs.h"

namespace UHS {

Downloader::Downloader(Logger const& logger)
    : httpClient_{httplib::Client(tfm::format("%s://%s", Protocol, Host))}
    , httpHeaders_{{{"User-Agent", "UHSWIN/6.10"}}}
    , logger_{logger} {}

void Downloader::download(std::string const& dir, std::string const& file) {
	std::vector<std::string> files{file};
	this->download(dir, files);
}

void Downloader::download(std::string const& dir, std::vector<std::string> const& files) {

	if (fileIndex_.size() == 0) {
		this->loadFileIndex();
	}

	std::string outdir = dir;
	if (outdir.empty()) {
		outdir = std::filesystem::current_path();
	}

	std::filesystem::path path{outdir};
	if (!std::filesystem::exists(path)) {
		throw FileError("directory does not exist: %s", outdir);
	}

	for (auto file : files) {
		FileMetadata metadata;
		try {
			metadata = fileIndex_.at(file);
		} catch (std::out_of_range const& err) {
			throw FileError("unknown file: %s", file);
		}

		std::string buffer;
		auto res = httpClient_.Get(
		    metadata.url, httpHeaders_, [&](char const* data, size_t data_length) {
			    buffer.append(data, data_length);
			    return true;
		    });
		if (res->status != 200) {
			throw HTTPError(res, "could not download file: %s", file);
		}
		logger_.info("downloaded %s", file);

		Zip zip{buffer};

		try {
			zip.unzip(outdir);
		} catch (FileError const& err) {
			std::throw_with_nested(FileError("could not decompress ZIP file: %s", file));
		}
	}
}

Downloader::FileIndex const& Downloader::fileIndex() {
	if (fileIndex_.size() == 0) {
		this->loadFileIndex();
	}
	return fileIndex_;
}

void Downloader::loadFileIndex() {
	auto res = httpClient_.Get(FileIndexPath, httpHeaders_);
	if (res->status != 200) {
		throw HTTPError(res, "could not download file index");
	}

	pugi::xml_document doc;
	doc.load_string(res->body.c_str());

	auto numAdded = 0;
	for (auto node = doc.child("uhsfiles").child("file"); node;
	     node = node.next_sibling("file")) {

		auto compressedSize = node.child("fsize").text().as_int();
		std::string filename = node.child("fname").text().as_string();
		auto uncompressedSize = node.child("ffullsize").text().as_int();

		std::vector<std::string> otherTitles;
		for (auto n = node.child("falttitle"); n; n = n.next_sibling("falttitle")) {
			otherTitles.push_back(n.text().as_string());
		}

		std::vector<std::string> relatedKeys;
		for (auto n = node.child("frelated"); n; n = n.next_sibling("frelated")) {
			relatedKeys.push_back(n.text().as_string());
		}

		auto title = node.child("ftitle").text().as_string();
		auto url = node.child("furl").text().as_string();

		fileIndex_.try_emplace(filename,
		    FileMetadata{
		        .compressedSize = compressedSize,
		        .filename = filename,
		        .otherTitles = otherTitles,
		        .relatedKeys = relatedKeys,
		        .title = title,
		        .uncompressedSize = uncompressedSize,
		        .url = url,
		    });

		++numAdded;
	}

	logger_.debug("added metadata for %d files to file index", numAdded);
}

} // namespace UHS

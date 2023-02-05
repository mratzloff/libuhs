#include "uhs.h"

namespace UHS {

Downloader::Downloader(Logger const logger, Options const options)
    : httpClient_{httplib::Client(tfm::format("%s://%s", Protocol, Host))}
    , httpHeaders_{{{"User-Agent", "UHSWIN/6.10"}}}
    , logger_{logger}
    , options_{options} {}

void Downloader::download(std::string const& filename, std::string const& dir) {
	FileMetadata metadata;
	try {
		metadata = fileMetadata_.at(filename);
	} catch (std::out_of_range const& err) {
		throw Error("unknown filename: %s", filename);
	}

	std::string outdir = dir;
	if (outdir.empty()) {
		outdir = std::filesystem::current_path();
	}

	std::filesystem::path path{outdir};
	if (!std::filesystem::exists(path)) {
		throw Error("directory does not exist: %s", outdir);
	}

	std::string buffer;
	auto res = httpClient_.Get(
	    metadata.url, httpHeaders_, [&](char const* data, size_t data_length) {
		    buffer.append(data, data_length);
		    return true;
	    });
	if (res->status != 200) {
		throw HTTPError(res, "could not download file");
	}

	Zip zip{buffer};
	zip.unzip(outdir);
}

void Downloader::loadIndex() {
	auto res = httpClient_.Get(IndexPath, httpHeaders_);
	if (res->status != 200) {
		throw HTTPError(res, "could not download index of file metadata");
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

		fileMetadata_.try_emplace(filename,
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

	logger_.debug("Added metadata for %d files to index", numAdded);
}

} // namespace UHS

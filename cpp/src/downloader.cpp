#include "uhs.h"

namespace UHS {

Downloader::Downloader(Logger const logger, Options const options)
    : logger_{logger}, options_{options} {}

void Downloader::fetchIndex() {
	httplib::Client client(tfm::format("%s://%s", Protocol, Host));
	auto res = client.Get(IndexPath, {{"User-Agent", "UHSWIN/6.10"}});

	if (res->status != 200) {
		throw HTTPError(res, "could not fetch index");
	}

	pugi::xml_document doc;
	doc.load_string(res->body.c_str());

	auto filesAdded = 0;
	for (auto node = doc.child("uhsfiles").child("file"); node;
	     node = node.next_sibling("file")) {

		auto compressedSize = node.child("fsize").text().as_int();
		std::string filename = node.child("fname").text().as_string();
		auto key = filename.substr(0, filename.find_last_of("."));

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

		files_.try_emplace(key,
		    File{
		        .compressedSize = compressedSize,
		        .key = key,
		        .filename = filename,
		        .otherTitles = otherTitles,
		        .relatedKeys = relatedKeys,
		        .title = title,
		        .url = url,
		    });

		++filesAdded;
	}

	logger_.info("Added %d files to index", filesAdded);
}

} // namespace UHS

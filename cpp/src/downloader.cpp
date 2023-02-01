#include "uhs.h"

namespace UHS {

void Downloader::getIndex() {
	httplib::Client client(tfm::format("%s://%s", Protocol, Host));
	auto res = client.Get(IndexPath, {{"User-Agent", "UHSWIN/6.10"}});

	tfm::printf("status = %d\n", res->status);

	if (res->status == 200) {
		pugi::xml_document doc;
		doc.load_string(res->body.c_str());

		pugi::xml_node node = doc.child("uhsfiles");
		for (node = node.child("file"); node; node = node.next_sibling("file")) {
			tfm::printf("%s\n", node.child("ftitle").text().as_string());
		}
	};
}

} // namespace UHS

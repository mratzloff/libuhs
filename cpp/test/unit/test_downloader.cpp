#include <catch2/catch_test_macros.hpp>

#include "uhs.h"

namespace UHS {

class MockHTTPClient : public Downloader::HTTPClient {
public:
	struct Response {
		int status;
		std::string body;
	};

	void setResponse(std::string const& path, Response response) {
		responses_[path] = std::move(response);
	}

	httplib::Result Get(
	    std::string const& path, httplib::Headers const& headers) override {
		auto it = responses_.find(path);
		if (it == responses_.end()) {
			auto response = std::make_unique<httplib::Response>();
			response->status = 404;
			return httplib::Result{std::move(response), httplib::Error::Success};
		}
		auto response = std::make_unique<httplib::Response>();
		response->status = it->second.status;
		response->body = it->second.body;

		return httplib::Result{std::move(response), httplib::Error::Success};
	}

	httplib::Result Get(std::string const& path, httplib::Headers const& headers,
	    httplib::ContentReceiver contentReceiver) override {
		auto it = responses_.find(path);
		if (it == responses_.end()) {
			auto response = std::make_unique<httplib::Response>();
			response->status = 404;
			return httplib::Result{std::move(response), httplib::Error::Success};
		}
		contentReceiver(it->second.body.data(), it->second.body.size());
		auto response = std::make_unique<httplib::Response>();
		response->status = it->second.status;

		return httplib::Result{std::move(response), httplib::Error::Success};
	}

private:
	std::map<std::string, Response> responses_;
};

static auto const FileIndexXML = R"(<?xml version="1.0"?>
<uhsfiles>
	<file>
		<fname>test.uhs</fname>
		<ftitle>Test Game Hints</ftitle>
		<fsize>1234</fsize>
		<ffullsize>5678</ffullsize>
		<furl>/files/test.uhs</furl>
		<falttitle>Test Alternate Title</falttitle>
		<frelated>other.uhs</frelated>
	</file>
	<file>
		<fname>demo.uhs</fname>
		<ftitle>Demo Hints</ftitle>
		<fsize>100</fsize>
		<ffullsize>200</ffullsize>
		<furl>/files/demo.uhs</furl>
	</file>
</uhsfiles>
)";

TEST_CASE("Downloader::fileIndex parses XML file index", "[downloader]") {
	auto mock = std::make_unique<MockHTTPClient>();
	mock->setResponse("/cgi-bin/update.cgi", {200, FileIndexXML});

	Logger logger{LogLevel::None};
	Downloader downloader{logger, std::move(mock)};

	auto const& index = downloader.fileIndex();

	SECTION("parses correct number of entries") {
		REQUIRE(index.size() == 2);
	}

	SECTION("parses file metadata") {
		auto const& entry = index.at("test.uhs");
		REQUIRE(entry.filename == "test.uhs");
		REQUIRE(entry.title == "Test Game Hints");
		REQUIRE(entry.compressedSize == 1234);
		REQUIRE(entry.uncompressedSize == 5678);
		REQUIRE(entry.url == "/files/test.uhs");
	}

	SECTION("parses alternate titles") {
		auto const& entry = index.at("test.uhs");
		REQUIRE(entry.otherTitles.size() == 1);
		REQUIRE(entry.otherTitles[0] == "Test Alternate Title");
	}

	SECTION("parses related keys") {
		auto const& entry = index.at("test.uhs");
		REQUIRE(entry.relatedKeys.size() == 1);
		REQUIRE(entry.relatedKeys[0] == "other.uhs");
	}

	SECTION("parses entry with no alternate titles or related keys") {
		auto const& entry = index.at("demo.uhs");
		REQUIRE(entry.otherTitles.empty());
		REQUIRE(entry.relatedKeys.empty());
	}
}

TEST_CASE("Downloader::fileIndex throws on HTTP error", "[downloader]") {
	auto mock = std::make_unique<MockHTTPClient>();
	mock->setResponse("/cgi-bin/update.cgi", {500, ""});

	Logger logger{LogLevel::None};
	Downloader downloader{logger, std::move(mock)};

	REQUIRE_THROWS_AS(downloader.fileIndex(), HTTPError);
}

TEST_CASE("Downloader::download throws for unknown file", "[downloader]") {
	auto mock = std::make_unique<MockHTTPClient>();
	mock->setResponse("/cgi-bin/update.cgi", {200, FileIndexXML});

	Logger logger{LogLevel::None};
	Downloader downloader{logger, std::move(mock)};

	auto tempDir = std::filesystem::temp_directory_path().string();
	REQUIRE_THROWS_AS(downloader.download(tempDir, "nonexistent.uhs"), FileError);
}

TEST_CASE("Downloader::download throws for nonexistent directory", "[downloader]") {
	auto mock = std::make_unique<MockHTTPClient>();
	mock->setResponse("/cgi-bin/update.cgi", {200, FileIndexXML});

	Logger logger{LogLevel::None};
	Downloader downloader{logger, std::move(mock)};

	REQUIRE_THROWS_AS(downloader.download("/nonexistent/path", "test.uhs"), FileError);
}

TEST_CASE("Downloader::download throws on HTTP error", "[downloader]") {
	auto mock = std::make_unique<MockHTTPClient>();
	mock->setResponse("/cgi-bin/update.cgi", {200, FileIndexXML});
	mock->setResponse("/files/test.uhs", {500, ""});

	Logger logger{LogLevel::None};
	Downloader downloader{logger, std::move(mock)};

	auto tempDir = std::filesystem::temp_directory_path().string();
	REQUIRE_THROWS_AS(downloader.download(tempDir, "test.uhs"), HTTPError);
}

} // namespace UHS

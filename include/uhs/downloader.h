#if !defined UHS_DOWNLOADER_H
#define UHS_DOWNLOADER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "httplib.h"

#include "uhs/logger.h"

namespace UHS {

class Downloader {
public:
	struct FileMetadata {
		int compressedSize;                   // fsize
		std::string filename;                 // fname
		std::vector<std::string> otherTitles; // falttitle
		std::vector<std::string> relatedKeys; // frelated
		std::string title;                    // ftitle
		int uncompressedSize;                 // ffullsize
		std::string url;                      // furl
	};

	class HTTPClient {
	public:
		virtual ~HTTPClient() = default;

		virtual httplib::Result Get(
		    std::string const& path, httplib::Headers const& headers) = 0;
		virtual httplib::Result Get(std::string const& path,
		    httplib::Headers const& headers,
		    httplib::ContentReceiver contentReceiver) = 0;
	};

	class DefaultHTTPClient : public HTTPClient {
	public:
		DefaultHTTPClient(std::string const& url);

		httplib::Result Get(
		    std::string const& path, httplib::Headers const& headers) override;
		httplib::Result Get(std::string const& path, httplib::Headers const& headers,
		    httplib::ContentReceiver contentReceiver) override;

	private:
		httplib::Client client_;
	};

	using FileIndex = std::map<std::string, FileMetadata const>;

	Downloader(Logger const& logger);
	Downloader(Logger const& logger, std::unique_ptr<HTTPClient> httpClient);

	void download(std::string const& dir, std::string const& file);
	void download(std::string const& dir, std::vector<std::string> const& files);
	FileIndex const& fileIndex();

private:
	static constexpr auto BaseURL = "http://www.invisiclues.com";
	static constexpr auto FileIndexPath = "/cgi-bin/update.cgi";

	FileIndex fileIndex_;
	std::unique_ptr<HTTPClient> httpClient_;
	httplib::Headers httpHeaders_;
	Logger const logger_;

	void loadFileIndex();
};

} // namespace UHS

#endif

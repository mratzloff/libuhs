#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/options.h"
#include "uhs/uhs_write.h"
#include "uhs/writer/uhs_writer.h"

namespace UHS {

static Logger const logger{LogLevel::None};

static std::string buildTestUHS() {
	auto document = Document::create(VersionType::Version88a);
	document->title("Test Game");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Hints");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("Where is the key?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Under the mat."));

	std::ostringstream out;
	UHSWriter writer{logger, out};
	writer.write(document);
	return out.str();
}

static std::filesystem::path writeTestFile() {
	auto data = buildTestUHS();
	auto path = std::filesystem::temp_directory_path() / "test_uhs_write.uhs";
	std::ofstream out{path, std::ios::out | std::ios::binary | std::ios::trunc};
	out << data;
	return path;
}

static std::string readTempFile(std::filesystem::path const& path) {
	std::ifstream in{path, std::ios::binary};
	return std::string{
	    std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

// write(logger, format, infile, options) — stdout

TEST_CASE("write to stdout rejects invalid format", "[uhs_write]") {
	auto path = writeTestFile();
	auto result = write(logger, "xml", path.string());
	REQUIRE_FALSE(result);
	std::filesystem::remove(path);
}

TEST_CASE("write to stdout rejects missing file", "[uhs_write]") {
	auto result = write(logger, "html", "/nonexistent/file.uhs");
	REQUIRE_FALSE(result);
}

TEST_CASE("write to stdout succeeds with valid input", "[uhs_write]") {
	auto path = writeTestFile();

	std::ostringstream capture;
	auto* original = std::cout.rdbuf(capture.rdbuf());
	auto result = write(logger, "html", path.string());
	std::cout.rdbuf(original);

	REQUIRE(result);
	REQUIRE(capture.str().size() > 0);
	REQUIRE(capture.str().find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(capture.str().find("Test Game") != std::string::npos);
	REQUIRE(capture.str().find("Under the mat.") != std::string::npos);

	std::filesystem::remove(path);
}

// write(logger, format, infile, outfile, options) — file

TEST_CASE("write to file rejects invalid format", "[uhs_write]") {
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_uhs_write_bad_fmt.html";
	auto result = write(logger, "xml", inpath.string(), outpath);
	REQUIRE_FALSE(result);
	REQUIRE_FALSE(std::filesystem::exists(outpath));
	std::filesystem::remove(inpath);
}

TEST_CASE("write to file rejects missing input file", "[uhs_write]") {
	auto outpath = std::filesystem::temp_directory_path() / "test_uhs_write_missing.html";
	auto result = write(logger, "html", "/nonexistent/file.uhs", outpath);
	REQUIRE_FALSE(result);
	REQUIRE_FALSE(std::filesystem::exists(outpath));
}

TEST_CASE("write to file rejects invalid output path", "[uhs_write]") {
	auto path = writeTestFile();
	auto result = write(
	    logger, "html", path.string(), std::filesystem::path{"/nonexistent/out.html"});
	REQUIRE_FALSE(result);
	std::filesystem::remove(path);
}

TEST_CASE("write to file produces valid HTML output", "[uhs_write]") {
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_uhs_write.html";
	auto result = write(logger, "html", inpath.string(), outpath);
	REQUIRE(result);
	REQUIRE(std::filesystem::exists(outpath));

	auto content = readTempFile(outpath);
	REQUIRE(content.size() > 0);
	REQUIRE(content.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(content.find("Test Game") != std::string::npos);
	REQUIRE(content.find("Under the mat.") != std::string::npos);

	std::filesystem::remove(inpath);
	std::filesystem::remove(outpath);
}

TEST_CASE("write to file produces valid JSON output", "[uhs_write]") {
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_uhs_write.json";
	auto result = write(logger, "json", inpath.string(), outpath);
	REQUIRE(result);

	auto content = readTempFile(outpath);
	REQUIRE(content.size() > 0);
	REQUIRE(content.find("\"title\"") != std::string::npos);
	REQUIRE(content.find("Test Game") != std::string::npos);

	std::filesystem::remove(inpath);
	std::filesystem::remove(outpath);
}

TEST_CASE("write to file produces valid UHS output", "[uhs_write]") {
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_uhs_write_out.uhs";
	auto result = write(logger, "uhs", inpath.string(), outpath);
	REQUIRE(result);

	auto content = readTempFile(outpath);
	REQUIRE(content.size() > 0);
	REQUIRE(content.substr(0, 3) == "UHS");

	std::filesystem::remove(inpath);
	std::filesystem::remove(outpath);
}

// write(logger, format, infile, output, options) — string

TEST_CASE("write to string rejects invalid format", "[uhs_write]") {
	auto path = writeTestFile();
	std::string output;
	auto result = write(logger, "pdf", path.string(), output);
	REQUIRE_FALSE(result);
	REQUIRE(output.empty());
	std::filesystem::remove(path);
}

TEST_CASE("write to string rejects missing file", "[uhs_write]") {
	std::string output;
	auto result = write(logger, "html", "/nonexistent/file.uhs", output);
	REQUIRE_FALSE(result);
	REQUIRE(output.empty());
}

TEST_CASE("write to string produces HTML with expected content", "[uhs_write]") {
	auto path = writeTestFile();
	std::string output;
	auto result = write(logger, "html", path.string(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(output.find("Test Game") != std::string::npos);
	REQUIRE(output.find("Where is the key?") != std::string::npos);
	REQUIRE(output.find("Under the mat.") != std::string::npos);
	std::filesystem::remove(path);
}

TEST_CASE("write to string produces JSON with expected content", "[uhs_write]") {
	auto path = writeTestFile();
	std::string output;
	auto result = write(logger, "json", path.string(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("\"title\"") != std::string::npos);
	REQUIRE(output.find("Test Game") != std::string::npos);
	REQUIRE(output.find("Under the mat.") != std::string::npos);
	std::filesystem::remove(path);
}

TEST_CASE("write to string produces tree with expected content", "[uhs_write]") {
	auto path = writeTestFile();
	std::string output;
	auto result = write(logger, "tree", path.string(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("Test Game") != std::string::npos);
	std::filesystem::remove(path);
}

TEST_CASE("write to string produces UHS with signature", "[uhs_write]") {
	auto path = writeTestFile();
	std::string output;
	auto result = write(logger, "uhs", path.string(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.substr(0, 3) == "UHS");
	std::filesystem::remove(path);
}

// write(logger, format, data, length, output, options) — buffer

TEST_CASE("write from buffer rejects invalid format", "[uhs_write]") {
	auto data = buildTestUHS();
	std::string output;
	auto result = write(logger, "yaml", data.data(), data.size(), output);
	REQUIRE_FALSE(result);
	REQUIRE(output.empty());
}

TEST_CASE("write from buffer produces HTML with expected content", "[uhs_write]") {
	auto data = buildTestUHS();
	std::string output;
	auto result = write(logger, "html", data.data(), data.size(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(output.find("Test Game") != std::string::npos);
	REQUIRE(output.find("Under the mat.") != std::string::npos);
}

TEST_CASE("write from buffer produces JSON with expected content", "[uhs_write]") {
	auto data = buildTestUHS();
	std::string output;
	auto result = write(logger, "json", data.data(), data.size(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("\"title\"") != std::string::npos);
	REQUIRE(output.find("Test Game") != std::string::npos);
}

TEST_CASE("write from buffer produces tree with expected content", "[uhs_write]") {
	auto data = buildTestUHS();
	std::string output;
	auto result = write(logger, "tree", data.data(), data.size(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.find("Test Game") != std::string::npos);
}

TEST_CASE("write from buffer produces UHS with signature", "[uhs_write]") {
	auto data = buildTestUHS();
	std::string output;
	auto result = write(logger, "uhs", data.data(), data.size(), output);
	REQUIRE(result);
	REQUIRE(output.size() > 0);
	REQUIRE(output.substr(0, 3) == "UHS");
}

TEST_CASE("write from buffer produces same output as write from file", "[uhs_write]") {
	auto data = buildTestUHS();
	auto path = writeTestFile();

	std::string fromFile;
	auto fileResult = write(logger, "html", path.string(), fromFile);
	REQUIRE(fileResult);

	std::string fromBuffer;
	auto bufferResult = write(logger, "html", data.data(), data.size(), fromBuffer);
	REQUIRE(bufferResult);

	REQUIRE(fromFile == fromBuffer);

	std::filesystem::remove(path);
}

// Validation

TEST_CASE("write rejects invalid media directory", "[uhs_write]") {
	auto path = writeTestFile();
	Options options;
	options.mediaDir = "/nonexistent/media/";

	SECTION("write to string") {
		std::string output;
		auto result = write(logger, "json", path.string(), output, options);
		REQUIRE_FALSE(result);
		REQUIRE(output.empty());
	}

	SECTION("write to file") {
		auto outpath =
		    std::filesystem::temp_directory_path() / "test_uhs_write_media.html";
		auto result = write(logger, "html", path.string(), outpath, options);
		REQUIRE_FALSE(result);
		REQUIRE_FALSE(std::filesystem::exists(outpath));
	}

	std::filesystem::remove(path);
}

} // namespace UHS

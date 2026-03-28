#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "uhs/constants.h"
#include "uhs/document.h"
#include "uhs/element.h"
#include "uhs/logger.h"
#include "uhs/node/group_node.h"
#include "uhs/node/text_node.h"
#include "uhs/writer/uhs_writer.h"

extern "C" {
#include "uhs_bridge.h"
}

namespace UHS {

static Logger const logger{LogLevel::None};

struct StderrSilencer {
	std::ostringstream sink;
	std::streambuf* original;

	StderrSilencer() : original{std::cerr.rdbuf(sink.rdbuf())} {}
	~StderrSilencer() { std::cerr.rdbuf(original); }
};

static std::string buildTestUHS() {
	auto document = Document::create(VersionType::Version88a);
	document->title("Bridge Test");

	auto subject = Element::create(ElementType::Subject);
	subject->title("Hints");
	document->appendChild(subject);

	auto hint = Element::create(ElementType::Hint);
	hint->title("How do I cross?");
	subject->appendChild(hint);

	auto group = GroupNode::create(0, 0);
	hint->appendChild(group);
	group->appendChild(TextNode::create("Use the rope."));

	std::ostringstream out;
	UHSWriter writer{logger, out};
	writer.write(document);
	return out.str();
}

static std::filesystem::path writeTestFile() {
	auto data = buildTestUHS();
	auto path = std::filesystem::temp_directory_path() / "test_bridge.uhs";
	std::ofstream out{path, std::ios::out | std::ios::binary | std::ios::trunc};
	out << data;
	return path;
}

// uhs_write_string

TEST_CASE("uhs_write_string succeeds with valid input", "[bridge]") {
	auto path = writeTestFile();
	char* output = nullptr;
	auto result = uhs_write_string("html", path.string().c_str(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);

	std::string html{output};
	REQUIRE(html.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(html.find("Bridge Test") != std::string::npos);
	REQUIRE(html.find("Use the rope.") != std::string::npos);

	std::free(output);
	std::filesystem::remove(path);
}

TEST_CASE("uhs_write_string fails with invalid format", "[bridge]") {
	StderrSilencer silencer;
	auto path = writeTestFile();
	char* output = nullptr;
	auto result = uhs_write_string("xml", path.string().c_str(), &output);
	REQUIRE(result == -1);
	std::filesystem::remove(path);
}

TEST_CASE("uhs_write_string fails with missing file", "[bridge]") {
	StderrSilencer silencer;
	char* output = nullptr;
	auto result = uhs_write_string("html", "/nonexistent/file.uhs", &output);
	REQUIRE(result == -1);
}

TEST_CASE("uhs_write_string produces valid JSON", "[bridge]") {
	auto path = writeTestFile();
	char* output = nullptr;
	auto result = uhs_write_string("json", path.string().c_str(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);

	std::string json{output};
	REQUIRE(json.find("\"title\"") != std::string::npos);
	REQUIRE(json.find("Bridge Test") != std::string::npos);

	std::free(output);
	std::filesystem::remove(path);
}

TEST_CASE("uhs_write_string produces valid UHS", "[bridge]") {
	auto path = writeTestFile();
	char* output = nullptr;
	auto result = uhs_write_string("uhs", path.string().c_str(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);
	REQUIRE(std::strncmp(output, "UHS", 3) == 0);

	std::free(output);
	std::filesystem::remove(path);
}

// uhs_write_file

TEST_CASE("uhs_write_file succeeds with valid input", "[bridge]") {
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_bridge.html";
	auto result =
	    uhs_write_file("html", inpath.string().c_str(), outpath.string().c_str());
	REQUIRE(result == 0);
	REQUIRE(std::filesystem::exists(outpath));

	std::ifstream in{outpath};
	std::string content{
	    std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
	REQUIRE(content.size() > 0);
	REQUIRE(content.find("Bridge Test") != std::string::npos);

	std::filesystem::remove(inpath);
	std::filesystem::remove(outpath);
}

TEST_CASE("uhs_write_file fails with invalid format", "[bridge]") {
	StderrSilencer silencer;
	auto inpath = writeTestFile();
	auto outpath = std::filesystem::temp_directory_path() / "test_bridge_bad_fmt.html";
	auto result =
	    uhs_write_file("xml", inpath.string().c_str(), outpath.string().c_str());
	REQUIRE(result == -1);
	REQUIRE_FALSE(std::filesystem::exists(outpath));
	std::filesystem::remove(inpath);
}

TEST_CASE("uhs_write_file fails with missing input file", "[bridge]") {
	StderrSilencer silencer;
	auto outpath = std::filesystem::temp_directory_path() / "test_bridge_missing.html";
	auto result =
	    uhs_write_file("html", "/nonexistent/file.uhs", outpath.string().c_str());
	REQUIRE(result == -1);
	REQUIRE_FALSE(std::filesystem::exists(outpath));
}

TEST_CASE("uhs_write_file fails with invalid output path", "[bridge]") {
	StderrSilencer silencer;
	auto inpath = writeTestFile();
	auto result =
	    uhs_write_file("html", inpath.string().c_str(), "/nonexistent/out.html");
	REQUIRE(result == -1);
	std::filesystem::remove(inpath);
}

// uhs_write_buffer

TEST_CASE("uhs_write_buffer succeeds with valid input", "[bridge]") {
	auto data = buildTestUHS();
	char* output = nullptr;
	auto result = uhs_write_buffer("html", data.data(), data.size(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);

	std::string html{output};
	REQUIRE(html.find("<!DOCTYPE html>") != std::string::npos);
	REQUIRE(html.find("Bridge Test") != std::string::npos);
	REQUIRE(html.find("Use the rope.") != std::string::npos);

	std::free(output);
}

TEST_CASE("uhs_write_buffer fails with invalid format", "[bridge]") {
	StderrSilencer silencer;
	auto data = buildTestUHS();
	char* output = nullptr;
	auto result = uhs_write_buffer("xml", data.data(), data.size(), &output);
	REQUIRE(result == -1);
}

TEST_CASE("uhs_write_buffer produces valid JSON", "[bridge]") {
	auto data = buildTestUHS();
	char* output = nullptr;
	auto result = uhs_write_buffer("json", data.data(), data.size(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);

	std::string json{output};
	REQUIRE(json.find("\"title\"") != std::string::npos);
	REQUIRE(json.find("Bridge Test") != std::string::npos);

	std::free(output);
}

TEST_CASE("uhs_write_buffer produces valid UHS", "[bridge]") {
	auto data = buildTestUHS();
	char* output = nullptr;
	auto result = uhs_write_buffer("uhs", data.data(), data.size(), &output);
	REQUIRE(result == 0);
	REQUIRE(output != nullptr);
	REQUIRE(std::strncmp(output, "UHS", 3) == 0);

	std::free(output);
}

TEST_CASE("uhs_write_buffer and uhs_write_string produce same output", "[bridge]") {
	auto data = buildTestUHS();
	auto path = writeTestFile();

	char* bufferOutput = nullptr;
	auto bufferResult = uhs_write_buffer("html", data.data(), data.size(), &bufferOutput);
	REQUIRE(bufferResult == 0);
	REQUIRE(bufferOutput != nullptr);

	char* stringOutput = nullptr;
	auto stringResult = uhs_write_string("html", path.string().c_str(), &stringOutput);
	REQUIRE(stringResult == 0);
	REQUIRE(stringOutput != nullptr);

	REQUIRE(std::strcmp(bufferOutput, stringOutput) == 0);

	std::free(bufferOutput);
	std::free(stringOutput);
	std::filesystem::remove(path);
}

} // namespace UHS

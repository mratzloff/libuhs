#include "../../integration/builders.h"

#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
	std::string outputDir = "build/integration";
	if (argc > 1) {
		outputDir = argv[1];
	}

	auto const& htmlDir = outputDir + "/html";
	auto const& jsonDir = outputDir + "/json";
	auto const& uhsDir = outputDir + "/uhs";
	auto const& uhsSourceDir = outputDir + "/uhs_source";

	std::filesystem::create_directories(htmlDir);
	std::filesystem::create_directories(jsonDir);
	std::filesystem::create_directories(uhsDir);
	std::filesystem::create_directories(uhsSourceDir);

	for (auto const& entry : UHS::allBuilders()) {
		auto document = entry.builder();

		UHS::Options options;
		options.preserve = entry.preserve;

		auto sourceData = UHS::writeUHS(document, options);
		auto parsed = UHS::roundTrip(document, options);

		auto htmlData = UHS::writeHTML(parsed);
		auto htmlPath = htmlDir + "/" + entry.filename + ".html";
		std::ofstream htmlOut(htmlPath);
		htmlOut.write(htmlData.data(), htmlData.size());
		std::cout << htmlPath << std::endl;

		auto jsonData = UHS::writeJSON(parsed);
		auto jsonPath = jsonDir + "/" + entry.filename + ".json";
		std::ofstream jsonOut(jsonPath);
		jsonOut.write(jsonData.data(), jsonData.size());
		std::cout << jsonPath << std::endl;

		auto uhsData = UHS::writeUHS(parsed, options);
		auto uhsPath = uhsDir + "/" + entry.filename + ".uhs";
		std::ofstream uhsOut(uhsPath, std::ios::binary);
		uhsOut.write(uhsData.data(), uhsData.size());
		std::cout << uhsPath << std::endl;

		auto uhsSourcePath = uhsSourceDir + "/" + entry.filename + ".uhs";
		std::ofstream uhsSourceOut(uhsSourcePath, std::ios::binary);
		uhsSourceOut.write(sourceData.data(), sourceData.size());
		std::cout << uhsSourcePath << std::endl;
	}

	return 0;
}

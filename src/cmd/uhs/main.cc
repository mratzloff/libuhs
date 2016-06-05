#include <chrono>
#include <fstream>
#include <iostream>
#include "uhs.h"

int main(int argc, const char* argv[]) {
	auto start = std::chrono::steady_clock::now();

	std::ifstream in {"/Users/matt/Desktop/UHS/hints/maniac.uhs", std::ifstream::in};
	UHS::Parser p {in};
	auto document = p.parse();

	auto end = std::chrono::steady_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
	std::cout << "Time: " << elapsed << std::endl;
}

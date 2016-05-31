#include <fstream>
#include <iostream>
#include "uhs.h"

int main(int argc, const char* argv[]) {
	std::ifstream in {"/Users/matt/Desktop/UHS/hints/maniac.uhs", std::ifstream::in};

	UHS::Parser p {in};
	p.parse();
}

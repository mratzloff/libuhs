#include "uhs_bridge.h"

int main(int argc, char* argv[]) {
	(void) argc;
	(void) argv;

	if (argc < 3) {
		return -1;
	}

	return uhs_write("html", argv[1], argv[2]);
}

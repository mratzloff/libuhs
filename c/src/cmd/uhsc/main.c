#include <stdlib.h>
#include <string.h>

#include "uhs_bridge.h"

int main(int argc, char const* argv[]) {
	(void) argc;
	(void) argv;

	if (argc < 3) {
		return -1;
	}

	char const* dir = argv[1];
	int numFiles = argc - 2;

	char const* files[numFiles];
	for (int i = 0; i < numFiles; ++i) {
		files[i] = argv[i + 2];
	}

	int status = uhs_download(dir, numFiles, files);
	if (status != 0) {
		return -1;
	}

	for (int i = 0; i < numFiles; ++i) {
		char const* file = argv[i + 2];

		char* infile = (char*) malloc(strlen(dir) + strlen(file) + 2);
		strcat(infile, dir);
		strcat(infile, "/");
		strncat(infile, file, strlen(file) - 4);
		strcat(infile, ".uhs");

		char* outfile = (char*) malloc(strlen(dir) + strlen(file) + 3);
		strcat(outfile, dir);
		strcat(outfile, "/");
		strncat(outfile, file, strlen(file) - 4);
		strcat(outfile, ".html");

		uhs_write("html", infile, outfile);
	}
}

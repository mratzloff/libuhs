#include <stddef.h>

int uhs_download(char const* dir, int numFiles, char const** files);
int uhs_write_buffer(char const* format, char const* data, size_t length, char** output);
int uhs_write_file(char const* format, char const* infile, char const* outfile);
int uhs_write_string(char const* format, char const* infile, char** output);

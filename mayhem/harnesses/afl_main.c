/* main() for the AFL (forkserver, one input per process) targets: reads the input file named by
 * argv[1] and runs the harness entry once. The harness sources are compiled for these targets
 * with -DLLVMFuzzerTestOneInput=fz_one_input, so the AFL binaries are plain file-input programs.
 * Used for loaders that keep process-global state their *_Uninit does not reset, or that fault
 * on nearly every short input (libFuzzer stops at the first crash; AFL keeps going). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int fz_one_input(const uint8_t *data, size_t size);

int main(int argc, char **argv)
{
	FILE *f;
	long n;
	uint8_t *buf;

	if (argc < 2) { fprintf(stderr, "usage: %s <input file>\n", argv[0]); return 2; }
	f = fopen(argv[1], "rb");
	if (f == NULL) { perror(argv[1]); return 2; }
	if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0) { fclose(f); return 2; }
	rewind(f);
	buf = malloc(n > 0 ? (size_t)n : 1);
	if (buf == NULL || fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); return 2; }
	fclose(f);
	fz_one_input(buf, (size_t)n);
	free(buf);
	return 0;
}

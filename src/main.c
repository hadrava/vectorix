#include <stdio.h>
#include "pnm_handler.h"

int main(int argc, char **argv) {
	struct pnm_image *test;
	if (argc == 1)
		test = pnm_read(stdin);
	else {
		FILE *input = fopen(argv[1], "r");
		test = pnm_read(input);
		fclose(input);
	}
	struct pnm_image * converted = pnm_convert(test, PNM_BINARY_PBM);
	pnm_write(stdout, converted);
}

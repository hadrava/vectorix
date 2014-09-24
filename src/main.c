#include <stdio.h>
#include "pnm_handler.h"
#include "svg_handler.h"
#include "vectorizer.h"

int main(int argc, char **argv) {
	struct pnm_image *test;
	if (argc == 1)
		test = pnm_read(stdin);
	else {
		FILE *input = fopen(argv[1], "r");
		test = pnm_read(input);
		fclose(input);
	}
	struct pnm_image * converted = pnm_convert(test, PNM_BINARY_PGM);
	pnm_free(test);

	//pnm_write(stdout, converted);
	struct svg_image * vector = vectorize(converted);
	pnm_free(converted);
	svg_write(stdout, vector);
}

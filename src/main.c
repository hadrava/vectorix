#include <stdio.h>
#include "pnm_handler.h"
#include "svg_handler.h"
#include "vectorizer.h"

int main(int argc, char **argv) {
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	struct pnm_image *test;
	if (argc == 1)
		test = pnm_read(stdin);
	else {
		FILE *input = fopen(argv[1], "r");
		test = pnm_read(input);
		fclose(input);
	}
	if (argc > 2)
		svg_output = fopen(argv[2], "w");
	if (argc > 3)
		pnm_output = fopen(argv[3], "w");

	struct pnm_image * converted = pnm_convert(test, PNM_BINARY_PGM);
	pnm_free(test);

	struct svg_image * vector = vectorize(converted);
	//struct svg_image * vector = vectorize_bare(converted);
	render(converted, vector);

	if (svg_output)
		svg_write(svg_output, vector);
	if (pnm_output)
		pnm_write(pnm_output, converted);

	if (argc > 2)
		fclose(svg_output);
	if (argc > 3)
		fclose(pnm_output);
}

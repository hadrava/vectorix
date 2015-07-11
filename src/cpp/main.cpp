#include <iostream>
#include <utility>
#include "lines.h"
#include "pnm_handler.h"
#include "export_svg.h"
#include "vectorizer.h"
#include "render.h"

using namespace std;
using namespace vect;
using namespace pnm;

int main(int argc, char **argv) {
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image;
	if (argc == 1)
		input_image.read(stdin);
	else {
		FILE *input = fopen(argv[1], "r");
		input_image.read(input);
		fclose(input);
	}
	if (argc > 2)
		svg_output = fopen(argv[2], "w");
	if (argc > 3)
		pnm_output = fopen(argv[3], "w");

	input_image.convert(PNM_BINARY_PPM);

	auto vector = vectorize(input_image);
	//auto vector = vectorize_bare(input_image);

	if (svg_output)
		export_svg<editable>::write(svg_output, vector);
	if (pnm_output) {
		input_image = render(vector);
		input_image.write(pnm_output);
	}

	if (argc > 2)
		fclose(svg_output);
	if (argc > 3)
		fclose(pnm_output);
	return 0;
}

#include <iostream>
#include <utility>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "export_svg.h"
#include "vectorizer.h"
#include "potrace_handler.h"
#include "custom_vectorizer.h"
#include "render.h"
#include "time_measurement.h"

using namespace std;
using namespace vect;
using namespace pnm;
using namespace time_measurement;

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

	timer vectorization_timer(0);
	vectorization_timer.start();
		//auto vector = vectorizer<stupid>::run(input_image);
		auto vector = vectorizer<custom>::run(input_image);
		//auto vector = vectorizer<potrace>::run(input_image);
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read()/1e6);

	if (svg_output)
		export_svg<editable>::write(svg_output, vector);
	if (pnm_output) {
		timer render_timer(0);
		render_timer.start();
			input_image = renderer::render(vector);
		render_timer.stop();
		fprintf(stderr, "Render time: %fs\n", render_timer.read()/1e6);
		input_image.write(pnm_output);
	}

	if (argc > 2)
		fclose(svg_output);
	if (argc > 3)
		fclose(pnm_output);
	return 0;
}

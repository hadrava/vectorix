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
#include "opencv_render.h"
#include "parameters.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace vect;
using namespace pnm;
using namespace tmea;

int main(int argc, char **argv) { // main [input.pnm [output.svg [output.pnm]]]
	if (argc == 1)
		load_params(stdin);
	else {
		FILE *input = fopen(argv[1], "r");
		load_params(input);
		fclose(input);
	}

	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image;
	if (global_params.input.pnm_input_name.empty())
		input_image.read(stdin);
	else {
		FILE *input = fopen(global_params.input.pnm_input_name.c_str(), "r");
		input_image.read(input); // Read from file.
		fclose(input);
	}
	if (!global_params.output.svg_output_name.empty())
		svg_output = fopen(global_params.output.svg_output_name.c_str(), "w");
	if (!global_params.output.pnm_output_name.empty())
		pnm_output = fopen(global_params.output.pnm_output_name.c_str(), "w");


	input_image.convert(PNM_BINARY_PPM);
	v_image vector;
	timer vectorization_timer(0); // Measure time (if compiled with TIMER_MEASURE), without cpu preheating.
	vectorization_timer.start();
		switch (global_params.vectorization_method) {
			case 0: // Custom center-line based vectorizer.
				vector = vectorizer<custom>::run(input_image);
				break;
			case 1:
				vector = vectorizer<potrace>::run(input_image);
				break;
			case 2: // Stupid vectorizer - just output simple line.
				vector = vectorizer<stupid>::run(input_image);
		}
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read()/1e6);

	//show output
	if (global_params.output.show_opencv_rendered_window || !global_params.output.save_opencv_rendered_name.empty()) {
		cv::Mat output = cv::Mat::zeros(vector.height, vector.width, CV_8UC(3));
		opencv_render(vector, output);
		if (global_params.output.show_opencv_rendered_window) {
			cv::imshow("Opencv render", output);
			cv::waitKey();
		}
		if (!global_params.output.save_opencv_rendered_name.empty())
			imwrite(global_params.output.save_opencv_rendered_name, output);
	}

	if (svg_output) {
		export_svg<editable>::write(svg_output, vector); // Write output to stdout / file specified by second parameter.
		//export_svg<grouped>::write(svg_output, vector); // Write output to stdout / file specified by second parameter.
		if (svg_output != stdout)
			fclose(svg_output);
	}
	if (pnm_output) {
		timer render_timer(0);
		render_timer.start();
			input_image = renderer::render(vector); // Render bezier curves.
		render_timer.stop();
		fprintf(stderr, "Render time: %fs\n", render_timer.read()/1e6);
		input_image.write(pnm_output); // Write rendered image to file specified by third parameter.
		fclose(pnm_output);
	}
	return 0;
}

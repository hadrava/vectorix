#include <iostream>
#include <utility>
#include <cstdio>
#include "v_image.h"
#include "pnm_handler.h"
#include "export_svg.h"
#include "export_ps.h"
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

int main(int argc, char **argv) { // ./main [configuration]
	/*
	 * Load parameters
	 */
	global_params = default_params(); // Set default parameters
	if (argc == 1) {
		fprintf(stderr, "Reading parameters from standard input...\n");
		load_params(stdin);
	}
	else {
		fprintf(stderr, "Reading parameters from file...\n");
		FILE *input = fopen(argv[1], "r");
		load_params(input);
		fclose(input);
	}

	/*
	 * Load input image
	 */
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image;
	if ((global_params.vectorization_method == 0) && (!global_params.input.custom_input_name.empty())) { // Load input by OpenCV
		fprintf(stderr, "File will be loaded by OpenCV.\n");
	}
	else if (global_params.input.pnm_input_name.empty() && (global_params.vectorization_method != 0)) {
		fprintf(stderr, "No PNM input file speficied.\n");
		return 1; // No input file, halting
	}
	else {
		FILE *input = fopen(global_params.input.pnm_input_name.c_str(), "r"); // Load input from PNM image
		input_image.read(input); // Read from file.
		fclose(input);
	}

	/*
	 * Vectorize image
	 */
	input_image.convert(PNM_BINARY_PPM);
	v_image vector;
	timer vectorization_timer(0); // Measure time (if compiled with TIMER_MEASURE), 0 -- without cpu preheating
	vectorization_timer.start();
		switch (global_params.vectorization_method) {
			case 0: // Custom center-line based vectorizer
				vector = vectorizer<custom>::run(input_image);
				break;
			case 1: // Use potracelib
				vector = vectorizer<potrace>::run(input_image);
				break;
			case 2: // Stupid - just output simple line; frankly, it ignores input image
				vector = vectorizer<stupid>::run(input_image);
		}
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read()/1e6);

	if (global_params.output.false_colors)
		vector.false_colors(global_params.output.false_colors);

	/*
	 * Show and save output using OpenCV
	 */
	if (global_params.output.show_opencv_rendered_window || !global_params.output.save_opencv_rendered_name.empty()) {
		cv::Mat output = cv::Mat::zeros(vector.height, vector.width, CV_8UC(3));
		opencv_render(vector, output); // Render whole image
		if (global_params.output.show_opencv_rendered_window) {
			cv::imshow("Opencv render", output); // Display output
			cv::waitKey();
		}
		if (!global_params.output.save_opencv_rendered_name.empty())
			imwrite(global_params.output.save_opencv_rendered_name, output); // Save image to file
	}
	/*
	 * Save output to PNM
	 */
	if (!global_params.output.pnm_output_name.empty())
		pnm_output = fopen(global_params.output.pnm_output_name.c_str(), "w");
	if (pnm_output) {
		timer render_timer(0);
		render_timer.start();
			input_image = renderer::render(vector); // Render bezier curves
		render_timer.stop();
		fprintf(stderr, "Render time: %fs\n", render_timer.read()/1e6);
		input_image.write(pnm_output); // Write rendered image to file
		fclose(pnm_output);
	}

	/*
	 * Save vector output to stdout / file specified in configfile
	 */
	if (!global_params.output.vector_output_name.empty())
		svg_output = fopen(global_params.output.vector_output_name.c_str(), "w");
	if (svg_output) {
		vector.convert_to_variable_width(global_params.output.export_type, global_params.output); // Convert image before writing
		if (global_params.output.output_engine == 0) {
			export_svg<editable>::write(svg_output, vector); // Write svg
		}
		else {
			export_ps::write(svg_output, vector); // Write postscript
		}
		if (svg_output != stdout)
			fclose(svg_output);
	}
	/*
	 * Save parameters
	 */
	if (!global_params.save_parameters_name.empty()) {
		FILE *config = fopen(global_params.save_parameters_name.c_str(), (global_params.save_parameters_append != 0) ? "a" : "w");
		save_params(config);
		fclose(config);
	}
	return 0;
}

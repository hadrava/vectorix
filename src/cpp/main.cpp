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
	params parameters = default_params(); // Set default parameters
	if (argc == 1) {
		fprintf(stderr, "Reading parameters from standard input...\n");
		load_params(stdin, parameters);
	}
	else {
		fprintf(stderr, "Reading parameters from file...\n");
		//FILE *input = fopen(argv[1], "r");
		load_params(argv[1], parameters);
		//fclose(input);
	}

	/*
	 * Load input image
	 */
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image;
	if ((parameters.vectorization_method == 0) && (!parameters.input.custom_input_name.empty())) { // Load input by OpenCV
		fprintf(stderr, "File will be loaded by OpenCV.\n");
	}
	else if (parameters.input.pnm_input_name.empty()) {
		fprintf(stderr, "No PNM input file speficied.\n");
		if (!parameters.save_parameters_name.empty()) { // We should write config file (useful for creating empty config)
			save_params(parameters.save_parameters_name, parameters);
		}
		return 1; // No input file, halting
	}
	else {
		FILE *input = fopen(parameters.input.pnm_input_name.c_str(), "r"); // Load input from PNM image
		if (!input) {
			fprintf(stderr, "Failed to read input image.\n");
			return 1;
		}
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
		switch (parameters.vectorization_method) {
			case 0: // Custom center-line based vectorizer
				vector = vectorizer<custom>::run(input_image, parameters);
				break;
			case 1: // Use potracelib
				vector = vectorizer<potrace>::run(input_image, parameters);
				break;
			case 2: // Stupid - just output simple line; frankly, it ignores input image
				vector = vectorizer<stupid>::run(input_image, parameters);
		}
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read()/1e6);

	if (parameters.output.false_colors)
		vector.false_colors(parameters.output.false_colors);

	/*
	 * Show and save output using OpenCV
	 */
	if (parameters.output.show_opencv_rendered_window || !parameters.output.save_opencv_rendered_name.empty()) {
		cv::Mat output = cv::Mat::zeros(vector.height, vector.width, CV_8UC(3));
		opencv_render(vector, output, parameters); // Render whole image
		if (parameters.output.show_opencv_rendered_window) {
			cv::imshow("Opencv render", output); // Display output
			cv::waitKey();
		}
		if (!parameters.output.save_opencv_rendered_name.empty())
			imwrite(parameters.output.save_opencv_rendered_name, output); // Save image to file
	}
	/*
	 * Save output to PNM
	 */
	if (!parameters.output.pnm_output_name.empty())
		pnm_output = fopen(parameters.output.pnm_output_name.c_str(), "w");
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
	if (!parameters.output.vector_output_name.empty())
		svg_output = fopen(parameters.output.vector_output_name.c_str(), "w");
	if (svg_output) {
		vector.convert_to_variable_width(parameters.output.export_type, parameters.output); // Convert image before writing
		if (parameters.output.output_engine == 0) {
			export_svg<editable>::write(svg_output, vector, parameters); // Write svg
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
	if (!parameters.save_parameters_name.empty()) {
		save_params(parameters.save_parameters_name, parameters);
	}
	return 0;
}

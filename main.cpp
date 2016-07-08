#include <iostream>
#include <utility>
#include <cstdio>
#include "geom.h"
#include "v_image.h"
#include "pnm_handler.h"
#include "exporter_svg.h"
#include "exporter_ps.h"
#include "vectorizer.h"
#include "potrace_handler.h"
#include "custom_vectorizer.h"
#include "render.h"
#include "timer.h"
#include "opencv_render.h"
#include "parameters.h"
#include "finisher.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace vectorix;

int main(int argc, char **argv) { // ./main [configuration]
	/*
	 * Load parameters
	 */
	params parameters; // Set default parameters
	if (argc == 1) {
		fprintf(stderr, "Reading parameters from standard input...\n");
		parameters.load_params(stdin);
	}
	else {
		fprintf(stderr, "Reading parameters from file...\n");
		//FILE *input = fopen(argv[1], "r");
		parameters.load_params(argv[1]);
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
			parameters.save_params(parameters.save_parameters_name);
		}
		return 1; // No input file, halting
	}
	else {
		FILE *input = fopen(parameters.input.pnm_input_name.c_str(), "r"); // Load input from PNM image
		if (!input) {
			fprintf(stderr, "Failed to read input image.\n");
			return 1;
		}
		input_image.read(input); // Read from file
		fclose(input);
	}

	/*
	 * Vectorize image
	 */
	input_image.convert(pnm_variant_type::binary_ppm);
	v_image vector;
	timer vectorization_timer(0); // Measure time
	vectorization_timer.start();
		vectorizer *ve;
		switch (parameters.vectorization_method) {
			case 0: // Custom center-line based vectorizer
				ve = new vectorizer_vectorix;
				break;
			case 1: // Use potracelib
				ve = new vectorizer_potrace;
				break;
			case 2: // Stupid - just output simple line; frankly, it ignores input image
				ve = new vectorizer_example;
		}
		vector = ve->vectorize(input_image, parameters);
		delete ve;
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read());

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
		fprintf(stderr, "Render time: %fs\n", render_timer.read());
		input_image.write(pnm_output); // Write rendered image to file
		fclose(pnm_output);
	}

	// Pre-export changes & transformations
	finisher fin;
	fin.apply_settings(vector, parameters);
	fin.apply_settings(vector, parameters);

	/*
	 * Save vector output to stdout / file specified in configfile
	 */
	if (!parameters.output.vector_output_name.empty())
		svg_output = fopen(parameters.output.vector_output_name.c_str(), "w");
	if (svg_output) {
		if (parameters.output.output_engine == 0) {
			exporter_svg ex;
			ex.write(svg_output, vector); // Write svg
		}
		else {
			exporter_ps ex;
			ex.write(svg_output, vector); // Write postscript
		}
		if (svg_output != stdout)
			fclose(svg_output);
	}
	/*
	 * Save parameters
	 */
	if (!parameters.save_parameters_name.empty()) {
		parameters.save_params(parameters.save_parameters_name);
	}
	return 0;
}

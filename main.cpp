#include <iostream>
#include <utility>
#include <cstdio>
#include "geom.h"
#include "v_image.h"
#include "pnm_handler.h"
#include "exporter_svg.h"
#include "exporter_ps.h"
#include "vectorizer.h"
#include "vectorizer_potrace.h"
#include "vectorizer_vectorix.h"
#include "render.h"
#include "timer.h"
#include "opencv_render.h"
#include "parameters.h"
#include "finisher.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace vectorix;

class main_params {
public:
	int *vectorization_method;
	int *save_parameters_append;
	std::string *save_parameters_name;
	std::string *pnm_input_name;
	int *show_opencv_rendered_window;
	std::string *custom_input_name;
	int *output_engine;
	std::string *vector_output_name;
	std::string *pnm_output_name;
	std::string *save_opencv_rendered_name;
	void bind(parameters &par) {
		par.add_comment("# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #");
		par.add_comment("Vectorization method: 0: Custom, 1: Potrace, 2: Stupid");
		par.bind_param(vectorization_method, "vectorization_method", 0);
		par.add_comment("Save parameters to file on exit: 0: overwrite, 1: append");
		par.bind_param(save_parameters_append, "parameters_append", 0);
		par.bind_param(save_parameters_name, "file_parameters", (std::string) "vectorix.conf");
		par.add_comment("General input pnm file");
		par.bind_param(pnm_input_name, "file_pnm_input", (std::string) "");
		par.add_comment("Input from any file format supported by OpenCV (only with Custom vectorization method, this option overrides pnm_input):");
		par.bind_param(custom_input_name, "file_input", (std::string) "");
		par.bind_param(show_opencv_rendered_window, "show_rendered_window", 0);
		par.add_comment("Output file format: 0: svg, 1: ps");
		par.bind_param(output_engine, "output_engine", 0);
		par.bind_param(vector_output_name, "file_vector_output", (std::string) "");
		par.bind_param(pnm_output_name, "file_pnm_output", (std::string) "");
		par.bind_param(save_opencv_rendered_name, "file_opencv_output", (std::string) "");
	}
};

int main(int argc, char **argv) { // ./main [configuration]
	/*
	 * Load parameters
	 */
	parameters par;
	main_params my_pars;
	my_pars.bind(par);

	if (argc == 1) {
		fprintf(stderr, "Reading parameters from standard input...\n");
		par.load_params(stdin);
	}
	else {
		fprintf(stderr, "Reading parameters from file...\n");
		//FILE *input = fopen(argv[1], "r");
		par.load_params(argv[1]);
		//fclose(input);
	}

	/*
	 * Load input image
	 */
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image(par);
	if ((*my_pars.vectorization_method == 0) && (!my_pars.custom_input_name->empty())) { // Load input by OpenCV
		fprintf(stderr, "File will be loaded by OpenCV.\n");
	}
	else if (my_pars.pnm_input_name->empty()) {
		fprintf(stderr, "No PNM input file speficied.\n");
		if (!my_pars.save_parameters_name->empty()) { // We should write config file (useful for creating empty config)
			par.save_params(*my_pars.save_parameters_name, *my_pars.save_parameters_append == 1);
		}
		return 1; // No input file, halting
	}
	else {
		FILE *input = fopen(my_pars.pnm_input_name->c_str(), "r"); // Load input from PNM image
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
	vectorizer *ve;
	switch (*my_pars.vectorization_method) {
		case 0: // Custom center-line based vectorizer
			ve = new vectorizer_vectorix(par);
			break;
		case 1: // Use potracelib
			ve = new vectorizer_potrace(par);
			break;
		case 2: // Stupid - just output simple line; frankly, it ignores input image
			ve = new vectorizer_example(par);
	}
	timer vectorization_timer(0); // Measure time
	vectorization_timer.start();
		vector = ve->vectorize(input_image);
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read());
	delete ve;

	/*
	 * Show and save output using OpenCV
	 */
	if (*my_pars.show_opencv_rendered_window || !my_pars.save_opencv_rendered_name->empty()) {
		cv::Mat output = cv::Mat::zeros(vector.height, vector.width, CV_8UC(3));
		opencv_render(vector, output, par); // Render whole image
		if (*my_pars.show_opencv_rendered_window) {
			cv::imshow("Opencv render", output); // Display output
			cv::waitKey();
		}
		if (!my_pars.save_opencv_rendered_name->empty())
			imwrite(*my_pars.save_opencv_rendered_name, output); // Save image to file
	}
	/*
	 * Save output to PNM
	 */
	if (!my_pars.pnm_output_name->empty())
		pnm_output = fopen(my_pars.pnm_output_name->c_str(), "w");
	if (pnm_output) {
		timer render_timer(0);
		render_timer.start();
		renderer re(par);
		input_image = re.render(vector); // Render bezier curves
		render_timer.stop();
		fprintf(stderr, "Render time: %fs\n", render_timer.read());
		input_image.write(pnm_output); // Write rendered image to file
		fclose(pnm_output);
	}

	// Pre-export changes & transformations
	finisher fin(par);
	fin.apply_settings(vector);

	/*
	 * Save vector output to stdout / file specified in configfile
	 */
	if (!my_pars.vector_output_name->empty())
		svg_output = fopen(my_pars.vector_output_name->c_str(), "w");
	if (svg_output) {
		if (*my_pars.output_engine == 0) {
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
	if (!my_pars.save_parameters_name->empty()) {
		par.save_params(*my_pars.save_parameters_name);
	}
	return 0;
}

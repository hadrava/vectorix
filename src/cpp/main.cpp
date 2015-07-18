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
#include <opencv2/opencv.hpp>

using namespace std;
using namespace vect;
using namespace pnm;
using namespace tmea;

int main(int argc, char **argv) { // main [input.pnm [output.svg [output.pnm]]]
	FILE *svg_output = stdout;
	FILE *pnm_output = NULL;
	pnm_image input_image;
	if (argc == 1)
		input_image.read(stdin); // Without parameter read from stdin.
	else {
		FILE *input = fopen(argv[1], "r");
		input_image.read(input); // Read from file.
		fclose(input);
	}
	if (argc > 2)
		svg_output = fopen(argv[2], "w");
	if (argc > 3)
		pnm_output = fopen(argv[3], "w");

	input_image.convert(PNM_BINARY_PPM);

	custom::par.step1.threshold_type = 0;
	custom::par.step1.threshold = 127;
	custom::par.step1.save_threshold_name = "out/threshold.png";

	custom::par.step2.type = 0;
	custom::par.step2.show_window = 1;
	custom::par.step2.save_peeled_name = "out/skeletonization_%03d.png";
	custom::par.step2.save_skeleton_name = "out/skeleton.png";
	custom::par.step2.save_distance_name = "out/distance.png";
	custom::par.step2.save_skeleton_normalized_name = "out/skeleton_norm.png";
	custom::par.step2.save_distance_normalized_name = "out/distance_norm.png";
	custom::par.step3.depth_auto_choose = 1; // error allowed (optimization)
	custom::par.step3.max_dfs_depth = 1; // take first, no dfs allowed

	timer vectorization_timer(0); // Measure time (if compiled with TIMER_MEASURE), without cpu preheating.
	vectorization_timer.start();
		//auto vector = vectorizer<stupid>::run(input_image); // Stupid vectorizer - just output simple line.
		//auto vector = vectorizer<custom>::run(input_image); // Custom center-line based vectorizer.
		auto vector = vectorizer<potrace>::run(input_image);
	vectorization_timer.stop();
	fprintf(stderr, "Vectorization time: %fs\n", vectorization_timer.read()/1e6);

	//show output
	cv::Mat output = cv::Mat::zeros(vector.height, vector.width, CV_8UC(3));
	opencv_render(vector, output);
	cv::imshow("vectorizer", output);
	cv::waitKey();


	if (svg_output) {
		export_svg<editable>::write(svg_output, vector); // Write output to stdout / file specified by second parameter.
		//export_svg<grouped>::write(svg_output, vector); // Write output to stdout / file specified by second parameter.
	}
	if (pnm_output) {
		timer render_timer(0);
		render_timer.start();
			input_image = renderer::render(vector); // Render bezier curves.
		render_timer.stop();
		fprintf(stderr, "Render time: %fs\n", render_timer.read()/1e6);
		input_image.write(pnm_output); // Write rendered image to file specified by third parameter.
	}

	if (argc > 2)
		fclose(svg_output);
	if (argc > 3)
		fclose(pnm_output);
	return 0;
}

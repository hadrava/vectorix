#ifndef VECTORIX__THRESHOLDER_H
#define VECTORIX__THRESHOLDER_H

#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"

namespace vectorix {

class thresholder {
public:
	thresholder(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);

		par->add_comment("Phase 1: Thresholding");
		par->add_comment("Invert colors: 0: white lines, 1: black lines");
		par->bind_param(param_invert_input, "invert_colors", 1);
		par->add_comment("Threshold type: 0: Otsu's algorithm, 1: Fixed value");
		par->bind_param(param_threshold_type, "threshold_type", 0);
		par->add_comment("Threshold value: 0-255");
		par->bind_param(param_threshold, "threshold", 127);
		par->add_comment("Adaptive threshold size: 3, 5, 7, ...");
		par->bind_param(param_adaptive_threshold_size, "adaptive_threshold_size", 7);
		par->add_comment("Save thresholded image to file: empty: no output");
		par->bind_param(param_save_threshold_name, "file_threshold_output", (std::string) "");
		par->add_comment("Fill holes (of given size) in lines: 0 = no filling");
		par->bind_param(param_fill_holes, "fill_holes", 0);
		par->add_comment("Save image with filled holes to file: empty = no output");
		par->bind_param(param_save_filled_name, "file_filled_output", (std::string) "");
	}
	void run(const cv::Mat &original, cv::Mat &binary);
	void interactive(cv::TrackbarCallback onChange = 0, void *userdata = 0);

private:
	int *param_invert_input;
	int *param_threshold_type;
	int *param_threshold;
	int *param_adaptive_threshold_size;
	std::string *param_save_threshold_name;
	int *param_fill_holes;
	std::string *param_save_filled_name;

	logger log;
	parameters *par;

	cv::Mat grayscale;
	cv::Mat binary;
	cv::Mat filled;
	int max_image_size;
};

}; // namespace

#endif

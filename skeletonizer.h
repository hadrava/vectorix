#ifndef VECTORIX__SKELETONIZER_H
#define VECTORIX__SKELETONIZER_H

#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"

namespace vectorix {

class skeletonizer {
public:
	skeletonizer(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);

		par->add_comment("Phase 2: Skeletonization");
		par->add_comment("Skeletonization type: 0: diamond-square, 1: square, 2: diamond, 3: circle (slow)");
		par->bind_param(param_skeletonization_type, "type", 0); //TODO rename param
		par->add_comment("Save steps to files, # will be replaced with iteration number");
		par->bind_param(param_save_peeled_name, "files_steps_output", (std::string) "out/skeletonization_#.png");
		par->add_comment("Save skeleton/distance with/without normalization");
		par->bind_param(param_save_skeleton_name, "file_skeleton", (std::string) "out/skeleton.png");
		par->bind_param(param_save_distance_name, "file_distance", (std::string) "out/distance.png");
		par->bind_param(param_save_skeleton_normalized_name, "file_skeleton_norm", (std::string) "out/skeleton_norm.png");
		par->bind_param(param_save_distance_normalized_name, "file_distance_norm", (std::string) "out/distance_norm.png");
	}
	void run(const cv::Mat &binary_input, cv::Mat &skeleton, cv::Mat &distance);
	void interactive(cv::TrackbarCallback onChange = 0, void *userdata = 0);

private:
	void add_to_skeleton(cv::Mat &out, cv::Mat &bw, int iteration); // Add pixels from `bw' to `out'. Something like image `or', but with more information
	void normalize(cv::Mat &out, int max); // Normalize grayscale image for displaying (0-255)

	int *param_skeletonization_type;
	std::string *param_save_peeled_name;
	std::string *param_save_skeleton_name;
	std::string *param_save_distance_name;
	std::string *param_save_skeleton_normalized_name;
	std::string *param_save_distance_normalized_name;

	logger log;
	parameters *par;

	cv::Mat skeleton;
	cv::Mat distance;
	int iteration; // Count of iterations in skeletonization step
};

}; // namespace

#endif

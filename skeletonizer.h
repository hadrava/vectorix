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
		par->add_comment("Skeletonization type: 0: diamond-square, 1: square, 2: diamond, 3: circle (slow), 4: zhang-suen + diamod-square");
		par->bind_param(param_skeletonization_type, "skeletonization_type", 0);
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
	// Add pixels from `bw' to `out'. Something like image `or', but with more information
	template <typename T>
	void add_to_skeleton(cv::Mat &out, const cv::Mat &bw, int iteration) {
		for (int i = 0; i < out.rows; i++) {
			for (int j = 0; j < out.cols; j++) {
				if (!out.at<T>(i, j)) { // Non-zero pixel
					out.at<T>(i, j) = (!!bw.at<uint8_t>(i, j)) * iteration;
				}
			}
		}
	}
	void normalize(const cv::Mat &in, cv::Mat &out, int max);

	void skeletonize_circle(const cv::Mat &input, cv::Mat &skeleton, cv::Mat &distance);
	void skeletonize_diamond_square(const cv::Mat &input, cv::Mat &skeleton, cv::Mat &distance);
	int sum_8_connected(const cv::Mat &img, cv::Point p);
	int sum_4_connected(const cv::Mat &img, cv::Point p);

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

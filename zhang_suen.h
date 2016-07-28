/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__ZHANG_SUEN_H
#define VECTORIX__ZHANG_SUEN_H

#include <opencv2/opencv.hpp>
#include <vector>
#include "parameters.h"
#include "logger.h"

namespace vectorix {

class zhang_suen {
public:
	int skeletonize(const cv::Mat &inupt, cv::Mat &skeleton, cv::Mat &distance);
	zhang_suen(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);

		par->bind_param(param_save_peeled_name, "files_steps_output", (std::string) "out/skeletonization_#.png");
	}
private:
	std::string *param_save_peeled_name;

	logger log;
	parameters *par;

	cv::Mat *itp;
	cv::Mat inq;
	cv::Mat *skel;
	std::vector<cv::Point> border_queue;
	std::vector<cv::Point> delete_queue;

	int B(cv::Point pt) const;
	int A(cv::Point pt) const;
	void init_queue();
};

}; // namespace

#endif

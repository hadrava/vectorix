#ifndef VECTORIX__TRACER_HELPER_H
#define VECTORIX__TRACER_HELPER_H

#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include <vector>

namespace vectorix {

class starting_point {
public:
	void prepare(const cv::Mat &skeleton);
	int get_max(const cv::Mat &used_pixels, cv::Point &max_pos);
	starting_point(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);
	}
private:
	class st_point { // Structure for handle all unused starting points
	public:
		cv::Point pt; // Position in input image
		int val; // Distance to edge of object
	};

	logger log;
	parameters *par;

	std::vector<st_point> queue;
};

class changed_pix_roi { // Rectangle in which are all changed pixels
public:
	void clear(int x, int y); // remove all pixels from roi
	cv::Rect get() const; // Return OpenCV roi
	void update(int x, int y); // Add pixel (x,y) to roi
private:
	int min_x;
	int min_y;
	int max_x;
	int max_y;
};

class used_Mat: public cv::Mat {
public:
	
};

}; // namespace (vectorix)

#endif

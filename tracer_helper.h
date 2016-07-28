/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#ifndef VECTORIX__TRACER_HELPER_H
#define VECTORIX__TRACER_HELPER_H

#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "v_image.h"
#include <vector>

namespace vectorix {

class starting_point {
public:
	void prepare(const cv::Mat &skeleton);
	int get_max(const cv::Mat &used_pixels, cv::Point &max_pos);
	starting_point() = default;
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


/*
 * Used pixels
 */

class labeled_Mat {
public:
	int label_near_pixels(int value, const v_pt &point, p near = 1);
	void drop_smaller_or_equal_labels(int value);
	void drop_smaller_labels_equal_or_higher_make_permanent(int value);
	void init(const cv::Mat &mat);
	labeled_Mat() = default;
	labeled_Mat(parameters &params): par(&params) {
		int *param_vectorizer_verbosity;
		par->bind_param(param_vectorizer_verbosity, "vectorizer_verbosity", (int) log_level::warning);
		log.set_verbosity((log_level) *param_vectorizer_verbosity);
	}

	int get_max_unlabeled(cv::Point &max_pos);
	int label_pix(int value, const v_pt &point);
	uint8_t safeat(int row, int col, bool unlabeled);
	p apxat(v_pt pt, bool unlabeled);
private:

	cv::Mat label;
	cv::Mat matrix;

	changed_pix_roi changed_roi; // Roi with pixels (label < 254)
	changed_pix_roi changed_start_roi; // Roi with pixels (label == 254)

	// Speedup by creating queue with unprocessed pixels;
	starting_point st;

	logger log;
	parameters *par;
};

}; // namespace (vectorix)

#endif

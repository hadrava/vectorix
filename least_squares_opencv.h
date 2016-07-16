#ifndef VECTORIX__LEAST_SQUARES_OPENCV_H
#define VECTORIX__LEAST_SQUARES_OPENCV_H

#include <vector>
#include "config.h"
#include "logger.h"
#include "parameters.h"
#include <opencv2/opencv.hpp>
#include "least_squares.h"

namespace vectorix {

class least_squares_opencv: public least_squares {
public:
	least_squares_opencv(unsigned int variable_count, parameters &params): least_squares(variable_count, params) {};
	virtual void add_equation(p *arr);
	virtual void evaluate();
	virtual p calc_error() const;
	virtual p operator[](unsigned int i) const;
private:
	cv::Mat A;
	cv::Mat x_vector;
	cv::Mat y_vector;

	logger log;
	parameters *par;
};

}; // namespace
#endif

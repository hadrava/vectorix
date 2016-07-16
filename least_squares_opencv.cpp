#include "least_squares_opencv.h"
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
namespace vectorix {

void least_squares_opencv::add_equation(p *arr) {
	Mat row = Mat::zeros(1, count, CV_64F);
	for (int i = 0; i < count; i++)
		row.at<double>(0, i) = arr[i];
	A.push_back(row);

	row = Mat::zeros(1, 1, CV_64F);
	row.at<double>(0, 0) = arr[count];
	y_vector.push_back(row);
}

void least_squares_opencv::evaluate() {
	//solve(A, y_vector, x_vector, DECOMP_SVD);
	solve(A, y_vector, x_vector, DECOMP_NORMAL | DECOMP_CHOLESKY);
}

p least_squares_opencv::calc_error() const {
	Mat x_mat = A * x_vector;

	unsigned int rows = x_mat.rows;
	p error = 0;
	for (int j = 0; j < rows; j++) {
		p err = x_mat.at<double>(j, 0) - y_vector.at<double>(j, 0);
		error += err * err;
	}

	return error;
}

p least_squares_opencv::operator[](unsigned int i) const{
	return x_vector.at<double>(i, 0);
}

}; // namespace

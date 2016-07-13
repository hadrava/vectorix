#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "skeletonizer.h"

using namespace cv;

namespace vectorix {

// Add pixels from `bw' to `out'. Something like image `or', but with more information
void skeletonizer::add_to_skeleton(Mat &out, Mat &bw, int iteration) {
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			if (!out.data[i*out.step + j]) { // Non-zero pixel
				out.data[i*out.step + j] = (!!bw.data[i*bw.step + j]) * iteration;
			}
		}
	}
}

// Normalize grayscale image for displaying (0-255)
void skeletonizer::normalize(Mat &out, int max) {
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] *= 255/max;
		}
	}
}


void skeletonizer::run(const Mat &binary_input, Mat &skeleton, Mat &distance) {
	// Create boarders around image, white (background) pixels
	Mat source;
	copyMakeBorder(binary_input, source, 1, 1, 1, 1, BORDER_REPLICATE, Scalar(255, 255, 255));

	Mat bw          (source.rows, source.cols, CV_8UC(1));
	Mat next_peeled (source.rows, source.cols, CV_8UC(1));
	skeleton = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	distance = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	Mat peeled = source.clone(); // Objects in this image are peeled in every step by 1 px

	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3)); // diamond
	Mat kernel_2 = getStructuringElement(MORPH_RECT, Size(3,3)); // square

	double max = 1;
	iteration = 1;
	if (*param_skeletonization_type == 1)
		std::swap(kernel, kernel_2); // use only square

	while (max != 0) {
		if (!param_save_peeled_name->empty()) { // Save every step of skeletonization
			// TODO fix fixed length
			char filename [128];
			std::snprintf(filename, sizeof(filename), param_save_peeled_name->c_str(), iteration);
			imwrite(filename, peeled);
		}
		int size = iteration * 2 + 1;
		// skeleton
		morphologyEx(peeled, bw, MORPH_OPEN, kernel);
		bitwise_not(bw, bw);
		bitwise_and(peeled, bw, bw); // Pixels destroyed by opening
		add_to_skeleton(skeleton, bw, iteration); // Add them to skeleton

		// distance
		if (*param_skeletonization_type == 3) {
			// Most precise peeling - with circle
			kernel_2 = getStructuringElement(MORPH_ELLIPSE, Size(size,size));
			erode(source, next_peeled, kernel_2);
		}
		else {
			// Diamond / square / diamond-square
			erode(peeled, next_peeled, kernel);
		}
		bitwise_not(next_peeled, bw);
		bitwise_and(peeled, bw, bw); // Pixels removed by next peeling
		add_to_skeleton(distance, bw, iteration++); // calculate distance for all pixels

		std::swap(peeled, next_peeled);
		minMaxLoc(peeled, NULL, &max, NULL, NULL); // Check for non-zero pixel
		if (*param_skeletonization_type == 0)
			std::swap(kernel, kernel_2); // diamond-square
	}

	if (!param_save_skeleton_name->empty()) { // Save output to file
		imwrite(*param_save_skeleton_name, skeleton);
	}
	if (!param_save_distance_name->empty()) { // Save output to file
		imwrite(*param_save_distance_name, distance);
	}
	if (!param_save_skeleton_normalized_name->empty()) { // Display skeletonization outcome
		Mat skeleton_normalized = skeleton.clone();
		normalize(skeleton_normalized, iteration-1); // Make image more contrast
		imwrite(*param_save_skeleton_normalized_name, skeleton_normalized);
	}
	if (!param_save_distance_normalized_name->empty()) { // Display skeletonization outcome
		Mat distance_normalized = distance.clone();
		normalize(distance_normalized, iteration-1); // Make image more contrast
		imwrite(*param_save_distance_normalized_name, distance_normalized);
	}
	this->skeleton = skeleton;
	this->distance = distance;
}

void skeletonizer::interactive(TrackbarCallback onChange, void *userdata) {
	Mat distance_show; // Images normalized for displaying
	Mat skeleton_show; // Images normalized for displaying

	//show distance
	distance_show = distance.clone();
	normalize(distance_show, iteration-1); // Normalize image before displaying
	/*// TODO vectorize_*/imshow("Distance", distance_show);

	//show skeleton
	skeleton_show = skeleton.clone();
	threshold(skeleton_show, skeleton_show, 0, 255, THRESH_BINARY);
	/*// TODO vectorize_*/imshow("Skeleton", skeleton_show);

	createTrackbar("Skeletonization", "Skeleton", param_skeletonization_type, 3, onChange, userdata);
	waitKey(1);
};

}; // namespace

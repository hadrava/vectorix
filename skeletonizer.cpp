#include <opencv2/opencv.hpp>
#include <vector>
#include "parameters.h"
#include "logger.h"
#include "skeletonizer.h"
#include "zoom_window.h"
#include "zhang_suen.h"

using namespace cv;

namespace vectorix {

void skeletonizer::skeletonize_circle(const Mat &source, Mat &skeleton, Mat &distance) {
	Mat bw          (source.rows, source.cols, CV_8UC(1));
	Mat next_peeled (source.rows, source.cols, CV_8UC(1));
	skeleton = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	distance = Mat::zeros(source.rows, source.cols, CV_32SC1);
	Mat peeled = source.clone(); // Objects in this image are peeled in every step by 1 px

	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3)); // diamond
	Mat kernel_2 = getStructuringElement(MORPH_RECT, Size(3,3)); // square

	double max = 1;
	iteration = 1;
	if (*param_skeletonization_type == 1)
		std::swap(kernel, kernel_2); // use only square

	while (max != 0) {
		if (!param_save_peeled_name->empty()) { // Save every step of skeletonization
			size_t number_sign = param_save_peeled_name->find("#");
			std::string filename = std::to_string(iteration);
			int zero = 3 - filename.length();
			zero = (zero >= 0) ? zero : 0;
			filename = param_save_peeled_name->substr(0, number_sign)
				 + std::string(zero, '0')
				 + filename
				 + param_save_peeled_name->substr(number_sign + 1);
			imwrite(filename, peeled);
		}
		int size = iteration * 2 + 1;
		// skeleton
		morphologyEx(peeled, bw, MORPH_OPEN, kernel);
		bitwise_not(bw, bw);
		bitwise_and(peeled, bw, bw); // Pixels destroyed by opening
		add_to_skeleton<uint8_t>(skeleton, bw, iteration); // Add them to skeleton

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
		add_to_skeleton<int32_t>(distance, bw, iteration++); // calculate distance for all pixels

		std::swap(peeled, next_peeled);
		minMaxLoc(peeled, NULL, &max, NULL, NULL); // Check for non-zero pixel
		if (*param_skeletonization_type == 0)
			std::swap(kernel, kernel_2); // diamond-square

		log.log<log_level::info>("Skeletonizer iteration: %i\n", iteration - 1);
	}
}


int skeletonizer::sum_8_connected(const Mat &img, Point p) {
	int i = p.y;
	int j = p.x;
	return !!img.at<uint8_t>(i - 1, j) +
	       !!img.at<uint8_t>(i - 1, j + 1) +
	       !!img.at<uint8_t>(i,     j + 1) +
	       !!img.at<uint8_t>(i + 1, j + 1) +
	       !!img.at<uint8_t>(i + 1, j) +
	       !!img.at<uint8_t>(i + 1, j - 1) +
	       !!img.at<uint8_t>(i,     j - 1) +
	       !!img.at<uint8_t>(i - 1, j - 1);
}

int skeletonizer::sum_4_connected(const Mat &img, Point p) {
	int i = p.y;
	int j = p.x;
	return !!img.at<uint8_t>(i - 1, j) +
	       !!img.at<uint8_t>(i,     j + 1) +
	       !!img.at<uint8_t>(i + 1, j) +
	       !!img.at<uint8_t>(i,     j - 1);
}

void skeletonizer::skeletonize_diamond_square(const Mat &source, Mat &skeleton, Mat &distance) {
	std::vector<Point> border_queue;
	std::vector<Point> delete_queue;

	skeleton = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	distance = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	Mat peeled = source.clone(); // Objects in this image are peeled in every step by 1 px
	Mat in_queue = Mat::zeros(source.rows, source.cols, CV_8UC(1));
	for (int i = 1 ; i < source.rows - 1; i++) {
		for (int j = 1 ; j < source.cols - 1; j++) {
			if (source.at<uint8_t>(i, j) && sum_8_connected(source, Point(j, i)) < 8) {
				border_queue.emplace_back(Point(j, i));
				in_queue.at<uint8_t>(i, j) = 1;
			}
		}
	}

	iteration = 1;
	while (border_queue.size()) {
		if ((*param_skeletonization_type & 1) == 0) {
			log.log<log_level::info>("Skeletonizer (Diamond) iteration: %i (%i points)\n", iteration, border_queue.size());
			delete_queue.clear();
			for (auto p: border_queue) {
				int i = p.y;
				int j = p.x;
				if (peeled.at<uint8_t>(i, j) && sum_4_connected(peeled, Point(j, i)) < 4) {
					in_queue.at<uint8_t>(i, j) = 2;
					delete_queue.emplace_back(Point(j, i));
				}
				else
					in_queue.at<uint8_t>(i, j) = 0;
			}
			border_queue.clear();
			for (auto p: delete_queue) {
				int i = p.y;
				int j = p.x;
				if ((in_queue.at<uint8_t>(i - 1, j    ) == 2 || !peeled.at<uint8_t>(i - 1, j)) &&
				    (in_queue.at<uint8_t>(i,     j + 1) == 2 || !peeled.at<uint8_t>(i,     j + 1)) &&
				    (in_queue.at<uint8_t>(i + 1, j    ) == 2 || !peeled.at<uint8_t>(i + 1, j)) &&
				    (in_queue.at<uint8_t>(i,     j - 1) == 2 || !peeled.at<uint8_t>(i,     j - 1))) {
					skeleton.at<uint8_t>(i, j) = iteration;
				}
				peeled.at<uint8_t>(i, j) = 0;
				distance.at<uint8_t>(i, j) = iteration;

				for (int i = p.y - 1; i <= p.y + 1; i++) {
					for (int j = p.x - 1; j <= p.x + 1; j++) {
						if (!in_queue.at<uint8_t>(i, j) && peeled.at<uint8_t>(i, j)) {
							border_queue.emplace_back(Point(j, i));
							in_queue.at<uint8_t>(i, j) = 1;
						}
					}
				}
			}
			iteration++;
		}
		if ((*param_skeletonization_type & 2) == 0) {
			log.log<log_level::info>("Skeletonizer (Square) iteration: %i (%i points)\n", iteration, border_queue.size());
			for (auto p: border_queue) {
				in_queue.at<uint8_t>(p) = 2;
			}
			std::swap(border_queue, delete_queue);
			border_queue.clear();
			for (auto p: delete_queue) {
				int i = p.y;
				int j = p.x;
				if ((in_queue.at<uint8_t>(i - 1, j    ) == 2 || !peeled.at<uint8_t>(i - 1, j)) &&
				    (in_queue.at<uint8_t>(i - 1, j + 1) == 2 || !peeled.at<uint8_t>(i - 1, j + 1)) &&
				    (in_queue.at<uint8_t>(i,     j + 1) == 2 || !peeled.at<uint8_t>(i,     j + 1)) &&
				    (in_queue.at<uint8_t>(i + 1, j + 1) == 2 || !peeled.at<uint8_t>(i + 1, j + 1)) &&
				    (in_queue.at<uint8_t>(i + 1, j    ) == 2 || !peeled.at<uint8_t>(i + 1, j)) &&
				    (in_queue.at<uint8_t>(i + 1, j - 1) == 2 || !peeled.at<uint8_t>(i + 1, j - 1)) &&
				    (in_queue.at<uint8_t>(i,     j - 1) == 2 || !peeled.at<uint8_t>(i,     j - 1)) &&
				    (in_queue.at<uint8_t>(i - 1, j - 1) == 2 || !peeled.at<uint8_t>(i - 1, j - 1))) {
					skeleton.at<uint8_t>(i, j) = iteration;
				}
				peeled.at<uint8_t>(i, j) = 0;
				distance.at<uint8_t>(i, j) = iteration;

				for (int i = p.y - 1; i <= p.y + 1; i++) {
					for (int j = p.x - 1; j <= p.x + 1; j++) {
						if (!in_queue.at<uint8_t>(i, j) && peeled.at<uint8_t>(i, j)) {
							border_queue.emplace_back(Point(j, i));
							in_queue.at<uint8_t>(i, j) = 1;
						}
					}
				}
			}
			iteration++;
		}
	}
}

void skeletonizer::run(const Mat &binary_input, Mat &skeleton, Mat &distance) {
	// Create boarders around image, white (background) pixels
	Mat source;
	copyMakeBorder(binary_input, source, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0, 0, 0));

	log.log<log_level::debug>("Image size without border: %i x %i\n", binary_input.cols, binary_input.rows);
	log.log<log_level::debug>("Image size with border: %i x %i\n", source.cols, source.rows);

	if (*param_skeletonization_type == 4) {
		zhang_suen zs(*par);
		iteration = zs.skeletonize(source, skeleton, distance);
	}
	else if (*param_skeletonization_type == 3)
		skeletonize_circle(source, skeleton, distance);
	else
		skeletonize_diamond_square(source, skeleton, distance);

	Rect crop(1, 1, binary_input.cols, binary_input.rows);
	skeleton = skeleton(crop);
	distance = distance(crop);
	log.log<log_level::debug>("Image size after cropping: %i x %i\n", skeleton.cols, skeleton.rows);

	if (!param_save_skeleton_name->empty()) { // Save output to file
		imwrite(*param_save_skeleton_name, skeleton);
	}
	if (!param_save_distance_name->empty()) { // Save output to file
		imwrite(*param_save_distance_name, distance);
	}
	// Display skeletonization outcome
	if (!param_save_skeleton_normalized_name->empty()) {
		Mat skeleton_normalized;
		normalize(skeleton, skeleton_normalized, iteration-1); // Make image more contrast
		imwrite(*param_save_skeleton_normalized_name, skeleton_normalized);
	}
	// Display skeletonization outcome
	if (!param_save_distance_normalized_name->empty()) {
		Mat distance_normalized;
		normalize(distance, distance_normalized, iteration-1); // Make image more contrast
		imwrite(*param_save_distance_normalized_name, distance_normalized);
	}

	this->skeleton = skeleton;
	this->distance = distance;
}

void skeletonizer::normalize(const cv::Mat &in, cv::Mat &out, int max) {
	in.convertTo(out, CV_8U, 255. / max);
	applyColorMap(out, out, COLORMAP_JET);
}

void skeletonizer::interactive(TrackbarCallback onChange, void *userdata) {
	Mat distance_show; // Images normalized for displaying
	Mat skeleton_show; // Images normalized for displaying

	// Show distance (normalized)
	distance_show = distance.clone();
	normalize(distance, distance_show, iteration-1);
	zoom_imshow("Distance", distance_show);

	// Show skeleton
	skeleton_show = skeleton.clone();
	threshold(skeleton_show, skeleton_show, 0, 255, THRESH_BINARY);
	zoom_imshow("Skeleton", skeleton_show);

	createTrackbar("Skeletonization", "Skeleton", param_skeletonization_type, 4, onChange, userdata);
	waitKey(1);
};

}; // namespace

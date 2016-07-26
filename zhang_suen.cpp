#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "zhang_suen.h"

using namespace cv;

namespace vectorix {

int zhang_suen::skeletonize(const Mat &input, Mat &it, Mat &distance) {
	it = input.clone();
	inq = Mat::zeros(it.rows, it.cols, CV_8UC(1));
	distance = Mat::zeros(it.rows, it.cols, CV_32SC1);
	itp = &it;

	init_queue();
	bool first_iteration = 1;
	int iteration = 1;
	while (border_queue.size()) {
		log.log<log_level::info>("Skeletonizer (Zhang-Suen) iteration: %i (%i points)\n", iteration, border_queue.size());
		if (!param_save_peeled_name->empty()) { // Save every step of skeletonization
			size_t number_sign = param_save_peeled_name->find("#");
			std::string filename = std::to_string(iteration);
			int zero = 3 - filename.length();
			zero = (zero >= 0) ? zero : 0;
			filename = param_save_peeled_name->substr(0, number_sign)
				 + std::string(zero, '0')
				 + filename
				 + param_save_peeled_name->substr(number_sign + 1);
			imwrite(filename, it);
		}
		delete_queue.clear();
		for (auto p: border_queue) {
			int i = p.y;
			int j = p.x;

			if (2 <= B(p) && B(p) <= 6 &&
			    A(p) == 1 &&
			    it.at<uint8_t>(i - 1, j) * it.at<uint8_t>(i,     j + 1) * (it.at<uint8_t>(i + 1, j) + !first_iteration) * (it.at<uint8_t>(i,     j - 1) + first_iteration) == 0 &&
			    (it.at<uint8_t>(i - 1, j) + first_iteration) * (it.at<uint8_t>(i,     j + 1) + !first_iteration) * it.at<uint8_t>(i + 1, j) * it.at<uint8_t>(i,     j - 1) == 0) {
				delete_queue.emplace_back(Point(j, i));
			}
			inq.at<uint8_t>(i, j) = 0;
		}

		border_queue.clear();
		for (auto p: delete_queue) {
			it.at<uint8_t>(p) = 0;
			distance.at<int32_t>(p) = iteration;

			for (int i = p.y - 1; i <= p.y + 1; i++) {
				for (int j = p.x - 1; j <= p.x + 1; j++) {
					if (!inq.at<uint8_t>(i, j) && it.at<uint8_t>(i, j)) {
						border_queue.emplace_back(Point(j, i));
						inq.at<uint8_t>(i, j) = 1;
					}
				}
			}
		}
		first_iteration ^= 1;
		iteration++;
	}
	return iteration;
}

int zhang_suen::B(Point pt) const {
	int i = pt.y;
	int j = pt.x;
	return !!itp->at<uint8_t>(i - 1, j) +
	       !!itp->at<uint8_t>(i - 1, j + 1) +
	       !!itp->at<uint8_t>(i,     j + 1) +
	       !!itp->at<uint8_t>(i + 1, j + 1) +
	       !!itp->at<uint8_t>(i + 1, j) +
	       !!itp->at<uint8_t>(i + 1, j - 1) +
	       !!itp->at<uint8_t>(i,     j - 1) +
	       !!itp->at<uint8_t>(i - 1, j - 1);
}

int zhang_suen::A(Point pt) const {
	int i = pt.y;
	int j = pt.x;
	return (int)(!itp->at<uint8_t>(i - 1, j    ) && itp->at<uint8_t>(i - 1, j + 1)) +
	       (int)(!itp->at<uint8_t>(i - 1, j + 1) && itp->at<uint8_t>(i,     j + 1)) +
	       (int)(!itp->at<uint8_t>(i,     j + 1) && itp->at<uint8_t>(i + 1, j + 1)) +
	       (int)(!itp->at<uint8_t>(i + 1, j + 1) && itp->at<uint8_t>(i + 1, j    )) +
	       (int)(!itp->at<uint8_t>(i + 1, j    ) && itp->at<uint8_t>(i + 1, j - 1)) +
	       (int)(!itp->at<uint8_t>(i + 1, j - 1) && itp->at<uint8_t>(i,     j - 1)) +
	       (int)(!itp->at<uint8_t>(i,     j - 1) && itp->at<uint8_t>(i - 1, j - 1)) +
	       (int)(!itp->at<uint8_t>(i - 1, j - 1) && itp->at<uint8_t>(i - 1, j    ));
}

void zhang_suen::init_queue() {
	border_queue.clear();
	for (int i = 1 ; i < itp->rows - 1; i++) {
		for (int j = 1 ; j < itp->cols - 1; j++) {
			if (itp->at<uint8_t>(i, j) && B(Point(j, i)) <= 6) {
				border_queue.emplace_back(Point(j, i));
			}
		}
	}
}

}; // namespace

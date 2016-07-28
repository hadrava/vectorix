/*
 * Vectorix -- line-based image vectorizer
 * (c) 2016 Jan Hadrava <had@atrey.karlin.mff.cuni.cz>
 */
#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "tracer_helper.h"

using namespace cv;

namespace vectorix {

/*
 * Starting points for tracer
 */

// Find all possible startingpoints
void starting_point::prepare(const Mat &skeleton) {
	for (int i = 0; i < skeleton.rows; i++) {
		for (int j = 0; j < skeleton.cols; j++) {
			if (skeleton.at<uint8_t>(i, j)) { // Pixel is in skeleton
				st_point pt;
				pt.val = skeleton.at<uint8_t>(i, j);
				pt.pt = Point(j, i);
				queue.push_back(pt); // Add it to queue (std::vector)
			}
		}
	}
	std::sort(queue.begin(), queue.end(), [&](st_point a, st_point b)->bool {
			return a.val < b.val;
			}); // Sort pixels by distance to object borders
}

// Find first unused starting point in queue
int starting_point::get_max(const Mat &used_pixels, Point &max_pos) {
	int max = 0;
	while ((max == 0) && !queue.empty()) {
		st_point pt = queue.back();
		queue.pop_back();
		if (!used_pixels.at<uint8_t>(pt.pt.y, pt.pt.x)) { // Pixel was not used
			max = pt.val;
			max_pos = pt.pt;
		}
	}
	log.log<log_level::debug>("New start point value: %i\n", max);
	return max;
}


/*
 * Regions of interest
 */

void changed_pix_roi::clear(int x, int y) { // remove all pixels from roi
	min_x = x;
	min_y = y;
	max_x = -1;
	max_y = -1;
}

Rect changed_pix_roi::get() const{ // Return OpenCV roi
	if ((min_x <= max_x) && (min_y <= max_y))
		return Rect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
	else
		return Rect(0, 0, 0, 0); // Empty roi
}

void changed_pix_roi::update(int x, int y) { // Add pixel (x,y) to roi
	min_x = min(min_x, x);
	min_y = min(min_y, y);
	max_x = max(max_x, x);
	max_y = max(max_y, y);
}


/*
 * Used pixels
 */

int labeled_Mat::label_near_pixels(int value, const v_pt &point, p near) {
	int sum = 0;
	// TODO check ranges, near used to be int, now it is p.…
	for (int i = -near; i<=near; i++) {
		for (int j = -near; j<=near; j++) {
			v_pt a = point;
			a.x+=i;
			a.y+=j;
			if ((a.x >= 0) && (a.y >= 0) && (a.x < label.cols) && (a.y < label.rows))
				sum += label_pix(value, a);
		}
	}
	return sum; // Count of increased labels
}

int labeled_Mat::label_pix(int value, const v_pt &point) {
	if ((matrix.at<unsigned char>(point.y, point.x) > 0) && (label.at<unsigned char>(point.y, point.x) < value)) {
		label.at<unsigned char>(point.y, point.x) = value; // Set pixel

		// Add pixel to corresponding roi
		if (value < 254) {
			changed_roi.update(point.x, point.y);
		}
		if (value == 254) {
			changed_start_roi.update(point.x, point.y);
		}

		return 1; // one pixel changed
	}
	else
		return 0; // no pixel changed
}

void labeled_Mat::drop_smaller_or_equal_labels(int value) {
	if (value == 254) {
		threshold(label(changed_roi.get()), label(changed_roi.get()), value, 255, THRESH_BINARY);
		threshold(label(changed_start_roi.get()), label(changed_start_roi.get()), value, 255, THRESH_BINARY);

		changed_roi.clear(label.cols, label.rows); // There is no pixel with value < 254
		changed_start_roi.clear(label.cols, label.rows); // There is no pixel with value == 254
	}
	else {
		threshold(label(changed_roi.get()), label(changed_roi.get()), value, 255, THRESH_TOZERO); // All changed pixels are in roi step3_changed
		if (value == 253)
			changed_roi.clear(label.cols, label.rows);
	}
}

void labeled_Mat::drop_smaller_labels_equal_or_higher_make_permanent(int value) {
	value--;

	threshold(label(changed_roi.get()), label(changed_roi.get()), value, 255, THRESH_BINARY); // save first point
	threshold(label(changed_start_roi.get()), label(changed_start_roi.get()), value, 255, THRESH_BINARY); // save first point
	changed_roi.clear(label.cols, label.rows);
	changed_start_roi.clear(label.cols, label.rows);
}

int labeled_Mat::get_max_unlabeled(cv::Point &max_pos) {
	return st.get_max(label, max_pos);
}

void labeled_Mat::init(const cv::Mat &mat) {
	matrix = mat;
	int rows = mat.rows;
	int cols = mat.cols;
	label = Mat::zeros(rows, cols, CV_8UC(1));

	// Speedup label removing using OpenCV region of interest
	changed_roi.clear(cols, rows);
	changed_start_roi.clear(cols, rows);

	st = starting_point(*par);
	st.prepare(mat);
}

uint8_t labeled_Mat::safeat(int row, int col, bool unlabeled) {
	if ((row < 0) || (col < 0) || (row >= label.rows) || (col >= label.cols))
		return 0;
	if (unlabeled && label.at<uint8_t>(row, col))
		return 0;
	return matrix.at<uint8_t>(row, col);
}

p labeled_Mat::apxat(v_pt pt, bool unlabeled) {
	int x = pt.x - 0.5f;
	int y = pt.y - 0.5f;
	pt.x -= x + 0.5f;
	pt.y -= y + 0.5f;
	// Weight is equal to area covered by rectangle 1px x 1px
	p out = safeat(y,   x,   unlabeled) * ((1-pt.x) * (1-pt.y)) +
	        safeat(y,   x+1, unlabeled) * (pt.x     * (1-pt.y)) +
	        safeat(y+1, x,   unlabeled) * ((1-pt.x) * pt.y    ) +
	        safeat(y+1, x+1, unlabeled) * (pt.x     * pt.y    );
	return out;
};

}; // namespace (vectorix)

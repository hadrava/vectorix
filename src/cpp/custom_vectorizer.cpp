#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "geom.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "custom_vectorizer.h"
#include "time_measurement.h"
#include "parameters.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <vector>

// Vectorizer

using namespace cv;
using namespace pnm;

namespace vect {

changed_pix_roi custom::step3_changed; // Roi with pixels (value < 254)
changed_pix_roi custom::step3_changed_start; // Roi with pixels (value == 254)

void custom::step3_roi_clear(changed_pix_roi &roi, int x, int y) { // remove all pixels from roi
	roi.min_x = x;
	roi.min_y = y;
	roi.max_x = -1;
	roi.max_y = -1;
}

Rect custom::step3_roi_get(const changed_pix_roi &roi) { // Return OpenCV roi
	if ((roi.min_x <= roi.max_x) && (roi.min_y <= roi.max_y))
		return Rect(roi.min_x, roi.min_y, roi.max_x-roi.min_x + 1, roi.max_y-roi.min_y + 1);
	else
		return Rect(0, 0, 0, 0); // Empty roi
}

void custom::step3_roi_update(changed_pix_roi &roi, int x, int y) { // Add pixel (x,y) to roi
	roi.min_x = min(roi.min_x, x);
	roi.min_y = min(roi.min_y, y);
	roi.max_x = max(roi.max_x, x);
	roi.max_y = max(roi.max_y, y);
}

uchar custom::nullpixel; // Virtual null pixel for safe access to image matrix

float custom::apxat(const Mat &image, v_pt pt) { // Get value at non-integer position (aproximate from neighbors)
	int x = pt.x - 0.5f;
	int y = pt.y - 0.5f;
	pt.x-=x+0.5f;
	pt.y-=y+0.5f;
	// Weight is equal to area covered by rectangle 1px x 1px
	float out = safeat(image, y,   x  ) * ((1-pt.x) * (1-pt.y)) + \
		    safeat(image, y,   x+1) * (pt.x     * (1-pt.y)) + \
		    safeat(image, y+1, x  ) * ((1-pt.x) * pt.y    ) + \
		    safeat(image, y+1, x+1) * (pt.x     * pt.y    );
	return out;
}

v_co custom::safeat_co(const Mat &image, int i, int j) { // Safely access rgb image data
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return v_co(image.data[i*image.step + j*3 + 2], image.data[i*image.step + j*3 + 1], image.data[i*image.step + j*3]);
	else {
		return v_co(0, 0, 0); // Pixel is outside of image
	}
}

v_co custom::apxat_co(const Mat &image, v_pt pt) { // Get rgb at non-integer position (aproximate from neighbors)
	int x = pt.x - 0.5f;
	int y = pt.y - 0.5f;
	pt.x-=x+0.5f;
	pt.y-=y+0.5f;
	// Weight is equal to area covered by rectangle 1px x 1px
	v_co out = safeat_co(image, y,   x  ) * ((1-pt.x) * (1-pt.y)) + \
		   safeat_co(image, y,   x+1) * (pt.x     * (1-pt.y)) + \
		   safeat_co(image, y+1, x  ) * ((1-pt.x) * pt.y    ) + \
		   safeat_co(image, y+1, x+1) * (pt.x     * pt.y    );
	return out;
}

uchar &custom::safeat(const Mat &image, int i, int j) { // Safely acces image data
	if (i>=0 && i<image.rows && j>=0 && j<image.step) // Pixel is inside of an image
		return image.data[i*image.step + j];
	else { // Outside of an image
		nullpixel = 0; // clean data in nullpixel
		return nullpixel;
	}
}

#ifdef VECTORIZER_HIGHGUI
// For compatibility issues we use const cv::Mat instead of cv::InputArray.
void custom::vectorize_imshow(const std::string& winname, const cv::Mat mat, const params &parameters) { // Display image in highgui named window.
	if (parameters.zoom_level) { // (Down)scale image before displaying
		Mat scaled_mat;
		int w = mat.cols / (log10(parameters.zoom_level+10));
		int h = mat.rows / (log10(parameters.zoom_level+10));
		resize(mat, scaled_mat, Size(w, h));
		return imshow(winname, scaled_mat);
	}
	else
		return imshow(winname, mat);
}
int custom::vectorize_waitKey(int delay) { // Wait for key press in any window
	return waitKey(delay);
}
void custom::vectorize_destroyWindow(const std::string& winname) { // Close window
	return destroyWindow(winname);
}
#else
void custom::vectorize_imshow(const std::string& winname, const cv::Mat mat, const params &parameters) { // Highgui disabled, do nothing
	return;
}
int custom::vectorize_waitKey(int delay) { // Highgui disabled, return `no key pressed'
	return -1;
}
void custom::vectorize_destroyWindow(const std::string& winname) { // Nothing to destroy
	return;
}
#endif

void custom::add_to_skeleton(Mat &out, Mat &bw, int iteration) { // Add pixels from `bw' to `out'. Something like image `or', but with more information
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			if (!out.data[i*out.step + j]) { // Non-zero pixel
				out.data[i*out.step + j] = (!!bw.data[i*bw.step + j]) * iteration;
			}
		}
	}
}

void custom::normalize(Mat &out, int max) { // Normalize grayscale image for displaying (0-255)
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] *= 255/max;
		}
	}
}

void custom::step1_threshold(Mat &to_threshold, step1_params &par) {
	if (par.threshold_type == 0) { // Binary threshold guessed value
		vectorizer_debug("threshold: Using Otsu's algorithm\n");
		threshold(to_threshold, to_threshold, par.threshold, 255, THRESH_BINARY | THRESH_OTSU);
	}
	else if (par.threshold_type == 1) { // Fixed binary threshold
		vectorizer_debug("threshold: Using binary threshold %i.\n", par.threshold);
		threshold(to_threshold, to_threshold, par.threshold, 255, THRESH_BINARY);
	}
	else if ((par.threshold_type >= 2) && (par.threshold_type <= 3)) { // Adaptive threshold
		vectorizer_debug("threshold: Using adaptive thresholdbinary threshold %i.\n", par.threshold);
		par.adaptive_threshold_size |= 1; // Make threshold size odd
		if (par.adaptive_threshold_size < 3)
			par.adaptive_threshold_size = 3;
		int type = ADAPTIVE_THRESH_GAUSSIAN_C; // par.threshold_type==2 -> gaussian
		if (par.threshold_type == 3)
			type = ADAPTIVE_THRESH_MEAN_C; // par.threshold_type==3 -> mean
		adaptiveThreshold(to_threshold, to_threshold, 255, type, THRESH_BINARY, par.adaptive_threshold_size, par.threshold - 128);
	}

	for (int j = 0; j < to_threshold.rows; j += to_threshold.rows-1) { // Clear borders
		for (int i = 0; i < to_threshold.cols; i++) {
			to_threshold.data[i+j*to_threshold.step] = 0;
		}
	}
	for (int j = 0; j < to_threshold.rows; j++) {
		for (int i = 0; i < to_threshold.cols; i += to_threshold.cols-1) {
			to_threshold.data[i+j*to_threshold.step] = 0;
		}
	}

	if (!par.save_threshold_name.empty()) { // Save image after thresholding
		imwrite(par.save_threshold_name, to_threshold);
	}
}

void custom::step2_skeletonization(const Mat &binary_input, Mat &skeleton, Mat &distance, int &iteration, params &parameters) {
	step2_params &par = parameters.step2;
	Mat bw     (binary_input.rows, binary_input.cols, CV_8UC(1));
	Mat source = binary_input.clone();
	Mat peeled = binary_input.clone(); // Objects in this image are peeled in every step by 1 px
	Mat next_peeled (binary_input.rows, binary_input.cols, CV_8UC(1));
	skeleton = Scalar(0); // Clear output
	distance = Scalar(0); // Clear output

	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3)); // diamond
	Mat kernel_2 = getStructuringElement(MORPH_RECT, Size(3,3)); // square

	double max = 1;
	iteration = 1;
	if (par.type == 1)
		std::swap(kernel, kernel_2); // use only square

	while (max !=0) {
		if (!par.save_peeled_name.empty()) { // Save every step of skeletonization
			char filename [128];
			std::snprintf(filename, sizeof(filename), par.save_peeled_name.c_str(), iteration);
			imwrite(filename, peeled);
		}
		if (par.show_window == 1) { // Show every step of skeletonization
			vectorize_imshow("Boundary peeling", peeled, parameters);
			vectorize_waitKey(0);
		}
		int size = iteration * 2 + 1;
		// skeleton
		morphologyEx(peeled, bw, MORPH_OPEN, kernel);
		bitwise_not(bw, bw);
		bitwise_and(peeled, bw, bw); // Pixels destroyed by opening
		add_to_skeleton(skeleton, bw, iteration); // Add them to skeleton

		// distance
		if (par.type == 3) { // Most precise peeling - with circle
			kernel_2 = getStructuringElement(MORPH_ELLIPSE, Size(size,size));
			erode(source, next_peeled, kernel_2);
		}
		else {
			erode(peeled, next_peeled, kernel); // Diamond / square / diamond-square
		}
		bitwise_not(next_peeled, bw);
		bitwise_and(peeled, bw, bw); // Pixels removed by next peeling
		add_to_skeleton(distance, bw, iteration++); // calculate distance for all pixels

		std::swap(peeled, next_peeled);
		minMaxLoc(peeled, NULL, &max, NULL, NULL); // we have at least one non-zero pixel
		if (par.type == 0)
			std::swap(kernel, kernel_2); // diamond-square
	}
	if (par.show_window == 1) {
		vectorize_destroyWindow("Boundary peeling"); // Window is not needed anymore
	}
	if (!par.save_skeleton_name.empty()) { // Save output to file
		imwrite(par.save_skeleton_name, skeleton);
	}
	if (!par.save_distance_name.empty()) { // Save output to file
		imwrite(par.save_distance_name, distance);
	}
	if (!par.save_skeleton_normalized_name.empty()) { // Display skeletonization outcome
		Mat skeleton_normalized = skeleton.clone();
		normalize(skeleton_normalized, iteration-1); // Make image more contrast
		imwrite(par.save_skeleton_normalized_name, skeleton_normalized);
	}
	if (!par.save_distance_normalized_name.empty()) { // Display skeletonization outcome
		Mat distance_normalized = distance.clone();
		normalize(distance_normalized, iteration-1); // Make image more contrast
		imwrite(par.save_distance_normalized_name, distance_normalized);
	}
}

void custom::prepare_starting_points(const cv::Mat &skeleton, std::vector<start_point> &starting_points) { // Find all possible startingpoints
	for (int i = 0; i < skeleton.rows; i++) {
		for (int j = 0; j < skeleton.cols; j++) {
			if (skeleton.data[i*skeleton.step + j]) { // Pixel is in skeleton
				start_point pt;
				pt.val = skeleton.data[i*skeleton.step + j];
				pt.pt = Point(j, i);
				starting_points.push_back(pt); // Add it to queue (std::vector)
			}
		}
	}
	std::sort(starting_points.begin(), starting_points.end(), [&](start_point a, start_point b)->bool {
			return a.val < b.val;
			}); // Sort pixels by distance to object borders
}

void custom::find_max_starting_point(std::vector<start_point> &starting_points, const cv::Mat &used_pixels, int &max, cv::Point &max_pos) { // Find first unused starting point from queue (std::vector)
	max = 0;
	while ((max == 0) && !starting_points.empty()) {
		start_point pt = starting_points.back();
		starting_points.pop_back();
		if (!used_pixels.data[pt.pt.y*used_pixels.step + pt.pt.x]) { // Pixel was not used
			max = pt.val;
			max_pos = pt.pt;
		}
	}
	vectorizer_debug("New start point value: %i\n", max);
}

void custom::step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output, step3_params &par) {
	used_pixels = Scalar(0); // Pixels already used for tracing
#ifdef VECTORIZER_USE_ROI
	// Speedup using OpenCV region of interest
	// This optimization should NOT change program outcome
	step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
	step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows);
#endif

#ifdef VECTORIZER_STARTING_POINTS
	// Use queue for all possible starting points
	// This optimization CAN change program outcome by changing order
	std::vector<start_point> starting_points;
	int max;
	Point max_pos;
	prepare_starting_points(skeleton, starting_points); // Find all startingpoints
	find_max_starting_point(starting_points, used_pixels, max, max_pos); // Get first startingpoint
#else
	// Find every point in input image
	Mat starting_points = skeleton.clone();
	double max;
	Point max_pos;
	minMaxLoc(starting_points, NULL, &max, NULL, &max_pos); // Get first starting point
#endif

	int count = 0;
	while (max !=0) { // While we have unused pixel.
		//vectorizer_debug("start tracing from: %i %i\n", max_pos.x, max_pos.y);
		v_line line;
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par); // Trace first part of a line
		line.reverse();

		// Drop everything (<= 254)
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 254, 255, THRESH_BINARY);
		threshold(used_pixels(step3_roi_get(step3_changed_start)), used_pixels(step3_roi_get(step3_changed_start)), 254, 255, THRESH_BINARY);
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows); // There is no pixel with value < 254
		step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows); // There is no pixel with value == 254
#else
		threshold(used_pixels, used_pixels, 254, 255, THRESH_BINARY);
#endif
		if (line.empty()) {
			vectorizer_error("Vectorizer warning: No new point found!\n"); // Vectorization started from one point, but no new line was found. This should not happen
			spix(used_pixels, max_pos, 255); // Clear pixel to prevent infinite loop
		}
		else
			line.segment.pop_back();
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par); //Trace second part of a line
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 253, 255, THRESH_BINARY); // save first point
		threshold(used_pixels(step3_roi_get(step3_changed_start)), used_pixels(step3_roi_get(step3_changed_start)), 253, 255, THRESH_BINARY); // save first point
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
		step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows);
#else
		threshold(used_pixels, used_pixels, 253, 255, THRESH_BINARY); // Save first point (value 254)
#endif

		vectorization_output.add_line(line); // Add line to output
		count++;

#ifdef VECTORIZER_STARTING_POINTS
		find_max_starting_point(starting_points, used_pixels, max, max_pos); // Get next possible starting point
#else
		bitwise_and(Mat::zeros(used_pixels.rows, used_pixels.cols, CV_8UC(1)), Scalar(), starting_points, used_pixels);
		minMaxLoc(starting_points, NULL, &max, NULL, &max_pos); // Find next starting point
#endif
	}
	vectorizer_debug("lines found: %i\n", count);
}

int custom::pix(const Mat &img, const v_pt &point) { // Get pixel
	int x = point.x;
	int y = point.y;
	return img.data[y*img.step + x];
}

void custom::spix(const Mat &img, const v_pt &point, int value) { // Set pixel (at floating-point coordinates)
	int x = point.x;
	int y = point.y;
	img.data[y*img.step + x] = value;
}

void custom::spix(const Mat &img, const Point &point, int value) { // Set pixel
	int x = point.x;
	int y = point.y;
	img.data[y*img.step + x] = value;
}

int custom::inc_pix_to(const Mat &mask, int value, const v_pt &point, Mat &used_pixels) { // Increase value of pixel
	if ((pix(mask, point) > 0) && (pix(used_pixels, point) < value)) {
		spix(used_pixels, point, value); // Set pixel
#ifdef VECTORIZER_USE_ROI
		// Add pixel to corresponding roi
		if (value < 254) {
			step3_roi_update(step3_changed, point.x, point.y);
		}
		if (value == 254) {
			step3_roi_update(step3_changed_start, point.x, point.y);
		}
#endif
		return 1; // one pixel changed
	}
	else
		return 0; // no pixel changed
}

int custom::inc_pix_to_near(const Mat &mask, int value, const v_pt &point, Mat &used_pixels, int near) { // Increase all pixels in neighborhood
	int sum = 0;
	for (int i = -near; i<=near; i++) {
		for (int j = -near; j<=near; j++) {
			v_pt a = point;
			a.x+=i;
			a.y+=j;
			if ((a.x >= 0) && (a.y >= 0) && (a.x < used_pixels.cols) && (a.y < used_pixels.rows))
				sum += inc_pix_to(mask, value, a, used_pixels);
		}
	}
	return sum; // Count of increased pixels
}

int custom::place_next_point_at(const Mat &skeleton, v_point &new_point, int current_depth, v_line &line, Mat &used_pixels) { // Add point to line and mark them as used
	int sum = 0;
	if (line.empty()) {
		sum += inc_pix_to_near(skeleton, current_depth, new_point.main, used_pixels); // add First point
	}
	else {
		line.segment.back().control_next = new_point.control_next;
		v_point op = line.segment.back();
		v_point np = new_point;
		v_line new_segment;
		new_segment.segment.push_back(op);
		new_segment.segment.push_back(np);
		geom::chop_line(new_segment, 0.1); // make control point in every pixel along new_segment
		for (v_point point: new_segment.segment) {
			sum += inc_pix_to_near(skeleton, current_depth, point.main, used_pixels);
		}
	}
	line.segment.push_back(new_point);
	vectorizer_debug("place_next_point_at: %f %f, %i = %i\n", new_point.main.x, new_point.main.y, current_depth, sum);
	return sum;
}

float custom::calculate_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, step3_params &par) { // Get average value from neighborhood with gaussian distribution
	int limit = par.nearby_limit_gauss;
	float res = 0;
	for (int y = -limit; y<=limit; y++) {
		for (int x = -limit; x<=limit; x++) { // Limit to rectangular area
			int j = center.x + x;
			int i = center.y + y;
			if (safeat(skeleton, i, j) && (!safeat(used_pixels, i, j))) {
				v_pt pixel(j+0.5f, i+0.5f);
				pixel -= center;
				res += std::exp(-(pixel.x*pixel.x + pixel.y*pixel.y)/par.distance_coef) * safeat(distance, i, j);
			}
		}
	}
	return res;
}

v_pt custom::find_best_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, step3_params &par, float size) { // Find best value in given area
	if (size < par.gauss_precision) // Our area is lower than desired precision
		return center;
	float a[3][3];
	for (int y = -1; y<=1; y++) {
		for (int x = -1; x<=1; x++) { // Calculate at nine positions (3x3)
			v_pt offset(x, y);
			offset *= size;
			offset += center;
			a[x+1][y+1] = calculate_gaussian(skeleton, distance, used_pixels, offset, par);
		}
	}
	float i = a[0][0] + a[0][1] + a[1][0] + a[1][1]; // Upper left
	float j = a[1][0] + a[1][1] + a[2][0] + a[2][1]; // Upper right
	float k = a[0][1] + a[0][2] + a[1][1] + a[1][2]; // Lower left
	float l = a[1][1] + a[1][2] + a[2][1] + a[2][2]; // Lower right
	float m = max(max(i,j),max(k,l));
	size /= 2;
	// Move center
	if (m == i) {
		center += v_pt(-size, -size); // up + left
	}
	else if (m == j) {
		center += v_pt(size, -size); // up + right
	}
	else if (m == k) {
		center += v_pt(-size, size); // down + left
	}
	else {
		center += v_pt(size, size); // down + right
	}
	return find_best_gaussian(skeleton, distance, used_pixels, center, par, size); // continue in one quarter
}

float custom::calculate_line_fitness(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, v_pt end, float min_dist, float max_dist, step3_params &par) { // Calculate how 'good' is given line
	Point corner1(center.x - max_dist, center.y - max_dist); // Upper left corner
	Point corner2(center.x + max_dist + 1, center.y + max_dist + 1); // Lower right corner
	float res = 0;
	for (int i = corner1.y; i < corner2.y; i++) {
		for (int j = corner1.x; j < corner2.x; j++) { // for every pixel in rectangle
			if (safeat(skeleton, i, j) && (!safeat(used_pixels, i, j))) { // pixel is in skeleton and was not used
				v_pt pixel(j+0.5f, i+0.5f);
				if ((geom::distance(center, pixel) > max_dist) || (geom::distance(center, pixel) < std::fabs(min_dist)))
					continue; // Pixel is too far from center
				pixel -= center;
				v_pt en = end - center;
				en /= en.len();
				float base = pixel.x*en.x + pixel.y*en.y; // distance to center squared
				if ((base < 0) && (min_dist >= 0))
					continue;
				en *= base;
				en -= pixel;
				res += std::exp(-(en.x*en.x + en.y*en.y)/par.distance_coef) * safeat(distance, i, j); // Weight * value
			}
		}
	}
	return res;
}

v_pt custom::try_line_point(v_pt center, float angle, step3_params &par) { // Return point in distance par.nearby_limit from center in given angle
	v_pt distpoint(cos(angle), sin(angle));
	distpoint *= par.nearby_limit;
	distpoint += center;
	return distpoint;
}

float custom::find_best_line(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, float angle, step3_params &par, float size, float min_dist) { // Find best line continuation in given angle
	if (size < par.angular_precision) // we found maximum with enought precision
		return angle;
	size /= 2;
	v_pt a = try_line_point(center, angle - size, par);
	v_pt b = try_line_point(center, angle + size, par);
	float af = calculate_line_fitness(skeleton, distance, used_pixels, center, a, min_dist, par.nearby_limit, par);
	float bf = calculate_line_fitness(skeleton, distance, used_pixels, center, b, min_dist, par.nearby_limit, par);
	if (af>bf) // Go in a direction of better fitness
		return find_best_line(skeleton, distance, used_pixels, center, angle - size, par, size, min_dist);
	else
		return find_best_line(skeleton, distance, used_pixels, center, angle + size, par, size, min_dist);
}

void custom::find_best_variant_first_point(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par) { // Returns best placement of first point
	v_pt best = find_best_gaussian(skeleton, distance, used_pixels, last, par, 1); // Find best point in neighborhood of `last'
	match.emplace_back(match_variant(v_point(best, apxat_co(color_input, best), apxat(distance, best)*2)));
	if (geom::distance(best, last) > epsilon) { // We find something else than `last'
		match.emplace_back(match_variant(v_point(last, apxat_co(color_input, last), apxat(distance, last)*2))); // Return also second variant with exactly `last'
	}
}

void custom::find_best_variant_smooth(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par) { // Find best variant for next point, assuming line smoothness
	v_point pred;
	v_pt prediction = line.segment.back().main;
	auto hist = line.segment.end();
	hist--;
	if (hist == line.segment.begin())
		return; // We have no history, cannot use this predictor
	hist--;
	if (geom::distance(hist->main, prediction) > epsilon) // Last two points are not identical
		prediction = hist->main;
	if (geom::distance(line.segment.back().main, line.segment.back().control_prev) > epsilon) // Last point has previous control point
		prediction = line.segment.back().control_prev; // Use it for prediction

	// Move prediction forward (flip around last main point)
	prediction -= line.segment.back().main;
	prediction *= -1;
	prediction /= prediction.len(); // Normalize
	pred.control_next = line.segment.back().main + prediction*(par.nearby_limit/3); // Move by parameter

	step3_params pp = par;
	pp.nearby_limit = par.nearby_limit + par.size_nearby_smooth;
	float angle = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, prediction.angle(), par, par.max_angle_search_smooth, par.nearby_limit - par.size_nearby_smooth); // Find best line in given angle
	pred.main = line.segment.back().main + v_pt(std::cos(angle), std::sin(angle))*par.nearby_limit;

	pp.nearby_limit = par.nearby_control_smooth;
	float angle2 = find_best_line(skeleton, distance, used_pixels, pred.main, angle, pp, par.max_angle_search_smooth, -par.size_nearby_smooth);
	// find best positon for control point
	pred.control_prev = pred.main - v_pt(std::cos(angle2), std::sin(angle2))*(par.nearby_limit/3);

	p smoothness = fabs(angle2 - prediction.angle()); // Calculate smoothness
	if (apxat(skeleton, pred.main) && (!apxat(used_pixels, pred.main))) {
		if (smoothness < par.smoothness) { // Line is smooth enought
			pred.color = apxat_co(color_input, pred.main);
			pred.width = apxat(distance, pred.main)*2;
			match.push_back(match_variant(pred)); // Use default coef
		}
		else {
			// It looks more like corner
			vectorizer_debug("Corner detected\n");
			pred.main = geom::intersect(line.segment.back().main, prediction, pred.main, pred.control_prev - pred.main); // Find best position for corner
			float len = (pred.main - line.segment.back().main).len();
			pred.control_next = line.segment.back().main + prediction*(len/3);
			pred.control_prev = line.segment.back().main + prediction*(len*2/3); // Recalculate control points

			if (geom::distance(line.segment.back().main - prediction*len, pred.main) > len) { // Corner is between last point and new point
				if (apxat(skeleton, pred.main)) { // Use this point
					pred.color = apxat_co(color_input, pred.main);
					pred.width = apxat(distance, pred.main)*2;
					match.push_back(match_variant(pred)); // Use default coef
				}
				else
					vectorizer_debug("Corner is not in skeleton, refusing to add\n");
			}
			else
				// Corner is somewhere else, this predictos failed
				vectorizer_debug("Sorry, we already missed it, try it with other detector\n");
		}
	}
}

void custom::find_best_variant_straight(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par) {
	// Leave corner (or first point) with straight continuation
	int size = par.nearby_limit;
	float *fit = new float[par.angle_steps+2];
	fit++;
	for (int dir = 0; dir < par.angle_steps; dir++) { // Try every direction
		v_pt distpoint = try_line_point(line.segment.back().main, 2*M_PI/par.angle_steps*dir, par); // Place point
		fit[dir] = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, distpoint, par.min_nearby_straight, par.nearby_limit, par); // Calculate point fitness
	}
	fit[-1] = fit[par.angle_steps-1]; // Make "borders" to array
	fit[par.angle_steps] = fit[0];

	float *sortedfit = new float[par.angle_steps]; // Array of local maximas
	int sortedfiti = 0;
	for (int dir = 0; dir < par.angle_steps; dir++) {
		if ((fit[dir] > fit[dir+1]) && (fit[dir] > fit[dir-1]) && (fit[dir] > epsilon)) { // Look if direction is local maximum
			sortedfit[sortedfiti++] = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, 2*M_PI/par.angle_steps*dir, par, 2*M_PI/par.angle_steps); // Move each direction a little
		}
	}
	fit--;
	delete []fit;

	std::sort(sortedfit, sortedfit+sortedfiti, [&](float a, float b)->bool { // Sort by line fitness
			v_pt da = try_line_point(line.segment.back().main, a, par);
			float fa = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, da, par.min_nearby_straight, par.nearby_limit, par);
			v_pt db = try_line_point(line.segment.back().main, b, par);
			float fb = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, db, par.min_nearby_straight, par.nearby_limit, par);
			return fa > fb;
			});

	//vectorizer_debug("count of variants: %i\n", sortedfiti);
	for (int dir = 0; dir < sortedfiti; dir++) {
		//v_pt distpoint = try_line_point(line.segment.back().main, sortedfit[dir], par);
		//float my = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, distpoint, 0, par.nearby_limit, par);
		//vectorizer_debug("Sorted variants: %f: %f\n", sortedfit[dir], my);

		v_pt distpoint = try_line_point(line.segment.back().main, sortedfit[dir], par);
		v_point out = v_point(distpoint, apxat_co(color_input, distpoint), apxat(distance, distpoint)*2); // Get color and width
		out.control_next = out.main - line.segment.back().main; // Calculate control points
		out.control_next /= 3; // should be in one third between main points
		out.control_prev = out.main - out.control_next;
		out.control_next += line.segment.back().main;
		out.color = apxat_co(color_input, out.main);
		out.width = apxat(distance, out.main)*2;
		match.push_back(match_variant(out)); // Add to possible variants // Use default coef
	}
	delete []sortedfit;
}

void custom::filter_best_variant_end(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match, step3_params &par) {
	// Detect if first variant is good as ending of line
	//for (auto var = match.begin(); var != match.end(); var++) {
	if (match.empty())
		return;
	auto var = match.begin();
	// Get last segment
	v_point op = line.segment.back();
	v_point np = var->pt;
	op.control_next = np.control_next;
	v_line new_segment;
	new_segment.segment.push_back(op);
	new_segment.segment.push_back(np);
	geom::chop_line(new_segment, 0.1); // Chop segment, so we can read it more precisely than with 1px step
	v_pt good = op.main;
	for (v_point point: new_segment.segment) {
		if (!apxat(skeleton, point.main)) {
			vectorizer_debug("I don't like it. %f %f -> %f %f\n", np.main.x, np.main.y, point.main.x, point.main.y); // Line is not continuing
			if ((good - op.main).len() < 3) { // Line is too short
				var->depth = 0;
			}
			else {
				var->pt.main = good;
				var->type = end;
				vectorizer_debug("Setting type to end\n"); // We found end of line
			}
			break;
		}
		good = point.main;
	}
}

void custom::find_best_variant(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, const match_variant &last, const v_line &line, std::vector<match_variant> &match, step3_params &par) {
	// Find all posible continuations of line
	if (line.segment.empty()) { // Place first point
		find_best_variant_first_point(color_input, skeleton, distance, used_pixels, last.pt.main, line, match, par);
		vectorizer_debug("find var first: %i\n", match.size());
		return;
	}
	vectorizer_debug("last type is %i\n", last.type);

	if (last.type == end) // Last point is marked as ending, do not predict anything
		return;

	find_best_variant_smooth(color_input, skeleton, distance, used_pixels, last.pt.main, line, match, par); // Try smooth continuation
	vectorizer_debug("find var smooth: %i\n", match.size());
	find_best_variant_straight(color_input, skeleton, distance, used_pixels, last.pt.main, line, match, par); // Make last point corner -- continue with straight line
	vectorizer_debug("find var straight: %i\n", match.size());

	filter_best_variant_end(color_input, skeleton, distance, used_pixels, last.pt.main, line, match, par); // Work as filter on existing variants -- if some point seems to be ending, fit its position and mark it
	vectorizer_debug("find var end: %i\n", match.size());

	return;
}

float custom::do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, const match_variant &last_placed, int allowed_depth, v_line &line, match_variant &new_point, step3_params &par) {
	if (allowed_depth <= 0) {
		return 0;
	}
	match_variant best_match;
	best_match.depth = -1;

	std::vector<match_variant> all_matches;
	find_best_variant(color_input, skeleton, distance, used_pixels, last_placed, line, all_matches, par); // Find all possible variants for next point
	for (int variant = 0; variant <= all_matches.size(); variant++) { // Try all variants
		match_variant last_match;
		if (variant < all_matches.size()) {
			last_match = all_matches[variant]; // Treat this variant as our point
		}
		if (last_match.depth > 0) { // We allowed to do recursion
			int sum = place_next_point_at(skeleton, last_match.pt, allowed_depth, line, used_pixels); // Mark point as used
			last_match.depth += do_prediction(color_input, skeleton, distance, used_pixels, last_match, allowed_depth - 1, line, new_point, par); // Do recursion with lower depth
			line.segment.pop_back();
		}

		if (last_match.depth > best_match.depth) { // We found better match
			best_match = last_match; // best so far ...
		}
		if (allowed_depth - best_match.depth <= par.depth_auto_choose) { // 0 = best depth need to be reached, 1 = one error is allowed, ... We found something good enought
			break; // Do not try anything else
		}
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), allowed_depth, 0, THRESH_TOZERO); // Drop all markings by recursive call do_prediction()
#else
		threshold(used_pixels, used_pixels, allowed_depth, 0, THRESH_TOZERO);
#endif
	}
	new_point = best_match; // Return best match
	return best_match.depth;
}

void custom::trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par) {
	match_variant last_placed;
	last_placed.pt.main.x = startpoint.x + 0.5f; // Move point to center of pixel
	last_placed.pt.main.y = startpoint.y + 0.5f;
	last_placed.type = start;
	int sum = 0;
	int first_point = 2;
	for (;;) {
		match_variant new_point;
		float depth_found = do_prediction(color_input, skeleton, distance, used_pixels, last_placed, par.max_dfs_depth, line, new_point, par); // Do prediction (by recursion) -- place one new point
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 253, 255, THRESH_TOZERO); // All changed pixels are in roi step3_changed
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
#else
		threshold(used_pixels, used_pixels, 253, 255, THRESH_TOZERO); // Threshold whole image, this is much slower for bigger images
#endif
		if (depth_found > 0) {
			if (first_point) { // First two points are marked with value 254 (means temporary, will be deleted) (So segment between them is also marked with 254)
				sum += place_next_point_at(skeleton, new_point.pt, 254, line, used_pixels);
				first_point--;
				if (!first_point) { // Second point is marked with 255 (means final, will never be unmarked)
					v_line empty;
					place_next_point_at(skeleton, new_point.pt, 255, empty, used_pixels);
				}
			}
			else
				sum += place_next_point_at(skeleton, new_point.pt, 255, line, used_pixels); // Third and others are marked with 255 (final)
		}
		else {
			break; // No new point, end of a line
		}
		last_placed = new_point;
	}
	if (sum == 0) {
		vectorizer_debug("trace_part: no new pixel used, program is cursed\n");
	}
}

int custom::interactive(int state, int key) { // Process key press and decide what to do
	int ret = state;
	switch (key & 0xFF) {
		case 0:
		case 0xFF:
		case -1:
			break; // Nothing
		case 'q':
		case 'Q':
		case 27: // Esc
			ret = 0; // Quit program
			break;
		case 'r':
		case 'R':
			ret = 1; // Rerun from step 1
			break;
		case '\n':
			ret++; // Next vectorization step
			break;
		case 'h':
		case 'H':
		default:
			vectorizer_info("Help:\n");
			vectorizer_info("\tEnter\tContinue with next step\n");
			vectorizer_info("\tr\tRerun vectorization from begining\n");
			vectorizer_info("\tq, Esc\tQuit\n");
			vectorizer_info("\th\tHelp\n");
			break;
	}
	return ret;
}

void custom::step1_changed(int, void *ptr) { // Parameter in step 1 changed, rerun
	trackbar_refs *data = static_cast<trackbar_refs *> (ptr);
	*data->adaptive_threshold_size |= 1;
	if (*data->adaptive_threshold_size < 3)
		*data->adaptive_threshold_size = 3;
	*data->state = 2; // first step
}

void custom::step2_changed(int, void *ptr) { // Parameter in step 2 changed, rerun from this state
	trackbar_refs *data = static_cast<trackbar_refs *> (ptr);
	if (*data->state >= 4)
		*data->state = 4; // second step
}

v_image custom::vectorize(const pnm_image &original, params &parameters) { // Original should be PPM image (color)
	Mat orig (original.height, original.width, CV_8UC(3));
	if (parameters.input.custom_input_name.empty()) {
		for (int j = 0; j < original.height; j++) { // Copy data from PNM image to OpenCV image structures
			for (int i = 0; i<original.width; i++) {
				orig.data[i*3+j*orig.step+2] = original.data[(i+j*original.width)*3 + 0];
				orig.data[i*3+j*orig.step+1] = original.data[(i+j*original.width)*3 + 1];
				orig.data[i*3+j*orig.step+0] = original.data[(i+j*original.width)*3 + 2];
			}
		}
	}
	else {
		// Configuration tell us to read image from file directly by OpenCV
		orig = imread(parameters.input.custom_input_name, CV_LOAD_IMAGE_COLOR);
	}

	copyMakeBorder(orig, orig, 1, 1, 1, 1, BORDER_REPLICATE, Scalar(255,255,255)); // Create boarders around image, replicate pixels

	Mat grayscale (orig.rows, orig.cols, CV_8UC(1)); // Grayscale original
	Mat binary (orig.rows, orig.cols, CV_8UC(1)); // Thresholded image

	tmea::timer threshold_timer;
	Mat skeleton; // Skeleton calculated in first step
	Mat distance; // Distance map calculated in first step
	int iteration; // Count of iterations in skeletonization step
	tmea::timer skeletonization_timer;
	Mat distance_show; // Images normalized for displaying
	Mat skeleton_show; // Images normalized for displaying
	v_image vect = v_image(orig.cols, orig.rows); // Vector output
	Mat used_pixels; // Pixels used by tracing
	tmea::timer tracing_timer;

	volatile int state = 2; // State of vectorizer, remembers in which step we are

	int max_image_size = (orig.cols+orig.rows)*2;
	trackbar_refs callback_data;
	callback_data.state = &state;
	callback_data.adaptive_threshold_size = &(parameters.step1.adaptive_threshold_size);
	while (state) { // state 0 = end
		switch (state) {
			case 2: // First step
				if (parameters.interactive) {
					vectorize_imshow("Original", orig, parameters); // Show original color image
					if (parameters.interactive == 2)
						createTrackbar("Zoom out", "Original", &parameters.zoom_level, 10000, step1_changed, &callback_data);
					vectorize_waitKey(parameters.interactive-1); // interactive == 1: wait until the key is pressed; interactive == 0: Continue after one milisecond
				}
				cvtColor(orig, grayscale, CV_RGB2GRAY);
				if (parameters.step1.invert_input)
					subtract(Scalar(255,255,255), grayscale, grayscale); // Invert input
				if (parameters.interactive) {
					vectorize_imshow("Grayscale", grayscale, parameters); // Show grayscale input image
					if (parameters.interactive == 2)
						createTrackbar("Invert input", "Grayscale", &parameters.step1.invert_input, 1, step1_changed, &callback_data);
					vectorize_waitKey(parameters.interactive-1);
				}
				binary = grayscale.clone();
				threshold_timer.start();
					step1_threshold(binary, parameters.step1); // First step -- thresholding
				threshold_timer.stop();
				if (parameters.interactive) {
					vectorize_imshow("Threshold", binary, parameters); // Show after thresholding
					if (parameters.interactive == 2) {
						createTrackbar("Threshold type", "Threshold", &parameters.step1.threshold_type, 3, step1_changed, &callback_data);
						createTrackbar("Threshold", "Threshold", &parameters.step1.threshold, 255, step1_changed, &callback_data);
						createTrackbar("Adaptive threshold", "Threshold", &parameters.step1.adaptive_threshold_size, max_image_size, step1_changed, &callback_data);
					}
					vectorize_waitKey(parameters.interactive-1);
				}
				vectorizer_info("Threshold time: %fs\n", threshold_timer.read()/1e6);
				if (parameters.interactive == 2)
					state++; // ... and wait in odd state for Enter
				else
					state+=2; // ... continue with next step
				break;
			case 4:
				skeleton = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				distance = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				skeletonization_timer.start();
					step2_skeletonization(binary, skeleton, distance, iteration, parameters); // Second step -- skeletonization
				skeletonization_timer.stop();
				vectorizer_info("Skeletonization time: %fs\n", skeletonization_timer.read()/1e6);

				if (parameters.interactive) {
					//show distance
					distance_show = distance.clone();
					normalize(distance_show, iteration-1); // Normalize image before displaying
					vectorize_imshow("Distance", distance_show, parameters);
					vectorize_waitKey(parameters.interactive-1);

					//show skeleton
					skeleton_show = skeleton.clone();
					threshold(skeleton_show, skeleton_show, 0, 255, THRESH_BINARY);
					vectorize_imshow("Skeleton", skeleton_show, parameters);
					if (parameters.interactive == 2)
						createTrackbar("Skeletonization", "Skeleton", &parameters.step2.type, 3, step2_changed, &callback_data);
					vectorize_waitKey(parameters.interactive-1);
				}
				if (parameters.interactive == 2)
					state++; // ... and wait in odd state for Enter
				else
					state+=2; // ... continue with next step
				break;
			case 6:
				used_pixels = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				tracing_timer.start();
					step3_tracing(orig, skeleton, distance, used_pixels, vect, parameters.step3); // Third step -- tracing
				tracing_timer.stop();
				vectorizer_info("Tracing time: %fs\n", tracing_timer.read()/1e6);
				if (parameters.interactive == 2)
					state++; // ... and wait in odd state for Enter
				else
					state+=2; // .. and quit
				break;
			case 8:
				state = 0;
				break;
			default:
				int key = vectorize_waitKey(1);
				if (key >= 0)
					vectorizer_debug("Key: %i\n", key);
				state = interactive(state, key);
		}
	}

	vectorizer_debug("end of vectorization\n");

	return vect;
}

}; // namespace

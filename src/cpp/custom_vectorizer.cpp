#include <opencv2/opencv.hpp>
#include "v_image.h"
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

using namespace cv;
using namespace pnm;

namespace vect {

changed_pix_roi step3_changed; // value < 254
changed_pix_roi step3_changed_start; // value == 254

void step3_roi_clear(changed_pix_roi &roi, int x, int y) {
	roi.min_x = x;
	roi.min_y = y;
	roi.max_x = -1;
	roi.max_y = -1;
}

Rect step3_roi_get(const changed_pix_roi &roi) {
	if ((roi.min_x <= roi.max_x) && (roi.min_y <= roi.max_y))
		return Rect(roi.min_x, roi.min_y, roi.max_x-roi.min_x + 1, roi.max_y-roi.min_y + 1);
	else
		return Rect(0, 0, 0, 0);
}

void step3_roi_update(changed_pix_roi &roi, int x, int y) {
	roi.min_x = min(roi.min_x, x);
	roi.min_y = min(roi.min_y, y);
	roi.max_x = max(roi.max_x, x);
	roi.max_y = max(roi.max_y, y);
}

uchar custom::nullpixel; // Virtual pixel for safe access to image matrix.

uchar &safeat(const Mat &image, int i, int j) {
	return custom::safeat(image, i, j);
}

float apxat(const Mat &image, v_pt pt) {
	int x = pt.x;
	int y = pt.y;
	pt.x-=x;
	pt.y-=y;
	float out = safeat(image, y,   x  ) * (pt.x     * pt.y) + \
		    safeat(image, y,   x+1) * ((1-pt.x) * pt.y) + \
		    safeat(image, y+1, x  ) * (pt.x     * (1-pt.y)) + \
		    safeat(image, y+1, x+1) * ((1-pt.x) * (1-pt.y));
	float sum = (pt.x     * pt.y) + \
		    ((1-pt.x) * pt.y) + \
		    (pt.x     * (1-pt.y)) + \
		    ((1-pt.x) * (1-pt.y));
	return out;
}

v_co safeat_co(const Mat &image, int i, int j) { // Safely acces image data.
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return v_co(image.data[i*image.step + j*3 + 2], image.data[i*image.step + j*3 + 1], image.data[i*image.step + j*3]);
	else {
		return v_co(0, 0, 0);
	}
}

v_co apxat_co(const Mat &image, v_pt pt) {
	int x = pt.x;
	int y = pt.y;
	pt.x-=x;
	pt.y-=y;
	v_co out = safeat_co(image, y,   x  ) * (pt.x     * pt.y) + \
		   safeat_co(image, y,   x+1) * ((1-pt.x) * pt.y) + \
		   safeat_co(image, y+1, x  ) * (pt.x     * (1-pt.y)) + \
		   safeat_co(image, y+1, x+1) * ((1-pt.x) * (1-pt.y));
	return out;
}

uchar &custom::safeat(const Mat &image, int i, int j) { // Safely acces image data.
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return image.data[i*image.step + j];
	else {
		nullpixel = 0;
		return nullpixel;
	}
}

#ifdef VECTORIZER_HIGHGUI
void custom::vectorize_imshow(const std::string& winname, InputArray mat) { // Display image in highgui named window.
	if (global_params.zoom_level) {
		Mat scaled_mat;
		int w = mat.cols() / (log10(global_params.zoom_level+10));
		int h = mat.rows() / (log10(global_params.zoom_level+10));
		resize(mat, scaled_mat, Size(w, h));
		return imshow(winname, scaled_mat);
	}
	else
		return imshow(winname, mat);
}
int custom::vectorize_waitKey(int delay) { // Wait for key press in any window.
	return waitKey(delay);
}
void custom::vectorize_destroyWindow(const std::string& winname) {
	return destroyWindow(winname);
}
#else
void custom::vectorize_imshow(const std::string& winname, InputArray mat) { // Highgui disabled, do nothing.
	return;
}
int custom::vectorize_waitKey(int delay) { // Highgui disabled, return `no key pressed'.
	return -1;
}
void custom::vectorize_destroyWindow(const std::string& winname) {
	return;
}
#endif

void custom::add_to_skeleton(Mat &out, Mat &bw, int iteration) { // Add pixels from `bw' to `out'. Something like image `or', but with more information.
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			if (!out.data[i*out.step + j]) {
				out.data[i*out.step + j] = (!!bw.data[i*bw.step + j]) * iteration;
			}
		}
	}
}

void custom::normalize(Mat &out, int max) { // Normalize grayscale image for displaying (0-255).
	for (int i = 0; i < out.rows; i++) {
		for (int j = 0; j < out.cols; j++) {
			out.data[i*out.step + j] *= 255/max;
		}
	}
}

Point custom::find_adj(const Mat &out, Point pos) { // Find adjacent point for tracing in square neighbourhood defined by step constant.
	int max = 0;
	int max_x = -1;
	int max_y = -1;
	for (int y = -step; y<=step; y++) {
		for (int x = -step; x<=step; x++) {
			int i = pos.y + y;
			int j = pos.x + x;
			if (max < (unsigned char)safeat(out, i, j)) { // Point should be in center of traced line.
				max = (unsigned char)safeat(out, i, j);
				max_x = j;
				max_y = i;
				//vectorizer_debug("find_adj: %i %i %i\n", i, j, safeat(out, i, j));
			}
		}
	}
	Point ret;
	ret.x = max_x;
	ret.y = max_y;
	return ret;
}

void custom::step1_threshold(Mat &to_threshold, step1_params &par) {
	int type = THRESH_BINARY | THRESH_OTSU; // Threshold found by Otsu's algorithm.
	if (par.threshold_type == 0) {
		vectorizer_debug("threshold: Using Otsu's algorithm\n");
		threshold(to_threshold, to_threshold, par.threshold, 255, THRESH_BINARY | THRESH_OTSU);
	}
	else if (par.threshold_type == 1) {
		vectorizer_debug("threshold: Using binary threshold %i.\n", par.threshold);
		threshold(to_threshold, to_threshold, par.threshold, 255, THRESH_BINARY);
	}
	else if ((par.threshold_type >= 2) && (par.threshold_type <= 3)) {
		vectorizer_debug("threshold: Using adaptive thresholdbinary threshold %i.\n", par.threshold);
		par.adaptive_threshold_size |= 1;
		if (par.adaptive_threshold_size < 3)
			par.adaptive_threshold_size = 3;
		int type = ADAPTIVE_THRESH_GAUSSIAN_C;
		if (par.threshold_type == 3)
			type = ADAPTIVE_THRESH_MEAN_C;
		adaptiveThreshold(to_threshold, to_threshold, 255, type, THRESH_BINARY, par.adaptive_threshold_size, par.threshold - 128);
	}

	for (int j = 0; j < to_threshold.rows; j += to_threshold.rows-1) { // Clear borders;
		for (int i = 0; i < to_threshold.cols; i++) {
			to_threshold.data[i+j*to_threshold.step] = 0;
		}
	}
	for (int j = 0; j < to_threshold.rows; j++) {
		for (int i = 0; i < to_threshold.cols; i += to_threshold.cols-1) {
			to_threshold.data[i+j*to_threshold.step] = 0;
		}
	}

	if (!par.save_threshold_name.empty()) {
		imwrite(par.save_threshold_name, to_threshold);
	}
}

void custom::step2_skeletonization(const Mat &binary_input, Mat &skeleton, Mat &distance, int &iteration, step2_params &par) {
	Mat bw     (binary_input.rows, binary_input.cols, CV_8UC(1));
	Mat source = binary_input.clone();
	Mat peeled = binary_input.clone();
	Mat next_peeled (binary_input.rows, binary_input.cols, CV_8UC(1));
	skeleton = Scalar(0);
	distance = Scalar(0);

	Mat kernel = getStructuringElement(MORPH_CROSS, Size(3,3)); // diamond
	Mat kernel_2 = getStructuringElement(MORPH_RECT, Size(3,3)); // square

	double max = 1;
	iteration = 1;
	if (par.type == 1)
		std::swap(kernel, kernel_2); // use only square

	while (max !=0) {
		if (!par.save_peeled_name.empty()) {
			char filename [128];
			std::snprintf(filename, sizeof(filename), par.save_peeled_name.c_str(), iteration);
			imwrite(filename, peeled);
		}
		if (par.show_window == 1) {
			vectorize_imshow("Boundary peeling", peeled);
			vectorize_waitKey(0);
		}
		int size = iteration * 2 + 1;
		// skeleton
		morphologyEx(peeled, bw, MORPH_OPEN, kernel);
		bitwise_not(bw, bw);
		bitwise_and(peeled, bw, bw);
		add_to_skeleton(skeleton, bw, iteration);

		// distance
		if (par.type == 3) {
			kernel_2 = getStructuringElement(MORPH_ELLIPSE, Size(size,size));
			erode(source, next_peeled, kernel_2);
		}
		else {
			erode(peeled, next_peeled, kernel);
		}
		bitwise_not(next_peeled, bw);
		bitwise_and(peeled, bw, bw);
		add_to_skeleton(distance, bw, iteration++);

		std::swap(peeled, next_peeled);
		minMaxLoc(peeled, NULL, &max, NULL, NULL);
		if (par.type == 0)
			std::swap(kernel, kernel_2);
	}
	if (par.show_window == 1) {
		vectorize_destroyWindow("Boundary peeling");
	}
	if (!par.save_skeleton_name.empty()) {
		imwrite(par.save_skeleton_name, skeleton);
	}
	if (!par.save_distance_name.empty()) {
		imwrite(par.save_distance_name, distance);
	}
	if (!par.save_skeleton_normalized_name.empty()) {
		Mat skeleton_normalized = skeleton.clone();
		normalize(skeleton_normalized, iteration-1);
		imwrite(par.save_skeleton_normalized_name, skeleton_normalized);
	}
	if (!par.save_distance_normalized_name.empty()) {
		Mat distance_normalized = distance.clone();
		normalize(distance_normalized, iteration-1);
		imwrite(par.save_distance_normalized_name, distance_normalized);
	}
}

void custom::step3_tracing(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_image &vectorization_output, step3_params &par) {
	Mat starting_points = skeleton.clone();
	used_pixels = Scalar(0);
#ifdef VECTORIZER_USE_ROI
	step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
	step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows);
#endif

	double max;
	Point max_pos;
	minMaxLoc(starting_points, NULL, &max, NULL, &max_pos);

	int count = 0;
	while (max !=0) { // While we have unused pixel.
		//vectorizer_debug("start tracing from: %i %i\n", max_pos.x, max_pos.y);
		v_line line;
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par);
		line.reverse();
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 254, 255, THRESH_BINARY);
		threshold(used_pixels(step3_roi_get(step3_changed_start)), used_pixels(step3_roi_get(step3_changed_start)), 254, 255, THRESH_BINARY);
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
		step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows);
#else
		threshold(used_pixels, used_pixels, 254, 255, THRESH_BINARY);
#endif
		if (line.empty()) {
			vectorizer_error("Vectorizer warning: No new point found!\n");
			spix(used_pixels, max_pos, 255);
		}
		else
			line.segment.pop_back();
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par);
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 253, 255, THRESH_BINARY); // save firsst point
		threshold(used_pixels(step3_roi_get(step3_changed_start)), used_pixels(step3_roi_get(step3_changed_start)), 253, 255, THRESH_BINARY); // save firsst point
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
		step3_roi_clear(step3_changed_start, used_pixels.cols, used_pixels.rows);
#else
		threshold(used_pixels, used_pixels, 253, 255, THRESH_BINARY); // save firsst point
#endif

		//TODO only for debuging
		//line.auto_smooth();
		//ODOT
		vectorization_output.add_line(line);
		count++;

		bitwise_and(Mat::zeros(used_pixels.rows, used_pixels.cols, CV_8UC(1)), Scalar(), starting_points, used_pixels);

		minMaxLoc(starting_points, NULL, &max, NULL, &max_pos);
	}
	vectorizer_debug("lines found: %i\n", count);
}

int pix(const Mat &img, const v_pt &point) {
	int x = point.x;
	int y = point.y;
	return img.data[y*img.step + x];
}

void spix(const Mat &img, const v_pt &point, int value) {
	int x = point.x;
	int y = point.y;
	img.data[y*img.step + x] = value;
}

void spix(const Mat &img, const Point &point, int value) {
	int x = point.x;
	int y = point.y;
	img.data[y*img.step + x] = value;
}

int inc_pix_to(const Mat &mask, int value, const v_pt &point, Mat &used_pixels) {
	if ((pix(mask, point) > 0) && (pix(used_pixels, point) < value)) {
		spix(used_pixels, point, value);
#ifdef VECTORIZER_USE_ROI
		if (value < 254) {
			step3_roi_update(step3_changed, point.x, point.y);
		}
		if (value == 254) {
			step3_roi_update(step3_changed_start, point.x, point.y);
		}
#endif
		return 1;
	}
	else
		return 0;
}

int inc_pix_to_near(const Mat &mask, int value, const v_pt &point, Mat &used_pixels, int near = 1) {
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
	return sum;
}

int place_next_point_at(const Mat &skeleton, v_point &new_point, int current_depth, v_line &line, Mat &used_pixels) { // přidej do linie, označ body jako využité
	int sum = 0;
	if (line.empty()) {
		sum += inc_pix_to_near(skeleton, current_depth, new_point.main, used_pixels);
	}
	else {
		line.segment.back().control_next = new_point.control_next;
		v_point op = line.segment.back();
		v_point np = new_point;
		v_line new_segment;
		new_segment.segment.push_back(op);
		new_segment.segment.push_back(np);
		chop_line(new_segment, 0.1);
		for (v_point point: new_segment.segment) {
			sum += inc_pix_to_near(skeleton, current_depth, point.main, used_pixels);
		}
	}
	line.segment.push_back(new_point);
	generic_vectorizer::vectorizer_debug("place_next_point_at: %f %f, %i = %i\n", new_point.main.x, new_point.main.y, current_depth, sum);
	return sum;
}

float calculate_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, step3_params &par) {
	int limit = par.nearby_limit_gauss;
	float res = 0;
	for (int y = -limit; y<=limit; y++) {
		for (int x = -limit; x<=limit; x++) {
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

v_pt find_best_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, step3_params &par, float size = 1) {
	if (size < par.gauss_precision)
		return center;
	float a[3][3];
	for (int y = -1; y<=1; y++) {
		for (int x = -1; x<=1; x++) {
			v_pt offset(x, y);
			offset *= size;
			offset += center;
			a[x+1][y+1] = calculate_gaussian(skeleton, distance, used_pixels, offset, par);
		}
	}
	float i = a[0][0] + a[0][1] + a[1][0] + a[1][1];
	float j = a[1][0] + a[1][1] + a[2][0] + a[2][1];
	float k = a[0][1] + a[0][2] + a[1][1] + a[1][2];
	float l = a[1][1] + a[1][2] + a[2][1] + a[2][2];
	float m = max(max(i,j),max(k,l));
	size /= 2;
	if (m == i) {
		center += v_pt(-size, -size);
	}
	else if (m == j) {
		center += v_pt(size, -size);
	}
	else if (m == k) {
		center += v_pt(-size, size);
	}
	else {
		center += v_pt(size, size);
	}
	return find_best_gaussian(skeleton, distance, used_pixels, center, par, size);
}

float calculate_line_fitness(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, v_pt end, float min_dist, float max_dist, step3_params &par) {
	Point corner1(center.x - max_dist, center.y - max_dist);
	Point corner2(center.x + max_dist + 1, center.y + max_dist + 1);
	float res = 0;
	for (int i = corner1.y; i < corner2.y; i++) {
		for (int j = corner1.x; j < corner2.x; j++) {
			if (safeat(skeleton, i, j) && (!safeat(used_pixels, i, j))) {
				v_pt pixel(j+0.5f, i+0.5f);
				if ((v_pt_distance(center, pixel) > max_dist) || (v_pt_distance(center, pixel) < std::fabs(min_dist)))
					continue;
				pixel -= center;
				v_pt en = end - center;
				en /= en.len();
				float base = pixel.x*en.x + pixel.y*en.y;
				if ((base < 0) && (min_dist >= 0))
					continue;
				en *= base;
				en -= pixel;
				res += std::exp(-(en.x*en.x + en.y*en.y)/par.distance_coef) * safeat(distance, i, j);
			}
		}
	}
	return res;
}

v_pt try_line_point(v_pt center, float angle, step3_params &par) {
	v_pt distpoint(cos(angle), sin(angle));
	distpoint *= par.nearby_limit;
	distpoint += center;
	return distpoint;
}

float find_best_line(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, float angle, step3_params &par, float size, float min_dist = 0) {
	if (size < par.angular_precision)
		return angle;
	size /= 2;
	v_pt a = try_line_point(center, angle - size, par);
	v_pt b = try_line_point(center, angle + size, par);
	float af = calculate_line_fitness(skeleton, distance, used_pixels, center, a, min_dist, par.nearby_limit, par);
	float bf = calculate_line_fitness(skeleton, distance, used_pixels, center, b, min_dist, par.nearby_limit, par);
	if (af>bf)
		return find_best_line(skeleton, distance, used_pixels, center, angle - size, par, size, min_dist);
	else
		return find_best_line(skeleton, distance, used_pixels, center, angle + size, par, size, min_dist);
}

void find_best_variant_first_point(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<std::tuple<v_point, float>> &match, step3_params &par) {
	v_pt best = find_best_gaussian(skeleton, distance, used_pixels, last, par, 1);
	match.emplace_back(std::tuple<v_point, float>(v_point(best, apxat_co(color_input, best), apxat(distance, best)*2), 1)); //TODO koeficient
	if (v_pt_distance(best, last) > epsilon) {
		match.emplace_back(std::tuple<v_point, float>(v_point(last, apxat_co(color_input, last), apxat(distance, last)*2), 1)); //TODO koeficient
	}
}

void find_best_variant_smooth(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<std::tuple<v_point, float>> &match, step3_params &par) {
	// predict smooth line
	v_point pred;
	v_pt prediction = line.segment.back().main;
	auto hist = line.segment.end();
	hist--;
	if (hist == line.segment.begin())
		return;
	hist--;
	if (v_pt_distance(hist->main, prediction) > epsilon)
		prediction = hist->main;
	if (v_pt_distance(line.segment.back().main, line.segment.back().control_prev) > epsilon)
		prediction = line.segment.back().control_prev;

	prediction -= line.segment.back().main;
	prediction *= -1;
	prediction /= prediction.len();
	pred.control_next = line.segment.back().main + prediction*(par.nearby_limit/3);

	step3_params pp = par;
	pp.nearby_limit = par.nearby_limit + par.size_nearby_smooth;
	float angle = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, prediction.angle(), par, par.max_angle_search_smooth, par.nearby_limit - par.size_nearby_smooth);
	pred.main = line.segment.back().main + v_pt(std::cos(angle), std::sin(angle))*par.nearby_limit;

	pp.nearby_limit = par.nearby_control_smooth;
	float angle2 = find_best_line(skeleton, distance, used_pixels, pred.main, angle, pp, par.max_angle_search_smooth, -par.size_nearby_smooth);
	pred.control_prev = pred.main - v_pt(std::cos(angle2), std::sin(angle2))*(par.nearby_limit/3);

	p smoothness = fabs(angle2 - prediction.angle());
	if (apxat(skeleton, pred.main) && (!apxat(used_pixels, pred.main))) {
		if (smoothness < par.smoothness) {
			pred.color = apxat_co(color_input, pred.main);
			pred.width = apxat(distance, pred.main)*2;
			match.push_back(std::tuple<v_point, float>(pred, 1)); //TODO koeficient)
		}
		else {
			generic_vectorizer::vectorizer_debug("Corner detected\n");
			pred.main = intersect(line.segment.back().main, prediction, pred.main, pred.control_prev - pred.main);
			float len = (pred.main - line.segment.back().main).len();
			pred.control_next = line.segment.back().main + prediction*(len/3);
			pred.control_prev = line.segment.back().main + prediction*(len*2/3);

			if (v_pt_distance(line.segment.back().main - prediction*len, pred.main) > len) {
				if (apxat(skeleton, pred.main)) {
					pred.color = apxat_co(color_input, pred.main);
					pred.width = apxat(distance, pred.main)*2;
					match.push_back(std::tuple<v_point, float>(pred, 1)); //TODO koeficient
				}
				else
					generic_vectorizer::vectorizer_debug("Corner is not in skeleton, refusing to add\n");
			}
			else
				generic_vectorizer::vectorizer_debug("Sorry, we already missed it, try it with other detector\n");
		}
	}
}

void find_best_variant_straight(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<std::tuple<v_point, float>> &match, step3_params &par) {
	// leave corner (or first point) with straight continuation
	int size = par.nearby_limit;
	float *fit = new float[par.angle_steps+2];
	fit++;
	for (int dir = 0; dir < par.angle_steps; dir++) {
		v_pt distpoint = try_line_point(line.segment.back().main, 2*M_PI/par.angle_steps*dir, par);
		fit[dir] = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, distpoint, par.min_nearby_straight, par.nearby_limit, par);
	}
	fit[-1] = fit[par.angle_steps-1];
	fit[par.angle_steps] = fit[0];

	float *sortedfit = new float[par.angle_steps];
	int sortedfiti = 0;
	for (int dir = 0; dir < par.angle_steps; dir++) {
		if ((fit[dir] > fit[dir+1]) && (fit[dir] > fit[dir-1]) && (fit[dir] > epsilon)) {
			sortedfit[sortedfiti++] = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, 2*M_PI/par.angle_steps*dir, par, 2*M_PI/par.angle_steps);
		}
	}
	fit--;
	delete []fit;

	std::sort(sortedfit, sortedfit+sortedfiti, [&](float a, float b)->bool {
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
		v_point out = v_point(distpoint, apxat_co(color_input, distpoint), apxat(distance, distpoint)*2);
		out.control_next = out.main - line.segment.back().main;
		out.control_next /= 3;
		out.control_prev = out.main - out.control_next;
		out.control_next += line.segment.back().main;
		out.color = apxat_co(color_input, out.main);
		out.width = apxat(distance, out.main)*2;
		match.push_back(std::tuple<v_point,float>(out, 1)); //TODO koeficient
	}
}

void find_best_variant(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<std::tuple<v_point, float>> &match, step3_params &par) {
	if (line.segment.empty()) { // place first point
		find_best_variant_first_point(color_input, skeleton, distance, used_pixels, last, line, match, par);
		generic_vectorizer::vectorizer_debug("find fst var: %i\n", match.size());
		return;
	}

	find_best_variant_smooth(color_input, skeleton, distance, used_pixels, last, line, match, par);
	generic_vectorizer::vectorizer_debug("find smooth: %i\n", match.size());
	find_best_variant_straight(color_input, skeleton, distance, used_pixels, last, line, match, par);
	generic_vectorizer::vectorizer_debug("find var: %i\n", match.size());

	return;


	//
	// try straight
	//auto history = get_history();
	//if ((history.straight()) && history.constant_width()) {
		//proj = project_forward(history);
		//proj.calculate_
	//}

	

	//TODO
		// TODO
		// do prediction:
		// odhadne směr, kterým hledat
		//   tam nastaví výhodnější váhu
		// najde 5 nejlepších variant bodů:
		//  1) zachování směru, rozměrů, skok bez kostry, navázání na kostru (2) - následně vynucený směr
		//  2) trasování kupředu (2) (+se skokem mimo kostru)
		//  3) trasování do zatáčky (2) (+se skokem mimo kostru)
		//  4) konec (1), a jsi si jistý - přidej 2.
		//
		//  if nejde najít return 0;
		//
		/*
	Point pos;
	pos.x = last.x;
	pos.y = last.y;
	Mat out = skeleton.clone();
	bitwise_and(Mat::zeros(used_pixels.rows, used_pixels.cols, CV_8UC(1)), Scalar(), out, used_pixels);

	pos = custom::find_adj(out, pos);
	p width = custom::safeat(out, pos.y, pos.x) * 2; // Compute `width' from distance of a skeleton pixel to boundary in the original image.
	if (pos.x >= 0) {
		match = v_point(v_pt(pos.x, pos.y), v_co(custom::safeat(color_input, pos.y, pos.x*3 + 2), custom::safeat(color_input, pos.y, pos.x*3 + 1), custom::safeat(color_input, pos.y, pos.x*3 + 0)), width);
		return 1;
	}
	return 0;
	*/
}

float do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_pt last_placed, int allowed_depth, v_line &line, v_point &new_point, step3_params &par) {
	if (allowed_depth <= 0) {
		return 0;
	}
	v_point best_match;
	float best_depth = -1;

	std::vector<std::tuple<v_point, float>> all_matches;
	find_best_variant(color_input, skeleton, distance, used_pixels, last_placed, line, all_matches, par);
	for (int variant = 0; variant <= all_matches.size(); variant++) {
		v_point last_match;
		float last_depth = 0;
		if (variant < all_matches.size()) {
			last_match = std::get<0>(all_matches[variant]);
			last_depth = std::get<1>(all_matches[variant]);
		}
		if (last_depth > 0) {
			int sum = place_next_point_at(skeleton, last_match, allowed_depth, line, used_pixels);
			last_depth += do_prediction(color_input, skeleton, distance, used_pixels, last_match.main, allowed_depth - 1, line, new_point, par);
			line.segment.pop_back();
		}

		if (last_depth > best_depth) {
			best_match = last_match;
			best_depth = last_depth;
		}
		if (allowed_depth - best_depth <= par.depth_auto_choose) { // 0 = best depth need to be reached, 1 = one error is allowed, ...
			break;
		}
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), allowed_depth, 0, THRESH_TOZERO); //TODO stack with used pixels
#else
		threshold(used_pixels, used_pixels, allowed_depth, 0, THRESH_TOZERO);
#endif
	}
	new_point = best_match;
	return best_depth;
}

void custom::trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par) {
	v_pt last_placed;
	last_placed.x = startpoint.x + 0.5f;
	last_placed.y = startpoint.y + 0.5f;
	int sum = 0;
	int first_point = 2;
	for (;;) {
		v_point new_point;
		float depth_found = do_prediction(color_input, skeleton, distance, used_pixels, last_placed, par.max_dfs_depth, line, new_point, par);
#ifdef VECTORIZER_USE_ROI
		threshold(used_pixels(step3_roi_get(step3_changed)), used_pixels(step3_roi_get(step3_changed)), 253, 255, THRESH_TOZERO); // TODO use stack with changed pixels
		step3_roi_clear(step3_changed, used_pixels.cols, used_pixels.rows);
#else
		threshold(used_pixels, used_pixels, 253, 255, THRESH_TOZERO); // TODO use stack with changed pixels
#endif
		if (depth_found > 0) {
			if (first_point) {
				sum += place_next_point_at(skeleton, new_point, 254, line, used_pixels);
				first_point--;
				if (!first_point) {
					v_line empty;
					place_next_point_at(skeleton, new_point, 255, empty, used_pixels);
				}
			}
			else
				sum += place_next_point_at(skeleton, new_point, 255, line, used_pixels);
		}
		else {
			break;
		}
		last_placed = new_point.main;
	}
	if (sum == 0) {
		vectorizer_debug("trace_part: no new pixel used, program is doomed\n");//TODO
	}
}

int interactive(int state, int key) {
	int ret = state;
	switch (key) {
		case 0:
		case 0xFFFF:
		case -1:
			break;
		case 'q':
		case 'Q':
		case 27:
			ret = 0;
			break;
		case 'r':
		case 'R':
			ret = 1;
			break;
		case '\n':
			ret++;
			break;
		case 'h':
		case 'H':
		default:
			fprintf(stderr, "Help:\n");
			fprintf(stderr, "\tr\tRerun vectorization from begining\n");
			fprintf(stderr, "\tq, Esc\tQuit\n");
			fprintf(stderr, "\th\tHelp\n");
			break;
	}
	return ret;
}

volatile int state;

void step1_changed(int, void*) {
	global_params.step1.adaptive_threshold_size |= 1;
	if (global_params.step1.adaptive_threshold_size < 3)
		global_params.step1.adaptive_threshold_size = 3;
	state = 2;
}

void step2_changed(int, void*) {
	if (state >= 4)
		state = 4;
}

v_image custom::vectorize(const pnm_image &original) { // Original should be PPM image (color).
	Mat orig (original.height, original.width, CV_8UC(3));
	if (global_params.input.custom_input_name.empty()) {
		for (int j = 0; j < original.height; j++) { // Copy data from PNM image to OpenCV image structures.
			for (int i = 0; i<original.width; i++) {
				orig.data[i*3+j*orig.step+2] = original.data[(i+j*original.width)*3 + 0];
				orig.data[i*3+j*orig.step+1] = original.data[(i+j*original.width)*3 + 1];
				orig.data[i*3+j*orig.step+0] = original.data[(i+j*original.width)*3 + 2];
			}
		}
	}
	else {
		orig = imread(global_params.input.custom_input_name, CV_LOAD_IMAGE_COLOR);
	}

	//copyMakeBorder(orig, orig, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(255,255,255));
	copyMakeBorder(orig, orig, 1, 1, 1, 1, BORDER_REPLICATE, Scalar(255,255,255));

	Mat grayscale (orig.rows, orig.cols, CV_8UC(1));
	Mat binary (orig.rows, orig.cols, CV_8UC(1));

	tmea::timer threshold_timer;
	Mat skeleton;
	Mat distance;
	int iteration;
	tmea::timer skeletonization_timer;
	Mat distance_show;
	Mat skeleton_show;
	v_image vect = v_image(orig.cols, orig.rows);
	Mat used_pixels;
	tmea::timer tracing_timer;

	state = 2;
	int max_image_size = (orig.cols+orig.rows)*2;
	while (state) {
		switch (state) {
			case 2:
				if (global_params.interactive) {
					vectorize_imshow("Original", orig); // Show original color image.
					if (global_params.interactive == 2)
						createTrackbar("Zoom out", "Original", &global_params.zoom_level, 10000, step1_changed);
					vectorize_waitKey(global_params.interactive-1);
				}
				cvtColor(orig, grayscale, CV_RGB2GRAY);
				if (global_params.step1.invert_input)
					subtract(Scalar(255,255,255), grayscale, grayscale);
				if (global_params.interactive) {
					vectorize_imshow("Grayscale", grayscale); // Show grayscale input image.
					if (global_params.interactive == 2)
						createTrackbar("Invert input", "Grayscale", &global_params.step1.invert_input, 1, step1_changed);
					vectorize_waitKey(global_params.interactive-1);
				}
				binary = grayscale.clone();
				threshold_timer.start();
					step1_threshold(binary, global_params.step1);
				threshold_timer.stop();
				if (global_params.interactive) {
					vectorize_imshow("Threshold", binary); // Show after thresholding
					if (global_params.interactive == 2) {
						createTrackbar("Threshold type", "Threshold", &global_params.step1.threshold_type, 3, step1_changed);
						createTrackbar("Threshold", "Threshold", &global_params.step1.threshold, 255, step1_changed);
						createTrackbar("Adaptive threshold", "Threshold", &global_params.step1.adaptive_threshold_size, max_image_size, step1_changed);
					}
					vectorize_waitKey(global_params.interactive-1);
				}
				fprintf(stderr, "Threshold time: %fs\n", threshold_timer.read()/1e6);
				if (global_params.interactive == 2)
					state++;
				else
					state+=2;
				break;
			case 4:
				skeleton = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				distance = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				skeletonization_timer.start();
					step2_skeletonization(binary, skeleton, distance, iteration, global_params.step2);
				skeletonization_timer.stop();
				fprintf(stderr, "Skeletonization time: %fs\n", skeletonization_timer.read()/1e6);

				if (global_params.interactive) {
					//show distance
					distance_show = distance.clone();
					normalize(distance_show, iteration-1);
					vectorize_imshow("Distance", distance_show);
					vectorize_waitKey(global_params.interactive-1);

					//show skeleton
					skeleton_show = skeleton.clone();
					threshold(skeleton_show, skeleton_show, 0, 255, THRESH_BINARY);
					vectorize_imshow("Skeleton", skeleton_show);
					if (global_params.interactive == 2)
						createTrackbar("Skeletonization", "Skeleton", &global_params.step2.type, 3, step2_changed);
					vectorize_waitKey(global_params.interactive-1);
				}
				if (global_params.interactive == 2)
					state++;
				else
					state+=2;
				break;
			case 6:
				used_pixels = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
				tracing_timer.start();
					step3_tracing(orig, skeleton, distance, used_pixels, vect, global_params.step3);
				tracing_timer.stop();
				fprintf(stderr, "Tracing time: %fs\n", tracing_timer.read()/1e6);
				if (global_params.interactive == 2)
					state++;
				else
					state+=2;
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

#include <opencv2/opencv.hpp>
#include "v_image.h"
#include "pnm_handler.h"
#include "vectorizer.h"
#include "custom_vectorizer.h"
#include "time_measurement.h"
#include <string>

using namespace cv;
using namespace pnm;

namespace vect {

uchar custom::nullpixel; // Virtual pixel for safe access to image matrix.

uchar &custom::safeat(const Mat &image, int i, int j) { // Safely acces image data.
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return image.data[i*image.step + j];
	else {
		nullpixel = 0;
		return nullpixel;
	}
}

#ifdef VECTORIZER_HIGHGUI
void custom::vectorize_imshow(const string& winname, InputArray mat) { // Display image in highgui named window.
	return imshow(winname, mat);
}
int custom::vectorize_waitKey(int delay) { // Wait for key press in any window.
	return waitKey(delay);
}
void custom::vectorize_destroyWindow(const string& winname) {
	return destroyWindow(winname);
}
#else
void custom::vectorize_imshow(const string& winname, InputArray mat) { // Highgui disabled, do nothing.
	return;
}
int custom::vectorize_waitKey(int delay) { // Highgui disabled, return `no key pressed'.
	return -1;
}
void custom::vectorize_destroyWindow(const string& winname) {
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
				vectorizer_debug("find_adj: %i %i %i\n", i, j, safeat(out, i, j));
			}
		}
	}
	Point ret;
	ret.x = max_x;
	ret.y = max_y;
	return ret;
}

params custom::par;

void custom::step1_threshold(Mat &to_threshold, step1_params &par) {
	int type = THRESH_BINARY | THRESH_OTSU; // Threshold found by Otsu's algorithm.
	if (par.threshold_type == 1) {
		type = THRESH_BINARY;
		vectorizer_debug("threshold: Using binary threshold %i.\n", par.threshold);
	}
	else {
		vectorizer_debug("threshold: Using Otsu's algorithm\n");
	}
	threshold(to_threshold, to_threshold, par.threshold, 255, type);
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

	double max;
	Point max_pos;
	minMaxLoc(starting_points, NULL, &max, NULL, &max_pos);

	int count = 0;
	while (max !=0) { // While we have unused pixel.
		vectorizer_debug("start tracing from: %i %i\n", max_pos.x, max_pos.y);
		v_line line;
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par);
		line.reverse();
		threshold(used_pixels, used_pixels, 254, 255, THRESH_BINARY);
		if (line.empty()) {
			vectorizer_error("Vectorizer error: No new point found!\n");
			spix(used_pixels, max_pos, 255);
		}
		else
			line.segment.pop_back();
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line, par);
		threshold(used_pixels, used_pixels, 253, 255, THRESH_BINARY); // save firsst point

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
		return 1;
	}
	else
		return 0;
}

int place_next_point_at(const Mat &skeleton, v_point &new_point, int current_depth, v_line &line, Mat &used_pixels) { // přidej do linie, označ body jako využité
	//TODO
	int sum = 0;
	if (line.empty()) {
		sum += inc_pix_to(skeleton, current_depth, new_point.main, used_pixels);
	}
	else {
		v_point old_point = line.segment.back();
		//TODO přidej vše, co leží mezi old_point a new_point;
		sum += inc_pix_to(skeleton, current_depth, new_point.main, used_pixels);
	}
	line.segment.push_back(new_point);
	fprintf(stderr, "place_next_point_at: %f %f, %i = %i\n", new_point.main.x, new_point.main.y, current_depth, sum);
	return sum;
}

float find_best_variant(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, int variant, v_point &match, step3_params &par) {
	
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
}

float do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, v_pt last_placed, int allowed_depth, v_line &line, v_point &new_point, step3_params &par) {
	if (allowed_depth <= 0) {
		return 0;
	}
	v_point best_match;
	float best_depth = -1;
	float last_depth = 1;

	for (int variant = 0; last_depth > 0; variant++) {
		v_point last_match;

		last_depth = find_best_variant(color_input, skeleton, distance, used_pixels, last_placed, line, variant, last_match, par);
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
		threshold(used_pixels, used_pixels, allowed_depth, 0, THRESH_TOZERO);
	}
	new_point = best_match;
	return best_depth;
}

void custom::trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line, step3_params &par) {
	v_pt last_placed;
	last_placed.x = startpoint.x;
	last_placed.y = startpoint.y;
	int sum = 0;
	int first_point = 1;
	for (;;) {
		v_point new_point;
		float depth_found = do_prediction(color_input, skeleton, distance, used_pixels, last_placed, par.max_dfs_depth, line, new_point, par);
		threshold(used_pixels, used_pixels, 253, 255, THRESH_TOZERO);
		if (depth_found > 0) {
			if (first_point == 1) {
				sum += place_next_point_at(skeleton, new_point, 254, line, used_pixels);
				first_point = 0;
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

v_image custom::vectorize(const pnm_image &original) { // Original should be PPM image (color).
	Mat orig (original.height, original.width, CV_8UC(3));
	for (int j = 0; j < original.height; j++) { // Copy data from PNM image to OpenCV image structures.
		for (int i = 0; i<original.width; i++) {
			orig.data[i*3+j*orig.step+2] = original.data[(i+j*original.width)*3 + 0];
			orig.data[i*3+j*orig.step+1] = original.data[(i+j*original.width)*3 + 1];
			orig.data[i*3+j*orig.step+0] = original.data[(i+j*original.width)*3 + 2];
		}
	}

	copyMakeBorder(orig, orig, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(255,255,255));

	Mat source (orig.rows, orig.cols, CV_8UC(1));
	cvtColor(orig, source, CV_RGB2GRAY);
	subtract(Scalar(255,255,255), source,source);


	Mat seg    (orig.rows, orig.cols, CV_8UC(3));
	bitwise_not(seg, seg);
	v_image vect = v_image(orig.cols, orig.rows);

	vectorize_imshow("vectorizer", orig); // Show original color image.
	vectorize_waitKey(0);
	vectorize_imshow("vectorizer", source); // Show grayscale input image.
	vectorize_waitKey(0);
	tmea::timer threshold_timer;
	threshold_timer.start();
		step1_threshold(source, par.step1);
	threshold_timer.stop();
	fprintf(stderr, "Threshold time: %fs\n", threshold_timer.read()/1e6);

	Mat skeleton = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
	Mat distance = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
	int iteration;
	tmea::timer skeletonization_timer;
	skeletonization_timer.start();
		step2_skeletonization(source, skeleton, distance, iteration, par.step2);
	skeletonization_timer.stop();
	fprintf(stderr, "Skeletonization time: %fs\n", skeletonization_timer.read()/1e6);

	//show distance
	Mat distance_show = distance.clone();
	normalize(distance_show, iteration-1);
	vectorize_imshow("vectorizer", distance_show);
	vectorize_waitKey(0);

	//show skeleton
	Mat skeleton_show = skeleton.clone();
	normalize(skeleton_show, iteration-1);
	vectorize_imshow("vectorizer", skeleton_show);
	vectorize_waitKey(0);

	Mat used_pixels = Mat::zeros(orig.rows, orig.cols, CV_8UC(1));
	tmea::timer tracing_timer;
	tracing_timer.start();
		step3_tracing(orig, skeleton, distance, used_pixels, vect, par.step3);
	tracing_timer.stop();
	fprintf(stderr, "Tracing time: %fs\n", tracing_timer.read()/1e6);

	vectorize_imshow("vectorizer", seg);
	vectorize_waitKey(0);
	vectorizer_debug("end of vectorization\n");

	return vect;
}

}; // namespace

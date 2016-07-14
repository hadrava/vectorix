#include <opencv2/opencv.hpp>
#include "parameters.h"
#include "logger.h"
#include "tracer.h"
#include "tracer_helper.h"
#include "geom.h"

using namespace cv;

namespace vectorix {

void tracer::run(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, v_image &vectorization_output) {
	used_pixels = Mat::zeros(color_input.rows, color_input.cols, CV_8UC(1));

	// Speedup using OpenCV region of interest
	changed_roi.clear(used_pixels.cols, used_pixels.rows);
	changed_start_roi.clear(used_pixels.cols, used_pixels.rows);

	starting_point st(*par);
	st.prepare(skeleton); // Find all startingpoints
	Point max_pos;
	int max = st.get_max(used_pixels, max_pos); // Get first startingpoint

	int count = 0;
	while (max !=0) { // While we have unused pixel.
		//log.log<log_level::debug>("start tracing from: %i %i\n", max_pos.x, max_pos.y);
		v_line line;
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line); // Trace first part of a line
		line.reverse();

		// Drop everything (<= 254)

		threshold(used_pixels(changed_roi.get()), used_pixels(changed_roi.get()), 254, 255, THRESH_BINARY);
		threshold(used_pixels(changed_start_roi.get()), used_pixels(changed_start_roi.get()), 254, 255, THRESH_BINARY);
		changed_roi.clear(used_pixels.cols, used_pixels.rows); // There is no pixel with value < 254
		changed_start_roi.clear(used_pixels.cols, used_pixels.rows); // There is no pixel with value == 254

		if (line.empty()) {
			log.log<log_level::error>("Vectorizer warning: No new point found!\n"); // Vectorization started from one point, but no new line was found. This should not happen
			used_pixels.at<unsigned char>(max_pos.y, max_pos.x) = 255; // Clear pixel to prevent infinite loop
		}
		else
			line.segment.pop_back();
		trace_part(color_input, skeleton, distance, used_pixels, max_pos, line); //Trace second part of a line

		threshold(used_pixels(changed_roi.get()), used_pixels(changed_roi.get()), 253, 255, THRESH_BINARY); // save first point
		threshold(used_pixels(changed_start_roi.get()), used_pixels(changed_start_roi.get()), 253, 255, THRESH_BINARY); // save first point
		changed_roi.clear(used_pixels.cols, used_pixels.rows);
		changed_start_roi.clear(used_pixels.cols, used_pixels.rows);

		vectorization_output.add_line(line); // Add line to output
		count++;

		max = st.get_max(used_pixels, max_pos); // Get next possible starting point
	}
	log.log<log_level::debug>("lines found: %i\n", count);
}

//void tracer::interactive(TrackbarCallback onChange, void *userdata) {
//};


void tracer::trace_part(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, cv::Point startpoint, v_line &line) {
	match_variant last_placed;
	last_placed.pt.main.x = startpoint.x + 0.5f; // Move point to center of pixel
	last_placed.pt.main.y = startpoint.y + 0.5f;
	last_placed.type = variant_type::start;
	int sum = 0;
	int first_point = 2;
	for (;;) {
		match_variant new_point;
		float depth_found = do_prediction(color_input, skeleton, distance, used_pixels, last_placed, *param_max_dfs_depth, line, new_point); // Do prediction (by recursion) -- place one new point

		threshold(used_pixels(changed_roi.get()), used_pixels(changed_roi.get()), 253, 255, THRESH_TOZERO); // All changed pixels are in roi step3_changed
		changed_roi.clear(used_pixels.cols, used_pixels.rows);

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
		log.log<log_level::debug>("trace_part: no new pixel used, program is cursed\n");
	}
}



/*
 * Tracing and other functions
 */

int tracer::place_next_point_at(const Mat &skeleton, v_point &new_point, int current_depth, v_line &line, Mat &used_pixels) { // Add point to line and mark them as used
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
	log.log<log_level::debug>("place_next_point_at: %f %f, %i = %i\n", new_point.main.x, new_point.main.y, current_depth, sum);
	return sum;
}

float tracer::calculate_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center) { // Get average value from neighborhood with gaussian distribution
	int limit = *param_nearby_limit_gauss;
	float res = 0;
	for (int y = -limit; y<=limit; y++) {
		for (int x = -limit; x<=limit; x++) { // Limit to rectangular area
			int j = center.x + x;
			int i = center.y + y;
			if (safeat(skeleton, i, j) && (!safeat(used_pixels, i, j))) {
				v_pt pixel(j+0.5f, i+0.5f);
				pixel -= center;
				res += std::exp(-(pixel.x*pixel.x + pixel.y*pixel.y) / *param_distance_coef) * safeat(distance, i, j);
			}
		}
	}
	return res;
}

v_pt tracer::find_best_gaussian(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, float size) { // Find best value in given area
	if (size < *param_gauss_precision) // Our area is lower than desired precision
		return center;
	float a[3][3];
	for (int y = -1; y<=1; y++) {
		for (int x = -1; x<=1; x++) { // Calculate at nine positions (3x3)
			v_pt offset(x, y);
			offset *= size;
			offset += center;
			a[x+1][y+1] = calculate_gaussian(skeleton, distance, used_pixels, offset);
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
	return find_best_gaussian(skeleton, distance, used_pixels, center, size); // continue in one quarter
}

float tracer::calculate_line_fitness(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, v_pt end, float min_dist, float max_dist) { // Calculate how 'good' is given line
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
				res += std::exp(-(en.x*en.x + en.y*en.y) / *param_distance_coef) * safeat(distance, i, j); // Weight * value
			}
		}
	}
	return res;
}

v_pt tracer::try_line_point(v_pt center, float angle) { // Return point in distance *param_nearby_limit from center in given angle
	v_pt distpoint(cos(angle), sin(angle));
	distpoint *= *param_nearby_limit;
	distpoint += center;
	return distpoint;
}

float tracer::find_best_line(const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt center, float angle, float size, float min_dist) { // Find best line continuation in given angle
	if (size < *param_angular_precision) // we found maximum with enought precision
		return angle;
	size /= 2;
	v_pt a = try_line_point(center, angle - size);
	v_pt b = try_line_point(center, angle + size);
	float af = calculate_line_fitness(skeleton, distance, used_pixels, center, a, min_dist, *param_nearby_limit);
	float bf = calculate_line_fitness(skeleton, distance, used_pixels, center, b, min_dist, *param_nearby_limit);
	if (af>bf) // Go in a direction of better fitness
		return find_best_line(skeleton, distance, used_pixels, center, angle - size, size, min_dist);
	else
		return find_best_line(skeleton, distance, used_pixels, center, angle + size, size, min_dist);
}

void tracer::find_best_variant_first_point(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match) { // Returns best placement of first point
	v_pt best = find_best_gaussian(skeleton, distance, used_pixels, last, 1); // Find best point in neighborhood of `last'
	match.emplace_back(match_variant(v_point(best, apxat_co(color_input, best), apxat(distance, best)*2)));
	if (geom::distance(best, last) > epsilon) { // We find something else than `last'
		match.emplace_back(match_variant(v_point(last, apxat_co(color_input, last), apxat(distance, last)*2))); // Return also second variant with exactly `last'
	}
}

void tracer::find_best_variant_smooth(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match) { // Find best variant for next point, assuming line smoothness
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
	pred.control_next = line.segment.back().main + prediction*(*param_nearby_limit/3); // Move by parameter

	p stored_nearby_limit = *param_nearby_limit;
	*param_nearby_limit = *param_nearby_limit + *param_size_nearby_smooth;
	float angle = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, prediction.angle(), *param_max_angle_search_smooth, stored_nearby_limit - *param_size_nearby_smooth); // Find best line in given angle
	pred.main = line.segment.back().main + v_pt(std::cos(angle), std::sin(angle))*stored_nearby_limit;

	*param_nearby_limit = *param_nearby_control_smooth;
	float angle2 = find_best_line(skeleton, distance, used_pixels, pred.main, angle, *param_max_angle_search_smooth, -*param_size_nearby_smooth);
	// find best positon for control point
	*param_nearby_limit = stored_nearby_limit;
	pred.control_prev = pred.main - v_pt(std::cos(angle2), std::sin(angle2))*(*param_nearby_limit/3);

	p smoothness = fabs(angle2 - prediction.angle()); // Calculate smoothness
	if (apxat(skeleton, pred.main) && (!apxat(used_pixels, pred.main))) {
		if (smoothness < *param_smoothness) { // Line is smooth enought
			pred.color = apxat_co(color_input, pred.main);
			pred.width = apxat(distance, pred.main)*2;
			match.push_back(match_variant(pred)); // Use default coef
		}
		else {
			// It looks more like corner
			log.log<log_level::debug>("Corner detected\n");
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
					log.log<log_level::debug>("Corner is not in skeleton, refusing to add\n");
			}
			else
				// Corner is somewhere else, this predictos failed
				log.log<log_level::debug>("Sorry, we already missed it, try it with other detector\n");
		}
	}
}

void tracer::find_best_variant_straight(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match) {
	// Leave corner (or first point) with straight continuation
	int size = *param_nearby_limit;
	float *fit = new float[*param_angle_steps+2];
	fit++;
	for (int dir = 0; dir < *param_angle_steps; dir++) { // Try every direction
		v_pt distpoint = try_line_point(line.segment.back().main, 2*M_PI / *param_angle_steps*dir); // Place point
		fit[dir] = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, distpoint, *param_min_nearby_straight, *param_nearby_limit); // Calculate point fitness
	}
	fit[-1] = fit[*param_angle_steps-1]; // Make "borders" to array
	fit[*param_angle_steps] = fit[0];

	float *sortedfit = new float[*param_angle_steps]; // Array of local maximas
	int sortedfiti = 0;
	for (int dir = 0; dir < *param_angle_steps; dir++) {
		if ((fit[dir] > fit[dir+1]) && (fit[dir] > fit[dir-1]) && (fit[dir] > epsilon)) { // Look if direction is local maximum
			sortedfit[sortedfiti++] = find_best_line(skeleton, distance, used_pixels, line.segment.back().main, 2*M_PI / *param_angle_steps*dir, 2*M_PI / *param_angle_steps); // Move each direction a little
		}
	}
	fit--;
	delete []fit;

	std::sort(sortedfit, sortedfit+sortedfiti, [&](float a, float b)->bool { // Sort by line fitness
			v_pt da = try_line_point(line.segment.back().main, a);
			float fa = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, da, *param_min_nearby_straight, *param_nearby_limit);
			v_pt db = try_line_point(line.segment.back().main, b);
			float fb = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, db, *param_min_nearby_straight, *param_nearby_limit);
			return fa > fb;
			});

	//log.log<log_level::debug>("count of variants: %i\n", sortedfiti);
	for (int dir = 0; dir < sortedfiti; dir++) {
		//v_pt distpoint = try_line_point(line.segment.back().main, sortedfit[dir], par);
		//float my = calculate_line_fitness(skeleton, distance, used_pixels, line.segment.back().main, distpoint, 0, *param_nearby_limit, par);
		//log.log<log_level::debug>("Sorted variants: %f: %f\n", sortedfit[dir], my);

		v_pt distpoint = try_line_point(line.segment.back().main, sortedfit[dir]);
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

void tracer::filter_best_variant_end(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, v_pt last, const v_line &line, std::vector<match_variant> &match) {
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
			log.log<log_level::debug>("I don't like it. %f %f -> %f %f\n", np.main.x, np.main.y, point.main.x, point.main.y); // Line is not continuing
			if ((good - op.main).len() < 3) { // Line is too short
				var->depth = 0;
			}
			else {
				var->pt.main = good;
				var->type = variant_type::end;
				log.log<log_level::debug>("Setting type to end\n"); // We found end of line
			}
			break;
		}
		good = point.main;
	}
}

void tracer::find_best_variant(const Mat &color_input, const Mat &skeleton, const Mat &distance, const Mat &used_pixels, const match_variant &last, const v_line &line, std::vector<match_variant> &match) {
	// Find all posible continuations of line
	if (line.segment.empty()) { // Place first point
		find_best_variant_first_point(color_input, skeleton, distance, used_pixels, last.pt.main, line, match);
		log.log<log_level::debug>("find var first: %i\n", match.size());
		return;
	}
	log.log<log_level::debug>("last type is %i\n", last.type);

	if (last.type == variant_type::end) // Last point is marked as ending, do not predict anything
		return;

	find_best_variant_smooth(color_input, skeleton, distance, used_pixels, last.pt.main, line, match); // Try smooth continuation
	log.log<log_level::debug>("find var smooth: %i\n", match.size());
	find_best_variant_straight(color_input, skeleton, distance, used_pixels, last.pt.main, line, match); // Make last point corner -- continue with straight line
	log.log<log_level::debug>("find var straight: %i\n", match.size());

	filter_best_variant_end(color_input, skeleton, distance, used_pixels, last.pt.main, line, match); // Work as filter on existing variants -- if some point seems to be ending, fit its position and mark it
	log.log<log_level::debug>("find var end: %i\n", match.size());

	return;
}

float tracer::do_prediction(const cv::Mat &color_input, const cv::Mat &skeleton, const cv::Mat &distance, cv::Mat &used_pixels, const match_variant &last_placed, int allowed_depth, v_line &line, match_variant &new_point) {
	if (allowed_depth <= 0) {
		return 0;
	}
	match_variant best_match;
	best_match.depth = -1;

	std::vector<match_variant> all_matches;
	find_best_variant(color_input, skeleton, distance, used_pixels, last_placed, line, all_matches); // Find all possible variants for next point
	for (int variant = 0; variant <= all_matches.size(); variant++) { // Try all variants
		match_variant last_match;
		if (variant < all_matches.size()) {
			last_match = all_matches[variant]; // Treat this variant as our point
		}
		if (last_match.depth > 0) { // We allowed to do recursion
			int sum = place_next_point_at(skeleton, last_match.pt, allowed_depth, line, used_pixels); // Mark point as used
			last_match.depth += do_prediction(color_input, skeleton, distance, used_pixels, last_match, allowed_depth - 1, line, new_point); // Do recursion with lower depth
			line.segment.pop_back();
		}

		if (last_match.depth > best_match.depth) { // We found better match
			best_match = last_match; // best so far ...
		}
		if (allowed_depth - best_match.depth <= *param_depth_auto_choose) { // 0 = best depth need to be reached, 1 = one error is allowed, ... We found something good enought
			break; // Do not try anything else
		}
		threshold(used_pixels(changed_roi.get()), used_pixels(changed_roi.get()), allowed_depth, 0, THRESH_TOZERO); // Drop all markings by recursive call do_prediction()
	}
	new_point = best_match; // Return best match
	return best_match.depth;
}



/*
 * accesing image data (1)
 */


const uchar &tracer::safeat(const Mat &image, int i, int j) { // Safely acces image data
	if (i>=0 && i<image.rows && j>=0 && j<image.step) // Pixel is inside of an image
		return image.at<uint8_t>(i, j);
	else { // Outside of an image
		nullpixel = 0; // clean data in nullpixel
		return nullpixel;
	}
}

v_co tracer::apxat_co(const Mat &image, v_pt pt) { // Get rgb at non-integer position (aproximate from neighbors)
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

v_co tracer::safeat_co(const Mat &image, int i, int j) { // Safely access rgb image data
	if (i>=0 && i<image.rows && j>=0 && j<image.step)
		return v_co(image.at<Vec3b>(i, j)[2], image.at<Vec3b>(i, j)[1], image.at<Vec3b>(i, j)[0]);
	else {
		return v_co(0, 0, 0); // Pixel is outside of image
	}
}

float tracer::apxat(const Mat &image, v_pt pt) { // Get value at non-integer position (aproximate from neighbors)
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




/*
 * accesing image data (2)
 */


int tracer::inc_pix_to_near(const Mat &mask, int value, const v_pt &point, Mat &used_pixels, int near) { // Increase all pixels in neighborhood
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

int tracer::inc_pix_to(const Mat &mask, int value, const v_pt &point, Mat &used_pixels) { // Increase value of pixel
	if ((mask.at<unsigned char>(point.y, point.x) > 0) && (used_pixels.at<unsigned char>(point.y, point.x) < value)) {
		used_pixels.at<unsigned char>(point.y, point.x) = value; // Set pixel

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



}; // namespace

#include "geom.h"
#include "offset.h"
#include "v_image.h"
#include "parameters.h"
#include "least_squares.h"
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <iostream>

namespace vect {

void offset::convert_to_outline(v_line &line, p max_error) { // Calculate outline of each line
	if (line.get_type() == fill) // It is already outline
		return;

	auto two = line.segment.begin(); // Right point of current segment
	auto one = two; // Left point of current segment
	if (two == line.segment.end()) { // Only one point
		one_point_circle(line);
		return;
	}

	++two; // Change to second point

	std::list<std::vector<v_point>> smooth_segments;
	// Naming assumes that the line stored in segment continues to the left
	do {
		// TODO uncomment and implement longer segments: & make it working
		//while (two != line.segment.end() && two->get_smooth() != corner)
			//++two;
		//if (two != line.segment.end())
			++two;

		std::vector<v_point> segment;

		if (smooth_segment_outline(one, two, segment)) {
			smooth_segments.push_back(segment);
			one = two;
			--one;
		}
		else {
			v_point middle;
			--two;
			geom::bezier_chop_in_half(*one, *two, middle);
			line.segment.insert(two, middle);
			--two;
		}
	} while (two != line.segment.end());

	// TODO merge segments:
	v_line a;
	auto x = smooth_segments.back();
	std::copy(x.begin(), x.end(), std::back_inserter(a.segment));
	std::swap(line, a);
	line.set_type(fill);
	//
}

void offset::one_point_circle(v_line &line) {
	v_point point = line.segment.front();
	line.segment.erase(line.segment.begin(), line.segment.end());

	p w = point.width/2;
	p c = w * 4 / 3 * (sqrt(2) - 1); // = (w - (sqrt(2)/2*w))*4/3*sqrt(2)

	v_point curr = point;

	// Right
	curr.main = point.main + v_pt(w, 0);
	curr.control_prev = curr.main + v_pt(0, c);
	curr.control_next = curr.main + v_pt(0, -c);
	line.segment.push_back(curr);

	// Up
	curr.main = point.main + v_pt(0, -w);
	curr.control_prev = curr.main + v_pt(c, 0);
	curr.control_next = curr.main + v_pt(-c, 0);
	line.segment.push_back(curr);

	// Left
	curr.main = point.main + v_pt(-w, 0);
	curr.control_prev = curr.main + v_pt(0, -c);
	curr.control_next = curr.main + v_pt(0, c);
	line.segment.push_back(curr);

	// Down
	curr.main = point.main + v_pt(0, w);
	curr.control_prev = curr.main + v_pt(-c, 0);
	curr.control_next = curr.main + v_pt(c, 0);
	line.segment.push_back(curr);

	// Close with bezier curve
	line.segment.push_back(line.segment.front());

	line.set_type(fill);
}

bool offset::smooth_segment_outline(std::list<v_point>::iterator one, std::list<v_point>::iterator two, std::vector<v_point> &outline) {
	--two;

	if (geom::distance(one->control_next, one->main) < epsilon) {
		one->control_next = one->main * 2.0/3.0 + two->main / 3.0;
	}

	if (geom::distance(two->control_prev, two->main) < epsilon) {
		two->control_prev = two->main * 2.0/3.0 + one->main / 3.0;
	}

	v_pt pt;
	v_point bt;

	// One
	// [0]
	pt = find_cap_end(one->main, one->control_next, one->width);
	bt = calculate_control_points_perpendicular(pt, one->main);
	outline.push_back(bt);

	// [1]
	pt = find_tangent(one->main, one->control_next, one->width, two->width, 1.0);
	bt = calculate_control_points_perpendicular(pt, one->main);
	outline.push_back(bt);

	// Two
	// [2]
	pt = find_tangent(two->main, two->control_prev, two->width, one->width, -1.0);
	bt = calculate_control_points_perpendicular(pt, two->main);
	outline.push_back(bt);

	// [3]
	pt = find_cap_end(two->main, two->control_prev, two->width);
	bt = calculate_control_points_perpendicular(pt, two->main);
	outline.push_back(bt);

	// [4]
	pt = find_tangent(two->main, two->control_prev, two->width, one->width, 1.0);
	bt = calculate_control_points_perpendicular(pt, two->main);
	outline.push_back(bt);

	// One
	// [5]
	pt = find_tangent(one->main, one->control_next, one->width, two->width, -1.0);
	bt = calculate_control_points_perpendicular(pt, one->main);
	outline.push_back(bt);

	// [6]
	outline.push_back(outline.front());

	set_circle_control_point_lengths(outline[5], outline[6], one->main, one->width);
	set_circle_control_point_lengths(outline[0], outline[1], one->main, one->width);

	set_circle_control_point_lengths(outline[2], outline[3], two->main, two->width);
	set_circle_control_point_lengths(outline[3], outline[4], two->main, two->width);

	bool a = optimize_offset_control_point_lengths(outline[1], outline[2], one->main, one->control_next, two->control_prev, two->main, one->width, two->width);
	bool b = optimize_offset_control_point_lengths(outline[4], outline[5], two->main, two->control_prev, one->control_next, one->main, two->width, one->width);

	return a & b;
}

v_pt offset::find_cap_end(v_pt main, v_pt next, p width) {
	v_pt base = next - main;
	base /= base.len();
	base *= -width/2;

	base += main;
	return base;
}

v_pt offset::find_tangent(v_pt main, v_pt next, p width, p width_next, p sign) {
	v_pt base = next - main;
	p dl = 3.0 * base.len();
	p dw = (width - width_next)/2;
	p l = width/dw/2 * dl;

	p angle = std::acos(width/2 / l);
	base /= base.len();
	base *= width/2;
	base = geom::rotate(base, angle*sign);

	base += main;
	return base;
}

v_point offset::calculate_control_points_perpendicular(v_pt pt, v_pt inside) {
	v_point ans;
	ans.main = pt;

	inside -= pt;
	inside /= inside.len();

	ans.control_next = geom::rotate_right_angle(inside, 1);
	ans.control_next += pt;

	ans.control_prev = geom::rotate_right_angle(inside, -1);
	ans.control_prev += pt;

	return ans;
}

void offset::set_circle_control_point_lengths(v_point &a, v_point &b, const v_pt &center, p width) {
	v_pt middle = (a.main + b.main)/2;
	p final_length = width / 2 - geom::distance(middle, center);
	final_length *= 4.0 / 3.0;

	middle -= center;
	middle /= middle.len();

	a.control_next -= a.main;
	b.control_prev -= b.main;
	p current_length = geom::dot_product(middle, a.control_next);

	a.control_next *= final_length / current_length;
	b.control_prev *= final_length / current_length;

	a.control_next += a.main;
	b.control_prev += b.main;
}

bool offset::optimize_offset_control_point_lengths(v_point &a, v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width) {
	// 0, Prepare parametrization:
	std::vector<p> center_times;
	std::vector<p> offset_times;
	std::vector<v_pt> center_pt;
	int check_point_count = 7;
	for (int i = 0; i < check_point_count; i++) {
		center_times.push_back((i + 1.0) / (check_point_count + 1.0));
		offset_times.push_back((i + 1.0) / (check_point_count + 1.0));
	}

	// 1, Calculate points with tangent offset
	v_point center_one;
	center_one.main = c_main;
	center_one.control_next = c_next;
	center_one.width = c_width;
	v_point center_two;
	center_two.main = d_main;
	center_two.control_prev = d_prev;
	center_two.width = d_width;
	for (int i = 0; i < check_point_count; i++) {
		v_point middle;
		geom::bezier_chop_in_t(center_one, center_two, middle, center_times[i], true);
		v_pt pt = find_tangent(middle.main, middle.control_next, middle.width, d_width, 1.0);
		center_pt.push_back(pt);
	}

	// 2 -- 5
	return optimize_control_point_lengths(center_pt, offset_times, a.main, a.control_next, b.control_prev, b.main);
}

bool offset::optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main) {
	v_pt a_n = a_next - a_main;
	v_pt b_p = b_prev - b_main;

	int iteration = 0;
	while (true) {
		// 2, Compute coefficients by least squares
		least_squares mat(2);
		for (int i = 0; i < times.size(); i++) {
			p t = times[i];
			p s = 1 - t;

			// Equation (before subtraction):
			//s*s*s*a_main + 3*s*s*t*a_n + 3*s*t*t*b_p + t*t*t*b_main == points[i];
			//s*s*s*a_main + 3*s*s*t*(a_main + a_n) + 3*s*t*t*(b_main + b_p) + t*t*t*b_main == points[i];
			//s*s*s*a_main + 3*s*s*t*a_main + 3*s*s*t*a_n + 3*s*t*t*b_main + 3*s*t*t*b_p + t*t*t*b_main == points[i];

			v_pt c0 = a_n*3*s*s*t;
			v_pt c1 = b_p*3*s*t*t;
			v_pt c2 = points[i] - a_main*s*s*s - a_main*3*s*s*t - b_main*3*s*t*t - b_main*t*t*t;

			p eqx[] = {c0.x, c1.x, c2.x};
			mat.add_equation(eqx);
			p eqy[] = {c0.y, c1.y, c2.y};
			mat.add_equation(eqy);
		}
		mat.evaluate();
		p error = mat.calc_error();

		// TODO 3
		// 3, Find improved parametrization
		//
		// 4
		iteration++;

		// 5
		if (error < 10) { //TODO const
			a_next = a_n * mat[0] + a_main;
			b_prev = b_p * mat[1] + b_main;

#ifdef OUTLINE_DEBUG
			v_point a, b;
			a.main = a_main;
			a.control_next = a_next;

			b.main = b_main;
			b.control_prev = b_prev;

			for (int i = 0; i < times.size(); i++) {
				v_point middle;
				geom::bezier_chop_in_t(a, b, middle, times[i], true);
				v_image::add_debug_line(middle.main, points[i]);
			}
#endif

			return true;
		}
		else if (iteration > 5) { // TODO const
			a_next = a_n * mat[0] + a_main;
			b_prev = b_p * mat[1] + b_main;

			return false; // Split segment and try again
		}
	}
}

#ifdef OUTLINE_DEBUG
void offset::offset_debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
#else
void offset::offset_debug(const char *format, ...) {}; // Does nothing
#endif

}; // namespace

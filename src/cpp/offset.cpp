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

namespace vectorix {

void offset::convert_to_outline(v_line &line, p max_error) { // Calculate outline of each line
	if (line.get_type() == fill) // It is already outline
		return;

	auto two = line.segment.begin(); // Right point of current segment
	auto one = two; // Left point of current segment
	++two; // Change to the second point

	if (two == line.segment.end()) { // Only one point
		one_point_circle(line);
		return;
	}

	std::list<std::vector<v_point>> segments;
	do {
		std::vector<v_point> segment;
		if (segment_outline(*one, *two, segment)) {
			std::vector<v_point> s;
			s.push_back(segment[0]);
			s.push_back(segment[1]);
			s.push_back(*two);
			segments.push_back(s);

			s.clear();
			s.push_back(segment[2]);
			s.push_back(segment[3]);
			s.push_back(*one);
			segments.push_front(s);

			one = two;
			++two;
		}
		else {
			v_point middle;
			geom::bezier_chop_in_half(*one, *two, middle);
			line.segment.insert(two, middle);
			--two;
		}
	} while (two != line.segment.end());

	// Connect segments
	for (auto seg = segments.begin(); seg != segments.end(); ++seg) {
		auto seg2 = seg;
		++seg2;
		if (seg2 == segments.end())
			seg2 = segments.begin();

		// Now insert seg second point ((*seg)[1]), seg2 first point ((*seg2)[0])
		// + something between them, calculated from central point between seg and seg2 ((*seg)[2]).

		p t1, t2; // Times of intersection
		if (geom::distance((*seg)[1].main, (*seg2)[0].main) < epsilon) {
			(*seg)[1].control_next = (*seg2)[0].control_next;
			(*seg2)[0].control_prev = (*seg)[1].control_prev;

			(*seg)[1].main == ((*seg)[1].main + (*seg2)[0].main) / 2;
			(*seg2)[0].main = (*seg)[1].main;
			seg->pop_back(); // remove (*seg)[2], we do not need it anymore
		}
		else if (geom::bezier_intersection((*seg)[0], (*seg)[1], (*seg2)[0], (*seg2)[1], t1, t2)) {
			v_point intersection_1, intersection_2;
			geom::bezier_chop_in_t((*seg)[0], (*seg)[1], intersection_1, t1);
			geom::bezier_chop_in_t((*seg2)[0], (*seg2)[1], intersection_2, t2);

			intersection_1.control_next = intersection_2.control_next;
			intersection_1.main = (intersection_1.main + intersection_2.main)/2;
			(*seg)[1] = intersection_1;
			(*seg2)[0] = intersection_1;
			seg->pop_back(); // remove (*seg)[2], we do not need it anymore
		}
		else {
			v_pt center = (*seg)[2].main;
			p width = (*seg)[2].width;
			seg->pop_back(); // remove (*seg)[2], we do not need it anymore

			p angle = geom::angle_absolute(center, (*seg2)[0].main, (*seg)[1].main);
			int middle_points = angle; // TODO const
			angle /= middle_points + 1;

			v_pt base_dir = (*seg2)[0].main - center;
			for (int i = 0; i < middle_points; ++i) {
				v_pt pt = geom::rotate(base_dir, angle * (middle_points - i));
				pt += center;
				seg->push_back(calculate_control_points_perpendicular(pt, center));
				set_circle_control_point_lengths((*seg)[i+1], (*seg)[i+2], center, width);
			}
			set_circle_control_point_lengths(seg->back(), (*seg2)[0], center, width);
		}
	}

	std::list<v_point> ans;
	for (auto seg = segments.begin(); seg != segments.end(); ++seg) {
		for (auto j = seg->begin(); j != seg->end(); ++j) {
			if (ans.empty() || (ans.back().main != j->main))
				ans.push_back(*j);
			else
				ans.back() = *j;
		}
	}
	if (ans.front().main != ans.back().main)
		ans.push_back(ans.front());

	std::swap(line.segment, ans);
	line.set_type(fill);
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

bool offset::segment_outline(v_point &one, v_point &two, std::vector<v_point> &outline) {
	if (geom::distance(one.control_next, one.main) < epsilon) {
		one.control_next = one.main * 2.0/3.0 + two.main / 3.0;
	}

	if (geom::distance(two.control_prev, two.main) < epsilon) {
		two.control_prev = two.main * 2.0/3.0 + one.main / 3.0;
	}

	v_pt pt;
	v_point bt;

	// One
	// [0]
	pt = find_tangent(one.main, one.control_next, two.main, one.width, two.width, 1.0);
	bt = calculate_control_points_perpendicular(pt, one.main);
	outline.push_back(bt);

	// Two
	// [1]
	pt = find_tangent(two.main, two.control_prev, one.main, two.width, one.width, -1.0);
	bt = calculate_control_points_perpendicular(pt, two.main);
	outline.push_back(bt);

	// Two
	// [2]
	pt = find_tangent(two.main, two.control_prev, one.main, two.width, one.width, 1.0);
	bt = calculate_control_points_perpendicular(pt, two.main);
	outline.push_back(bt);

	// One
	// [3]
	pt = find_tangent(one.main, one.control_next, two.main, one.width, two.width, -1.0);
	bt = calculate_control_points_perpendicular(pt, one.main);
	outline.push_back(bt);

	bool a = optimize_offset_control_point_lengths(outline[0], outline[1], one.main, one.control_next, two.control_prev, two.main, one.width, two.width);
	bool b = optimize_offset_control_point_lengths(outline[2], outline[3], two.main, two.control_prev, one.control_next, one.main, two.width, one.width);
	bool segment_is_short = geom::bezier_maximal_length(one, two) < 1; //TODO const

	return (a & b) | segment_is_short;
}

v_pt offset::find_cap_end(v_pt main, v_pt next, p width) {
	v_pt base = next - main;
	base /= base.len();
	base *= -width/2;

	base += main;
	return base;
}

v_pt offset::find_tangent(v_pt main, v_pt next, v_pt next_backup, p width, p width_next, p sign) {
	v_pt base = next - main;
	p dl = 3.0 * base.len();
	p dw = (width - width_next)/2;
	p l = width/dw/2 * dl;
	p acos = width/2 / l;

	if ((acos < -1.) || (acos > 1.)) {
		base = next_backup - main;
		dl = base.len();
		dw = (width - width_next)/2;
		l = width/dw/2 * dl;
		acos = width/2 / l;
	}

	if (acos < -1.)
		acos = -1.;
	else if (acos > 1.)
		acos = 1.;

	p angle = std::acos(acos);
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
		v_pt pt = find_tangent(middle.main, middle.control_next, center_two.main, middle.width, d_width, 1.0);
		center_pt.push_back(pt);
	}

	// 2 -- 5
	return optimize_control_point_lengths(center_pt, offset_times, a.main, a.control_next, b.control_prev, b.main);
}

bool offset::optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main) {
	v_pt a_n = a_next - a_main;
	v_pt b_p = b_prev - b_main;

	int iteration = 0;

	// 1.5, preacalculate length for step 3
	p total_length = geom::distance(a_main, points[0]);
	for (int i = 1; i < points.size(); i++) {
		total_length += geom::distance(points[i-1], points[i]);
	}
	total_length += geom::distance(points.back(), b_main);

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

		// 3, Find improved parametrization
		v_point a, b;
		a.main = a_main;
		a.control_next = a_n * mat[0] + a_main;

		b.main = b_main;
		b.control_prev = b_p * mat[1] + b_main;
		for (int i = 0; i < times.size(); i++) {
			v_point middle;
			geom::bezier_chop_in_t(a, b, middle, times[i], true);

			v_pt error_vector = points[i] - middle.main;
			middle.control_next -= middle.main;
			middle.control_next /= middle.control_next.len();

			times[i] += geom::dot_product(middle.control_next, error_vector) / total_length;
			if (times[i] < 0.)
				times[i] = 0.;
			else if (times[i] > 1.)
				times[i] = 1.;
		}

		// 4
		iteration++;

		// 5
		if (error < 1) { //TODO const
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

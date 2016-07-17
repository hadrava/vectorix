#include "geom.h"
#include "offset.h"
#include "v_image.h"
#include "parameters.h"
#include "least_squares_simple.h"
#include "least_squares_opencv.h"
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <memory>

namespace vectorix {

void offset::convert_to_outline(v_line &line) { // Calculate outline of each line
	if (line.get_type() == v_line_type::fill) // It is already outline
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
		if (!(
			((*seg)[0].main.x == (*seg)[0].main.x) &&
			((*seg)[0].main.y == (*seg)[0].main.y) &&
			((*seg)[0].control_next.x == (*seg)[0].control_next.x) &&
			((*seg)[0].control_next.y == (*seg)[0].control_next.y) &&
			((*seg)[1].control_prev.x == (*seg)[1].control_prev.x) &&
			((*seg)[1].control_prev.y == (*seg)[1].control_prev.y) &&
			((*seg)[1].main.x == (*seg)[1].main.x) &&
			((*seg)[1].main.y == (*seg)[1].main.y) &&
			((*seg2)[0].main.x == (*seg2)[0].main.x) &&
			((*seg2)[0].main.y == (*seg2)[0].main.y) &&
			((*seg2)[0].control_next.x == (*seg2)[0].control_next.x) &&
			((*seg2)[0].control_next.y == (*seg2)[0].control_next.y) &&
			((*seg2)[1].control_prev.x == (*seg2)[1].control_prev.x) &&
			((*seg2)[1].control_prev.y == (*seg2)[1].control_prev.y) &&
			((*seg2)[1].main.x == (*seg2)[1].main.x) &&
			((*seg2)[1].main.y == (*seg2)[1].main.y)
			)) {
			std::cout << "Found NaN in bezier\n";
		}
		if (geom::distance((*seg)[1].main, (*seg2)[0].main) < epsilon) {
			std::cout << "Merguju\n";
			// Segments can be connected directly by main points.
			(*seg)[1].control_next = (*seg2)[0].control_next;
			(*seg2)[0].control_prev = (*seg)[1].control_prev;

			(*seg)[1].main == ((*seg)[1].main + (*seg2)[0].main) / 2;
			(*seg2)[0].main = (*seg)[1].main;
			seg->pop_back(); // remove (*seg)[2], we do not need it anymore
		}
		else if (geom::bezier_intersection((*seg)[0], (*seg)[1], (*seg2)[0], (*seg2)[1], t1, t2)) {
			std::cout << "Sektím\n";
			// Segments are intersecting, chop them in this intersection.
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
			std::cout << "Cyklím\n";
			v_pt center_dir = (*seg)[2].control_next - (*seg)[2].control_prev;
			v_pt offset_dir = (*seg2)[0].main - (*seg)[1].main;
			p projection = geom::dot_product(center_dir, offset_dir);
			if (projection < 0.) {
				std::cout << "Tady se to neprotina, ale pritom je podezreleporadi\n";
				image->add_debug_line((*seg2)[0].main, (*seg)[1].main);
			}

			// Connect segments with arc.
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
	line.set_type(v_line_type::fill);
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

	line.set_type(v_line_type::fill);
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

void offset::prepare_tangent_offset_points(std::vector<p> &times, std::vector<v_pt> &center_pt, std::vector<p> &width, std::vector<v_pt> &offset_pt, int check_point_count, const v_point &a, const v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width) {
	for (int i = 0; i < check_point_count; i++) {
		times.push_back((i + 1.0) / (check_point_count + 1.0));
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
		geom::bezier_chop_in_t(center_one, center_two, middle, times[i], true);
		center_pt.push_back(middle.main);
		v_pt pt = find_tangent(middle.main, middle.control_next, center_two.main, middle.width, d_width, 1.0);
		width.push_back(middle.width);
		offset_pt.push_back(pt);
	}
}

int offset::remove_hidden_offset_points(std::vector<v_pt> &center_pt, std::vector<v_pt> &offset_pt, std::vector<p> &times, std::vector<p> &width) {
	/*
	// Old version
	// 1.5, Check for backward directions
	int bagr = 0; // TODO remove
	for (int i = 1; i < center_pt.size(); i++) {
		v_pt center_dir = center_pt[i] - center_pt[i-1];
		v_pt offset_dir = offset_pt[i] - offset_pt[i-1];
		p projection = geom::dot_product(center_dir, offset_dir);
		if (projection < 0.) {
			bagr++;
		//if (projection < -18) {
			//v_image::add_debug_line(center_pt[i], center_pt[i-1]);
			//v_image::add_debug_line(offset_pt[i], offset_pt[i-1]);
			//Trochu lepší debug:
			v_image::add_debug_line(center_pt[i], offset_pt[i]);
			//TODO něco s tím udělat
			//std::cout << "projection: " << projection << "\n";
			//std::cout << "projection: " << i << "\n";
		}
	}
	if (bagr == 8) {
		// TODO
		//Trochu lepší debug:
		//v_image::add_debug_line(center_pt[0], offset_pt[0]);
		//v_image::add_debug_line(offset_pt[8], center_pt[8]);
		//std::cout << "bagr\n";
	}
	*/
	/*
	int ret = 0;
	for (int i = 1; i < center_pt.size() - 1; i++) {
		if ((geom::distance(offset_pt[i], center_pt[i-1]) < width[i-1]/2) || (geom::distance(offset_pt[i], center_pt[i+1]) < width[i+1]/2)) {
			v_image::add_debug_line(center_pt[i], offset_pt[i]);

			center_pt.erase(center_pt.begin() + i);
			offset_pt.erase(offset_pt.begin() + i);
			times.erase(times.begin() + i-1);
			width.erase(width.begin() + i);
			ret++;
			i--;

		}
	}
	return ret;
	*/
	int ret = 0;
	for (int i = 1; i < center_pt.size(); i++) {
		v_pt center_dir = center_pt[i] - center_pt[i-1];
		v_pt offset_dir = offset_pt[i] - offset_pt[i-1];
		p projection = geom::dot_product(center_dir, offset_dir);
		if (projection < 0.) {
			int del = i;
			if (width[i-1] < width[i])
				del = i-1;
			if (del && (del < center_pt.size() - 1)) {
				center_pt.erase(center_pt.begin() + del);
				offset_pt.erase(offset_pt.begin() + del);
				times.erase(times.begin() + del-1);
				width.erase(width.begin() + del);

				ret++;
				i--;
			}
		}
	}
	return ret;
}

bool offset::optimize_offset_control_point_lengths(v_point &a, v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width) {
	// 0, Prepare parametrization:
	std::vector<p> times;
	std::vector<p> width;
	std::vector<v_pt> center_pt;
	std::vector<v_pt> offset_pt;

	prepare_tangent_offset_points(times, center_pt, width, offset_pt, 7, a, b, c_main, c_next, d_prev, d_main, c_width, d_width);

	// add first and last (fixed) points
	//
	v_point center_one;
	center_one.main = c_main;
	center_one.control_next = c_next;
	center_one.width = c_width;
	v_point center_two;
	center_two.main = d_main;
	center_two.control_prev = d_prev;
	center_two.width = d_width;

	center_pt.push_back(center_two.main);
	offset_pt.push_back(b.main);
	width.push_back(d_width);
	center_pt.insert(center_pt.begin(), center_one.main);
	offset_pt.insert(offset_pt.begin(), a.main);
	width.insert(width.begin(), c_width);
	//

	int r = 0;
	r = remove_hidden_offset_points(center_pt, offset_pt, times, width);

	offset_pt.pop_back();
	offset_pt.erase(offset_pt.begin());
	// and delete them back ^, ^^
	assert(7 - r == offset_pt.size());
	std::cout << "deleted " << r << "\n";
	if (r > 5)
		return 1;
	// not enought points left ^, do not run optimization


	// guess time from ditances
	assert(times.size() == offset_pt.size());

	p total_length = geom::distance(a.main, offset_pt[0]);
	times[0] = total_length;
	for (int i = 1; i < offset_pt.size(); i++) {
		total_length += geom::distance(offset_pt[i-1], offset_pt[i]);
		times[i] = total_length;
	}
	total_length += geom::distance(offset_pt.back(), b.main);

	for (int i = 0; i < times.size(); i++) {
		times[i] /= total_length;
	}


	// 2 -- 5
	bool ret = optimize_control_point_lengths(offset_pt, times, a.main, a.control_next, b.control_prev, b.main);
	std::cout << "ret: " << ret << "\n";
	return ret;
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
		std::unique_ptr<least_squares> mat;
		if (*param_lsq_method == 0)
			mat = std::unique_ptr<least_squares>(new least_squares_opencv(2, *par));
		else
			mat = std::unique_ptr<least_squares>(new least_squares_simple(2, *par));
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
			mat->add_equation(eqx);
			p eqy[] = {c0.y, c1.y, c2.y};
			mat->add_equation(eqy);
		}
		mat->evaluate();
		p error = mat->calc_error();

		// 3, Find improved parametrization
		v_point a, b;
		a.main = a_main;
		a.control_next = a_n * (*mat)[0] + a_main;

		b.main = b_main;
		b.control_prev = b_p * (*mat)[1] + b_main;
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
			a_next = a_n * (*mat)[0] + a_main;
			b_prev = b_p * (*mat)[1] + b_main;

#ifdef VECTORIX_OUTLINE_DEBUG
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
			a_next = a_n * (*mat)[0] + a_main;
			b_prev = b_p * (*mat)[1] + b_main;

			return false; // Split segment and try again
		}
	}
}

}; // namespace

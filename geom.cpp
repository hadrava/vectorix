#include "v_image.h"
#include "geom.h"
#include "offset.h"
#include "parameters.h"
#include <list>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

namespace vectorix {
namespace geom {

p distance(const v_pt &a, const v_pt &b) { // Calculate distance between two points
	p x = (a.x - b.x);
	p y = (a.y - b.y);
	return std::sqrt(x*x + y*y);
}

v_pt rotate(const v_pt &vector, p angle) {
	v_pt ans;
	p c = std::cos(angle);
	p s = std::sin(angle);

	ans.x = vector.x * c - vector.y * s;
	ans.y = vector.x * s + vector.y * c;
	return ans;
}

v_pt rotate_right_angle(const v_pt &vector, int sign) { // 1 == rotate left (in Cartesian c. s.), -1 == rotate right
	v_pt ans;
	ans.x = -vector.y * sign;
	ans.y = vector.x * sign;
	return ans;
}

p dot_product(const v_pt &direction, const v_pt &vector) {
	return direction.x * vector.x + direction.y * vector.y;
}

p bezier_maximal_length(const v_point &a, const v_point &b) { // Calculate maximal length of given segment
	return distance(a.main, a.control_next) + distance(a.control_next, b.control_prev) + distance(b.control_prev, b.main);
}

p bezier_minimal_length(const v_point &a, const v_point &b) { // Calculate minimal length of given segment
	return distance(a.main, b.main);
}

void bezier_chop_in_half(v_point &one, v_point &two, v_point &newpoint) { // Add newpoint in the middle of bezier segment
	bezier_chop_in_t(one, two, newpoint, 0.5);
}

void bezier_chop_in_t(v_point &one, v_point &two, v_point &newpoint, p t, bool constant) {
	p s = 1 - t;
	newpoint.control_prev = (one.main*s + one.control_next*t)*s + (one.control_next*s + two.control_prev*t)*t;

	newpoint.control_next = (one.control_next*s + two.control_prev*t)*s + (two.control_prev*s + two.main*t)*t;

	newpoint.main = newpoint.control_prev*s  + newpoint.control_next*t;

	newpoint.opacity = one.opacity*s + two.opacity*t;
	newpoint.width = one.width*s + two.width*t;
	newpoint.color = one.color*s + two.color*t;

	if (!constant) {
		one.control_next = one.main*s + one.control_next*t;
		two.control_prev = two.control_prev*s + two.main*t;
	}
}

void chop_line(v_line &line, p max_distance) { // Chop whole line, so the maximal length of segment is max_distance
	auto two = line.segment.begin();
	auto one = two;
	if (two != line.segment.end())
		++two;
	while (two != line.segment.end()) {
		while (bezier_maximal_length(*one, *two) > max_distance) {
			v_point newpoint;
			bezier_chop_in_half(*one, *two, newpoint);
			line.segment.insert(two, newpoint);
			--two;
		}
		one=two;
		two++;
	}
}


v_pt intersect(v_pt a, v_pt b, v_pt c, v_pt d) { // Calculate intersection of line AB with line CD. A and C are absolute coordinates. B is relative to A, D is relative to C
	// Geometry background is described in a documentation
	/*
	// Alternative version with unpleasant singularity
	c -= a;
	p dt = d.y/d.x;
	p t = c.y - dt*c.x;
	t /= b.y - dt*b.x;
	b *= t;
	a += b;
	return a;
	*/
	c -= a;
	b /= b.len(); // Change length to 1 unit
	v_pt c_proj = b*(c.x*b.x + c.y*b.y); // Project C to direction b
	v_pt d_proj = b*(d.x*b.x + d.y*b.y); // Project D to direction b
	p cl = distance(c_proj, c); // Distance of c from line AB
	p dl = distance(d_proj, d); // Distance of d from line AB
	d *= cl/dl; // Scale d to make it in same distance as c (from line AB)
	v_pt intersection = d + c; // Calculate position of intersection
	v_pt proj = b*(intersection.x*b.x + intersection.y*b.y);
	if (distance(proj, intersection) > epsilon)
		// Vector d is leading away from line AB. Intersection is placed at c - d
		d *= -1;
	return d+c+a; // Absolute position of intersection
}

bool right_of(const v_pt &center, v_pt heading, v_pt right) {
	right -= center;
	heading -= center;
	return (heading.y * right.x - heading.x * right.y) >= 0;
}

int four_points_to_hull(v_pt *x) {
	int mini = 0;
	v_pt minpt = x[0];
	for (int i = 1; i < 4; ++i) {
		if ((x[i].x < minpt.x) || ((x[i].x == minpt.x) && (x[i].y < minpt.y))) {
			mini = i;
			minpt = x[i];
		}
	}
	if (mini)
		std::swap(x[0], x[mini]);

	int minai = 1;
	int maxai = 1;
	v_pt tmp = x[1] - x[0];
	p mina = tmp.y / tmp.x;
	p maxa = mina;
	for (int i = 2; i < 4; ++i) {
		tmp = x[i] - x[0];
		p angle = tmp.y / tmp.x;
		if (angle < mina) {
			mina = angle;
			minai = i;
		}
		else {
			maxa = angle;
			maxai = i;
		}
	}

	if (minai != 1)
		std::swap(x[1], x[minai]);
	if (maxai == 1)
		maxai = minai;

	int lasti = 2 + 3 - maxai;

	if (right_of(x[2], x[maxai], x[lasti])) {
		if (maxai == 3)
			std::swap(x[2], x[3]);
		return 4;
	}
	else {
		if (maxai == 4)
			std::swap(x[2], x[3]);
		return 3;
	}
}

bool segment_intersect(const v_pt &a, const v_pt &b, const v_pt &c, const v_pt &d) {
	if (right_of(a, b, c) == right_of(a, b, d))
		return false;
	else if (right_of(c, d, a) == right_of(c, d, b))
		return false;
	else
		return true;
}

bool bezier_may_intersect(const v_point &a, const v_point &b, const v_point &c, const v_point &d) {
	v_pt x[4], y[4];
	x[0] = a.main;
	x[1] = a.control_next;
	x[2] = b.control_prev;
	x[3] = b.main;

	y[0] = c.main;
	y[1] = c.control_next;
	y[2] = d.control_prev;
	y[3] = d.main;

	int xc = four_points_to_hull(x);
	int yc = four_points_to_hull(y);

	for (int i = 0; i < xc; ++i) {
		for (int j = 0; j < yc; ++j) {
			if (segment_intersect(x[i], x[(i+1) % xc], y[j], y[(j+1) % yc]))
				return true;
		}
	}

	return false;
}

bool bezier_intersection(const v_point &a, const v_point &b, const v_point &c, const v_point &d, p &t1, p &t2) {
	if (!bezier_may_intersect(a, b, c, d))
		return false;

	if (bezier_maximal_length(a, b) + bezier_maximal_length(c, d) < 0.001) { // TODO const
		t1 = 0.5;
		t2 = 0.5;
		return true;
	}

	v_point x[3];
	v_point y[3];
	x[0] = a;
	x[2] = b;
	y[0] = c;
	y[2] = d;

	/*
	 * Move segment to center to reduce precision problems.
	 *
	 * This is necesery when p is only a float. Consider coordinates larger
	 * than 2048. Precision is only about 0.0001 in decimal.
	 */
	v_pt center = ((x[0].main + x[0].control_next) + (x[2].control_prev + x[2].main))
	            + ((y[0].main + y[0].control_next) + (y[2].control_prev + y[2].main));
	center /= 8;
	x[0].main         -= center;
	x[0].control_next -= center;
	x[2].control_prev -= center;
	x[2].main         -= center;
	y[0].main         -= center;
	y[0].control_next -= center;
	y[2].control_prev -= center;
	y[2].main         -= center;

	bezier_chop_in_half(x[0], x[2], x[1]);
	bezier_chop_in_half(y[0], y[2], y[1]);

	p tx, ty;
	if (bezier_intersection(x[0], x[1], y[1], y[2], tx, ty)) {
		t1 = tx / 2;
		t2 = ty / 2 + 0.5;
		return true;
	}
	else if (bezier_intersection(x[0], x[1], y[0], y[1], tx, ty)) {
		t1 = tx / 2;
		t2 = ty / 2;
		return true;
	}
	else if (bezier_intersection(x[1], x[2], y[1], y[2], tx, ty)) {
		t1 = tx / 2 + 0.5;
		t2 = ty / 2 + 0.5;
		return true;
	}
	else if (bezier_intersection(x[1], x[2], y[0], y[1], tx, ty)) {
		t1 = tx / 2 + 0.5;
		t2 = ty / 2;
		return true;
	}
	else
		return false;
}

p angle_absolute(const v_pt &center, const v_pt &dir1, const v_pt &dir2) {
	v_pt a = dir2 - center;
	p angle = a.angle();
	a = dir1 - center;
	angle -= a.angle();
	if (angle < 0)
		angle += 2*M_PI;
	return angle;
}


void group_line(std::list<v_line> &list, const v_line &line) { // Convert one line to list of lines. Each created line consists of one segment. Created lines are marked as group
	auto two = line.segment.begin();
	auto one = two;
	if ((two != line.segment.end()) && (line.get_type() == v_line_type::stroke))
		++two;
	else {
		list.push_back(line);
		return; // fill or empty lines cannot be converted
	}
	int segment_count = 0;
	while (two != line.segment.end()) {
		v_line new_line;
		new_line.segment.push_back(*one);
		new_line.segment.push_back(*two);
		new_line.set_type(v_line_type::stroke);
		new_line.set_group(v_line_group::group_continue);
		list.push_back(new_line); // Add segment to list

		one=two;
		two++;
		segment_count++;
	}
	if (segment_count >= 2) {
		list.front().set_group(v_line_group::group_first);
		list.back().set_group(v_line_group::group_last);
	}
	else
		list.front().set_group(v_line_group::group_normal);
}

void convert_to_variable_width(v_image &img, int type, parameters &params) { // Convert lines before exporting to support variable-width lines
	p *param_auto_contour_variance;
	params.add_comment("How often draw as contour: higher values: less often, lower: more often, negative: always use contours");
	params.bind_param(param_auto_contour_variance, "auto_contour_variance", (p) 5);

	for (auto c = img.line.begin(); c != img.line.end(); c++) {
		std::list<v_line> new_list;
		int new_type = type;
		if (type == 3) { // Automatic convert - chnage only lines with variable width
			p mean = 0;
			p count = 0;
			for (auto a: c->segment) { // Calculate mean width
				mean += a.width;
				count++;
			}
			if (count == 0)
				new_type = 0;
			else {
				mean /= count;
				p variance = 0;
				for (auto a: c->segment) { // Calculate variance of width
					variance += (a.width - mean) * (a.width - mean);
				}
				variance /= count;
				if (variance > *param_auto_contour_variance)
					new_type = 2; // Width is changing too much, calculate outline and fill it
				else
					new_type = 0; // Line has (almost) constant width, do nothing
			}
		}
		switch (new_type) {
			case 0: // Do not convert anything
				break;
			case 1: // Chop each line to separate segments
				group_line(new_list, *c);
				img.line.splice(c, new_list);
				img.line.erase(c);
				c--;
				break;
			case 2: // Convert line to its outline and fill it
				offset convertor(img, params);
				convertor.convert_to_outline(*c);
				break;
		}
	}
}

void auto_smooth(v_line &line) { // Forget all control points (except unused - first and last) and place them so the line is smooth
	for (auto pt = line.segment.begin(); pt != line.segment.end(); pt++) {
		auto prev = pt;
		if (prev == line.segment.begin()) // Skip first point
			continue;
		prev--;
		auto next = pt;
		next++;
		if (next == line.segment.end()) // Skip last point
			continue;
		v_pt vecp = prev->main - pt->main; // Direction to previous
		v_pt vecn = next->main - pt->main; // Direction to next
		p pl = vecp.len();
		p nl = vecn.len();
		if ((pl <= epsilon) || (nl <= epsilon)) // Current point is corner -> do not make it smooth
			continue;
		vecp /= pl; // Make direction unit vector
		vecn /= nl;
		v_pt control = vecn - vecp; // Direction for next control point
		control /= control.len(); // Normalize
		pt->control_prev = pt->main - (control * pl/3); // Place control point to 1/3 distance of next main point
		pt->control_next = pt->main + (control * nl/3);
	}
}


}; // namespace geom
}; // namespace vectorix

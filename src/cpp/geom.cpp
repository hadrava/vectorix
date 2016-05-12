#include "v_image.h"
#include "geom.h"
#include "offset.h"
#include "parameters.h"
#include <list>
#include <cmath>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>

namespace vect {


p geom::distance(const v_pt &a, const v_pt &b) { // Calculate distance between two points
	p x = (a.x - b.x);
	p y = (a.y - b.y);
	return std::sqrt(x*x + y*y);
}

v_pt geom::rotate(const v_pt &vector, p angle) {
	v_pt ans;
	p c = std::cos(angle);
	p s = std::sin(angle);

	ans.x = vector.x * c - vector.y * s;
	ans.y = vector.x * s + vector.y * c;
	return ans;
}

p geom::bezier_maximal_length(const v_point &a, const v_point &b) { // Calculate maximal length of given segment
	return distance(a.main, a.control_next) + distance(a.control_next, b.control_prev) + distance(b.control_prev, b.main);
}

p geom::bezier_minimal_length(const v_point &a, const v_point &b) { // Calculate minimal length of given segment
	return distance(a.main, b.main);
}

void geom::bezier_chop_in_half(v_point &one, v_point &two, v_point &newpoint) { // Add newpoint in the middle of bezier segment
	bezier_chop_in_t(one, two, newpoint, 0.5);
}

void geom::bezier_chop_in_t(v_point &one, v_point &two, v_point &newpoint, p t, bool constant) {
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

void geom::chop_line(v_line &line, p max_distance) { // Chop whole line, so the maximal length of segment is max_distance
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


v_pt geom::intersect(v_pt a, v_pt b, v_pt c, v_pt d) { // Calculate intersection of line AB with line CD. A and C are absolute coordinates. B is relative to A, D is relative to C
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


void geom::group_line(std::list<v_line> &list, const v_line &line) { // Convert one line to list of lines. Each created line consists of one segment. Created lines are marked as group
	auto two = line.segment.begin();
	auto one = two;
	if ((two != line.segment.end()) && (line.get_type() == stroke))
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
		new_line.set_type(stroke);
		new_line.set_group(group_continue);
		list.push_back(new_line); // Add segment to list

		one=two;
		two++;
		segment_count++;
	}
	if (segment_count >= 2) {
		list.front().set_group(group_first);
		list.back().set_group(group_last);
	}
	else
		list.front().set_group(group_normal);
}

void geom::convert_to_variable_width(v_image &img, int type, output_params &par) { // Convert lines before exporting to support variable-width lines
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
				if (variance > par.auto_contour_variance)
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
				offset::convert_to_outline(*c, par.max_contour_error);
				break;
		}
	}
}

void geom::auto_smooth(v_line &line) { // Forget all control points (except unused - first and last) and place them so the line is smooth
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


}; // namespace

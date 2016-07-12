#ifndef VECTORIX__GEOM_H
#define VECTORIX__GEOM_H

// Basic vector data structures and transformations

#include <list>
#include <cmath>
#include "config.h"
#include "parameters.h"
#include "v_image.h"

namespace vectorix {

namespace geom {
	p distance(const v_pt &a, const v_pt &b); // Distance of two points
	v_pt rotate(const v_pt &vector, p angle);
	v_pt rotate_right_angle(const v_pt &vector, int sign);
	p dot_product(const v_pt &direction, const v_pt &vector);
	// True length lies somewhere between theese two:
	p bezier_maximal_length(const v_point &a, const v_point &b); // Maximal length of given segment
	p bezier_minimal_length(const v_point &a, const v_point &b); // Minimal length of given segment
	void bezier_chop_in_half(v_point &one, v_point &two, v_point &newpoint); // Chop line segment in half, warning: has to change control points of one and two
	void bezier_chop_in_t(v_point &one, v_point &two, v_point &newpoint, p t, bool constant = false); // Chop line segment in time t, warning: has to change control points of one and two
	void chop_line(v_line &line, p max_distance = 1); // Chop line segments, so the maximal length of one segment is max_distance
	void group_line(std::list<v_line> &list, const v_line &line); // Split line to group of separate one-segment lines

	v_pt intersect(v_pt a, v_pt b, v_pt c, v_pt d); // Calculate intersection between line A-B and C-D; points A and C are absolute, B is relative to A and D to C


	// TODO: private: (4 functions)
	bool right_of(const v_pt &center, v_pt heading, v_pt right);
	int four_points_to_hull(v_pt *x);
	bool segment_intersect(const v_pt &a, const v_pt &b, const v_pt &c, const v_pt &d);
	bool bezier_may_intersect(const v_point &a, const v_point &b, const v_point &c, const v_point &d);


	bool bezier_intersection(const v_point &a, const v_point &b, const v_point &c, const v_point &d, p &t1, p &t2);


	void convert_to_variable_width(v_image &img, int type, parameters &par); // Convert variable-width lines:
	void auto_smooth(v_line &line); // Make line auto-smooth (drop all control points and calculate them from begining

	p angle_absolute(const v_pt &center, const v_pt &dir1, const v_pt &dir2);
};



}; // namespace
#endif

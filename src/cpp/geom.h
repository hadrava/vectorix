#ifndef _GEOM_H
#define _GEOM_H

// Basic vector data structures and transformations

#include <list>
#include <cmath>
#include "config.h"
#include "parameters.h"
#include "v_image.h"

namespace vect {

class geom {
	public:
		static p distance(const v_pt &a, const v_pt &b); // Distance of two points
		static p maximal_bezier_length(const v_point &a, const v_point &b); // Maximal length of given segment
		static void chop_in_half(v_point &one, v_point &two, v_point &newpoint); // Chop line segment in half, warning: has to change control points of one and two
		static void chop_line(v_line &line, p max_distance = 1); // Chop line segments, so the maximal length of one segment is max_distance
		static void group_line(std::list<v_line> &list, const v_line &line); // Split line to group of separate one-segment lines

		static v_pt intersect(v_pt a, v_pt b, v_pt c, v_pt d); // Calculate intersection between line A-B and C-D; points A and C are absolute, B is relative to A and D to C


		static void convert_to_variable_width(v_image &img, int type, output_params &par); // Convert variable-width lines:
		static void auto_smooth(v_line &line); // Make line auto-smooth (drop all control points and calculate them from begining
};



}; // namespace
#endif

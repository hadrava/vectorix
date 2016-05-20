#ifndef _OFFSET_H
#define _OFFSET_H

// Basic vector data structures and transformations

#include <list>
#include <vector>
#include <cmath>
#include "v_image.h"
#include "config.h"
#include "parameters.h"

namespace vectorix {

class offset {
public:
	static void convert_to_outline(v_line &line, p max_error = 1); // Convert from stroke to fill (calculate line outline)
private:
	static void one_point_circle(v_line &line); // Line containing one point will be converted to outline
	static bool segment_outline(v_point &one, v_point &two, std::vector<v_point> &outline);

	static v_pt find_cap_end(v_pt main, v_pt next, p width);
	static v_pt find_tangent(v_pt main, v_pt next, v_pt next_backup, p width, p width_next, p sign);
	static v_point calculate_control_points_perpendicular(v_pt pt, v_pt inside);
	static void set_circle_control_point_lengths(v_point &a, v_point &b, const v_pt &center, p width);

	static bool optimize_offset_control_point_lengths(v_point &a, v_point &b, const v_pt &c_main, const v_pt &c_next, const v_pt &d_prev, const v_pt &d_main, p c_width, p d_width);
	static bool optimize_control_point_lengths(const std::vector<v_pt> &points, std::vector<p> &times, const v_pt &a_main, v_pt &a_next, v_pt &b_prev, const v_pt &b_main);

	static void offset_debug(const char *format, ...);
};


}; // namespace
#endif

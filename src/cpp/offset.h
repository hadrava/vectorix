#ifndef _OFFSET_H
#define _OFFSET_H

// Basic vector data structures and transformations

#include <list>
#include <cmath>
#include "v_image.h"
#include "config.h"
#include "parameters.h"

namespace vect {

class offset {
public:
	static void convert_to_outline(v_line &line, p max_error = 1); // Convert from stroke to fill (calculate line outline)
private:
	static void one_point_circle(v_line &line); // Line containing one point will be converted to outline
	static v_line smooth_segment_outline(std::list<v_point>::iterator one, std::list<v_point>::iterator two);

	static void offset_debug(const char *format, ...);
	/*
	 * Helper functions for convert_to_outline
	 */
	static void rot(v_pt &pt, int sign);
	static void shift(const std::list<v_point> &context, std::list<v_point>::iterator pts, std::list<v_point> &output, int sign);
	static p calculate_error(const v_point &uc, const v_point &cc, const v_point &lc);
};


}; // namespace
#endif
